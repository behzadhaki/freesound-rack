#include "TrackingSamplerVoice.h"
#include "PluginProcessor.h"
#include <cmath>

// ===================== TrackingSamplerVoice =====================

TrackingSamplerVoice::TrackingSamplerVoice(FreesoundAdvancedSamplerAudioProcessor& owner)
    : processor(owner)
{
    adsr.setParameters(adsrParams);
}

void TrackingSamplerVoice::resetRuntimeState()
{
    clampUserLoopPoints();
    // decide initial direction
    playDirection = (onsetDirection == Direction::Forward ? +1 : -1);

    // choose actual start index (temp start wins first time)
    const double startIdx = (tempStartArmed ? tempStartSample : startSample);
    tempStartArmed = false; // one-shot

    // for reverse onset and no temp start, start at end-1
    if (playDirection < 0 && startIdx == startSample)
        samplePosition = std::max(startSample, endSample - 1.0);
    else
        samplePosition = jlimit(startSample, endSample, startIdx);

    firstRender = true;
}

void TrackingSamplerVoice::startNote (int midiNoteNumber,
                                      float velocity,
                                      SynthesiserSound* sound,
                                      int currentPitchWheelPosition)
{
    // Base bookkeeping
    SamplerVoice::startNote(midiNoteNumber, velocity, sound, currentPitchWheelPosition);

    noteHeld = true;                // <— NEW: start gating flag
    currentNoteNumber = midiNoteNumber;

    // Reset/capture source info
    sourceBuffer   = nullptr;
    sourceChannels = 0;
    sampleLength   = 0.0;

    if (auto* samplerSound = dynamic_cast<SamplerSound*>(sound))
    {
        if (auto* data = samplerSound->getAudioData())
        {
            sourceBuffer   = data;
            sourceChannels = sourceBuffer->getNumChannels();
            sampleLength   = sourceBuffer->getNumSamples();
        }
    }

    if (sourceBuffer == nullptr || sampleLength <= 0.0)
        return;

    // === Apply per-pad config ===
    const int padIdx = midiNoteNumber - 36;
    const auto& cfg  = processor.getPadPlaybackConfig(padIdx);

    auto normToSamps = [this](double n) -> double
    {
        if (sampleLength > 0.0)
            return juce::jlimit(0.0, sampleLength, n * sampleLength);
        return 0.0;
    };

    double startS = cfg.startSample;
    double endS   = cfg.endSample;         // -1 => full length

    const bool startLooksNorm = (startS >= 0.0 && startS <= 1.0);
    const bool endLooksNorm   = (endS   >= 0.0 && endS   <= 1.0);

    if (startLooksNorm && (endS < 0.0 || endLooksNorm))
    {
        startS = normToSamps(startS);
        endS   = (endS < 0.0 ? sampleLength : normToSamps(endS));
    }
    else
    {
        startS = juce::jlimit(0.0, sampleLength, startS);
        if (endS < 0.0) endS = sampleLength;
        endS   = juce::jlimit(0.0, sampleLength, endS);
    }

    if (endS < startS)
        std::swap(startS, endS);

    startSample = startS;
    endSample   = endS;

    // Temporary start (consumed once)
    if (cfg.tempStartArmed)
    {
        const bool tempLooksNorm = (cfg.tempStartSample >= 0.0 && cfg.tempStartSample <= 1.0);
        tempStartSample = tempLooksNorm ? normToSamps(cfg.tempStartSample)
                                        : juce::jlimit(0.0, sampleLength, cfg.tempStartSample);
        tempStartArmed  = true;
    }
    else
    {
        tempStartArmed  = false;
        tempStartSample = 0.0;
    }

    // Signal & transport
    setPitchShiftSemitones(cfg.pitchShiftSemitones);
    setStretchRatio(cfg.stretchRatio);
    setGain(cfg.gain);
    setADSR(cfg.adsr);
    setOnsetDirection(cfg.direction);
    setPlayMode(cfg.playMode);

    // Decide initial direction & position
    playDirection = (onsetDirection == Direction::Forward ? +1 : -1);

    double chosenStart = startSample;
    if (tempStartArmed)
    {
        chosenStart   = juce::jlimit(startSample, endSample, tempStartSample);
        tempStartArmed = false; // one-shot consumed
    }
    else if (playDirection < 0)
    {
        chosenStart = juce::jlimit(startSample, endSample, endSample - 1.0);
    }

    samplePosition = juce::jlimit(startSample, juce::jmax(startSample, endSample - 1.0), chosenStart);
    firstRender    = false;  // <— NEW: we’ll drive from samplePosition directly

    // Envelope
    adsr.setSampleRate(getSampleRate());
    adsr.setParameters(adsrParams);
    adsr.noteOn();

    // Pad playback state
    if (padIdx >= 0 && padIdx < 16)
    {
        auto& state = processor.padPlaybackStates[padIdx];
        state.isPlaying     = true;
        state.noteNumber    = midiNoteNumber;
        state.sampleLength  = sampleLength;
        state.velocity      = velocity;
        state.currentPosition = 0.0f;
    }

    processor.notifyNoteStarted(midiNoteNumber, velocity);
}

void TrackingSamplerVoice::stopNote (float velocity, bool allowTailOff)
{
    noteHeld = false;   // <— NEW: freeze UI immediately after this block

    if (currentNoteNumber >= 0)
    {
        const int padIndex = currentNoteNumber - 36;
        if (padIndex >= 0 && padIndex < 16)
        {
            auto& state = processor.padPlaybackStates[padIndex];
            state.isPlaying = false;
            state.currentPosition = 0.0f;
            state.currentFreesoundId = "";
        }

        // Snap UI now; audio may tail off separately
        processor.notifyPlayheadPositionChanged(currentNoteNumber, 0.0f);
        processor.notifyNoteStopped(currentNoteNumber);
    }

    // Drive our envelope tail if used
    if (allowTailOff)
        adsr.noteOff();

    // If tail is NOT allowed, fully clear this voice immediately
    if (!allowTailOff)
    {
        currentNoteNumber = -1;
        samplePosition = 0.0;
        firstRender = true;
        clearCurrentNote();
    }

    SamplerVoice::stopNote(velocity, allowTailOff);
}


void TrackingSamplerVoice::renderNextBlock (AudioBuffer<float>& outputBuffer,
                                            int startSampleOut,
                                            int numSamples)
{
    // Render audio first so isVoiceActive() reflects this block
    SamplerVoice::renderNextBlock(outputBuffer, startSampleOut, numSamples);

    if (currentNoteNumber < 0 || sampleLength <= 0.0)
        return;

    const int padIndex = currentNoteNumber - 36;
    if (padIndex < 0 || padIndex >= 16)
        return;

    auto& state = processor.padPlaybackStates[padIndex];

    // If the JUCE voice has actually finished (natural end or forced), finalize once.
    if (!isVoiceActive())
    {
        state.isPlaying = false;
        state.currentPosition = 0.0f;

        processor.notifyPlayheadPositionChanged(currentNoteNumber, 1.0f);
        processor.notifyNoteStopped(currentNoteNumber);

        currentNoteNumber = -1;
        samplePosition = 0.0;
        firstRender = true;
        return;
    }

    // NOTE-HELD GATE: if mouse/note released, stop advancing the visual cursor
    if (!noteHeld)
        return;

    // Advance in host-sample domain (UI will rescale by fileSR/hostSR)
    samplePosition += (double) numSamples;

    // Normalize over the active span [startSample, endSample)
    const double span = std::max(1.0, endSample - startSample);
    const double posInSpan = std::clamp(samplePosition - startSample, 0.0, span);
    const float pos = jlimit(0.0f, 1.0f, (float)(posInSpan / span));

    state.currentPosition = pos;
    processor.notifyPlayheadPositionChanged(currentNoteNumber, pos);
}

// ===================== TrackingPreviewSamplerVoice (unchanged) =====================

TrackingPreviewSamplerVoice::TrackingPreviewSamplerVoice(FreesoundAdvancedSamplerAudioProcessor& owner)
    : processor(owner) {}

void TrackingPreviewSamplerVoice::startNote(int midiNoteNumber, float velocity, SynthesiserSound* sound, int currentPitchWheelPosition)
{
    SamplerVoice::startNote(midiNoteNumber, velocity, sound, currentPitchWheelPosition);

    if (auto* samplerSound = dynamic_cast<SamplerSound*>(sound))
    {
        String soundName = samplerSound->getName();
        if (soundName.startsWith("preview_"))
        {
            currentFreesoundId = soundName.substring(8);
            samplePosition = 0.0;

            if (auto* data = samplerSound->getAudioData())
                sampleLength = data->getNumSamples();

            processor.notifyPreviewStarted(currentFreesoundId);
        }
    }
}

void TrackingPreviewSamplerVoice::stopNote(float velocity, bool allowTailOff)
{
    if (currentFreesoundId.isNotEmpty())
    {
        processor.notifyPreviewStopped(currentFreesoundId);
        currentFreesoundId = {};
        samplePosition = 0.0;
        sampleLength = 0.0;
    }
    SamplerVoice::stopNote(velocity, allowTailOff);
}

void TrackingPreviewSamplerVoice::renderNextBlock(AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    SamplerVoice::renderNextBlock(outputBuffer, startSample, numSamples);

    if (currentFreesoundId.isNotEmpty() && sampleLength > 0 && isVoiceActive())
    {
        samplePosition += numSamples;
        float position = (float) (samplePosition / sampleLength);
        processor.notifyPreviewPlayheadPositionChanged(currentFreesoundId, jlimit(0.0f, 1.0f, position));
        if (position >= 0.99f)
            processor.notifyPreviewPlayheadPositionChanged(currentFreesoundId, 1.0f);
    }
}
