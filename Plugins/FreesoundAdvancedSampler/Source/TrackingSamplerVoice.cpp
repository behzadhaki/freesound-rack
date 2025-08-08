#include "TrackingSamplerVoice.h"
#include "PluginProcessor.h"
#include <cmath>

// ===================== TrackingSamplerVoice =====================

TrackingSamplerVoice::TrackingSamplerVoice(FreesoundAdvancedSamplerAudioProcessor& owner)
    : processor(owner)
{
    // No ADSR initialization needed anymore
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

float TrackingSamplerVoice::calculateFadeMultiplier(float normalizedPosition, float fadeTime, FadeCurve curve, bool isFadeIn) const
{
    if (fadeTime <= 0.0f) return 1.0f;

    double sampleDuration = (endSample - startSample) / currentSampleRate;
    if (sampleDuration <= 0.0) return 1.0f;

    float fadeRatio = fadeTime / (float)sampleDuration;
    fadeRatio = jlimit(0.0f, 1.0f, fadeRatio);

    float t = 0.0f;

    if (isFadeIn)
    {
        // Fade-in: t goes from 0 to 1 during the fade-in period
        if (normalizedPosition >= fadeRatio) return 1.0f;
        t = normalizedPosition / fadeRatio;
    }
    else
    {
        // Fade-out: t goes from 1 to 0 during the fade-out period
        float fadeOutStart = 1.0f - fadeRatio;
        if (normalizedPosition <= fadeOutStart) return 1.0f;
        t = 1.0f - ((normalizedPosition - fadeOutStart) / fadeRatio);
    }

    t = jlimit(0.0f, 1.0f, t);

    // Apply curve shaping
    switch (curve)
    {
        case FadeCurve::Linear:
            return t;

        case FadeCurve::Exponential:
            return t * t; // Quadratic (starts slow, ends fast)

        case FadeCurve::Logarithmic:
            return std::sqrt(t); // Square root (starts fast, ends slow)

        case FadeCurve::SCurve:
            // Smooth S-curve using cosine
            return 0.5f * (1.0f - std::cos(t * MathConstants<float>::pi));

        default:
            return t;
    }
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
    currentSampleRate = getSampleRate();

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
    setOnsetDirection(cfg.direction);
    setPlayMode(cfg.playMode);

    // NEW: Set up fade parameters from config
    setFadeIn(cfg.fadeInTime, cfg.fadeInCurve);
    setFadeOut(cfg.fadeOutTime, cfg.fadeOutCurve);

    // TEST: Hardcoded fade values (remove this later)
    // setFadeIn(1.f, FadeCurve::SCurve);    // 0.5 second S-curve fade-in
    // setFadeOut(1.0f, FadeCurve::Linear);   // 1.0 second linear fade-out

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
    firstRender    = false;  // <— NEW: we'll drive from samplePosition directly

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

    DBG("Voice started with " + String(fadeInTime, 2) + "s fade-in, " + String(fadeOutTime, 2) + "s fade-out");
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

    // Apply fade curves
    if (currentNoteNumber >= 0 && isVoiceActive() && sourceBuffer != nullptr)
    {
        const double span = std::max(1.0, endSample - startSample);

        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
        {
            float* channelData = outputBuffer.getWritePointer(channel, startSampleOut);

            for (int sample = 0; sample < numSamples; ++sample)
            {
                // Calculate current position (0.0 to 1.0 across the active span)
                double currentPos = samplePosition + sample;
                double posInSpan = std::clamp(currentPos - startSample, 0.0, span);
                float normalizedPos = (float)(posInSpan / span);

                // Calculate fade multipliers
                float fadeInMult = calculateFadeMultiplier(normalizedPos, fadeInTime, fadeInCurve, true);
                float fadeOutMult = calculateFadeMultiplier(normalizedPos, fadeOutTime, fadeOutCurve, false);

                // Apply both fades (multiplicative)
                float totalFade = fadeInMult * fadeOutMult * gain;
                channelData[sample] *= totalFade;

                // Debug occasionally
                static int debugCounter = 0;
                if (++debugCounter % 5000 == 0 && currentNoteNumber == 36) // Pad 1 only
                {
                    DBG("Pos: " + String(normalizedPos, 3) +
                        " FadeIn: " + String(fadeInMult, 3) +
                        " FadeOut: " + String(fadeOutMult, 3));
                }
            }
        }
    }

    // Continue with existing position tracking...
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

    // CRITICAL FIX: Calculate position based on ACTUAL sample rate relationship
    // Don't advance by host samples - advance by the actual playback increment

    // Get the current sample rate from the processor
    double currentHostSampleRate = processor.getSampleRate();

    // Calculate how much we've actually advanced in the source file
    double actualIncrement = (double)numSamples;

    // If we have source sample rate info, adjust the increment
    if (sourceBuffer && currentHostSampleRate > 0.0)
    {
        // Get the source sample rate from the file
        // (This should be stored when the sample is loaded)
        double sourceSampleRate = processor.getSourceSampleRate(padIndex);

        if (sourceSampleRate > 0.0 && sourceSampleRate != currentHostSampleRate)
        {
            // Convert host samples to source samples
            actualIncrement = (double)numSamples * (sourceSampleRate / currentHostSampleRate);
        }
    }

    samplePosition += actualIncrement;

    // Normalize over the active span [startSample, endSample)
    const double span = std::max(1.0, endSample - startSample);
    const double posInSpan = std::clamp(samplePosition - startSample, 0.0, span);
    const float pos = jlimit(0.0f, 1.0f, (float)(posInSpan / span));

    state.currentPosition = pos;
    processor.notifyPlayheadPositionChanged(currentNoteNumber, pos);
}

// ===================== TrackingPreviewSamplerVoice (updated with sample rate fix) =====================

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
            {
                sampleLength = data->getNumSamples();

                // NEW: Get source sample rate from the audio file
                if (auto* cm = processor.getCollectionManager())
                {
                    File audioFile = cm->getSampleFile(currentFreesoundId);
                    if (audioFile.existsAsFile())
                    {
                        AudioFormatManager tempFormatManager;
                        tempFormatManager.registerBasicFormats();

                        std::unique_ptr<AudioFormatReader> reader(tempFormatManager.createReaderFor(audioFile));
                        if (reader)
                        {
                            sourceSampleRate = reader->sampleRate;
                        }
                    }
                }

                // Fallback to default if we couldn't get the source sample rate
                if (sourceSampleRate <= 0.0)
                    sourceSampleRate = 44100.0;
            }

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
        sourceSampleRate = 0.0;  // NEW: Reset source sample rate
    }
    SamplerVoice::stopNote(velocity, allowTailOff);
}

void TrackingPreviewSamplerVoice::renderNextBlock(AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    SamplerVoice::renderNextBlock(outputBuffer, startSample, numSamples);

    if (currentFreesoundId.isNotEmpty() && sampleLength > 0 && isVoiceActive())
    {
        // FIXED: Apply sample rate correction like the main voice
        double currentHostSampleRate = processor.getSampleRate();
        double actualIncrement = (double)numSamples;

        if (currentHostSampleRate > 0.0 && sourceSampleRate > 0.0 && sourceSampleRate != currentHostSampleRate)
        {
            // Convert host samples to source samples
            actualIncrement = (double)numSamples * (sourceSampleRate / currentHostSampleRate);
        }

        samplePosition += actualIncrement;
        float position = (float) (samplePosition / sampleLength);
        processor.notifyPreviewPlayheadPositionChanged(currentFreesoundId, jlimit(0.0f, 1.0f, position));
        if (position >= 0.99f)
            processor.notifyPreviewPlayheadPositionChanged(currentFreesoundId, 1.0f);
    }
}