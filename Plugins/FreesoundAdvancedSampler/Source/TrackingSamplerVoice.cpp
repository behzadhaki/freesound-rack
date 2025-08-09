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

// Fixed renderNextBlock implementation for TrackingSamplerVoice
void TrackingSamplerVoice::renderNextBlock(AudioBuffer<float>& outputBuffer,
                                          int startSampleOut,
                                          int numSamples)
{
    // CRITICAL: Do NOT call base class renderNextBlock - we're replacing it entirely
    // SamplerVoice::renderNextBlock(outputBuffer, startSampleOut, numSamples); // REMOVE THIS LINE

    // Check if we should be playing
    if (!isVoiceActive() || getCurrentlyPlayingSound() == nullptr)
        return;

    auto* sound = dynamic_cast<SamplerSound*>(getCurrentlyPlayingSound().get());
    if (!sound || !sourceBuffer)
        return;

    // Calculate the actual playback range
    double actualStartSample = startSample;
    double actualEndSample = (endSample < 0) ? sampleLength : endSample;

    // Ensure valid range
    if (actualEndSample <= actualStartSample)
    {
        actualEndSample = sampleLength;
    }

    const double span = std::max(1.0, actualEndSample - actualStartSample);

    // Get pad index for sample rate calculation
    const int padIndex = currentNoteNumber - 36;

    // Get the correct sample rate ratio
    double sourceSampleRate = 44100.0; // Default fallback
    if (padIndex >= 0 && padIndex < 16)
    {
        sourceSampleRate = processor.getSourceSampleRate(padIndex);
        if (sourceSampleRate <= 0.0) sourceSampleRate = 44100.0;
    }

    // Calculate the sample rate ratio for proper playback speed
    double sampleRateRatio = sourceSampleRate / currentSampleRate;

    // Process audio with our custom parameters
    for (int i = 0; i < numSamples; ++i)
    {
        // Check boundaries based on play mode
        bool shouldStop = false;

        if (playMode == PlayMode::Normal)
        {
            // Normal mode - stop at boundaries
            if (playDirection > 0 && samplePosition >= actualEndSample - 1)
            {
                shouldStop = true;
            }
            else if (playDirection < 0 && samplePosition <= actualStartSample)
            {
                shouldStop = true;
            }
        }
        else if (playMode == PlayMode::Loop)
        {
            // Loop mode - wrap around
            if (playDirection > 0 && samplePosition >= actualEndSample - 1)
            {
                samplePosition = actualStartSample;
            }
            else if (playDirection < 0 && samplePosition <= actualStartSample)
            {
                samplePosition = actualEndSample - 1.0;
            }
        }
        else if (playMode == PlayMode::PingPong)
        {
            // Ping-pong mode - reverse direction at boundaries
            if (samplePosition >= actualEndSample - 1 && playDirection > 0)
            {
                playDirection = -1;
                samplePosition = actualEndSample - 2.0;
            }
            else if (samplePosition <= actualStartSample && playDirection < 0)
            {
                playDirection = 1;
                samplePosition = actualStartSample + 1.0;
            }
        }

        if (shouldStop)
        {
            clearCurrentNote();
            break;
        }

        // Calculate normalized position for fades
        double posInSpan = std::clamp(samplePosition - actualStartSample, 0.0, span);
        float normalizedPos = (float)(posInSpan / span);

        // Calculate fade multipliers
        float fadeInMult = calculateFadeMultiplier(normalizedPos, fadeInTime, fadeInCurve, true);
        float fadeOutMult = calculateFadeMultiplier(normalizedPos, fadeOutTime, fadeOutCurve, false);
        float totalFade = fadeInMult * fadeOutMult * gain;

        // Get sample value with interpolation
        int sourceSampleIndex = (int)samplePosition;
        float fraction = (float)(samplePosition - sourceSampleIndex);

        // Clamp to valid range
        sourceSampleIndex = jlimit(0, (int)sampleLength - 2, sourceSampleIndex);

        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
        {
            int sourceChannel = channel % sourceChannels;

            // Linear interpolation between samples
            float sample1 = sourceBuffer->getSample(sourceChannel, sourceSampleIndex);
            float sample2 = sourceBuffer->getSample(sourceChannel, sourceSampleIndex + 1);
            float interpolatedSample = sample1 + fraction * (sample2 - sample1);

            // Apply total gain and fades
            interpolatedSample *= totalFade;

            // Add to output buffer (not addSample - we want to set it)
            float* channelData = outputBuffer.getWritePointer(channel, startSampleOut);
            channelData[i] += interpolatedSample;
        }

        // Advance position based on pitch shift AND sample rate ratio
        double pitchRatio = std::pow(2.0, pitchShiftSemitones / 12.0);

        // Combined ratio: pitch shift * sample rate correction
        double combinedRatio = pitchRatio * sampleRateRatio;

        samplePosition += playDirection * combinedRatio;

        // Clamp position to valid range
        samplePosition = jlimit(0.0, sampleLength - 1.0, samplePosition);
    }

    // Update playhead tracking for UI
    if (currentNoteNumber >= 0 && sampleLength > 0.0)
    {
        // padIndex already declared above
        if (padIndex >= 0 && padIndex < 16)
        {
            auto& state = processor.padPlaybackStates[padIndex];

            // Check if voice has finished
            if (!isVoiceActive())
            {
                state.isPlaying = false;
                state.currentPosition = 0.0f;
                processor.notifyPlayheadPositionChanged(currentNoteNumber, 0.0f);
                processor.notifyNoteStopped(currentNoteNumber);
                currentNoteNumber = -1;
                samplePosition = 0.0;
                firstRender = true;
            }
            else if (noteHeld)  // Only update position if note is held
            {
                // Calculate normalized position for UI
                const double posInSpan = std::clamp(samplePosition - actualStartSample, 0.0, span);
                const float pos = jlimit(0.0f, 1.0f, (float)(posInSpan / span));

                state.currentPosition = pos;
                processor.notifyPlayheadPositionChanged(currentNoteNumber, pos);
            }
        }
    }
}

// Updated startNote to match the actual parameters
void TrackingSamplerVoice::startNote(int midiNoteNumber,
                                    float velocity,
                                    SynthesiserSound* sound,
                                    int currentPitchWheelPosition)
{
    // Call base class to set up basic voice state
    // But we'll override the rendering
    SamplerVoice::startNote(midiNoteNumber, velocity, sound, currentPitchWheelPosition);

    noteHeld = true;
    currentNoteNumber = midiNoteNumber;
    currentSampleRate = getSampleRate();

    // Reset/capture source info
    sourceBuffer = nullptr;
    sourceChannels = 0;
    sampleLength = 0.0;

    if (auto* samplerSound = dynamic_cast<SamplerSound*>(sound))
    {
        if (auto* data = samplerSound->getAudioData())
        {
            sourceBuffer = data;
            sourceChannels = sourceBuffer->getNumChannels();
            sampleLength = sourceBuffer->getNumSamples();
        }
    }

    if (sourceBuffer == nullptr || sampleLength <= 0.0)
        return;

    // Apply per-pad config
    const int padIdx = midiNoteNumber - 36;
    const auto& cfg = processor.getPadPlaybackConfig(padIdx);

    // Normalize values if needed
    auto normToSamps = [this](double n) -> double
    {
        if (sampleLength > 0.0)
            return juce::jlimit(0.0, sampleLength - 1.0, n * sampleLength);
        return 0.0;
    };

    double startS = cfg.startSample;
    double endS = cfg.endSample;

    const bool startLooksNorm = (startS >= 0.0 && startS <= 1.0);
    const bool endLooksNorm = (endS >= 0.0 && endS <= 1.0);

    if (startLooksNorm && (endS < 0.0 || endLooksNorm))
    {
        startS = normToSamps(startS);
        endS = (endS < 0.0 ? sampleLength : normToSamps(endS));
    }
    else
    {
        startS = juce::jlimit(0.0, sampleLength - 1.0, startS);
        if (endS < 0.0) endS = sampleLength;
        endS = juce::jlimit(0.0, sampleLength, endS);
    }

    if (endS <= startS)
    {
        endS = sampleLength;
        startS = 0.0;
    }

    startSample = startS;
    endSample = endS;

    // Handle temporary start
    if (cfg.tempStartArmed)
    {
        const bool tempLooksNorm = (cfg.tempStartSample >= 0.0 && cfg.tempStartSample <= 1.0);
        tempStartSample = tempLooksNorm ? normToSamps(cfg.tempStartSample)
                                        : juce::jlimit(0.0, sampleLength - 1.0, cfg.tempStartSample);
        tempStartArmed = true;
    }
    else
    {
        tempStartArmed = false;
        tempStartSample = 0.0;
    }

    // Apply all parameters (no stretchRatio)
    pitchShiftSemitones = cfg.pitchShiftSemitones;   // working
    gain = cfg.gain;            // working
    onsetDirection = cfg.direction;
    playMode = cfg.playMode;  // all working BUT they always work in a non gated manner. Onset triggers playback indefinitely
    fadeInTime = cfg.fadeInTime;
    fadeInCurve = cfg.fadeInCurve;
    fadeOutTime = cfg.fadeOutTime;
    fadeOutCurve = cfg.fadeOutCurve;

    // Set initial direction based on onset direction
    playDirection = (cfg.direction == Direction::Forward) ? 1 : -1;

    // Set initial position based on direction and start point
    double chosenStart = startSample;

    if (tempStartArmed)
    {
        chosenStart = juce::jlimit(startSample, endSample - 1.0, tempStartSample);
        tempStartArmed = false; // one-shot consumed
    }
    else if (playDirection < 0)
    {
        // For reverse playback, start from the end - 1
        chosenStart = endSample - 1.0;
    }
    else
    {
        // For forward playback, start from the configured start
        chosenStart = startSample;
    }

    samplePosition = juce::jlimit(0.0, sampleLength - 1.0, chosenStart);
    firstRender = false;

    // Update pad state
    if (padIdx >= 0 && padIdx < 16)
    {
        auto& state = processor.padPlaybackStates[padIdx];
        state.isPlaying = true;
        state.noteNumber = midiNoteNumber;
        state.sampleLength = sampleLength;
        state.velocity = velocity;
        state.currentPosition = 0.0f;
    }

    processor.notifyNoteStarted(midiNoteNumber, velocity);

    DBG("Voice started - Direction: " + String(playDirection) +
        ", Start: " + String(samplePosition) + " (startSample: " + String(startSample) + ")" +
        ", Range: " + String(startSample) + " to " + String(endSample) +
        ", Mode: " + String((int)playMode) +
        ", Pitch: " + String(pitchShiftSemitones, 1) + " semitones" +
        ", Gain: " + String(gain, 2));
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