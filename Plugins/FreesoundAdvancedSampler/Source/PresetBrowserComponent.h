#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "PluginProcessor.h"
#include "PresetManager.h"
#include "CustomButtonStyle.h"

//==============================================================================
// Slot Button Component
//==============================================================================
class SlotButton : public Button
{
public:
    SlotButton(int slotIndex);
    ~SlotButton() override;

    void paintButton(Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void setHasData(bool hasData);
    void setIsActive(bool isActive);
    bool hasData() const { return hasDataFlag; }
    int getSlotIndex() const { return slotIndex; }

private:
    int slotIndex;
    bool hasDataFlag;
    bool isActiveFlag;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SlotButton)
};

//==============================================================================
// Preset List Item with Slots
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
    std::function<void(const PresetInfo&, int)> onLoadSlotClicked;
    std::function<void(const PresetInfo&, int)> onSaveSlotClicked;
    std::function<void(const PresetInfo&, int)> onDeleteSlotClicked;

private:
    PresetInfo presetInfo;
    bool isSelectedState = false;

    StyledButton deleteButton {"DEL", 8.0f, true};
    TextEditor renameEditor;
    std::array<std::unique_ptr<SlotButton>, 8> slotButtons;

    void confirmRename();
    void handleSlotClicked(int slotIndex, const ModifierKeys& modifiers);

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

    std::function<void(const PresetInfo&, int)> onPresetLoadRequested;

    void scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;

    void createNewPresetBank();

private:
    FreesoundAdvancedSamplerAudioProcessor* processor;

    Label titleLabel;
    StyledButton addBankButton { "+ New Bank", 10.0f };
    bool shouldHighlightFirstSlot = false;

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
    void handleLoadSlotClicked(const PresetInfo& presetInfo, int slotIndex);
    void handleSaveSlotClicked(const PresetInfo& presetInfo, int slotIndex);
    void handleDeleteSlotClicked(const PresetInfo& presetInfo, int slotIndex);

    void showInlineRenameEditor(PresetListItem* item);
    void hideInlineRenameEditor(bool applyRename);

    void performRename(const PresetInfo& presetInfo, const String& newName);
    void saveCurrentPreset();
    void updatePresetList();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserComponent)
};