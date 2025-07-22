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


//==============================================================================
/**
*/
class FreesoundSimpleSamplerAudioProcessorEditor  : public AudioProcessorEditor,
                                                  public FreesoundSimpleSamplerAudioProcessor::DownloadListener // Add this
{
public:
    FreesoundSimpleSamplerAudioProcessorEditor (FreesoundSimpleSamplerAudioProcessor&);
    ~FreesoundSimpleSamplerAudioProcessorEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

    // Add download listener methods
    void downloadProgressChanged(const AudioDownloadManager::DownloadProgress& progress) override;
    void downloadCompleted(bool success) override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    FreesoundSimpleSamplerAudioProcessor& processor;

    FreesoundSearchComponent freesoundSearchComponent;

    // Add progress bar components
    ProgressBar progressBar;
    Label statusLabel;
    TextButton cancelButton;
    double currentProgress = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreesoundSimpleSamplerAudioProcessorEditor)
};