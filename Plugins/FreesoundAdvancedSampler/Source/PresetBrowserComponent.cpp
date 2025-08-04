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
    if (hasDataFlag)
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
    setSize(200, 85);

    // Sample Check Button
    sampleCheckButton.onClick = [this]() {
        if (onSampleCheckClicked)
            onSampleCheckClicked(this);
    };
    addAndMakeVisible(sampleCheckButton);

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

    // Line 1: Name editor, sample check button, and delete button with more margin
    auto topLine = bounds.withHeight(lineHeight);

    // Delete button
    auto deleteBounds = topLine.removeFromRight(30).reduced(5, 8);
    deleteButton.setBounds(deleteBounds);

    // Sample Check button (leftmost)
    topLine.removeFromRight(15); // Leave space for delete button
    auto sampleCheckBounds = topLine.removeFromRight(30).reduced(5, 8);
    sampleCheckButton.setBounds(sampleCheckBounds);

    // Text editor with remaining space
    int editorHeight = 16;
    int verticalMargin = (lineHeight - editorHeight) / 2;
    auto editorBounds = topLine.reduced(6, verticalMargin);
    editorBounds.setHeight(editorHeight);
    renameEditor.setBounds(editorBounds);

    // Set font again in resized to ensure it takes effect
    renameEditor.setFont(Font(10.0f, Font::bold));
    renameEditor.setIndents(8, (renameEditor.getHeight() - renameEditor.getTextHeight()) / 2);

    // Line 3: Centered slot buttons (unchanged)
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
        if (onSaveSlotClicked)
            onSaveSlotClicked(presetInfo, slotIndex);
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

        item->onSampleCheckClicked = [this](PresetListItem* clickedItem) {
            handleSampleCheckClicked(clickedItem);
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

void PresetBrowserComponent::performSaveToSlot(const PresetInfo& presetInfo, int slotIndex,
                                              const Array<PadInfo>& padInfos, const String& description,
                                              const String& query)
{
    if (processor->getPresetManager().saveToSlot(presetInfo.presetFile, slotIndex, description, padInfos, query))
    {
        refreshPresetList();
    }
    else
    {
        AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
            "Save Failed", "Failed to save to slot " + String(slotIndex + 1));
    }
}

bool PresetBrowserComponent::isContentDifferent(const Array<PadInfo>& newPadInfos,
                                               const Array<PadInfo>& existingPadInfos,
                                               const String& newQuery,
                                               const PresetInfo& presetInfo,
                                               int slotIndex) const
{
    // First check if queries are different
    String existingQuery = presetInfo.slots[slotIndex].searchQuery;
    if (newQuery != existingQuery)
    {
        // DBG("Query different: '" + newQuery + "' vs '" + existingQuery + "'");
        return true;
    }

    // Check if number of samples is different
    if (newPadInfos.size() != existingPadInfos.size())
    {
        // DBG("Sample count different: " + String(newPadInfos.size()) + " vs " + String(existingPadInfos.size()));
        return true;
    }

    // Create maps for easy lookup by pad index
    std::map<int, const PadInfo*> newPadMap;
    std::map<int, const PadInfo*> existingPadMap;

    // Fill the maps
    for (const auto& pad : newPadInfos)
    {
        newPadMap[pad.padIndex] = &pad;
    }

    for (const auto& pad : existingPadInfos)
    {
        existingPadMap[pad.padIndex] = &pad;
    }

    // Compare all 16 possible pad positions (0-15)
    for (int padIndex = 0; padIndex < 16; ++padIndex)
    {
        bool newHasPad = newPadMap.find(padIndex) != newPadMap.end();
        bool existingHasPad = existingPadMap.find(padIndex) != existingPadMap.end();

        // Check if one has a sample at this position and the other doesn't
        if (newHasPad != existingHasPad)
        {
            return true;
        }

        // If both have samples at this position, compare them
        if (newHasPad && existingHasPad)
        {
            const PadInfo* newPad = newPadMap[padIndex];
            const PadInfo* existingPad = existingPadMap[padIndex];

            if (newPad->freesoundId != existingPad->freesoundId)
            {
                // DBG("Freesound ID different at pad " + String(padIndex) + ": '" + newPad->freesoundId + "' vs '" + existingPad->freesoundId + "'");
                return true;
            }

            if (newPad->originalName != existingPad->originalName)
            {
                // DBG("Sample name different at pad " + String(padIndex) + ": '" + newPad->originalName + "' vs '" + existingPad->originalName + "'");
                return true;
            }

            if (newPad->author != existingPad->author)
            {
                // DBG("Author different at pad " + String(padIndex) + ": '" + newPad->author + "' vs '" + existingPad->author + "'");
                return true;
            }

            if (newPad->searchQuery != existingPad->searchQuery)
            {
                // DBG("Search query different at pad " + String(padIndex) + ": '" + newPad->searchQuery + "' vs '" + existingPad->searchQuery + "'");
                return true;
            }
        }
    }

    // DBG("Content is identical - no overwrite warning needed");
    return false; // Content is identical
}

void PresetBrowserComponent::handleSaveSlotClicked(const PresetInfo& presetInfo, int slotIndex)
{
    if (!processor)
        return;

    // Get current pad infos that we want to save
    Array<PadInfo> newPadInfos = processor->getCurrentPadInfos();
    String newDescription = "Saved to slot " + String(slotIndex + 1);
    String newQuery = processor->getQuery();

    // Check if slot already has data
    if (presetInfo.slots[slotIndex].hasData)
    {
        // Load existing data from the slot to compare
        Array<PadInfo> existingPadInfos;
        if (processor->getPresetManager().loadPreset(presetInfo.presetFile, slotIndex, existingPadInfos))
        {
            // Compare the content
            bool contentIsDifferent = isContentDifferent(newPadInfos, existingPadInfos, newQuery, presetInfo, slotIndex);

            if (contentIsDifferent)
            {
                // Show warning dialog
                AlertWindow::showOkCancelBox(
                    AlertWindow::QuestionIcon,
                    "Overwrite Slot",
                    "Slot " + String(slotIndex + 1) + " contains different content. Do you want to overwrite it?",
                    "Yes, Overwrite", "No, Cancel",
                    nullptr,
                    ModalCallbackFunction::create([this, presetInfo, slotIndex, newPadInfos, newDescription, newQuery](int result) {
                        if (result == 1) // User clicked "Yes, Overwrite"
                        {
                            performSaveToSlot(presetInfo, slotIndex, newPadInfos, newDescription, newQuery);
                        }
                        // If result == 0, user clicked "No, Cancel" - do nothing
                    })
                );
                return; // Exit early, let the callback handle the save
            }
            // If content is the same, fall through to save without warning
        }
    }

    // Save directly (either slot is empty or content is identical)
    performSaveToSlot(presetInfo, slotIndex, newPadInfos, newDescription, newQuery);
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

void PresetBrowserComponent::restoreActiveState()
{
    if (!processor)
        return;

    File activePresetFile = processor->getPresetManager().getActivePresetFile();
    int activeSlotIndex = processor->getPresetManager().getActiveSlotIndex();

    if (!activePresetFile.existsAsFile() || activeSlotIndex < 0)
        return;

    // DBG("Restoring active state: " + activePresetFile.getFileName() + ", slot " + String(activeSlotIndex));

    // Find the matching preset item and update its active slot
    for (auto* item : presetItems)
    {
        if (item && item->getPresetInfo().presetFile == activePresetFile)
        {
            item->updateActiveSlot(activeSlotIndex);
            handleItemClicked(item); // Also select the preset item
            break;
        }
    }
}

void PresetBrowserComponent::handleSampleCheckClicked(PresetListItem* item)
{
    if (!processor)
        return;

    const auto& presetInfo = item->getPresetInfo();

    // Collect all unique sample IDs from all slots
    StringArray allUniqueSampleIds;  // Track unique sample IDs
    Array<PadInfo> allMissingPadInfos;
    StringArray uniqueMissingSampleIds;  // Track unique missing IDs

    for (int slotIndex = 0; slotIndex < 8; ++slotIndex)
    {
        if (!presetInfo.slots[slotIndex].hasData)
            continue;

        Array<PadInfo> slotPadInfos;
        if (processor->getPresetManager().loadPreset(presetInfo.presetFile, slotIndex, slotPadInfos))
        {
            for (const auto& padInfo : slotPadInfos)
            {
                // Add to unique samples list
                allUniqueSampleIds.addIfNotAlreadyThere(padInfo.freesoundId);

                File sampleFile = processor->getPresetManager().getSampleFile(padInfo.freesoundId);
                if (!sampleFile.existsAsFile())
                {
                    // For missing samples, add unique IDs for download
                    uniqueMissingSampleIds.addIfNotAlreadyThere(padInfo.freesoundId);
                    allMissingPadInfos.add(padInfo);
                }
            }
        }
    }

    int totalUniqueSamples = allUniqueSampleIds.size();

    if (totalUniqueSamples == 0)
    {
        AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon,
            "No Samples", "This preset bank contains no samples.");
        return;
    }

    int totalMissingUniqueSamples = uniqueMissingSampleIds.size();

    if (totalMissingUniqueSamples == 0)
    {
        AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon,
            "All Samples Available",
            "All " + String(totalUniqueSamples) + " samples are available in the resources folder.");
        return;
    }

    // Some samples are missing
    String message;
    if (totalMissingUniqueSamples == totalUniqueSamples)
    {
        message = "All " + String(totalUniqueSamples) + " samples are missing from the resources folder.\n\n";
    }
    else
    {
        message = String(totalMissingUniqueSamples) + " of " + String(totalUniqueSamples) +
                 " samples are missing from the resources folder.\n\n";
    }

    message += "Do you want to download the missing samples?";

    AlertWindow::showOkCancelBox(
        AlertWindow::QuestionIcon,
        "Missing Samples",
        message,
        "Yes, Download", "No",
        nullptr,
        ModalCallbackFunction::create([this, allMissingPadInfos](int result) {
            if (result == 1) { // User clicked "Yes, Download"
                downloadMissingSamples(allMissingPadInfos);
            }
        })
    );
}

void PresetBrowserComponent::downloadMissingSamples(const Array<PadInfo>& missingPadInfos)
{
    if (!processor || missingPadInfos.isEmpty())
        return;

    // Get unique sample IDs to avoid downloading the same sample multiple times
    StringArray uniqueMissingIds;
    for (const auto& padInfo : missingPadInfos)
    {
        uniqueMissingIds.addIfNotAlreadyThere(padInfo.freesoundId);
    }

    // We need to fetch the missing sounds from Freesound API to get complete data including previews
    Array<FSSound> soundsToDownload;
    FreesoundClient client(FREESOUND_API_KEY);

    for (const String& freesoundId : uniqueMissingIds)
    {
        try
        {
            // Fetch the complete sound data from Freesound API including previews
            FSSound sound = client.getSound(freesoundId, "id,name,username,license,previews,duration,filesize");

            if (!sound.id.isEmpty())
            {
                soundsToDownload.add(sound);
            }
            else
            {
                DBG("Failed to fetch sound data for ID: " + freesoundId);
            }
        }
        catch (const std::exception& e)
        {
            DBG("Error fetching sound " + freesoundId + ": " + String(e.what()));
        }
    }

    if (soundsToDownload.isEmpty())
    {
        AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
            "Download Failed",
            "Could not retrieve sound information from Freesound. Please check your internet connection.");
        return;
    }

    // Start download using processor's download system
    processor->getDownloadManager().startDownloads(
        soundsToDownload,
        processor->getPresetManager().getSamplesFolder(),
        "Missing samples download"
    );

    // Show progress notification
    AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon,
        "Download Started",
        "Downloading " + String(soundsToDownload.size()) + " unique samples...");
}
