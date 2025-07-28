/*
  ==============================================================================

    PresetBrowserComponent.cpp
    Created: Preset browser and management UI
    Author: Generated

  ==============================================================================
*/

#include "PresetBrowserComponent.h"

//==============================================================================
// PresetListItem Implementation
//==============================================================================

PresetListItem::PresetListItem(const PresetInfo& info)
    : presetInfo(info)
{
    setSize(200, 80); // Made taller to accommodate buttons better

    deleteButton.setButtonText("DEL");
    deleteButton.setSize(30, 18);
    deleteButton.onClick = [this]() {
        if (onDeleteClicked)
            onDeleteClicked(this);
    };
    addAndMakeVisible(deleteButton);

    // Create and add the rename button
    renameButton.setButtonText("REN");
    renameButton.setSize(30, 18);
    renameButton.onClick = [this]() {
        DBG("Rename button clicked for: " + presetInfo.name);
        if (onRenameClicked)
            onRenameClicked(this);
    };
    addAndMakeVisible(renameButton);

    DBG("Created PresetListItem with both DEL and REN buttons for: " + presetInfo.name);
}

PresetListItem::~PresetListItem()
{
}

void PresetListItem::paint(Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    if (isSelectedState)
    {
        g.setColour(Colours::blue.withAlpha(0.3f));
    }
    else
    {
        g.setColour(Colours::darkgrey.withAlpha(0.2f));
    }
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    // Border
    if (isSelectedState)
    {
        g.setColour(Colours::blue);
        g.drawRoundedRectangle(bounds.toFloat().reduced(1), 4.0f, 2.0f);
    }
    else
    {
        g.setColour(Colours::grey.withAlpha(0.3f));
        g.drawRoundedRectangle(bounds.toFloat().reduced(1), 4.0f, 1.0f);
    }

    // Text content
    auto textBounds = bounds.reduced(8);
    textBounds.removeFromRight(70); // Space for both buttons

    g.setColour(Colours::white);
    g.setFont(Font(14.0f, Font::bold));

    auto nameBounds = textBounds.removeFromTop(20);
    g.drawText(presetInfo.name, nameBounds, Justification::left, true);

    g.setFont(Font(11.0f));
    g.setColour(Colours::lightgrey);

    String infoText = "Samples: " + String(presetInfo.sampleCount);
    if (presetInfo.searchQuery.isNotEmpty())
        infoText += " | Query: " + presetInfo.searchQuery;

    g.drawText(infoText, textBounds.removeFromTop(15), Justification::left, true);

    // Date
    g.setFont(Font(10.0f));
    g.setColour(Colours::grey);
    g.drawText(presetInfo.createdDate, textBounds, Justification::left, true);
}

void PresetListItem::resized()
{
    auto bounds = getLocalBounds();

    // Position buttons in top-right corner with better spacing
    int buttonSpacing = 5;
    int buttonWidth = 30;
    int buttonHeight = 18;
    int margin = 5;

    deleteButton.setBounds(bounds.getRight() - margin - buttonWidth,
                          bounds.getY() + margin,
                          buttonWidth, buttonHeight);

    renameButton.setBounds(bounds.getRight() - margin - buttonWidth - buttonSpacing - buttonWidth,
                          bounds.getY() + margin,
                          buttonWidth, buttonHeight);
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

    // Set up viewport
    presetViewport.setViewedComponent(&presetListContainer, false);
    presetViewport.setScrollBarsShown(true, false);
    addAndMakeVisible(presetViewport);
}

PresetBrowserComponent::~PresetBrowserComponent()
{
}

void PresetBrowserComponent::paint(Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));

    // Draw border
    g.setColour(Colours::grey.withAlpha(0.3f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2), 4.0f, 1.0f);
}

void PresetBrowserComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    // Title
    titleLabel.setBounds(bounds.removeFromTop(25));
    bounds.removeFromTop(10);

    // Buttons
    auto buttonArea = bounds.removeFromTop(30);
    int buttonWidth = 80;
    int spacing = 10;

    savePresetButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
    buttonArea.removeFromLeft(spacing);
    refreshButton.setBounds(buttonArea.removeFromLeft(buttonWidth));

    bounds.removeFromTop(10);

    // Preset list viewport
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

    DBG("Refreshing preset list...");

    // Clear existing items
    presetItems.clear();
    selectedItem = nullptr;

    // Get available presets
    Array<PresetInfo> presets = processor->getPresetManager().getAvailablePresets();
    DBG("Found " + String(presets.size()) + " presets");

    // Create new items
    int yPosition = 0;
    const int itemHeight = 85;
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

        item->onRenameClicked = [this](PresetListItem* clickedItem) {
            DBG("Setting up rename callback for: " + clickedItem->getPresetInfo().name);
            handleRenameClicked(clickedItem);
        };

        item->setBounds(5, yPosition, getWidth() - 30, itemHeight);
        presetListContainer.addAndMakeVisible(*item);
        presetItems.add(item);

        yPosition += itemHeight + spacing;
    }

    // Update container size
    presetListContainer.setSize(getWidth(), yPosition);
    presetViewport.getViewedComponent()->repaint();

    DBG("Preset list refresh complete");
}

void PresetBrowserComponent::updatePresetList()
{
    // Adjust item widths when component is resized
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
    {
        auto* lastItem = presetItems.getLast();
        totalHeight = lastItem->getBottom() + 5;
    }

    presetListContainer.setSize(availableWidth, totalHeight);
}

void PresetBrowserComponent::handleItemClicked(PresetListItem* item)
{
    // Deselect previous item
    if (selectedItem && selectedItem != item)
        selectedItem->setSelected(false);

    // Select new item
    selectedItem = item;
    item->setSelected(true);
}

void PresetBrowserComponent::handleItemDoubleClicked(PresetListItem* item)
{
    // Load the preset
    if (onPresetLoadRequested)
        onPresetLoadRequested(item->getPresetInfo());
}

void PresetBrowserComponent::handleRenameClicked(PresetListItem* item)
{
    if (!processor)
        return;

    const PresetInfo& presetInfo = item->getPresetInfo();

    DBG("handleRenameClicked called for preset: " + presetInfo.name);

    // Create a member variable to store the text, then use a simple dialog
    // First, let's try a completely different approach - create our own simple input method

    // For now, let's use a series of simple dialogs to get the name
    showRenameDialog(presetInfo);
}

// Add this helper method to handle the rename dialog
void PresetBrowserComponent::showRenameDialog(const PresetInfo& presetInfo)
{
    // Create a simple message box that asks for confirmation to proceed with rename
    String message = "Current name: \"" + presetInfo.name + "\"\n\n";
    message += "Click OK to proceed with renaming, Cancel to abort.";

    AlertWindow::showMessageBoxAsync(
        AlertWindow::QuestionIcon,
        "Rename Preset",
        message,
        "OK",
        this,
        ModalCallbackFunction::create([this, presetInfo](int result) {
            if (result == 1) // OK clicked
            {
                // Now show a second dialog asking for the new name
                promptForNewName(presetInfo);
            }
        }));
}

// Add this method to prompt for the new name
void PresetBrowserComponent::promptForNewName(const PresetInfo& presetInfo)
{
    // Use a simple approach: create a new AlertWindow on the heap
    AlertWindow* dialog = new AlertWindow("Enter New Name", "Enter the new name for this preset:", AlertWindow::NoIcon);

    dialog->addTextEditor("newName", presetInfo.name, "New Name:");
    dialog->addButton("Rename", 1, KeyPress(KeyPress::returnKey));
    dialog->addButton("Cancel", 0, KeyPress(KeyPress::escapeKey));

    // Select all text in the editor
    if (auto* editor = dialog->getTextEditor("newName"))
    {
        editor->selectAll();
        editor->grabKeyboardFocus();
    }

    // Store the dialog pointer and launch it
    dialog->enterModalState(true, ModalCallbackFunction::create([this, presetInfo, dialog](int result) {
        String newName;

        if (result == 1) // Rename button clicked
        {
            newName = dialog->getTextEditorContents("newName").trim();
            DBG("User entered new name: '" + newName + "'");
        }

        // Clean up the dialog
        delete dialog;

        // Process the rename if we got a valid name
        if (result == 1 && newName.isNotEmpty() && newName != presetInfo.name)
        {
            performRename(presetInfo, newName);
        }
        else if (result == 1 && newName.isEmpty())
        {
            AlertWindow::showMessageBoxAsync(
                AlertWindow::WarningIcon,
                "Invalid Name",
                "Please enter a valid preset name.");
        }
        else
        {
            DBG("User cancelled rename or name unchanged");
        }
    }));
}

void PresetBrowserComponent::performRename(const PresetInfo& presetInfo, const String& newName)
{
    if (newName.isEmpty() || newName == presetInfo.name)
        return;

    DBG("performRename called: '" + presetInfo.name + "' -> '" + newName + "'");

    // Check if new name already exists
    Array<PresetInfo> existingPresets = processor->getPresetManager().getAvailablePresets();
    for (const auto& preset : existingPresets)
    {
        if (preset.name == newName && preset.presetFile != presetInfo.presetFile)
        {
            AlertWindow::showMessageBoxAsync(
                AlertWindow::WarningIcon,
                "Name Exists",
                "A preset with the name \"" + newName + "\" already exists.");
            return;
        }
    }

    // Perform the rename
    Array<PadInfo> padInfos;
    if (processor->getPresetManager().loadPreset(presetInfo.presetFile, padInfos))
    {
        DBG("Loaded preset data, " + String(padInfos.size()) + " pad infos");

        // Get original description from JSON
        String description = "";
        String searchQuery = "";

        FileInputStream inputStream(presetInfo.presetFile);
        if (inputStream.openedOk())
        {
            String jsonText = inputStream.readEntireStreamAsString();
            var parsedJson = JSON::parse(jsonText);
            if (parsedJson.isObject())
            {
                var info = parsedJson.getProperty("preset_info", var());
                if (info.isObject())
                {
                    description = info.getProperty("description", "");
                    searchQuery = info.getProperty("search_query", "");
                }
            }
        }

        DBG("Extracted metadata - description: '" + description + "', searchQuery: '" + searchQuery + "'");

        // Save with new name
        if (processor->getPresetManager().saveCurrentPreset(newName, description, padInfos, searchQuery))
        {
            DBG("Successfully saved new preset, now deleting old one");

            // Delete old preset
            if (processor->getPresetManager().deletePreset(presetInfo.presetFile))
            {
                DBG("Successfully deleted old preset");
                refreshPresetList();
                AlertWindow::showMessageBoxAsync(
                    AlertWindow::InfoIcon,
                    "Preset Renamed",
                    "Preset has been renamed to \"" + newName + "\" successfully!");
            }
            else
            {
                DBG("Failed to delete old preset file");
                AlertWindow::showMessageBoxAsync(
                    AlertWindow::WarningIcon,
                    "Partial Success",
                    "New preset created but failed to delete old one.");
            }
        }
        else
        {
            DBG("Failed to save new preset");
            AlertWindow::showMessageBoxAsync(
                AlertWindow::WarningIcon,
                "Rename Failed",
                "Failed to save preset with new name.");
        }
    }
    else
    {
        DBG("Failed to load preset data");
        AlertWindow::showMessageBoxAsync(
            AlertWindow::WarningIcon,
            "Rename Failed",
            "Failed to load preset data for renaming.");
    }
}

void PresetBrowserComponent::handleDeleteClicked(PresetListItem* item)
{
    AlertWindow::showOkCancelBox(
        AlertWindow::QuestionIcon,
        "Delete Preset",
        "Are you sure you want to delete the preset \"" + item->getPresetInfo().name + "\"?",
        "Yes",
        "No",
        this,
        ModalCallbackFunction::create([this, item](int result) {
            if (result == 1 && processor)
            {
                processor->getPresetManager().deletePreset(item->getPresetInfo().presetFile);
                refreshPresetList();
            }
        }));
}

void PresetBrowserComponent::saveCurrentPreset()
{
    if (!processor)
        return;

    // Generate suggested name and just use it directly for now
    String suggestedName = processor->getPresetManager().generatePresetName(processor->getQuery());

    if (processor->saveCurrentAsPreset(suggestedName, "Saved from grid interface"))
    {
        refreshPresetList();

        AlertWindow::showMessageBoxAsync(
            AlertWindow::InfoIcon,
            "Preset Saved",
            "Preset saved as \"" + suggestedName + "\"");
    }
    else
    {
        AlertWindow::showMessageBoxAsync(
            AlertWindow::WarningIcon,
            "Save Failed",
            "Failed to save preset. Please try again.");
    }
}

void PresetBrowserComponent::scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart)
{
    // Handle scroll if needed
}