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
    setSize(200, 60);

    deleteButton.setButtonText("X");  // Changed from "×" to "X"
    deleteButton.setSize(20, 20);
    deleteButton.onClick = [this]() {
        if (onDeleteClicked)
            onDeleteClicked(this);
    };
    addAndMakeVisible(deleteButton);
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
    textBounds.removeFromRight(25); // Space for delete button

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
    deleteButton.setBounds(bounds.getRight() - 25, bounds.getY() + 5, 20, 20);
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

    // Clear existing items
    presetItems.clear(); // OwnedArray automatically deletes the objects
    selectedItem = nullptr;

    // Get available presets
    Array<PresetInfo> presets = processor->getPresetManager().getAvailablePresets();

    // Create new items
    int yPosition = 0;
    const int itemHeight = 65;
    const int spacing = 5;

    for (const auto& presetInfo : presets)
    {
        auto* item = new PresetListItem(presetInfo); // Create raw pointer

        item->onItemClicked = [this](PresetListItem* clickedItem) {
            handleItemClicked(clickedItem);
        };

        item->onItemDoubleClicked = [this](PresetListItem* clickedItem) {
            handleItemDoubleClicked(clickedItem);
        };

        item->onDeleteClicked = [this](PresetListItem* clickedItem) {
            handleDeleteClicked(clickedItem);
        };

        item->setBounds(5, yPosition, getWidth() - 30, itemHeight);
        presetListContainer.addAndMakeVisible(*item);
        presetItems.add(item); // OwnedArray takes ownership

        yPosition += itemHeight + spacing;
    }

    // Update container size
    presetListContainer.setSize(getWidth(), yPosition);
    presetViewport.getViewedComponent()->repaint();
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

void PresetBrowserComponent::handleDeleteClicked(PresetListItem* item)
{
    // Use async message box - most compatible approach
    AlertWindow::showMessageBoxAsync(
        AlertWindow::QuestionIcon,
        "Delete Preset",
        "Are you sure you want to delete the preset \"" + item->getPresetInfo().name + "\"?",
        "Delete",
        this,
        ModalCallbackFunction::create([this, item](int result) {
            if (result == 1 && processor) // Delete clicked
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

    // For now, just save with the suggested name (user can rename files later)
    // This avoids all modal dialog compatibility issues
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