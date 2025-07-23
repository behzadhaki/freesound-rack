/*
==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "PluginProcessor.h"
#include "FreesoundSearchComponent.h"
#include "SampleGridComponent.h"

//==============================================================================
/**
*/
class FreesoundAdvancedSamplerAudioProcessorEditor  : public AudioProcessorEditor,
                                                  public FreesoundAdvancedSamplerAudioProcessor::DownloadListener
{
public:
    FreesoundAdvancedSamplerAudioProcessorEditor (FreesoundAdvancedSamplerAudioProcessor&);
    ~FreesoundAdvancedSamplerAudioProcessorEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

    // Download listener methods
    void downloadProgressChanged(const AudioDownloadManager::DownloadProgress& progress) override;
    void downloadCompleted(bool success) override;

private:
    FreesoundAdvancedSamplerAudioProcessor& processor;

    FreesoundSearchComponent freesoundSearchComponent;
    SampleGridComponent sampleGridComponent; // NEW: Replace results table with grid

    // Store sounds for grid update
    Array<FSSound> currentSounds;

    // Progress bar components
    ProgressBar progressBar;
    Label statusLabel;
    TextButton cancelButton;
    double currentProgress = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreesoundAdvancedSamplerAudioProcessorEditor)
};