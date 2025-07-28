#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "PluginProcessor.h"
#include "PresetManager.h"

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

    std::function<void(PresetListItem*)> onItemClicked;
    std::function<void(PresetListItem*)> onItemDoubleClicked;
    std::function<void(PresetListItem*)> onDeleteClicked;
    std::function<void(const PresetInfo&, const String&)> onRenameConfirmed;
    std::function<void(const PresetInfo&)> onLoadClicked;

private:
    PresetInfo presetInfo;
    bool isSelectedState = false;

    TextButton deleteButton;
    TextButton loadButton;
    TextEditor renameEditor;

    void confirmRename();

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

    std::function<void(const PresetInfo&)> onPresetLoadRequested;

    void scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;

private:
    FreesoundAdvancedSamplerAudioProcessor* processor;

    Label titleLabel;
    TextButton savePresetButton;
    TextButton refreshButton;

    Viewport presetViewport;
    Component presetListContainer;
    OwnedArray<PresetListItem> presetItems;

    PresetListItem* selectedItem = nullptr;

    TextEditor renameEditor;
    PresetListItem* renamingItem = nullptr;

    void handleItemClicked(PresetListItem* item);
    void handleItemDoubleClicked(PresetListItem* item);
    void handleDeleteClicked(PresetListItem* item);
    void handleRenameClicked(PresetListItem* item);

    void showInlineRenameEditor(PresetListItem* item);
    void hideInlineRenameEditor(bool applyRename);

    void performRename(const PresetInfo& presetInfo, const String& newName);
    void saveCurrentPreset();
    void updatePresetList();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserComponent)
};
