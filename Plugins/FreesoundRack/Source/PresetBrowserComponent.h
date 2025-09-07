#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "PluginProcessor.h"
#include "SampleCollectionManager.h"
#include "CustomButtonStyle.h"
#include "FreesoundKeys.h"

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
// Preset List Item Info (keeping this structure for UI compatibility)
//==============================================================================
struct PresetListItemInfo
{
    String presetId;
    String name;
    String description;
    String createdDate;
    std::array<bool, 8> slotHasData;
    int activeSlot = -1;

    PresetListItemInfo()
    {
        slotHasData.fill(false);
    }
};

//==============================================================================
// Preset List Item with Slots
//==============================================================================
class PresetListItem : public Component
{
public:
    PresetListItem(const PresetListItemInfo& info);
    ~PresetListItem() override;

    void paint(Graphics& g) override;
    void resized() override;

    void mouseDown(const MouseEvent& event) override;
    void mouseDoubleClick(const MouseEvent& event) override;

    const PresetListItemInfo& getPresetListItemInfo() const { return itemInfo; }
    void setSelected(bool selected);
    bool isSelected() const { return isSelectedState; }

    // Consistent callback signatures using String presetId
    std::function<void(const String&, int)> onLoadSlotClicked;      // presetId, slotIndex
    std::function<void(const String&, int)> onSaveSlotClicked;      // presetId, slotIndex
    std::function<void(const String&, int)> onDeleteSlotClicked;    // presetId, slotIndex
    std::function<void(const String&)> onDeleteClicked;             // presetId
    std::function<void(const String&, const String&)> onRenameConfirmed; // presetId, newName

    std::function<void(PresetListItem*)> onItemClicked;
    std::function<void(PresetListItem*)> onItemDoubleClicked;
    std::function<void(PresetListItem*)> onSampleCheckClicked;

    const String& getPresetId() const { return itemInfo.presetId; }
    void updateActiveSlot(int slotIndex);
    void refreshSlotStates(const PresetListItemInfo& updatedInfo);

private:
    PresetListItemInfo itemInfo;
    bool isSelectedState = false;

    StyledButton deleteButton {String(CharPointer_UTF8("\xF0\x9F\x97\x91")), 8.0f, true};
    StyledButton sampleCheckButton {String(CharPointer_UTF8("\xE2\xAC\x87")), 8.0f, false};

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

    // Updated callback signature to use String presetId
    std::function<void(const String&, int)> onPresetLoadRequested; // presetId, slotIndex

    void scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;
    void createNewPresetBank();
    void restoreActiveState();

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

    // Updated method signatures to use String presetId instead of PresetListItemInfo
    void handleItemClicked(PresetListItem* item);
    void handleItemDoubleClicked(PresetListItem* item);
    void handleDeleteClicked(PresetListItem* item);
    void handleRenameClicked(PresetListItem* item);
    void handleLoadSlotClicked(const String& presetId, int slotIndex);
    void handleSaveSlotClicked(const String& presetId, int slotIndex);
    void handleDeleteSlotClicked(const String& presetId, int slotIndex);

    void showInlineRenameEditor(PresetListItem* item);
    void hideInlineRenameEditor(bool applyRename);

    void performRename(const String& presetId, const String& newName);
    void saveCurrentPreset();
    void updatePresetList();

    void updateActiveSlotAcrossPresets(const String& presetId, int activeSlotIndex);
    void performSaveToSlot(const String& presetId, int slotIndex, const String& description);

    void handleSampleCheckClicked(PresetListItem* item);
    void downloadMissingSamples(const Array<SampleMetadata>& missingSamples);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserComponent)
};