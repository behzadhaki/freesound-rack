/*
  ==============================================================================

    TrackingSamplerVoice.cpp
    Created: Enhanced sampler voices with playback tracking and preview support
    Author: Generated

  ==============================================================================
*/

#include "TrackingSamplerVoice.h"
#include "PluginProcessor.h"

//==============================================================================
// TrackingSamplerVoice Implementation
//==============================================================================

TrackingSamplerVoice::TrackingSamplerVoice(FreesoundAdvancedSamplerAudioProcessor& owner)
    : processor(owner)
{
}

void TrackingSamplerVoice::startNote(int midiNoteNumber, float velocity, SynthesiserSound* sound, int currentPitchWheelPosition)
{
    SamplerVoice::startNote(midiNoteNumber, velocity, sound, currentPitchWheelPosition);

    currentNoteNumber = midiNoteNumber;
    int padIndex = midiNoteNumber - 36;
    firstRender = true;  // Reset first render flag

    // Get sample length from the sound
    if (auto* samplerSound = dynamic_cast<SamplerSound*>(sound))
    {
        sampleLength = samplerSound->getAudioData()->getNumSamples();
    }

    // Check if this is a continuation from a previous sample
    bool isContinuation = false;
    if (padIndex >= 0 && padIndex < 16)
    {
        const auto& state = processor.padPlaybackStates[padIndex];
        if (state.isPlaying && state.currentPosition > 0.0f)
        {
            // This is a continuation - start our position tracking from where we left off
            samplePosition = state.currentPosition * sampleLength;
            isContinuation = true;
        }
        else
        {
            samplePosition = 0.0;
        }

        // Update processor tracking
        processor.padPlaybackStates[padIndex].isPlaying = true;
        processor.padPlaybackStates[padIndex].noteNumber = midiNoteNumber;
        processor.padPlaybackStates[padIndex].sampleLength = sampleLength;
        processor.padPlaybackStates[padIndex].velocity = velocity;

        if (!isContinuation)
        {
            processor.padPlaybackStates[padIndex].currentPosition = 0.0f;
        }
    }

    processor.notifyNoteStarted(midiNoteNumber, velocity);
}

void TrackingSamplerVoice::stopNote(float velocity, bool allowTailOff)
{
    if (currentNoteNumber >= 0)
    {
        int padIndex = currentNoteNumber - 36;
        if (padIndex >= 0 && padIndex < 16)
        {
            // Clear state immediately when note is explicitly stopped
            auto& state = processor.padPlaybackStates[padIndex];
            state.isPlaying = false;
            state.currentPosition = 0.0f;
            state.currentFreesoundId = "";
        }

        processor.notifyNoteStopped(currentNoteNumber);
        currentNoteNumber = -1;
        samplePosition = 0.0;
        firstRender = true; // Reset for next time
    }

    SamplerVoice::stopNote(velocity, allowTailOff);
}

void TrackingSamplerVoice::renderNextBlock(AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    // Check if we need to force reload the current sound (for sample swapping)
    if (forceReload && currentNoteNumber >= 0)
    {
        forceReload = false;

        // Get the new sound for this note from the processor
        if (auto* newSound = processor.getSamplerSoundForNote(currentNoteNumber))
        {
            // Force this voice to switch to the new sound
            // We do this by stopping and immediately restarting with the new sound
            int noteToRestart = currentNoteNumber;
            float velocityToUse = processor.padPlaybackStates[noteToRestart - 36].velocity;

            // Stop current playback (but don't reset state)
            SamplerVoice::stopNote(0.0f, false);

            // Immediately restart with the new sound
            SamplerVoice::startNote(noteToRestart, velocityToUse, newSound, 0);

            // Update sample length from new sound
            sampleLength = newSound->getAudioData()->getNumSamples();

            // The position will be restored from the stored state
        }
    }

    SamplerVoice::renderNextBlock(outputBuffer, startSample, numSamples);

    // Update playhead position
    if (currentNoteNumber >= 0 && sampleLength > 0)
    {
        // For continuation, we adjust our position tracking to create smooth visual transition
        int padIndex = currentNoteNumber - 36;
        if (padIndex >= 0 && padIndex < 16)
        {
            auto& state = processor.padPlaybackStates[padIndex];

            // If this voice just started but we have a saved position (continuation case)
            if (firstRender && state.currentPosition > 0.0f)
            {
                // Start from the saved position for visual continuity
                samplePosition = state.currentPosition * sampleLength;
                firstRender = false;
            }
            else
            {
                // Normal increment
                samplePosition += numSamples;
                firstRender = false;
            }

            float position = (float)samplePosition / (float)sampleLength;

            // Check if we've reached or passed the end
            if (position >= 1.0f)
            {
                // Sample has finished - trigger note off and reset
                processor.addNoteOffToMidiBuffer(currentNoteNumber);

                // Reset state
                state.isPlaying = false;
                state.currentPosition = 0.0f;

                // Send final position update
                processor.notifyPlayheadPositionChanged(currentNoteNumber, 1.0f);
                processor.notifyNoteStopped(currentNoteNumber);

                // Clear our tracking
                currentNoteNumber = -1;
                samplePosition = 0.0;

                return; // Don't process further
            }

            // Update processor tracking
            state.currentPosition = position;

            processor.notifyPlayheadPositionChanged(currentNoteNumber, jlimit(0.0f, 1.0f, position));
        }
        else
        {
            // Fallback for non-pad notes
            samplePosition += numSamples;
            float position = (float)samplePosition / (float)sampleLength;

            // Check if finished for non-pad notes too
            if (position >= 1.0f)
            {
                processor.notifyPlayheadPositionChanged(currentNoteNumber, 1.0f);
                processor.notifyNoteStopped(currentNoteNumber);
                currentNoteNumber = -1;
                samplePosition = 0.0;
                return;
            }

            processor.notifyPlayheadPositionChanged(currentNoteNumber, jlimit(0.0f, 1.0f, position));
        }
    }
}

void TrackingSamplerVoice::forceReloadCurrentSound()
{
    if (currentNoteNumber >= 0)
    {
        forceReload = true;  // Will be processed in next renderNextBlock call
    }
}

//==============================================================================
// TrackingPreviewSamplerVoice Implementation
//==============================================================================

TrackingPreviewSamplerVoice::TrackingPreviewSamplerVoice(FreesoundAdvancedSamplerAudioProcessor& owner)
    : processor(owner)
{
}

void TrackingPreviewSamplerVoice::startNote(int midiNoteNumber, float velocity, SynthesiserSound* sound, int currentPitchWheelPosition)
{
    // Call parent first
    SamplerVoice::startNote(midiNoteNumber, velocity, sound, currentPitchWheelPosition);

    // Extract freesound ID from the sound name
    if (auto* samplerSound = dynamic_cast<SamplerSound*>(sound))
    {
        String soundName = samplerSound->getName();

        if (soundName.startsWith("preview_"))
        {
            currentFreesoundId = soundName.substring(8); // Remove "preview_" prefix
            samplePosition = 0.0;
            sampleLength = samplerSound->getAudioData()->getNumSamples();

            // Notify that preview started
            processor.notifyPreviewStarted(currentFreesoundId);
        }
        else
        {
            DBG("TrackingPreviewSamplerVoice: Sound name doesn't start with 'preview_', ignoring");
        }
    }
    else
    {
        DBG("TrackingPreviewSamplerVoice: Not a SamplerSound, ignoring");
    }
}

void TrackingPreviewSamplerVoice::stopNote(float velocity, bool allowTailOff)
{
    if (currentFreesoundId.isNotEmpty())
    {
        processor.notifyPreviewStopped(currentFreesoundId);
        currentFreesoundId = "";
        samplePosition = 0.0;
        sampleLength = 0.0;
    }

    SamplerVoice::stopNote(velocity, allowTailOff);
}

void TrackingPreviewSamplerVoice::renderNextBlock(AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    // Call parent first to actually render the audio
    SamplerVoice::renderNextBlock(outputBuffer, startSample, numSamples);

    // Update playhead position for preview
    if (currentFreesoundId.isNotEmpty() && sampleLength > 0 && isVoiceActive())
    {
        samplePosition += numSamples;
        float position = (float)samplePosition / (float)sampleLength;

        // Always send position updates for smooth animation
        processor.notifyPreviewPlayheadPositionChanged(currentFreesoundId, jlimit(0.0f, 1.0f, position));

        // Always send the final position when near the end
        if (position >= 0.99f)
        {
            processor.notifyPreviewPlayheadPositionChanged(currentFreesoundId, 1.0f);
        }
    }
}