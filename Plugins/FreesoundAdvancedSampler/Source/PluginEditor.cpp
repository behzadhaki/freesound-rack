/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
FreesoundAdvancedSamplerAudioProcessorEditor::FreesoundAdvancedSamplerAudioProcessorEditor (FreesoundAdvancedSamplerAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    // Set up custom look and feel for AlertWindows
    customAlertLookAndFeel = std::make_unique<CustomAlertWindowLookAndFeel>();
    LookAndFeel::setDefaultLookAndFeel(customAlertLookAndFeel.get());

    // Register as download listener
    processor.addDownloadListener(this);

    // Set up sample grid component
    sampleGridComponent.setProcessor(&processor);
    addAndMakeVisible(sampleGridComponent);

    // Set up sample drag area
    sampleDragArea.setSampleGridComponent(&sampleGridComponent);
    addAndMakeVisible(sampleDragArea);

    // Set up directory open button
    directoryOpenButton.setProcessor(&processor);
    addAndMakeVisible(directoryOpenButton);

    // Set up preset browser with multi-slot support
    presetBrowserComponent.setProcessor(&processor);
    presetBrowserComponent.refreshPresetList();
    presetBrowserComponent.onPresetLoadRequested = [this](const PresetInfo& presetInfo, int slotIndex) {
        handlePresetLoadRequested(presetInfo, slotIndex);
    };

    // Set up expandable panel with preset browser inside
    expandablePanelComponent.setOrientation(ExpandablePanel::Orientation::Right); // Right orientation = expands left
    expandablePanelComponent.setContentComponent(&presetBrowserComponent);
    expandablePanelComponent.setExpandedWidth(220); // Slightly wider for preset browser
    expandablePanelComponent.setExpanded(processor.getPresetPanelExpandedState());
    expandablePanelComponent.onExpandedStateChanged = [this](bool expanded) {
        processor.setPresetPanelExpandedState(expanded);
        updateWindowSizeForPanelState();
    };
    addAndMakeVisible(expandablePanelComponent);

    // Set up bookmark viewer with expandable panel
    bookmarkExpandablePanel.setOrientation(ExpandablePanel::Orientation::Left); // Left orientation = expands right
    bookmarkExpandablePanel.setContentComponent(&bookmarkViewerComponent);
    bookmarkExpandablePanel.setExpandedWidth(200); // Width for bookmark viewer
    bookmarkExpandablePanel.setExpanded(false); // Start collapsed
    bookmarkExpandablePanel.onExpandedStateChanged = [this](bool expanded) {
        // You can save the state if needed
        updateWindowSizeForBookmarkPanel();
    };
    addAndMakeVisible(bookmarkExpandablePanel);

    // Set processor for bookmark viewer
    bookmarkViewerComponent.setProcessor(&processor);

    // Restore samples if processor already has them loaded
    if (processor.isArrayNotEmpty())
    {
        // Update grid with current processor state
        sampleGridComponent.updateSamples(processor.getCurrentSounds(), processor.getData());
    }

    // Make sure the grid can receive keyboard focus and starts with focus
    sampleGridComponent.setWantsKeyboardFocus(true);
    setWantsKeyboardFocus(true);

    setSize (800, 700);  // Reduced base width since preset browser is now in expandable panel
    setResizable(true, true);  // Enable resizing

    // Set size constraints (minimum and maximum sizes)
    // Base constraints without expandable panel
    int baseMinWidth = 600;  // Reduced since preset browser is collapsible
    int baseMaxWidth = 1600;
    setResizeLimits(baseMinWidth, 500, baseMaxWidth, 1200);

    // Restore saved window size
    int savedWidth = processor.getSavedWindowWidth();
    int savedHeight = processor.getSavedWindowHeight();

    setSize(savedWidth, savedHeight);

    // FIXED: Use a WeakReference instead of Timer::callAfterDelay to prevent crashes
    // The WeakReference will become null if this component is destroyed
    Component::SafePointer<FreesoundAdvancedSamplerAudioProcessorEditor> safeThis(this);

    Timer::callAfterDelay(200, [safeThis]() {
        if (safeThis != nullptr && safeThis->isShowing()) {
            safeThis->grabKeyboardFocus();
        }
    });

    Timer::callAfterDelay(100, [this]() {
    if (auto* safeThis = this; safeThis != nullptr)
    {
        presetBrowserComponent.restoreActiveState();
    }
});

}

FreesoundAdvancedSamplerAudioProcessorEditor::~FreesoundAdvancedSamplerAudioProcessorEditor()
{
    LookAndFeel::setDefaultLookAndFeel(nullptr);
    processor.removeDownloadListener(this);
}
//==============================================================================
void FreesoundAdvancedSamplerAudioProcessorEditor::paint(Graphics& g)
{
    // Modern dark background with subtle gradient
    g.setGradientFill(ColourGradient(
        Colour(0xff1A1A1A), 0, 0,
        Colour(0xff0D0D0D), getWidth(), getHeight(), false));
    g.fillAll();
}

void FreesoundAdvancedSamplerAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    const int margin = 8;

    // Fixed values
    const int buttonHeight = 30;
    const int buttonWidth = 80;
    const int spacing = 4;

    bounds.removeFromTop(margin);
    bounds.removeFromRight(margin);

    // Main content area
    bounds.removeFromLeft(margin);
    auto contentBounds = bounds.withTrimmedBottom(margin);

    // LEFT: Preset expandable panel
    int presetPanelWidth = expandablePanelComponent.isExpanded() ?
                          expandablePanelComponent.getExpandedWidth() :
                          expandablePanelComponent.getCollapsedWidth();

    auto leftPanelArea = contentBounds.removeFromLeft(presetPanelWidth);
    if (expandablePanelComponent.isExpanded())
        leftPanelArea.removeFromRight(spacing);
    expandablePanelComponent.setBounds(leftPanelArea);

    // RIGHT: Bookmark expandable panel
    int bookmarkPanelWidth = bookmarkExpandablePanel.isExpanded() ?
                            bookmarkExpandablePanel.getExpandedWidth() :
                            bookmarkExpandablePanel.getCollapsedWidth();

    auto rightPanelArea = contentBounds.removeFromRight(bookmarkPanelWidth);
    if (bookmarkExpandablePanel.isExpanded())
        rightPanelArea.removeFromLeft(spacing);
    bookmarkExpandablePanel.setBounds(rightPanelArea);

    // CENTER: Sample grid and controls
    contentBounds.removeFromLeft(spacing);
    contentBounds.removeFromRight(spacing);

    // Control area above the sample grid (top-left)
    auto controlArea = contentBounds.removeFromTop(buttonHeight);
    controlArea = controlArea.removeFromBottom(buttonHeight);

    // Split control area between drag area and directory button
    sampleDragArea.setBounds(controlArea.removeFromLeft(buttonWidth));
    directoryOpenButton.setBounds(controlArea.removeFromRight(buttonWidth));

    // Sample grid - takes ALL remaining space below the controls
    contentBounds.removeFromTop(spacing);
    sampleGridComponent.setBounds(contentBounds);
}

void FreesoundAdvancedSamplerAudioProcessorEditor::updateWindowSizeForPanelState()
{
    int currentWidth = getWidth();
    int currentHeight = getHeight();

    // Calculate the width difference when panel expands/collapses
    int panelWidthDifference = expandablePanelComponent.getExpandedWidth() -
                               expandablePanelComponent.getCollapsedWidth();

    if (expandablePanelComponent.isExpanded())
    {
        // Panel just expanded - increase window width
        int newWidth = currentWidth + panelWidthDifference + 4; // +4 for spacing
        setSize(newWidth, currentHeight);

        // Update constraints to allow for expanded panel
        setResizeLimits(600 + panelWidthDifference + 4, 500, 1600 + panelWidthDifference + 4, 1200);
    }
    else
    {
        // Panel just collapsed - decrease window width
        int newWidth = currentWidth - panelWidthDifference - 4; // -4 for spacing
        setSize(newWidth, currentHeight);

        // Update constraints back to base size
        setResizeLimits(600, 500, 1600, 1200);
    }

    // Save the new window size
    processor.setWindowSize(getWidth(), getHeight());
}

void FreesoundAdvancedSamplerAudioProcessorEditor::updateWindowSizeForBookmarkPanel()
{
    int currentWidth = getWidth();
    int currentHeight = getHeight();

    // Calculate the width difference when panel expands/collapses
    int panelWidthDifference = bookmarkExpandablePanel.getExpandedWidth() -
                               bookmarkExpandablePanel.getCollapsedWidth();

    if (bookmarkExpandablePanel.isExpanded())
    {
        // Panel just expanded - increase window width
        int newWidth = currentWidth + panelWidthDifference + 4; // +4 for spacing
        setSize(newWidth, currentHeight);
    }
    else
    {
        // Panel just collapsed - decrease window width
        int newWidth = currentWidth - panelWidthDifference - 4; // -4 for spacing
        setSize(newWidth, currentHeight);
    }

    // Save the new window size
    processor.setWindowSize(getWidth(), getHeight());
}

void FreesoundAdvancedSamplerAudioProcessorEditor::downloadProgressChanged(const AudioDownloadManager::DownloadProgress& progress)
{
    // Delegate to master search panel
    sampleGridComponent.getMasterSearchPanel().updateDownloadProgress(progress);
}

void FreesoundAdvancedSamplerAudioProcessorEditor::downloadCompleted(bool success)
{
    // Delegate to master search panel
    sampleGridComponent.getMasterSearchPanel().downloadCompleted(success);
}

void FreesoundAdvancedSamplerAudioProcessorEditor::handlePresetLoadRequested(const PresetInfo& presetInfo, int slotIndex)
{
    // First, check sample availability before loading
    Array<String> allUniqueSampleIds;
    Array<String> missingSampleIds;

    // Load the preset data to check which samples it needs
    Array<PadInfo> padInfos;
    if (!processor.getPresetManager().loadPreset(presetInfo.presetFile, slotIndex, padInfos))
    {
        AlertWindow::showMessageBoxAsync(
            AlertWindow::WarningIcon,
            "Load Failed",
            "Failed to read preset data from \"" + presetInfo.name + "\" slot " + String(slotIndex + 1) + ".");
        return;
    }

    // Check sample availability
    for (const auto& padInfo : padInfos)
    {
        allUniqueSampleIds.addIfNotAlreadyThere(padInfo.freesoundId);

        File sampleFile = processor.getPresetManager().getSampleFile(padInfo.freesoundId);
        if (!sampleFile.existsAsFile())
        {
            missingSampleIds.addIfNotAlreadyThere(padInfo.freesoundId);
        }
    }

    int totalSamples = allUniqueSampleIds.size();
    int missingSamples = missingSampleIds.size();

    if (totalSamples == 0)
    {
        // Empty preset - load normally
        loadPresetNormally(presetInfo, slotIndex);
        return;
    }

    if (missingSamples == totalSamples)
    {
        // All samples are missing
        AlertWindow::showMessageBoxAsync(
            AlertWindow::WarningIcon,
            "All Samples Missing",
            "All " + String(totalSamples) + " samples for this preset are missing from the resources folder.\n\n" +
            "Press the ⬇ icon next to \"" + presetInfo.name + "\" in the preset browser to download the missing samples, then try loading again.");
        return;
    }
    else if (missingSamples > 0)
    {
        // Some samples are missing
        String message = String(missingSamples) + " of " + String(totalSamples) +
                        " samples are missing from the resources folder.\n\n" +
                        "The preset will load with available samples only. " +
                        "Press the ⬇ icon next to \"" + presetInfo.name + "\" to recover missing samples.";

        AlertWindow::showOkCancelBox(
            AlertWindow::QuestionIcon,
            "Some Samples Missing",
            message,
            "Load Anyway", "Cancel",
            nullptr,
            ModalCallbackFunction::create([this, presetInfo, slotIndex](int result) {
                if (result == 1) { // User clicked "Load Anyway"
                    loadPresetNormally(presetInfo, slotIndex);
                }
                // If result == 0, user clicked "Cancel" - do nothing
            })
        );
        return;
    }

    // All samples are available - load normally
    loadPresetNormally(presetInfo, slotIndex);
}

void FreesoundAdvancedSamplerAudioProcessorEditor::loadPresetNormally(const PresetInfo& presetInfo, int slotIndex)
{
    if (processor.loadPreset(presetInfo.presetFile, slotIndex))
    {
        // Update the sample grid with the loaded preset
        sampleGridComponent.updateSamples(processor.getCurrentSounds(), processor.getData());

        // Refresh preset browser to update active slot display
        presetBrowserComponent.refreshPresetList();
    }
    else
    {
        // Show error message
        AlertWindow::showMessageBoxAsync(
            AlertWindow::WarningIcon,
            "Load Failed",
            "Failed to load preset \"" + presetInfo.name + "\" slot " + String(slotIndex + 1) + ".");
    }
}

bool FreesoundAdvancedSamplerAudioProcessorEditor::keyPressed(const KeyPress& key)
{
    int padIndex = getKeyboardPadIndex(key);

    if (padIndex >= 0 && padIndex < 16)
    {
        // Check if this pad has a valid sample by getting info from the grid
        auto padInfo = sampleGridComponent.getPadInfo(padIndex);
        if (padInfo.hasValidSample)
        {
            int noteNumber = padIndex + 36;
            processor.addToMidiBuffer(noteNumber);

            // Optional: Add some visual feedback
            repaint();

            return true; // Key was handled
        }
    }

    return Component::keyPressed(key); // Let parent handle unrecognized keys
}

int FreesoundAdvancedSamplerAudioProcessorEditor::getKeyboardPadIndex(const KeyPress& key) const
{
    int keyCode = key.getKeyCode();

    // Map keyboard keys to pad indices
    // Grid layout (bottom row to top row):
    // Bottom row (pads 0-3): z x c v
    // Second row (pads 4-7): a s d f
    // Third row (pads 8-11): q w e r
    // Top row (pads 12-15): 1 2 3 4

    switch (keyCode)
    {
        // Bottom row (pads 0-3)
        case 'z': case 'Z': return 0;
        case 'x': case 'X': return 1;
        case 'c': case 'C': return 2;
        case 'v': case 'V': return 3;

        // Second row (pads 4-7)
        case 'a': case 'A': return 4;
        case 's': case 'S': return 5;
        case 'd': case 'D': return 6;
        case 'f': case 'F': return 7;

        // Third row (pads 8-11)
        case 'q': case 'Q': return 8;
        case 'w': case 'W': return 9;
        case 'e': case 'E': return 10;
        case 'r': case 'R': return 11;

        // Top row (pads 12-15)
        case '1': return 12;
        case '2': return 13;
        case '3': return 14;
        case '4': return 15;

        default: return -1; // Key not mapped
    }
}