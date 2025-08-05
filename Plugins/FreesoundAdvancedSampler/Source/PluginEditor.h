/*
==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "PluginProcessor.h"
#include "SampleGridComponent.h"
#include "PresetBrowserComponent.h"
#include "PresetManager.h"
#include "ExpandablePanel.h"
#include "CustomLookAndFeel.h"
#include "BookmarkViewerComponent.h"

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
    bool keyPressed(const KeyPress& key) override;

    void updateWindowSizeForBookmarkPanel();

private:

    FreesoundAdvancedSamplerAudioProcessor& processor;

    std::unique_ptr<CustomLookAndFeel> customLookAndFeel;

    SampleGridComponent sampleGridComponent;
    SampleDragArea sampleDragArea;
    DirectoryOpenButton directoryOpenButton;

    // Preset browser now inside expandable panel
    PresetBrowserComponent presetBrowserComponent;
    ExpandablePanel expandablePanelComponent;

    // Store sounds for grid update
    Array<FSSound> currentSounds;

    // Handle preset loading with slot support
    void handlePresetLoadRequested(const PresetInfo& presetInfo, int slotIndex);
    void loadPresetNormally(const PresetInfo& presetInfo, int slotIndex); // NEW helper method

    // Method for expandable panel on the right side
    void updateWindowSizeForPanelState();

    // Add to the private section after the existing expandable panel
    ExpandablePanel bookmarkExpandablePanel;
    BookmarkViewerComponent bookmarkViewerComponent;

    void updateSizeConstraintsForCurrentPanelStates();

    int getKeyboardPadIndex(const KeyPress& key) const;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreesoundAdvancedSamplerAudioProcessorEditor)
};