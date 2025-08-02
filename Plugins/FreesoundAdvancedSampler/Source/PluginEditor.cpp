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
    : AudioProcessorEditor (&p), processor (p), progressBar(currentProgress)
{
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
    expandablePanelComponent.onExpandedStateChanged = [this](bool expanded) {
        // When panel expands/collapses, update window size constraints and current size
        updateWindowSizeForPanelState();
    };
    addAndMakeVisible(expandablePanelComponent);

    // Set up progress components
    addAndMakeVisible(progressBar);
    addAndMakeVisible(statusLabel);
    addAndMakeVisible(cancelButton);

    statusLabel.setText("Ready", dontSendNotification);
    cancelButton.setButtonText("Cancel Download");
    cancelButton.setEnabled(false);

    cancelButton.onClick = [this]
    {
        processor.cancelDownloads();
        cancelButton.setEnabled(false);
        statusLabel.setText("Download cancelled", dontSendNotification);
    };

    // Initially hide progress components
    progressBar.setVisible(false);
    statusLabel.setVisible(false);
    cancelButton.setVisible(false);

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
}

FreesoundAdvancedSamplerAudioProcessorEditor::~FreesoundAdvancedSamplerAudioProcessorEditor()
{
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
    const int progressHeight = 50;

    // FIXED VALUES - Don't scale these with window size
    const int buttonHeight = 30;         // Keep this fixed
    const int buttonWidth = 80;        // Keep this fixed
    const int spacing = 4;               // Keep this fixed

    bounds.removeFromTop(margin);
    bounds.removeFromRight(margin);

    // Progress components at bottom (can scale height)
    auto progressBounds = bounds.removeFromBottom(progressHeight).reduced(margin, 0);
    statusLabel.setBounds(progressBounds.removeFromTop(18));
    progressBar.setBounds(progressBounds.removeFromTop(18));
    cancelButton.setBounds(progressBounds.removeFromTop(18));

    // Main content area
    bounds.removeFromLeft(margin);
    auto contentBounds = bounds.withTrimmedBottom(margin);

    // Expandable panel (with preset browser) on the left - FIXED WIDTH when expanded
    int panelWidth = expandablePanelComponent.isExpanded() ?
                     expandablePanelComponent.getExpandedWidth() :
                     expandablePanelComponent.getCollapsedWidth();

    auto leftPanelArea = contentBounds.removeFromLeft(panelWidth);
    if (expandablePanelComponent.isExpanded())
        leftPanelArea.removeFromRight(spacing); // Add spacing when expanded
    expandablePanelComponent.setBounds(leftPanelArea);

    // Remaining area for sample grid and controls
    contentBounds.removeFromLeft(spacing);

    // Control area above the sample grid (top-left)
    auto controlArea = contentBounds.removeFromTop(buttonHeight);
    controlArea = controlArea.removeFromBottom(buttonHeight);

    // Split control area between drag area and directory button
    sampleDragArea.setBounds(controlArea.removeFromLeft(buttonWidth));
    directoryOpenButton.setBounds(controlArea.removeFromRight(buttonWidth)); // Fixed width for directory button

    // Sample grid - takes ALL remaining space below the controls
    contentBounds.removeFromTop(spacing); // Small gap between controls and grid
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

void FreesoundAdvancedSamplerAudioProcessorEditor::downloadProgressChanged(const AudioDownloadManager::DownloadProgress& progress)
{
    // This might be called from background thread, so ensure UI updates happen on message thread
    MessageManager::callAsync([this, progress]()
    {
        currentProgress = progress.overallProgress;

        String statusText = "Downloading " + progress.currentFileName +
                           " (" + String(progress.completedFiles) +
                           "/" + String(progress.totalFiles) + ") - " +
                           String((int)(progress.overallProgress * 100)) + "%";

        statusLabel.setText(statusText, dontSendNotification);
        statusLabel.setVisible(true);
        progressBar.setVisible(true);
        cancelButton.setVisible(true);
        cancelButton.setEnabled(true);
        progressBar.repaint();
    });
}

// In PluginEditor.cpp - Replace the existing downloadCompleted method with this version:

void FreesoundAdvancedSamplerAudioProcessorEditor::downloadCompleted(bool success)
{
    MessageManager::callAsync([this, success]()
    {
        cancelButton.setEnabled(false);

        statusLabel.setText(success ? "All downloads completed!" :
                          "Download completed with errors",
                          dontSendNotification);

        currentProgress = success ? 1.0 : 0.0;
        progressBar.repaint();

        if (success)
        {
            // Add a small delay to ensure all files are written to disk
            Timer::callAfterDelay(500, [this]()
            {
                // DO NOT call updateSamples here for master search operations
                // The grid has already been updated by handleMasterSearch and should preserve text boxes
                // Only update for other operations like preset loading

                // Instead, just refresh the sampler sources and preset browser
                // The grid visual state should already be correct from handleMasterSearch

                // Refresh preset browser to show if this creates a new preset opportunity
                presetBrowserComponent.refreshPresetList();
            });
        }

        // Hide progress components after a delay
        Timer::callAfterDelay(2000, [this]()
        {
            progressBar.setVisible(false);
            statusLabel.setVisible(false);
            cancelButton.setVisible(false);
        });
    });
}

void FreesoundAdvancedSamplerAudioProcessorEditor::handlePresetLoadRequested(const PresetInfo& presetInfo, int slotIndex)
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
            "Failed to load preset \"" + presetInfo.name + "\" slot " + String(slotIndex + 1) + ". Some samples may be missing.");
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