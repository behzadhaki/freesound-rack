/*
  ==============================================================================

    TrackingSamplerVoice.h
    Created: Enhanced sampler voices with playback tracking and preview support
    Author: Generated

  ==============================================================================
*/

#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"

using namespace juce;

// Forward declaration
class FreesoundAdvancedSamplerAudioProcessor;

//==============================================================================
// Playback State Structure
//==============================================================================
struct PlaybackState {
    bool isPlaying = false;
    float currentPosition = 0.0f;  // 0.0 to 1.0
    int noteNumber = -1;
    float velocity = 0.0f;
    double sampleLength = 0.0;      // Length of current sample in samples
    String currentFreesoundId = "";  // Track which sample is playing
};

//==============================================================================
// Enhanced Sampler Voice for Main Playback with Position Tracking
//==============================================================================
class TrackingSamplerVoice : public SamplerVoice
{
public:
    TrackingSamplerVoice(FreesoundAdvancedSamplerAudioProcessor& owner);

    void startNote(int midiNoteNumber, float velocity, SynthesiserSound*, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void renderNextBlock(AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    // NEW: Force reload current sound for sample swapping
    void forceReloadCurrentSound();

    // NEW: Check if this voice is playing a specific note
    bool isPlayingNote(int midiNoteNumber) const { return currentNoteNumber == midiNoteNumber; }

private:
    FreesoundAdvancedSamplerAudioProcessor& processor;
    int currentNoteNumber = -1;
    double samplePosition = 0.0;
    double sampleLength = 0.0;
    bool firstRender = true;  // Track first render for continuation logic
    bool forceReload = false; // Flag to trigger sample reload
};

//==============================================================================
// Enhanced Preview Sampler Voice for Preview Playback
//==============================================================================
class TrackingPreviewSamplerVoice : public SamplerVoice
{
public:
    TrackingPreviewSamplerVoice(FreesoundAdvancedSamplerAudioProcessor& owner);

    void startNote(int midiNoteNumber, float velocity, SynthesiserSound*, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void renderNextBlock(AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

private:
    FreesoundAdvancedSamplerAudioProcessor& processor;
    String currentFreesoundId;
    double samplePosition = 0.0;
    double sampleLength = 0.0;
};