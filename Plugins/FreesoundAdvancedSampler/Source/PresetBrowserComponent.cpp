#include "PresetBrowserComponent.h"

//==============================================================================
// SlotButton Implementation
//==============================================================================

SlotButton::SlotButton(int index)
    : Button(String(index + 1))
    , slotIndex(index)
    , hasDataFlag(false)
    , isActiveFlag(false)
{
    setSize(20, 20);
    setButtonText(String(index + 1));
}

SlotButton::~SlotButton() {}

void SlotButton::paintButton(Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = getLocalBounds().toFloat();

    // Background color based on state with dramatic alpha variation
    Colour bgColour;
    if (hasDataFlag)
    {
        if (isActiveFlag)
        {
            // Selected slot: full opacity bright mustard gold
            bgColour = Colours::darkgoldenrod.brighter(0.2f).withAlpha(1.0f);
            // DBG("Drawing slot " + String(slotIndex) + " as ACTIVE (bright mustard)");
        }
        else
        {
            // Non-selected slots with data: very low alpha mustard gold
            bgColour = Colours::darkgoldenrod.withAlpha(0.25f);
            // DBG("Drawing slot " + String(slotIndex) + " as INACTIVE (dim mustard)");
        }
    }
    else
    {
        // Empty slots remain grey with low alpha
        bgColour = Colours::grey.withAlpha(0.2f);
        // DBG("Drawing slot " + String(slotIndex) + " as EMPTY (grey)");
    }

    if (shouldDrawButtonAsHighlighted)
        bgColour = bgColour.brighter(0.15f);
    if (shouldDrawButtonAsDown)
        bgColour = bgColour.darker(0.15f);

    g.setColour(bgColour);
    g.fillRoundedRectangle(bounds, 3.0f);

    // Border intensity based on selection
    if (isActiveFlag && hasDataFlag)
    {
        g.setColour(Colours::white.withAlpha(1.0f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 3.0f, 2.0f); // Thick bright border
    }
    else if (hasDataFlag)
    {
        g.setColour(Colours::white.withAlpha(0.4f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 3.0f, 1.0f); // Normal border
    }
    else
    {
        g.setColour(Colours::white.withAlpha(0.2f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 3.0f, 1.0f); // Dim border
    }

    // Text contrast based on selection
    if (isActiveFlag && hasDataFlag)
    {
        g.setColour(Colours::black.withAlpha(0.9f)); // Dark text on bright background
        g.setFont(Font(12.0f, Font::bold)); // Bold for selected
    }
    else if (hasDataFlag)
    {
        g.setColour(Colours::white.withAlpha(0.7f)); // Light text on dim background
        g.setFont(12.0f);
    }
    else
    {
        g.setColour(Colours::darkgrey.withAlpha(0.5f)); // Very dim for empty
        g.setFont(12.0f);
    }

    g.drawText(getButtonText(), bounds.toNearestInt(), Justification::centred);
}

void SlotButton::setHasData(bool hasData)
{
    if (hasDataFlag != hasData)
    {
        hasDataFlag = hasData;
        repaint();
    }
}

void SlotButton::setIsActive(bool isActive)
{
    if (isActiveFlag != isActive)
    {
        isActiveFlag = isActive;
        // DBG("SlotButton " + String(slotIndex) + " setIsActive: " + String(isActive ? "TRUE" : "FALSE"));
        repaint();
    }
}

//==============================================================================
// PresetListItem Implementation
//==============================================================================

PresetListItem::PresetListItem(const PresetInfo& info)
   : presetInfo(info)
{
    setSize(200, 85); // Reduced height from 120 to 85

    // Delete Button - smaller size
    deleteButton.onClick = [this]() {
        if (onDeleteClicked)
            onDeleteClicked(this);
    };
    addAndMakeVisible(deleteButton);

    // Rename Editor - consistent styling with smaller font
    renameEditor.setText(presetInfo.name, dontSendNotification);
    renameEditor.setJustification(Justification::centredLeft);
    renameEditor.setSelectAllWhenFocused(true);
    renameEditor.setFont(Font(20.0f, Font::bold)); // Reduced from 10.0f to 9.0f
    renameEditor.setColour(TextEditor::backgroundColourId, Colour(0xff2A2A2A));
    renameEditor.setColour(TextEditor::textColourId, Colours::white);
    renameEditor.setColour(TextEditor::outlineColourId, Colour(0xff404040));           // Normal border
    renameEditor.setColour(TextEditor::focusedOutlineColourId, Colour(0xff606060));   // Lighter grey when focused
    renameEditor.onReturnKey = [this]() { confirmRename(); };
    addAndMakeVisible(renameEditor);

    // Create slot buttons - smaller size
    for (int i = 0; i < 8; ++i)
    {
        slotButtons[i] = std::make_unique<SlotButton>(i);
        slotButtons[i]->setHasData(presetInfo.slots[i].hasData);
        slotButtons[i]->setIsActive(presetInfo.activeSlot == i);
        slotButtons[i]->setSize(16, 16); // Reduced from 20x20 to 16x16

        slotButtons[i]->onClick = [this, i]() {
            ModifierKeys modifiers = ModifierKeys::getCurrentModifiers();
            handleSlotClicked(i, modifiers);
        };

        addAndMakeVisible(*slotButtons[i]);
    }
}

PresetListItem::~PresetListItem() {}

void PresetListItem::paint(Graphics& g)
{
    auto bounds = getLocalBounds();
    const int lineHeight = bounds.getHeight() / 3;

    // Background
    g.setColour(Colour(0xff2A2A2A).withAlpha(0.0f)); // Slightly transparent dark background
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    // Border - use same color but different thickness for selection
    g.setColour(Colour(0xff404040)); // Same color for both selected and unselected
    g.drawRoundedRectangle(bounds.toFloat().reduced(1), 4.0f, isSelectedState ? 4.0f : 1.0f); // Thicker when selected

    // Line 2: Info
    auto infoLine = bounds.withHeight(lineHeight).translated(0, lineHeight);
    bool hasData = false;
    int totalSamples = 0;
    for (const auto& slot : presetInfo.slots) {
        if (slot.hasData) {
            hasData = true;
            totalSamples += slot.sampleCount;
        }
    }

    g.setFont(Font(10.0f, hasData ? Font::plain : Font::italic));
    g.setColour(hasData ? Colours::white : Colours::grey);
    g.drawText(hasData ? "  " + presetInfo.createdDate
                       : "Empty bank",
               infoLine.reduced(6, 0), Justification::left, true);
}

void PresetListItem::resized()
{
    auto bounds = getLocalBounds();
    const int lineHeight = bounds.getHeight() / 3;

    // Line 1: Name editor and delete button with more margin
    auto topLine = bounds.withHeight(lineHeight);

    // Text editor with more vertical margin and reduced height
    int editorHeight = 16;
    int verticalMargin = (lineHeight - editorHeight) / 2;
    auto editorBounds = topLine.removeFromLeft(bounds.getWidth() - 45).reduced(6, verticalMargin);
    editorBounds.setHeight(editorHeight);
    renameEditor.setBounds(editorBounds);

    // Set font again in resized to ensure it takes effect
    renameEditor.setFont(Font(10.0f, Font::bold)); // Try smaller size
    renameEditor.setIndents(8, (renameEditor.getHeight() - renameEditor.getTextHeight()) / 2);

    // Delete button with more margin
    deleteButton.setBounds(topLine.reduced(10, 8));

    // Line 3: Centered slot buttons
    auto slotsBounds = bounds.withHeight(lineHeight).translated(0, lineHeight*2);

    const int slotSize = 16;
    const int totalSlotsWidth = (slotSize * 8) + (2 * 7);
    const int startX = (bounds.getWidth() - totalSlotsWidth) / 2;

    for (size_t i = 0; i < slotButtons.size(); ++i) {
        if (slotButtons[i]) {
            int x = startX + static_cast<int>(i) * (slotSize + 2);
            slotButtons[i]->setBounds(x, slotsBounds.getY(), slotSize, slotSize);
        }
    }
}

void PresetListItem::mouseDown(const MouseEvent& event)
{
    // if (onItemClicked)
    //     onItemClicked(this);
}

void PresetListItem::mouseDoubleClick(const MouseEvent& event)
{
    // if (onItemDoubleClicked)
    //     onItemDoubleClicked(this);
}

void PresetListItem::setSelected(bool selected)
{
    if (isSelectedState != selected)
    {
        isSelectedState = selected;
        repaint();
    }
}

void PresetListItem::confirmRename()
{
    const String newName = renameEditor.getText().trim();

    if (newName.isEmpty() || newName == presetInfo.name || !onRenameConfirmed)
        return;

    MessageManager::callAsync([info = presetInfo, newName, cb = onRenameConfirmed]() {
        cb(info, newName);
    });
}

void PresetListItem::handleSlotClicked(int slotIndex, const ModifierKeys& modifiers)
{
    bool hasData = presetInfo.slots[slotIndex].hasData;

    // DBG("Slot " + String(slotIndex) + " clicked, hasData: " + String(hasData ? "YES" : "NO"));

    if (modifiers.isShiftDown() && !modifiers.isCommandDown())
    {
        // Shift+click: Save to slot
        if (hasData)
        {
            AlertWindow::showOkCancelBox(
                AlertWindow::QuestionIcon,
                "Overwrite Slot",
                "Slot " + String(slotIndex + 1) + " already has data. Overwrite?",
                "Yes", "No",
                nullptr,
                ModalCallbackFunction::create([this, slotIndex](int result) {
                    if (result == 1 && onSaveSlotClicked)
                        onSaveSlotClicked(presetInfo, slotIndex);
                })
            );
        }
        else
        {
            if (onSaveSlotClicked)
                onSaveSlotClicked(presetInfo, slotIndex);
        }
    }
    else if (modifiers.isCommandDown() && modifiers.isShiftDown())
    {
        // Cmd+Shift+click: Delete slot
        if (hasData)
        {
            AlertWindow::showOkCancelBox(
                AlertWindow::QuestionIcon,
                "Delete Slot",
                "Are you sure you want to delete slot " + String(slotIndex + 1) + "?",
                "Yes", "No",
                nullptr,
                ModalCallbackFunction::create([this, slotIndex](int result) {
                    if (result == 1 && onDeleteSlotClicked)
                        onDeleteSlotClicked(presetInfo, slotIndex);
                })
            );
        }
    }
    else
    {
        // Regular click: Load slot - let the browser handle the visual update
        // DBG("Regular click on slot " + String(slotIndex));

        if (hasData)
        {
            // DON'T call updateActiveSlot here - let handleLoadSlotClicked do it
            // updateActiveSlot(slotIndex);  // REMOVE THIS LINE

            if (onLoadSlotClicked)
                onLoadSlotClicked(presetInfo, slotIndex);
        }
        else
        {
            // DBG("Slot has no data, not loading");
        }
    }
}

void PresetListItem::updateActiveSlot(int slotIndex)
{
    // DBG("PresetListItem::updateActiveSlot called with slotIndex: " + String(slotIndex));

    // Update the stored preset info
    presetInfo.activeSlot = slotIndex;

    // Update all slot button states
    for (int i = 0; i < 8; ++i)
    {
        if (slotButtons[i])
        {
            bool shouldBeActive = (i == slotIndex);
            // DBG("Setting slot " + String(i) + " active state to: " + String(shouldBeActive ? "TRUE" : "FALSE"));
            slotButtons[i]->setIsActive(shouldBeActive);
        }
    }

    // Force a complete repaint
    repaint();

    // Also force repaint of each slot button individually
    for (int i = 0; i < 8; ++i)
    {
        if (slotButtons[i])
        {
            slotButtons[i]->repaint();
        }
    }
}

void PresetListItem::refreshSlotStates(const PresetInfo& updatedInfo)
{
    presetInfo = updatedInfo;

    // Update all slot buttons with new info
    for (int i = 0; i < 8; ++i)
    {
        if (slotButtons[i])
        {
            slotButtons[i]->setHasData(presetInfo.slots[i].hasData);
            slotButtons[i]->setIsActive(presetInfo.activeSlot == i);
        }
    }

    repaint();
}

//==============================================================================
// PresetBrowserComponent Implementation
//==============================================================================

PresetBrowserComponent::PresetBrowserComponent()
    : processor(nullptr)
{
    // Dark styling for title
    titleLabel.setText("Presets", dontSendNotification);
    titleLabel.setFont(Font(14.0f, Font::bold));
    titleLabel.setJustificationType(Justification::centred);
    titleLabel.setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(titleLabel);

    // Dark viewport styling
    presetViewport.setViewedComponent(&presetListContainer, false);
    presetViewport.setScrollBarsShown(true, false);
    presetViewport.setColour(ScrollBar::backgroundColourId, Colour(0xff2A2A2A));
    presetViewport.setColour(ScrollBar::thumbColourId, Colour(0xff555555));
    addAndMakeVisible(presetViewport);

    // Dark rename editor
    renameEditor.setVisible(false);
    renameEditor.setColour(TextEditor::backgroundColourId, Colour(0xff2A2A2A));
    renameEditor.setColour(TextEditor::textColourId, Colours::white);
    renameEditor.setColour(TextEditor::outlineColourId, Colour(0xff404040));           // Normal border
    renameEditor.setColour(TextEditor::focusedOutlineColourId, Colour(0xff606060));   // Lighter grey when focused
    renameEditor.onReturnKey = [this]() { hideInlineRenameEditor(true); };
    renameEditor.onEscapeKey = [this]() { hideInlineRenameEditor(false); };
    renameEditor.onFocusLost = [this]() { hideInlineRenameEditor(true); };
    addAndMakeVisible(renameEditor);

    // New add bank button at bottom
    addBankButton.onClick = [this]() {
        createNewPresetBank();
        shouldHighlightFirstSlot = true;
        repaint();
    };
    addAndMakeVisible(addBankButton);
}

PresetBrowserComponent::~PresetBrowserComponent() {}

void PresetBrowserComponent::paint(Graphics& g)
{
    // Dark preset browser background
    g.setColour(Colour(0xff1A1A1A));
    g.fillAll();

    // Modern border
    g.setColour(Colour(0xff404040).withAlpha(0.0f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2), 6.0f, 1.5f);
}

void PresetBrowserComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    titleLabel.setBounds(bounds.removeFromTop(10));
    bounds.removeFromTop(10);

    // Preset viewport takes most space
    presetViewport.setBounds(bounds.removeFromTop(bounds.getHeight() - 40));

    // Add button at bottom (full width)
    addBankButton.setBounds(bounds.withHeight(30));

    // No need to call updatePresetList() or force resized() on items
    // since we're using fixed widths now
}

void PresetBrowserComponent::createNewPresetBank()
{
    if (!processor) return;

    String newName = "Bank " + Time::getCurrentTime().formatted("%Y-%m-%d %H.%M");

    // Create empty array for pad infos
    Array<PadInfo> emptyPadInfos;

    if (processor->getPresetManager().saveCurrentPreset(
        newName, "New empty bank", emptyPadInfos, "", 0))
    {
        refreshPresetList();

        // Reset highlight after delay
        Timer::callAfterDelay(1000, [this]() {
            shouldHighlightFirstSlot = false;
            repaint();
        });
    }
}

void PresetBrowserComponent::setProcessor(FreesoundAdvancedSamplerAudioProcessor* p)
{
    processor = p;

    // Ensure the processor is valid and directories exist
    if (processor) {

        // Delay the refresh slightly to ensure UI is ready
        MessageManager::callAsync([this]() {
            refreshPresetList();
        });
    }
}

void PresetBrowserComponent::refreshPresetList()
{
    if (!processor)
        return;

    presetItems.clear();
    selectedItem = nullptr;

    Array<PresetInfo> presets = processor->getPresetManager().getAvailablePresets();

    // Get current active preset info
    File activePresetFile = processor->getPresetManager().getActivePresetFile();
    int activeSlotIndex = processor->getPresetManager().getActiveSlotIndex();

    int yPosition = 0;
    const int itemHeight = 85;
    const int spacing = 3;
    const int fixedItemWidth = 180;

    for (const auto& presetInfo : presets)
    {
        auto* item = new PresetListItem(presetInfo);

        // Set up callbacks...
        item->onItemClicked = [this](PresetListItem* clickedItem) {
            handleItemClicked(clickedItem);
        };

        item->onItemDoubleClicked = [this](PresetListItem* clickedItem) {
            handleItemDoubleClicked(clickedItem);
        };

        item->onDeleteClicked = [this](PresetListItem* clickedItem) {
            handleDeleteClicked(clickedItem);
        };

        item->onRenameConfirmed = [this](const PresetInfo& originalInfo, const String& newName) {
            performRename(originalInfo, newName);
        };

        item->onLoadSlotClicked = [this](const PresetInfo& info, int slotIndex) {
            handleLoadSlotClicked(info, slotIndex);
        };

        item->onSaveSlotClicked = [this](const PresetInfo& info, int slotIndex) {
            handleSaveSlotClicked(info, slotIndex);
        };

        item->onDeleteSlotClicked = [this](const PresetInfo& info, int slotIndex) {
            handleDeleteSlotClicked(info, slotIndex);
        };

        // Check if this is the currently active preset and set active slot
        if (presetInfo.presetFile == activePresetFile && activeSlotIndex >= 0)
        {
            item->updateActiveSlot(activeSlotIndex);
            selectedItem = item;
            item->setSelected(true);
        }

        item->setBounds(5, yPosition, fixedItemWidth, itemHeight);
        presetListContainer.addAndMakeVisible(*item);
        presetItems.add(item);

        yPosition += itemHeight + spacing;
    }

    presetListContainer.setSize(fixedItemWidth + 10, yPosition);
    presetViewport.getViewedComponent()->repaint();
}

void PresetBrowserComponent::updatePresetList()
{
    // FIXED: Use same width as refreshPresetList
    const int fixedItemWidth = 180;

    for (int i = 0; i < presetItems.size(); ++i)
    {
        auto* item = presetItems[i];
        if (item != nullptr)
        {
            auto bounds = item->getBounds();
            bounds.setWidth(fixedItemWidth);
            item->setBounds(bounds);
        }
    }

    int totalHeight = 0;
    if (presetItems.size() > 0 && presetItems.getLast() != nullptr)
        totalHeight = presetItems.getLast()->getBottom() + 5;

    // FIXED: Container width matches
    presetListContainer.setSize(fixedItemWidth + 10, totalHeight);
}

void PresetBrowserComponent::handleItemClicked(PresetListItem* item)
{
    if (selectedItem && selectedItem != item)
        selectedItem->setSelected(false);

    selectedItem = item;
    item->setSelected(true);
}

void PresetBrowserComponent::handleItemDoubleClicked(PresetListItem* item)
{
    // Load first available slot
    const auto& presetInfo = item->getPresetInfo();
    for (int i = 0; i < 8; ++i)
    {
        if (presetInfo.slots[i].hasData)
        {
            handleLoadSlotClicked(presetInfo, i);
            break;
        }
    }
}

void PresetBrowserComponent::handleLoadSlotClicked(const PresetInfo& presetInfo, int slotIndex)
{
    // Update active slot tracking across all presets
    updateActiveSlotAcrossPresets(presetInfo, slotIndex);

    if (onPresetLoadRequested)
        onPresetLoadRequested(presetInfo, slotIndex);
}

void PresetBrowserComponent::updateActiveSlotAcrossPresets(const PresetInfo& activePresetInfo, int activeSlotIndex)
{
    // DBG("updateActiveSlotAcrossPresets called for slot " + String(activeSlotIndex));

    // Find and update only the target preset
    for (auto* item : presetItems)
    {
        if (item && item->getPresetInfo().presetFile == activePresetInfo.presetFile)
        {
            // DBG("Found matching preset, setting active slot to " + String(activeSlotIndex));
            item->updateActiveSlot(activeSlotIndex);
            handleItemClicked(item); // Also select the preset item
            break;
        }
    }
}

void PresetBrowserComponent::handleSaveSlotClicked(const PresetInfo& presetInfo, int slotIndex)
{
    if (!processor)
        return;

    Array<PadInfo> padInfos = processor->getCurrentPadInfos();
    String description = "Saved to slot " + String(slotIndex + 1);

    if (processor->getPresetManager().saveToSlot(presetInfo.presetFile, slotIndex, description, padInfos, processor->getQuery()))
    {
        refreshPresetList();
        AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon,
            "Slot Saved", "Saved to slot " + String(slotIndex + 1));
    }
    else
    {
        AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
            "Save Failed", "Failed to save to slot " + String(slotIndex + 1));
    }
}

void PresetBrowserComponent::handleDeleteSlotClicked(const PresetInfo& presetInfo, int slotIndex)
{
    if (!processor)
        return;

    // Check if this is the last remaining slot with data
    bool hasOtherSlotsWithData = false;
    for (int i = 0; i < 8; ++i) {
        if (i != slotIndex && presetInfo.slots[i].hasData) {
            hasOtherSlotsWithData = true;
            break;
        }
    }

    if (!hasOtherSlotsWithData) {
        // This is the only slot with data - ask if they want to delete the whole preset
        AlertWindow::showOkCancelBox(
            AlertWindow::QuestionIcon,
            "Delete Last Slot",
            "This is the last slot with data. Delete the entire preset instead?",
            "Delete Preset",
            "Cancel",
            nullptr,
            ModalCallbackFunction::create([this, presetInfo](int result) {
                if (result == 1) { // User clicked "Delete Preset"
                    processor->getPresetManager().deletePreset(presetInfo.presetFile);
                    refreshPresetList();
                }
            })
        );
    }
    else {
        // There are other slots with data - proceed with normal slot deletion
        if (processor->getPresetManager().deleteSlot(presetInfo.presetFile, slotIndex)) {
            refreshPresetList();
        }
    }
}

void PresetBrowserComponent::handleDeleteClicked(PresetListItem* item)
{
    AlertWindow::showOkCancelBox(AlertWindow::QuestionIcon,
        "Delete Preset", "Are you sure you want to delete \"" + item->getPresetInfo().name + "\"?",
        "Yes", "No", this,
        ModalCallbackFunction::create([this, item](int result) {
            if (result == 1 && processor)
            {
                processor->getPresetManager().deletePreset(item->getPresetInfo().presetFile);
                refreshPresetList();
            }
        }));
}

void PresetBrowserComponent::handleRenameClicked(PresetListItem* item)
{
    showInlineRenameEditor(item);
}

void PresetBrowserComponent::showInlineRenameEditor(PresetListItem* item)
{
    renamingItem = item;

    auto itemBounds = item->getBounds();
    auto globalTop = item->getY() + presetListContainer.getY();
    auto editorBounds = Rectangle<int>(itemBounds.getX() + 10, globalTop - 28, itemBounds.getWidth() - 20, 24);

    renameEditor.setBounds(editorBounds);
    renameEditor.setText(item->getPresetInfo().name, dontSendNotification);
    renameEditor.setVisible(true);
    renameEditor.grabKeyboardFocus();
    renameEditor.selectAll();
}

void PresetBrowserComponent::hideInlineRenameEditor(bool applyRename)
{
    renameEditor.setVisible(false);

    if (applyRename && renamingItem)
    {
        String newName = renameEditor.getText().trim();
        if (newName.isNotEmpty() && newName != renamingItem->getPresetInfo().name)
        {
            performRename(renamingItem->getPresetInfo(), newName);
        }
    }

    renamingItem = nullptr;
}

void PresetBrowserComponent::performRename(const PresetInfo& presetInfo, const String& newName)
{
    if (newName.isEmpty() || newName == presetInfo.name || !processor)
        return;

    Array<PresetInfo> existingPresets = processor->getPresetManager().getAvailablePresets();
    for (const auto& preset : existingPresets)
    {
        if (preset.name == newName && preset.presetFile != presetInfo.presetFile)
        {
            AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
                "Name Exists", "A preset with the name \"" + newName + "\" already exists.");
            return;
        }
    }

    // Load the existing preset data completely
    String jsonText = presetInfo.presetFile.loadFileAsString();
    var parsedJson = JSON::parse(jsonText);

    if (parsedJson.isObject())
    {
        auto* rootObject = parsedJson.getDynamicObject();
        if (rootObject)
        {
            // Update the preset info name
            var presetInfoVar = parsedJson.getProperty("preset_info", var());
            if (presetInfoVar.isObject())
            {
                auto* presetInfoObj = presetInfoVar.getDynamicObject();
                if (presetInfoObj)
                {
                    presetInfoObj->setProperty("name", newName);
                }
            }

            // Create new file with sanitized name
            String newFileName = processor->getPresetManager().sanitizeFileName(newName) + ".json";
            File newPresetFile = processor->getPresetManager().getPresetsFolder().getChildFile(newFileName);

            // Save to new file
            String newJsonString = JSON::toString(parsedJson, true);
            if (newPresetFile.replaceWithText(newJsonString))
            {
                // Delete old file
                presetInfo.presetFile.deleteFile();
                refreshPresetList();
            }
            else
            {
                AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
                    "Rename Failed", "Failed to save preset with new name.");
            }
        }
    }
}

void PresetBrowserComponent::saveCurrentPreset()
{
    if (!processor)
        return;

    String suggestedName = processor->getPresetManager().generatePresetName(processor->getQuery());

    if (processor->saveCurrentAsPreset(suggestedName, "Saved from grid interface"))
    {
        refreshPresetList();
        AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon,
            "Preset Saved", "Preset saved as \"" + suggestedName + "\" in slot 1");
    }
    else
    {
        AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
            "Save Failed", "Failed to save preset.");
    }
}

void PresetBrowserComponent::scrollBarMoved(ScrollBar*, double) {}
