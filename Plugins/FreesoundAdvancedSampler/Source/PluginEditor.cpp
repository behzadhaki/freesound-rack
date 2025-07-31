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

    // Set up search component
    freesoundSearchComponent.setProcessor(&processor);
    addAndMakeVisible (freesoundSearchComponent);

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
    presetBrowserComponent.onPresetLoadRequested = [this](const PresetInfo& presetInfo, int slotIndex) {
        handlePresetLoadRequested(presetInfo, slotIndex);
    };
    addAndMakeVisible(presetBrowserComponent);

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

    // Set up sample grid component
    sampleGridComponent.setProcessor(&processor);
    addAndMakeVisible(sampleGridComponent);

    // Make sure the grid can receive keyboard focus and starts with focus
    sampleGridComponent.setWantsKeyboardFocus(true);

    setWantsKeyboardFocus(true);

    setSize (1200, 700);  // Increased width for preset browser
    setResizable(false, false);

    // Grab focus after a delay to ensure component is ready
    Timer::callAfterDelay(200, [this]() {
        if (isShowing()) {
            grabKeyboardFocus();
        }
    });
}

FreesoundAdvancedSamplerAudioProcessorEditor::~FreesoundAdvancedSamplerAudioProcessorEditor()
{
    processor.removeDownloadListener(this);
}

//==============================================================================
void FreesoundAdvancedSamplerAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
}

// Update this method in PluginEditor.cpp
void FreesoundAdvancedSamplerAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    int margin = 10;
    int searchHeight = 100;
    int progressHeight = 60;
    int bottomControlsHeight = 40;
    int buttonWidth = 60;
    int presetBrowserWidth = 300;
    int spacing = 5;

    // Search component at top
    freesoundSearchComponent.setBounds(bounds.removeFromTop(searchHeight).reduced(margin));

    // Progress components at bottom
    auto progressBounds = bounds.removeFromBottom(progressHeight).reduced(margin);
    statusLabel.setBounds(progressBounds.removeFromTop(20));
    progressBar.setBounds(progressBounds.removeFromTop(20));
    cancelButton.setBounds(progressBounds.removeFromTop(20));

    // Main content area
    auto contentBounds = bounds.reduced(margin);

    // Preset browser on the right side
    presetBrowserComponent.setBounds(contentBounds.removeFromRight(presetBrowserWidth));
    contentBounds.removeFromRight(margin); // Spacing between grid and preset browser

    // Bottom controls area for external controls only (View Files and Drag)
    auto bottomControlsBounds = contentBounds.removeFromBottom(bottomControlsHeight);

    // View Files and Drag area at bottom right
    directoryOpenButton.setBounds(bottomControlsBounds.removeFromRight(buttonWidth));
    bottomControlsBounds.removeFromRight(spacing);
    sampleDragArea.setBounds(bottomControlsBounds.removeFromRight(80)); // Drag area width

    // Sample grid takes remaining space (it will handle its own shuffle button internally)
    sampleGridComponent.setBounds(contentBounds);
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