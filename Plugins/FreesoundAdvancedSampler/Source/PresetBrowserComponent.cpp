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
        }
        else
        {
            // Non-selected slots with data: very low alpha mustard gold
            bgColour = Colours::darkgoldenrod.withAlpha(0.25f);
        }
    }
    else
    {
        // Empty slots remain grey with low alpha
        bgColour = Colours::grey.withAlpha(0.2f);
    }

    if (shouldDrawButtonAsHighlighted)
        bgColour = bgColour.brighter(0.15f);
    if (shouldDrawButtonAsDown)
        bgColour = bgColour.darker(0.15f);

    g.setColour(bgColour);
    g.fillRoundedRectangle(bounds, 3.0f);

    // Border intensity based on selection
    g.setColour(Colours::white.withAlpha(hasDataFlag ? 0.0f : 0.0f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 3.0f, 1.0f);

    // Text contrast based on selection
    if (isActiveFlag && hasDataFlag)
    {
        g.setColour(Colours::black.withAlpha(0.9f));
        g.setFont(Font(12.0f, Font::bold));
    }
    else if (hasDataFlag)
    {
        g.setColour(Colours::white.withAlpha(0.7f));
        g.setFont(12.0f);
    }
    else
    {
        g.setColour(Colours::darkgrey.withAlpha(0.5f));
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
        repaint();
    }
}

//==============================================================================
// PresetListItem Implementation
//==============================================================================

PresetListItem::PresetListItem(const PresetListItemInfo& info)
   : itemInfo(info)
{
    setSize(200, 85);

    // Sample Check Button
    sampleCheckButton.onClick = [this]() {
        if (onSampleCheckClicked)
            onSampleCheckClicked(this);
    };
    addAndMakeVisible(sampleCheckButton);

    // Delete Button
    deleteButton.onClick = [this]() {
        if (onDeleteClicked)
            onDeleteClicked(itemInfo.presetId);
    };
    addAndMakeVisible(deleteButton);

    // Rename Editor
    renameEditor.setText(itemInfo.name, dontSendNotification);
    renameEditor.setJustification(Justification::centredLeft);
    renameEditor.setSelectAllWhenFocused(true);
    renameEditor.setFont(Font(10.0f, Font::bold));
    renameEditor.setColour(TextEditor::backgroundColourId, Colour(0xff2A2A2A).brighter(0.0f));
    renameEditor.setColour(TextEditor::textColourId, Colours::white);
    renameEditor.setColour(TextEditor::outlineColourId, Colour(0xff404040).withAlpha(0.0f));
    renameEditor.setColour(TextEditor::focusedOutlineColourId, Colour(0xff606060).withAlpha(0.0f));
    renameEditor.onReturnKey = [this]() { confirmRename(); };
    addAndMakeVisible(renameEditor);

    // Create slot buttons
    for (int i = 0; i < 8; ++i)
    {
        slotButtons[i] = std::make_unique<SlotButton>(i);
        slotButtons[i]->setHasData(itemInfo.slotHasData[i]);
        slotButtons[i]->setIsActive(itemInfo.activeSlot == i);
        slotButtons[i]->setSize(16, 16);

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
    g.setColour(Colours::darkgrey.withAlpha(0.2f));
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    // Border
    g.setColour(Colour(0xff404040).withAlpha(0.0f));
    g.drawRoundedRectangle(bounds.toFloat().reduced(1), 6.0f, isSelectedState ? 0.5f : 0.0f);

    // Line 2: Info
    auto infoLine = bounds.withHeight(lineHeight).translated(0, lineHeight);
    bool hasData = false;
    for (bool slotHasData : itemInfo.slotHasData) {
        if (slotHasData) {
            hasData = true;
            break;
        }
    }

    g.setFont(Font(10.0f, hasData ? Font::plain : Font::italic));
    g.setColour(hasData ? Colours::white : Colours::grey);
    g.drawText(hasData ? "  " + itemInfo.createdDate : "Empty bank",
               infoLine.reduced(6, 0), Justification::left, true);
}

void PresetListItem::resized()
{
    auto bounds = getLocalBounds();
    const int lineHeight = bounds.getHeight() / 3;

    // Line 1: Name editor, sample check button, and delete button
    auto topLine = bounds.withHeight(lineHeight);

    // Delete button
    auto deleteBounds = topLine.removeFromRight(30).reduced(5, 8);
    deleteButton.setBounds(deleteBounds);

    // Sample Check button
    topLine.removeFromRight(15);
    auto sampleCheckBounds = topLine.removeFromRight(30).reduced(5, 8);
    sampleCheckButton.setBounds(sampleCheckBounds);

    // Text editor with remaining space
    int editorHeight = 16;
    int verticalMargin = (lineHeight - editorHeight) / 2;
    auto editorBounds = topLine.reduced(6, verticalMargin);
    editorBounds.setHeight(editorHeight);
    renameEditor.setBounds(editorBounds);

    renameEditor.setFont(Font(10.0f, Font::bold));
    renameEditor.setIndents(8, (renameEditor.getHeight() - renameEditor.getTextHeight()) / 2);

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
    if (onItemClicked)
        onItemClicked(this);
}

void PresetListItem::mouseDoubleClick(const MouseEvent& event)
{
    if (onItemDoubleClicked)
        onItemDoubleClicked(this);
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

    if (newName.isEmpty() || newName == itemInfo.name || !onRenameConfirmed)
        return;

    MessageManager::callAsync([presetId = itemInfo.presetId, newName, cb = onRenameConfirmed]() {
        cb(presetId, newName);
    });
}

void PresetListItem::handleSlotClicked(int slotIndex, const ModifierKeys& modifiers)
{
    bool hasData = itemInfo.slotHasData[slotIndex];

    if (modifiers.isShiftDown() && !modifiers.isCommandDown())
    {
        if (onSaveSlotClicked)
            onSaveSlotClicked(itemInfo.presetId, slotIndex);
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
                        onDeleteSlotClicked(itemInfo.presetId, slotIndex);
                })
            );
        }
    }
    else
    {
        // Regular click: Load slot
        if (hasData)
        {
            if (onLoadSlotClicked)
                onLoadSlotClicked(itemInfo.presetId, slotIndex);
        }
    }
}

void PresetListItem::updateActiveSlot(int slotIndex)
{
    // Update the stored preset info
    itemInfo.activeSlot = slotIndex;

    // Update all slot button states
    for (int i = 0; i < 8; ++i)
    {
        if (slotButtons[i])
        {
            bool shouldBeActive = (i == slotIndex);
            slotButtons[i]->setIsActive(shouldBeActive);
        }
    }

    repaint();

    // Force repaint of each slot button
    for (int i = 0; i < 8; ++i)
    {
        if (slotButtons[i])
        {
            slotButtons[i]->repaint();
        }
    }
}

void PresetListItem::refreshSlotStates(const PresetListItemInfo& updatedInfo)
{
    itemInfo = updatedInfo;

    // Update all slot buttons with new info
    for (int i = 0; i < 8; ++i)
    {
        if (slotButtons[i])
        {
            slotButtons[i]->setHasData(itemInfo.slotHasData[i]);
            slotButtons[i]->setIsActive(itemInfo.activeSlot == i);
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
    // Title styling
    titleLabel.setText("Presets", dontSendNotification);
    addAndMakeVisible(titleLabel);

    // Viewport styling
    presetViewport.setViewedComponent(&presetListContainer, false);
    presetViewport.setScrollBarsShown(true, false);
    addAndMakeVisible(presetViewport);

    // Rename editor (hidden by default)
    renameEditor.setVisible(false);
    renameEditor.setColour(TextEditor::backgroundColourId, Colour(0xff2A2A2A).withAlpha(0.0f));
    renameEditor.setColour(TextEditor::textColourId, Colours::white);
    renameEditor.setColour(TextEditor::outlineColourId, Colour(0xff404040));
    renameEditor.setColour(TextEditor::focusedOutlineColourId, Colour(0xff606060));
    renameEditor.setColour(TextEditor::highlightedTextColourId, Colours::black);
    renameEditor.onReturnKey = [this]() { hideInlineRenameEditor(true); };
    renameEditor.onEscapeKey = [this]() { hideInlineRenameEditor(false); };
    renameEditor.onFocusLost = [this]() { hideInlineRenameEditor(true); };
    addAndMakeVisible(renameEditor);

    // Add bank button
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
    // Dark background
    g.setColour(Colour(0xff1A1A1A).withAlpha(0.9f));
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

    // Add button at bottom
    addBankButton.setBounds(bounds.withHeight(30));
}

void PresetBrowserComponent::createNewPresetBank()
{
    if (!processor || !processor->getCollectionManager())
        return;

    String newName = "Bank " + Time::getCurrentTime().formatted("%Y-%m-%d %H.%M");

    String presetId = processor->getCollectionManager()->createPreset(newName, "New empty bank");

    if (!presetId.isEmpty())
    {
        refreshPresetList();

        Timer::callAfterDelay(1000, [this]() {
            shouldHighlightFirstSlot = false;
            repaint();
        });
    }
}

void PresetBrowserComponent::setProcessor(FreesoundAdvancedSamplerAudioProcessor* p)
{
    processor = p;

    if (processor) {
        MessageManager::callAsync([this]() {
            refreshPresetList();
        });
    }
}

void PresetBrowserComponent::refreshPresetList()
{
    if (!processor || !processor->getCollectionManager())
        return;

    presetItems.clear();
    selectedItem = nullptr;

    // Get all presets from collection manager
    Array<PresetInfo> presets = processor->getCollectionManager()->getAllPresets();

    // Get current active preset
    String activePresetId = processor->getActivePresetId();
    int activeSlotIndex = processor->getActiveSlotIndex();

    int yPosition = 0;
    const int itemHeight = 85;
    const int spacing = 8;
    const int fixedItemWidth = 180;

    for (const auto& preset : presets)
    {
        PresetListItemInfo itemInfo;
        itemInfo.presetId = preset.presetId;
        itemInfo.name = preset.name;
        itemInfo.description = preset.description;
        itemInfo.createdDate = preset.createdAt;

        // Check which slots have data
        for (int slotIndex = 0; slotIndex < 8; ++slotIndex)
        {
            Array<SampleCollectionManager::PadSlotData> slotData = processor->getCollectionManager()->getPresetSlot(preset.presetId, slotIndex);
            bool hasData = false;
            for (const auto& data : slotData)
            {
                if (data.freesoundId.isNotEmpty())
                {
                    hasData = true;
                    break;
                }
            }
            itemInfo.slotHasData[slotIndex] = hasData;
        }

        // Set active slot if this is the active preset
        if (preset.presetId == activePresetId)
        {
            itemInfo.activeSlot = activeSlotIndex;
        }

        auto* item = new PresetListItem(itemInfo);

        // Set up all callbacks
        item->onItemClicked = [this](PresetListItem* clickedItem) {
            handleItemClicked(clickedItem);
        };

        item->onItemDoubleClicked = [this](PresetListItem* clickedItem) {
            handleItemDoubleClicked(clickedItem);
        };

        item->onDeleteClicked = [this, item](const String& presetId) {
            handleDeleteClicked(item);
        };

        item->onRenameConfirmed = [this](const String& presetId, const String& newName) {
            performRename(presetId, newName);
        };

        item->onLoadSlotClicked = [this](const String& presetId, int slotIndex) {
            handleLoadSlotClicked(presetId, slotIndex);
        };

        item->onSaveSlotClicked = [this](const String& presetId, int slotIndex) {
            handleSaveSlotClicked(presetId, slotIndex);
        };

        item->onDeleteSlotClicked = [this](const String& presetId, int slotIndex) {
            handleDeleteSlotClicked(presetId, slotIndex);
        };

        item->onSampleCheckClicked = [this](PresetListItem* clickedItem) {
            handleSampleCheckClicked(clickedItem);
        };

        // Set active state
        if (preset.presetId == activePresetId && activeSlotIndex >= 0)
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
    const auto& itemInfo = item->getPresetListItemInfo();
    for (int i = 0; i < 8; ++i)
    {
        if (itemInfo.slotHasData[i])
        {
            handleLoadSlotClicked(itemInfo.presetId, i);
            break;
        }
    }
}

void PresetBrowserComponent::handleLoadSlotClicked(const String& presetId, int slotIndex)
{
    updateActiveSlotAcrossPresets(presetId, slotIndex);

    if (onPresetLoadRequested)
        onPresetLoadRequested(presetId, slotIndex);
}

void PresetBrowserComponent::updateActiveSlotAcrossPresets(const String& activePresetId, int activeSlotIndex)
{
    for (auto* item : presetItems)
    {
        if (item && item->getPresetId() == activePresetId)
        {
            item->updateActiveSlot(activeSlotIndex);
            handleItemClicked(item);
            break;
        }
    }
}

void PresetBrowserComponent::handleSaveSlotClicked(const String& presetId, int slotIndex)
{
    if (!processor || !processor->getCollectionManager())
        return;

    // Check if slot already has data
    Array<SampleCollectionManager::PadSlotData> existingSlotData = processor->getCollectionManager()->getPresetSlot(presetId, slotIndex);
    bool slotHasData = !existingSlotData.isEmpty();

    if (slotHasData)
    {
        AlertWindow::showOkCancelBox(
            AlertWindow::QuestionIcon,
            "Overwrite Slot",
            "Slot " + String(slotIndex + 1) + " already contains data. Do you want to overwrite it?",
            "Yes, Overwrite", "No, Cancel",
            nullptr,
            ModalCallbackFunction::create([this, presetId, slotIndex](int result) {
                if (result == 1)
                {
                    performSaveToSlot(presetId, slotIndex, "Saved from grid interface");
                }
            })
        );
    }
    else
    {
        performSaveToSlot(presetId, slotIndex, "Saved from grid interface");
    }
}

void PresetBrowserComponent::performSaveToSlot(const String& presetId, int slotIndex, const String& description)
{
    // CORRECT: Use the actual saveToSlot method
    if (processor->saveToSlot(presetId, slotIndex, description))
    {
        refreshPresetList();
    }
    else
    {
        AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
            "Save Failed", "Failed to save to slot " + String(slotIndex + 1));
    }
}

void PresetBrowserComponent::handleDeleteSlotClicked(const String& presetId, int slotIndex)
{
    if (!processor || !processor->getCollectionManager())
        return;

    if (processor->getCollectionManager()->clearPresetSlot(presetId, slotIndex))
    {
        refreshPresetList();
    }
}

void PresetBrowserComponent::handleDeleteClicked(PresetListItem* item)
{
    if (!processor || !processor->getCollectionManager() || !item)
        return;

    String presetId = item->getPresetId();
    String presetName = item->getPresetListItemInfo().name;

    AlertWindow::showOkCancelBox(AlertWindow::QuestionIcon,
        "Delete Preset", "Are you sure you want to delete \"" + presetName + "\"?",
        "Yes", "No", this,
        ModalCallbackFunction::create([this, presetId](int result) {
            if (result == 1)
            {
                if (processor->getCollectionManager()->deletePreset(presetId))
                {
                    refreshPresetList();
                }
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
   renameEditor.setText(item->getPresetListItemInfo().name, dontSendNotification);
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
       if (newName.isNotEmpty() && newName != renamingItem->getPresetListItemInfo().name)
       {
           performRename(renamingItem->getPresetId(), newName);
       }
   }

   renamingItem = nullptr;
}

void PresetBrowserComponent::performRename(const String& presetId, const String& newName)
{
   if (newName.isEmpty() || !processor || !processor->getCollectionManager())
       return;

   if (processor->getCollectionManager()->renamePreset(presetId, newName))
   {
       refreshPresetList();
   }
   else
   {
       AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
           "Rename Failed", "Failed to rename preset.");
   }
}

void PresetBrowserComponent::saveCurrentPreset()
{
    if (!processor || !processor->getCollectionManager())
        return;

    String suggestedName = "Preset " + Time::getCurrentTime().formatted("%Y-%m-%d %H.%M");

    // CORRECT: Use saveCurrentAsPreset to create new preset
    String presetId = processor->saveCurrentAsPreset(suggestedName, "Saved from grid interface", 0);
    if (!presetId.isEmpty())
    {
        refreshPresetList();
        AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon,
            "Preset Saved", "Preset saved as \"" + suggestedName + "\" in slot 0");
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

   String activePresetId = processor->getActivePresetId();
   int activeSlotIndex = processor->getActiveSlotIndex();

   if (activePresetId.isEmpty() || activeSlotIndex < 0)
       return;

   // Find the matching preset item and update its active slot
   for (auto* item : presetItems)
   {
       if (item && item->getPresetId() == activePresetId)
       {
           item->updateActiveSlot(activeSlotIndex);
           handleItemClicked(item);
           break;
       }
   }
}

void PresetBrowserComponent::handleSampleCheckClicked(PresetListItem* item)
{
   if (!processor || !processor->getCollectionManager())
       return;

   const String& presetId = item->getPresetId();

   // Collect all unique sample IDs from all slots
   StringArray allUniqueSampleIds;
   Array<SampleMetadata> allMissingSamples;
   StringArray uniqueMissingSampleIds;

    for (int slotIndex = 0; slotIndex < 8; ++slotIndex)
    {
        Array<SampleCollectionManager::PadSlotData> slotData = processor->getCollectionManager()->getPresetSlot(presetId, slotIndex);

        for (const auto& data : slotData)
        {
            if (data.freesoundId.isNotEmpty())
            {
                // Add to unique samples list
                allUniqueSampleIds.addIfNotAlreadyThere(data.freesoundId);

                File sampleFile = processor->getCollectionManager()->getSampleFile(data.freesoundId);
                if (!sampleFile.existsAsFile())
                {
                    // For missing samples, add unique IDs for download
                    uniqueMissingSampleIds.addIfNotAlreadyThere(data.freesoundId);

                    SampleMetadata sample = processor->getCollectionManager()->getSample(data.freesoundId);
                    if (!sample.freesoundId.isEmpty())
                    {
                        allMissingSamples.add(sample);
                    }
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
       ModalCallbackFunction::create([this, allMissingSamples](int result) {
           if (result == 1) { // User clicked "Yes, Download"
               downloadMissingSamples(allMissingSamples);
           }
       })
   );
}

void PresetBrowserComponent::downloadMissingSamples(const Array<SampleMetadata>& missingSamples)
{
   if (!processor || missingSamples.isEmpty())
       return;

   // Get unique sample IDs to avoid downloading the same sample multiple times
   StringArray uniqueMissingIds;
   for (const auto& sample : missingSamples)
   {
       uniqueMissingIds.addIfNotAlreadyThere(sample.freesoundId);
   }

   // We need to fetch the missing sounds from Freesound API to get complete data including previews
   Array<FSSound> soundsToDownload;
   FreesoundClient client(FREESOUND_API_KEY);

   for (const String& freesoundId : uniqueMissingIds)
   {
       try
       {
           // Fetch the complete sound data from Freesound API including previews
           FSSound sound = client.getSound(freesoundId, "id,name,username,license,previews,duration,filesize,tags,description");

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
       processor->getCollectionManager()->getSamplesFolder(),
       "Missing samples download"
   );

   // Show progress notification
   AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon,
       "Download Started",
       "Downloading " + String(soundsToDownload.size()) + " unique samples...");
}