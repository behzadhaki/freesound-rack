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

    setSize (800, 700);  // Increased size for grid layout
    setResizable(false, false);
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

void FreesoundAdvancedSamplerAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    int margin = 10;
    int searchHeight = 100;
    int progressHeight = 60;

    // Search component at top
    freesoundSearchComponent.setBounds(bounds.removeFromTop(searchHeight).reduced(margin));

    // Progress components at bottom
    auto progressBounds = bounds.removeFromBottom(progressHeight).reduced(margin);
    statusLabel.setBounds(progressBounds.removeFromTop(20));
    progressBar.setBounds(progressBounds.removeFromTop(20));
    cancelButton.setBounds(progressBounds.removeFromTop(20));

    // Sample grid takes remaining space
    sampleGridComponent.setBounds(bounds.reduced(margin));
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
            // Update the sample grid with downloaded samples
            sampleGridComponent.updateSamples(processor.getCurrentSounds(), processor.getData());
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