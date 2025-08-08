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
    // Let the base class do its bookkeeping (voice active, etc.)
    SamplerVoice::startNote(midiNoteNumber, velocity, sound, currentPitchWheelPosition);

    currentNoteNumber = midiNoteNumber;

    // Reset/capture source info
    sourceBuffer   = nullptr;
    sourceChannels = 0;
    sampleLength   = 0.0;

    if (auto* samplerSound = dynamic_cast<SamplerSound*>(sound))
    {
        // Your SamplerSound subclass should expose raw audio via getAudioData()
        if (auto* data = samplerSound->getAudioData())
        {
            sourceBuffer   = data;
            sourceChannels = sourceBuffer->getNumChannels();
            sampleLength   = sourceBuffer->getNumSamples();
        }
    }

    // Safety: if no buffer available, just bail (base class will treat as silent)
    if (sourceBuffer == nullptr || sampleLength <= 0.0)
        return;

    // ===== Pull per-pad config from the processor =====
    const int padIdx = midiNoteNumber - 36;
    const auto& cfg  = processor.getPadPlaybackConfig(padIdx);

    auto normToSamps = [this](double n) -> double
    {
        if (sampleLength > 0.0)
            return juce::jlimit(0.0, sampleLength, n * sampleLength);
        return 0.0;
    };

    // Start/end (support normalized [0..1] OR absolute samples)
    double startS = cfg.startSample;
    double endS   = cfg.endSample;         // -1 means "full length"

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
        tempStartArmed  = true; // we'll consume it below when we pick the initial position
    }
    else
    {
        tempStartArmed = false;
        tempStartSample = 0.0;
    }

    // Signal & transport setup
    setPitchShiftSemitones(cfg.pitchShiftSemitones);
    setStretchRatio(cfg.stretchRatio);
    setGain(cfg.gain);
    setADSR(cfg.adsr);
    setOnsetDirection(cfg.direction);
    setPlayMode(cfg.playMode);

    // ===== Decide initial direction and starting position =====
    playDirection = (onsetDirection == Direction::Forward ? +1 : -1);

    // Choose start index: temporary start (if armed) wins for the first trigger
    double chosenStart = startSample;
    if (tempStartArmed)
    {
        chosenStart  = juce::jlimit(startSample, endSample, tempStartSample);
        tempStartArmed = false; // one-shot consumed
    }
    else if (playDirection < 0)
    {
        // Reverse onset with no temp start: begin at the end
        chosenStart = juce::jlimit(startSample, endSample, endSample - 1.0);
    }

    samplePosition = juce::jlimit(startSample, juce::jmax(startSample, endSample - 1.0), chosenStart);
    firstRender    = true;

    // ===== ADSR =====
    adsr.setSampleRate(getSampleRate());
    adsr.setParameters(adsrParams);
    adsr.noteOn();

    // ===== Playback state & UI notifications (preserve your behavior) =====
    bool isContinuation = false;

    if (padIdx >= 0 && padIdx < 16)
    {
        auto& state = processor.padPlaybackStates[padIdx];

        // If pad was already playing and has a stored position, continue from there (normalized over full file)
        if (state.isPlaying && state.currentPosition > 0.0f && sampleLength > 0.0)
        {
            samplePosition = juce::jlimit(0.0, sampleLength, (double)state.currentPosition * sampleLength);
            isContinuation = true;
        }

        state.isPlaying    = true;
        state.noteNumber   = midiNoteNumber;
        state.sampleLength = sampleLength;
        state.velocity     = velocity;
        if (!isContinuation)
            state.currentPosition = 0.0f; // start-of-file normalized
    }

    processor.notifyNoteStarted(midiNoteNumber, velocity);
}

void TrackingSamplerVoice::stopNote(float velocity, bool allowTailOff)
{
    if (currentNoteNumber >= 0)
    {
        // Immediate UI state clear (preserves your current semantics)
        const int padIndex = currentNoteNumber - 36;
        if (padIndex >= 0 && padIndex < 16)
        {
            auto& state = processor.padPlaybackStates[padIndex];
            state.isPlaying = false;
            state.currentPosition = 0.0f;
            state.currentFreesoundId = "";
        }

        // Envelope tail or hard stop
        if (allowTailOff)
        {
            adsr.noteOff();
        }
        else
        {
            processor.notifyNoteStopped(currentNoteNumber);
            currentNoteNumber = -1;
            samplePosition = 0.0;
            firstRender = true;
            clearCurrentNote();
        }
    }

    SamplerVoice::stopNote(velocity, allowTailOff);
}

void TrackingSamplerVoice::renderNextBlock(AudioBuffer<float>& outputBuffer, int startSampleOut, int numSamples)
{
    if (currentNoteNumber < 0 || sourceBuffer == nullptr || sampleLength <= 0.0)
        return;

    // Render loop with naive linear interpolation, multi-channel aware.
    const int outChans = outputBuffer.getNumChannels();
    const int srcChans = jmax(1, sourceChannels);
    const float overallGain = gain;

    const double incBase = currentIncrement();
    const double startIdx = startSample;
    const double endIdx   = endSample; // exclusive boundary

    auto advance = [&](double& pos, int& dir)
    {
        pos += (incBase * dir);
    };

    auto hitForwardEnd = [&](double pos) { return pos >= (endIdx - 1.0); };
    auto hitReverseEnd = [&](double pos) { return pos <= (startIdx);     };

    for (int i = 0; i < numSamples; ++i)
    {
        // If ADSR is finished after a prior noteOff, stop the voice.
        if (! adsr.isActive())
        {
            processor.notifyPlayheadPositionChanged(currentNoteNumber, 1.0f);
            processor.notifyNoteStopped(currentNoteNumber);
            currentNoteNumber = -1;
            firstRender = true;
            clearCurrentNote();
            return;
        }

        // Boundary handling BEFORE reading if we'd interpolate past edges
        if (playDirection > 0 && hitForwardEnd(samplePosition))
        {
            if (playMode == PlayMode::Normal)
            {
                adsr.noteOff();
                // render silence for remaining samples (tail handled by ADSR next calls)
            }
            else if (playMode == PlayMode::Loop)
            {
                samplePosition = startIdx;
            }
            else // PingPong
            {
                playDirection = -1;
                samplePosition = jlimit(startIdx, endIdx - 1.0, samplePosition);
            }
        }
        else if (playDirection < 0 && hitReverseEnd(samplePosition))
        {
            if (playMode == PlayMode::Normal)
            {
                adsr.noteOff();
            }
            else if (playMode == PlayMode::Loop)
            {
                samplePosition = endIdx - 1.0;
            }
            else // PingPong
            {
                playDirection = +1;
                samplePosition = jlimit(startIdx, endIdx - 1.0, samplePosition);
            }
        }

        // Safe index + frac
        int idx = (int) jlimit(0.0, sampleLength - 1.0, std::floor(samplePosition));
        const double frac = jlimit(0.0, 1.0, samplePosition - (double)idx);

        // Read & interpolate each channel
        const float env = adsr.getNextSample();
        const float g   = overallGain * env;

        for (int ch = 0; ch < outChans; ++ch)
        {
            const int srcCh = (ch < srcChans ? ch : srcChans - 1);
            const float* src = sourceBuffer->getReadPointer(srcCh);

            const int idx2 = jmin(idx + 1, (int)sampleLength - 1);
            const float s0 = src[idx];
            const float s1 = src[idx2];
            const float s  = s0 + (float)((s1 - s0) * frac);

            outputBuffer.addSample(ch, startSampleOut + i, s * g);
        }

        // Advance
        advance(samplePosition, playDirection);
    }

    // Update UI tracking (normalized over full sample for continuity with your UI)
    if (currentNoteNumber >= 0)
    {
        const int padIndex = currentNoteNumber - 36;
        const float normPos = (float) jlimit(0.0, 1.0, samplePosition / sampleLength);

        if (padIndex >= 0 && padIndex < 16)
        {
            auto& st = processor.padPlaybackStates[padIndex];
            st.currentPosition = normPos;
        }

        processor.notifyPlayheadPositionChanged(currentNoteNumber, normPos);

        // Fire NoteOff when ADSR just ended (handled at top of next block too)
        if (!adsr.isActive())
        {
            processor.notifyNoteStopped(currentNoteNumber);
            currentNoteNumber = -1;
            firstRender = true;
            clearCurrentNote();
        }
    }
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
