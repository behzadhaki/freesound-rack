#include "PresetBrowserComponent.h"

PresetListItem::PresetListItem(const PresetInfo& info)
    : presetInfo(info)
{
    setSize(200, 80);

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
        if (onLoadClicked)
            onLoadClicked(presetInfo);
    };
    addAndMakeVisible(loadButton);

    // Rename Editor (always visible)
    renameEditor.setText(presetInfo.name, dontSendNotification);
    renameEditor.setSelectAllWhenFocused(true);
    renameEditor.setFont(Font(14.0f, Font::bold));
    renameEditor.onReturnKey = [this]() { confirmRename(); };
    addAndMakeVisible(renameEditor);
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
    textBounds.removeFromRight(90); // leave space for both buttons

    // Info text
    g.setFont(Font(11.0f));
    g.setColour(Colours::lightgrey);
    String infoText = "Samples: " + String(presetInfo.sampleCount);
    if (presetInfo.searchQuery.isNotEmpty())
        infoText += " | Query: " + presetInfo.searchQuery;
    g.drawText(infoText, textBounds.removeFromTop(15), Justification::left, true);

    g.setFont(Font(10.0f));
    g.setColour(Colours::grey);
    g.drawText(presetInfo.createdDate, textBounds, Justification::left, true);
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

        item->onRenameConfirmed = [this](const PresetInfo& originalInfo, const String& newName) {
            performRename(originalInfo, newName);
        };

        item->setBounds(5, yPosition, getWidth() - 30, itemHeight);
        presetListContainer.addAndMakeVisible(*item);
        presetItems.add(item);

        item->onLoadClicked = [this](const PresetInfo& info) {
            if (onPresetLoadRequested)
                onPresetLoadRequested(info);
        };

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
    if (onPresetLoadRequested)
        onPresetLoadRequested(item->getPresetInfo());
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

    Array<PadInfo> padInfos;
    if (processor->getPresetManager().loadPreset(presetInfo.presetFile, padInfos))
    {
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

        if (processor->getPresetManager().saveCurrentPreset(newName, description, padInfos, searchQuery))
        {
            processor->getPresetManager().deletePreset(presetInfo.presetFile);
            refreshPresetList();
        }
        else
        {
            AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
                "Rename Failed", "Failed to save preset with new name.");
        }
    }
    else
    {
        AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
            "Rename Failed", "Failed to load preset data.");
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

void PresetBrowserComponent::saveCurrentPreset()
{
    if (!processor)
        return;

    String suggestedName = processor->getPresetManager().generatePresetName(processor->getQuery());

    if (processor->saveCurrentAsPreset(suggestedName, "Saved from grid interface"))
    {
        refreshPresetList();
        AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon,
            "Preset Saved", "Preset saved as \"" + suggestedName + "\"");
    }
    else
    {
        AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
            "Save Failed", "Failed to save preset.");
    }
}

void PresetBrowserComponent::scrollBarMoved(ScrollBar*, double) {}
