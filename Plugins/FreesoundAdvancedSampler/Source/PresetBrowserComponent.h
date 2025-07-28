/*
  ==============================================================================

    PresetBrowserComponent.h
    Created: Preset browser and management UI
    Author: Generated

  ==============================================================================
*/

#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "PluginProcessor.h"
#include "PresetManager.h"

//==============================================================================
// Save Preset Dialog
//==============================================================================
class SavePresetDialog : public Component
{
public:
    SavePresetDialog();
    ~SavePresetDialog() override;

    void paint(Graphics& g) override;
    void resized() override;

    // Show modal dialog and return true if user saved
    bool showDialog(const String& suggestedName = "");

    String getPresetName() const { return presetNameEditor.getText(); }
    String getPresetDescription() const { return descriptionEditor.getText(); }

private:
    Label presetNameLabel;
    TextEditor presetNameEditor;
    Label descriptionLabel;
    TextEditor descriptionEditor;
    TextButton saveButton;
    TextButton cancelButton;

    bool userClickedSave = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SavePresetDialog)
};

//==============================================================================
// Preset List Item
//==============================================================================
class PresetListItem : public Component
{
public:
    PresetListItem(const PresetInfo& info);
    ~PresetListItem() override;

    void paint(Graphics& g) override;
    void resized() override;
    void mouseDown(const MouseEvent& event) override;
    void mouseDoubleClick(const MouseEvent& event) override;

    const PresetInfo& getPresetInfo() const { return presetInfo; }

    void setSelected(bool selected);
    bool isSelected() const { return isSelectedState; }

    // Callback for when this item is clicked
    std::function<void(PresetListItem*)> onItemClicked;
    std::function<void(PresetListItem*)> onItemDoubleClicked;
    std::function<void(PresetListItem*)> onDeleteClicked;

private:
    PresetInfo presetInfo;
    bool isSelectedState = false;
    TextButton deleteButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetListItem)
};

//==============================================================================
// Main Preset Browser Component
//==============================================================================
class PresetBrowserComponent : public Component,
                              public ScrollBar::Listener
{
public:
    PresetBrowserComponent();
    ~PresetBrowserComponent() override;

    void paint(Graphics& g) override;
    void resized() override;

    void setProcessor(FreesoundAdvancedSamplerAudioProcessor* p);
    void refreshPresetList();

    // Callback for when a preset is selected for loading
    std::function<void(const PresetInfo&)> onPresetLoadRequested;

    // ScrollBar::Listener
    void scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;

private:
    FreesoundAdvancedSamplerAudioProcessor* processor;

    // UI Components
    Label titleLabel;
    TextButton savePresetButton;
    TextButton refreshButton;

    // Preset list
    Viewport presetViewport;
    Component presetListContainer;
    OwnedArray<PresetListItem> presetItems; // Changed from Array<std::unique_ptr<PresetListItem>>

    // Currently selected item
    PresetListItem* selectedItem = nullptr;

    void handleItemClicked(PresetListItem* item);
    void handleItemDoubleClicked(PresetListItem* item);
    void handleDeleteClicked(PresetListItem* item);
    void saveCurrentPreset();
    void updatePresetList();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserComponent)
};