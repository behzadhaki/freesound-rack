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
#include "PresetBrowserComponent.h"
#include "PresetManager.h"

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

    bool saveCurrentAsPreset(const String& name, const String& description = "", int slotIndex = 0);
    bool loadPreset(const File& presetFile, int slotIndex = 0);
    bool saveToSlot(const File& presetFile, int slotIndex, const String& description = "");

    SampleGridComponent& getSampleGridComponent() { return sampleGridComponent; }

private:
    FreesoundAdvancedSamplerAudioProcessor& processor;

    FreesoundSearchComponent freesoundSearchComponent;
    SampleGridComponent sampleGridComponent;
    SampleDragArea sampleDragArea;
    DirectoryOpenButton directoryOpenButton;
    PresetBrowserComponent presetBrowserComponent;

    // Store sounds for grid update
    Array<FSSound> currentSounds;

    // Progress bar components
    ProgressBar progressBar;
    Label statusLabel;
    TextButton cancelButton;
    double currentProgress = 0.0;

    // Handle preset loading with slot support
    void handlePresetLoadRequested(const PresetInfo& presetInfo, int slotIndex);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreesoundAdvancedSamplerAudioProcessorEditor)
};