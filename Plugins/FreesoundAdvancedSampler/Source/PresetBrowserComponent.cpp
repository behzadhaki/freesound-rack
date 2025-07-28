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

    // Background color based on state
    Colour bgColour;
    if (hasDataFlag)
    {
        bgColour = Colours::darkgoldenrod; // Mustard gold for slots with data
    }
    else
    {
        bgColour = Colours::grey; // Grey for empty slots
    }

    if (shouldDrawButtonAsHighlighted)
        bgColour = bgColour.brighter(0.2f);
    if (shouldDrawButtonAsDown)
        bgColour = bgColour.darker(0.2f);

    g.setColour(bgColour);
    g.fillRoundedRectangle(bounds, 3.0f);

    // Border
    g.setColour(Colours::white.withAlpha(0.6f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 3.0f, 1.0f);

    // Underline if active
    if (isActiveFlag)
    {
        g.setColour(Colours::white);
        g.fillRect(bounds.getX() + 2, bounds.getBottom() - 2, bounds.getWidth() - 4, 2.0f);
    }

    // Text
    g.setColour(hasDataFlag ? Colours::white : Colours::darkgrey);
    g.setFont(12.0f);
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

PresetListItem::PresetListItem(const PresetInfo& info)
    : presetInfo(info)
{
    setSize(200, 120); // Increased height for slots

    // Delete Button
    deleteButton.setButtonText("DEL");
    deleteButton.setSize(40, 18);
    deleteButton.onClick = [this]() {
        if (onDeleteClicked)
            onDeleteClicked(this);
    };
    addAndMakeVisible(deleteButton);

    // Load Button
    loadButton.setButtonText("LOAD");
    loadButton.setSize(40, 18);
    loadButton.onClick = [this]() {
        // Load first available slot by default
        for (int i = 0; i < 8; ++i)
        {
            if (presetInfo.slots[i].hasData)
            {
                if (onLoadSlotClicked)
                    onLoadSlotClicked(presetInfo, i);
                break;
            }
        }
    };
    addAndMakeVisible(loadButton);

    // Rename Editor
    renameEditor.setText(presetInfo.name, dontSendNotification);
    renameEditor.setSelectAllWhenFocused(true);
    renameEditor.setFont(Font(14.0f, Font::bold));
    renameEditor.onReturnKey = [this]() { confirmRename(); };
    addAndMakeVisible(renameEditor);

    // Create slot buttons
    for (int i = 0; i < 8; ++i)
    {
        slotButtons[i] = std::make_unique<SlotButton>(i);
        slotButtons[i]->setHasData(presetInfo.slots[i].hasData);
        slotButtons[i]->setIsActive(presetInfo.activeSlot == i);

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

    g.setColour(isSelectedState ? Colours::blue.withAlpha(0.3f)
                                : Colours::darkgrey.withAlpha(0.2f));
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    g.setColour(isSelectedState ? Colours::blue
                                : Colours::grey.withAlpha(0.3f));
    g.drawRoundedRectangle(bounds.toFloat().reduced(1), 4.0f, isSelectedState ? 2.0f : 1.0f);

    auto textBounds = bounds.reduced(8);
    textBounds.removeFromRight(90); // leave space for buttons
    textBounds.removeFromBottom(30); // leave space for slots

    // Info text
    g.setFont(Font(11.0f));
    g.setColour(Colours::lightgrey);

    // Count total samples across all slots
    int totalSamples = 0;
    for (const auto& slot : presetInfo.slots)
    {
        if (slot.hasData)
            totalSamples += slot.sampleCount;
    }

    String infoText = "Total Samples: " + String(totalSamples);
    g.drawText(infoText, textBounds.removeFromTop(15), Justification::left, true);

    g.setFont(Font(10.0f));
    g.setColour(Colours::grey);
    g.drawText(presetInfo.createdDate, textBounds, Justification::left, true);

    // Draw "Slots:" label
    auto slotsBounds = bounds.reduced(8);
    slotsBounds.removeFromTop(bounds.getHeight() - 35);
    slotsBounds.removeFromBottom(5);

    g.setFont(Font(10.0f));
    g.setColour(Colours::lightgrey);
    g.drawText("Slots:", slotsBounds.removeFromLeft(35), Justification::left, true);
}

void PresetListItem::resized()
{
    auto bounds = getLocalBounds();
    auto textBounds = bounds.reduced(8);
    textBounds.removeFromRight(90);
    renameEditor.setBounds(textBounds.removeFromTop(20));

    int buttonWidth = 40;
    int buttonHeight = 18;
    int margin = 5;
    int spacing = 5;

    int rightEdge = bounds.getRight() - margin;

    deleteButton.setBounds(rightEdge - buttonWidth,
                           bounds.getY() + margin,
                           buttonWidth, buttonHeight);

    rightEdge -= buttonWidth + spacing;

    loadButton.setBounds(rightEdge - buttonWidth,
                         bounds.getY() + margin,
                         buttonWidth, buttonHeight);

    // Position slot buttons
    auto slotsBounds = bounds.reduced(8);
    slotsBounds.removeFromTop(bounds.getHeight() - 35);
    slotsBounds.removeFromBottom(5);
    slotsBounds.removeFromLeft(40); // Space for "Slots:" label

    int slotSpacing = 3;
    int slotSize = 20;

    for (size_t i = 0; i < slotButtons.size(); ++i)
    {
        if (slotButtons[i])
        {
            int x = slotsBounds.getX() + static_cast<int>(i) * (slotSize + slotSpacing);
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

    if (newName.isEmpty() || newName == presetInfo.name || !onRenameConfirmed)
        return;

    MessageManager::callAsync([info = presetInfo, newName, cb = onRenameConfirmed]() {
        cb(info, newName);
    });
}

void PresetListItem::handleSlotClicked(int slotIndex, const ModifierKeys& modifiers)
{
    bool hasData = presetInfo.slots[slotIndex].hasData;

    if (modifiers.isShiftDown() && !modifiers.isCommandDown())
    {
        // Shift+click: Save to slot
        if (hasData)
        {
            // Ask if overwrite
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
            // Save directly
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
        // Regular click: Load slot
        if (hasData && onLoadSlotClicked)
        {
            onLoadSlotClicked(presetInfo, slotIndex);
        }
    }
}

//==============================================================================
// PresetBrowserComponent Implementation
//==============================================================================

PresetBrowserComponent::PresetBrowserComponent()
    : processor(nullptr)
{
    titleLabel.setText("Preset Browser", dontSendNotification);
    titleLabel.setFont(Font(16.0f, Font::bold));
    titleLabel.setJustificationType(Justification::centred);
    addAndMakeVisible(titleLabel);

    savePresetButton.setButtonText("Save Current");
    savePresetButton.onClick = [this]() { saveCurrentPreset(); };
    addAndMakeVisible(savePresetButton);

    refreshButton.setButtonText("Refresh");
    refreshButton.onClick = [this]() { refreshPresetList(); };
    addAndMakeVisible(refreshButton);

    presetViewport.setViewedComponent(&presetListContainer, false);
    presetViewport.setScrollBarsShown(true, false);
    addAndMakeVisible(presetViewport);

    renameEditor.setVisible(false);
    renameEditor.onReturnKey = [this]() { hideInlineRenameEditor(true); };
    renameEditor.onEscapeKey = [this]() { hideInlineRenameEditor(false); };
    renameEditor.onFocusLost = [this]() { hideInlineRenameEditor(true); };
    addAndMakeVisible(renameEditor);
}

PresetBrowserComponent::~PresetBrowserComponent() {}

void PresetBrowserComponent::paint(Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
    g.setColour(Colours::grey.withAlpha(0.3f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2), 4.0f, 1.0f);
}

void PresetBrowserComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    titleLabel.setBounds(bounds.removeFromTop(25));
    bounds.removeFromTop(10);

    auto buttonArea = bounds.removeFromTop(30);
    int buttonWidth = 80;
    int spacing = 10;

    savePresetButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
    buttonArea.removeFromLeft(spacing);
    refreshButton.setBounds(buttonArea.removeFromLeft(buttonWidth));

    bounds.removeFromTop(10);
    presetViewport.setBounds(bounds);

    updatePresetList();
}

void PresetBrowserComponent::setProcessor(FreesoundAdvancedSamplerAudioProcessor* p)
{
    processor = p;
    refreshPresetList();
}

void PresetBrowserComponent::refreshPresetList()
{
    if (!processor)
        return;

    presetItems.clear();
    selectedItem = nullptr;

    Array<PresetInfo> presets = processor->getPresetManager().getAvailablePresets();

    int yPosition = 0;
    const int itemHeight = 125; // Increased height for slots
    const int spacing = 5;

    for (const auto& presetInfo : presets)
    {
        auto* item = new PresetListItem(presetInfo);

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

        item->setBounds(5, yPosition, getWidth() - 30, itemHeight);
        presetListContainer.addAndMakeVisible(*item);
        presetItems.add(item);

        yPosition += itemHeight + spacing;
    }

    presetListContainer.setSize(getWidth(), yPosition);
    presetViewport.getViewedComponent()->repaint();
}

void PresetBrowserComponent::updatePresetList()
{
    int availableWidth = presetViewport.getWidth() - 30;

    for (int i = 0; i < presetItems.size(); ++i)
    {
        auto* item = presetItems[i];
        auto bounds = item->getBounds();
        bounds.setWidth(availableWidth);
        item->setBounds(bounds);
    }

    int totalHeight = 0;
    if (presetItems.size() > 0)
        totalHeight = presetItems.getLast()->getBottom() + 5;

    presetListContainer.setSize(availableWidth, totalHeight);
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
    if (onPresetLoadRequested)
        onPresetLoadRequested(presetInfo, slotIndex);
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

    if (processor->getPresetManager().deleteSlot(presetInfo.presetFile, slotIndex))
    {
        refreshPresetList();
        AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon,
            "Slot Deleted", "Slot " + String(slotIndex + 1) + " deleted");
    }
    else
    {
        AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
            "Delete Failed", "Failed to delete slot " + String(slotIndex + 1));
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