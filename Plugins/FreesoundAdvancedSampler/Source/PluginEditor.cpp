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
   customLookAndFeel = std::make_unique<CustomLookAndFeel>();
   LookAndFeel::setDefaultLookAndFeel(customLookAndFeel.get());

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

   // Set up preset browser with collection manager support
   presetBrowserComponent.setProcessor(&processor);
   presetBrowserComponent.refreshPresetList();
   presetBrowserComponent.onPresetLoadRequested = [this](const String& presetId, int slotIndex) {
       handlePresetLoadRequested(presetId, slotIndex);
   };

   // Set up expandable panel with preset browser inside
   expandablePanelComponent.setOrientation(ExpandablePanel::Orientation::Right);
   expandablePanelComponent.setContentComponent(&presetBrowserComponent);
   expandablePanelComponent.setExpandedWidth(220);

   // RESTORE SAVED STATE instead of hardcoded false
   expandablePanelComponent.setExpanded(processor.getPresetPanelExpandedState());

   expandablePanelComponent.onExpandedStateChanged = [this](bool expanded) {
       processor.setPresetPanelExpandedState(expanded);
       updateWindowSizeForPanelState();
   };
   addAndMakeVisible(expandablePanelComponent);

   // Set up bookmark viewer with expandable panel
   bookmarkExpandablePanel.setOrientation(ExpandablePanel::Orientation::Left);
   bookmarkExpandablePanel.setContentComponent(&bookmarkViewerComponent);
   bookmarkExpandablePanel.setExpandedWidth(220);

   // RESTORE SAVED STATE instead of hardcoded false
   bookmarkExpandablePanel.setExpanded(processor.getBookmarkPanelExpandedState());

   bookmarkExpandablePanel.onExpandedStateChanged = [this](bool expanded) {
       processor.setBookmarkPanelExpandedState(expanded);
       updateWindowSizeForBookmarkPanel();
   };
   addAndMakeVisible(bookmarkExpandablePanel);

   // Set processor for bookmark viewer
   bookmarkViewerComponent.setProcessor(&processor);

   // Restore samples from current processor state using collection manager
   sampleGridComponent.refreshFromProcessor();

   // Make sure the grid can receive keyboard focus and starts with focus
   sampleGridComponent.setWantsKeyboardFocus(true);
   setWantsKeyboardFocus(true);

   // IMPORTANT: Calculate and restore window size AFTER setting panel states
   int savedWidth = processor.getSavedWindowWidth();
   int savedHeight = processor.getSavedWindowHeight();

   // Calculate the actual required width based on panel states
   int baseWidth = 800; // Base width when no panels expanded
   int totalPanelWidth = 0;

   if (expandablePanelComponent.isExpanded()) {
       totalPanelWidth += expandablePanelComponent.getExpandedWidth() - expandablePanelComponent.getCollapsedWidth() + 4;
   }

   if (bookmarkExpandablePanel.isExpanded()) {
       totalPanelWidth += bookmarkExpandablePanel.getExpandedWidth() - bookmarkExpandablePanel.getCollapsedWidth() + 4;
   }

   int calculatedWidth = baseWidth + totalPanelWidth;

   // Update size constraints based on current panel states
   updateSizeConstraintsForCurrentPanelStates();

   setSize(savedWidth, savedHeight);

   // FIXED: Use a WeakReference instead of Timer::callAfterDelay to prevent crashes
   Component::SafePointer safeThis(this);
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

   // Migration from old system (one-time operation)
   static bool migrationDone = false;
   if (!migrationDone)
   {
       processor.migrateFromOldSystem();
       migrationDone = true;
   }
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

void FreesoundAdvancedSamplerAudioProcessorEditor::updateSizeConstraintsForCurrentPanelStates()
{
   int baseMinWidth = 600;
   int baseMaxWidth = 1600;

   // Calculate additional width needed for expanded panels
   int additionalWidth = 0;

   if (expandablePanelComponent.isExpanded()) {
       additionalWidth += expandablePanelComponent.getExpandedWidth() - expandablePanelComponent.getCollapsedWidth() + 4;
   }

   if (bookmarkExpandablePanel.isExpanded()) {
       additionalWidth += bookmarkExpandablePanel.getExpandedWidth() - bookmarkExpandablePanel.getCollapsedWidth() + 4;
   }

   setResizeLimits(baseMinWidth + additionalWidth, 500, baseMaxWidth + additionalWidth, 1200);
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

   // LEFT: Bookmark expandable panel
   int bookmarkPanelWidth = bookmarkExpandablePanel.isExpanded() ?
                           bookmarkExpandablePanel.getExpandedWidth() :
                           bookmarkExpandablePanel.getCollapsedWidth();

   auto leftPanelArea = contentBounds.removeFromLeft(bookmarkPanelWidth);
   if (bookmarkExpandablePanel.isExpanded())
       leftPanelArea.removeFromRight(spacing);
   bookmarkExpandablePanel.setBounds(leftPanelArea);

   // RIGHT: Preset expandable panel
   int presetPanelWidth = expandablePanelComponent.isExpanded() ?
                         expandablePanelComponent.getExpandedWidth() :
                         expandablePanelComponent.getCollapsedWidth();

   auto rightPanelArea = contentBounds.removeFromRight(presetPanelWidth);
   if (expandablePanelComponent.isExpanded())
       rightPanelArea.removeFromLeft(spacing);
   expandablePanelComponent.setBounds(rightPanelArea);

   // CENTER: Sample grid and controls
   contentBounds.removeFromLeft(spacing);
   contentBounds.removeFromRight(spacing);

   // Control area above the sample grid (top-left)
   auto controlArea = contentBounds.removeFromTop(buttonHeight);
   controlArea = controlArea.removeFromBottom(buttonHeight);

   // Split control area between drag area and directory button
   controlArea = controlArea.reduced(4);
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

   int panelWidthDifference = expandablePanelComponent.getExpandedWidth() -
                              expandablePanelComponent.getCollapsedWidth();

   if (expandablePanelComponent.isExpanded())
   {
       int newWidth = currentWidth + panelWidthDifference + 4;
       setSize(newWidth, currentHeight);
   }
   else
   {
       int newWidth = currentWidth - panelWidthDifference - 4;
       setSize(newWidth, currentHeight);
   }

   // Update constraints for new panel state
   updateSizeConstraintsForCurrentPanelStates();

   // Save the new window size
   processor.setWindowSize(getWidth(), getHeight());
}

void FreesoundAdvancedSamplerAudioProcessorEditor::updateWindowSizeForBookmarkPanel()
{
   int currentWidth = getWidth();
   int currentHeight = getHeight();

   int panelWidthDifference = bookmarkExpandablePanel.getExpandedWidth() -
                              bookmarkExpandablePanel.getCollapsedWidth();

   if (bookmarkExpandablePanel.isExpanded())
   {
       int newWidth = currentWidth + panelWidthDifference + 4;
       setSize(newWidth, currentHeight);
   }
   else
   {
       int newWidth = currentWidth - panelWidthDifference - 4;
       setSize(newWidth, currentHeight);
   }

   // Update constraints for new panel state
   updateSizeConstraintsForCurrentPanelStates();

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

void FreesoundAdvancedSamplerAudioProcessorEditor::handlePresetLoadRequested(const String& presetId, int slotIndex)
{
   if (!processor.getCollectionManager())
   {
       AlertWindow::showMessageBoxAsync(
           AlertWindow::WarningIcon,
           "Load Failed",
           "Collection manager not available.");
       return;
   }

   // First, check sample availability before loading
   Array<SampleMetadata> slotSamples = processor.getCollectionManager()->loadPresetSlot(presetId, slotIndex);

   Array<String> allUniqueSampleIds;
   Array<String> missingSampleIds;

   // Check which samples exist
   for (const auto& sample : slotSamples)
   {
       if (sample.freesoundId.isNotEmpty())
       {
           allUniqueSampleIds.addIfNotAlreadyThere(sample.freesoundId);

           File sampleFile = processor.getCollectionManager()->getSampleFile(sample.freesoundId);
           if (!sampleFile.existsAsFile())
           {
               missingSampleIds.addIfNotAlreadyThere(sample.freesoundId);
           }
       }
   }

   int totalSamples = allUniqueSampleIds.size();
   int missingSamples = missingSampleIds.size();

   if (totalSamples == 0)
   {
       // Empty preset - load normally
       loadPresetNormally(presetId, slotIndex);
       return;
   }

   if (missingSamples == totalSamples)
   {
       // Get preset info for error message
       auto presetInfo = processor.getCollectionManager()->getPreset(presetId);
       AlertWindow::showMessageBoxAsync(
           AlertWindow::WarningIcon,
           "All Samples Missing",
           "All " + String(totalSamples) + " samples for this preset slot are missing from the resources folder.\n\n" +
           "The samples need to be re-downloaded. This preset may have been created with samples that are no longer available.");
   }
   else if (missingSamples > 0)
   {
       // Some samples are missing
       String message = String(missingSamples) + " of " + String(totalSamples) +
                       " samples are missing from the resources folder.\n\n" +
                       "The preset will load with available samples only. " +
                       "Missing samples may need to be re-downloaded.";

       AlertWindow::showOkCancelBox(
           AlertWindow::QuestionIcon,
           "Some Samples Missing",
           message,
           "Load Anyway", "Cancel",
           nullptr,
           ModalCallbackFunction::create([this, presetId, slotIndex](int result) {
               if (result == 1) { // User clicked "Load Anyway"
                   loadPresetNormally(presetId, slotIndex);
               }
           })
       );
       return;
   }

   // All samples are available - load normally
   loadPresetNormally(presetId, slotIndex);
}

void FreesoundAdvancedSamplerAudioProcessorEditor::loadPresetNormally(const String& presetId, int slotIndex)
{
   if (processor.loadPreset(presetId, slotIndex))
   {
       // Update the sample grid from processor state
       sampleGridComponent.refreshFromProcessor();

       // Refresh preset browser to update active slot display
       presetBrowserComponent.refreshPresetList();
   }
   else
   {
       // Get preset info for error message
       auto presetInfo = processor.getCollectionManager()->getPreset(presetId);
       AlertWindow::showMessageBoxAsync(
           AlertWindow::WarningIcon,
           "Load Failed",
           "Failed to load preset \"" + presetInfo.name + "\" slot " + String(slotIndex + 1) + ".");
   }
}

bool FreesoundAdvancedSamplerAudioProcessorEditor::saveCurrentAsPreset(const String& name, const String& description, int slotIndex)
{
   String presetId = processor.saveCurrentAsPreset(name, description, slotIndex);
   if (!presetId.isEmpty())
   {
       presetBrowserComponent.refreshPresetList();
       return true;
   }
   return false;
}

bool FreesoundAdvancedSamplerAudioProcessorEditor::loadPreset(const String& presetId, int slotIndex)
{
   if (processor.loadPreset(presetId, slotIndex))
   {
       sampleGridComponent.refreshFromProcessor();
       presetBrowserComponent.refreshPresetList();
       return true;
   }
   return false;
}

bool FreesoundAdvancedSamplerAudioProcessorEditor::saveToSlot(const String& presetId, int slotIndex, const String& description)
{
   if (processor.saveToSlot(presetId, slotIndex, description))
   {
       presetBrowserComponent.refreshPresetList();
       return true;
   }
   return false;
}

bool FreesoundAdvancedSamplerAudioProcessorEditor::keyPressed(const KeyPress& key)
{
   int padIndex = getKeyboardPadIndex(key);

   // Check if this is one of our valid sample trigger keys
   if (padIndex >= 0 && padIndex < 16)
   {
       // This is a valid sample trigger key - check if pad has a sample
       String freesoundId = processor.getPadFreesoundId(padIndex);
       if (freesoundId.isNotEmpty())
       {
           // Sample exists - trigger it
           int noteNumber = padIndex + 36;
           processor.addNoteOnToMidiBuffer(noteNumber);
           repaint(); // Optional visual feedback
       }

       // CRITICAL: Always return true for valid sample keys
       // This prevents OS click sound even for empty pads
       return true;
   }

   // Handle other common keys that might cause OS clicks
   int keyCode = key.getKeyCode();
   if (keyCode == KeyPress::spaceKey ||
       keyCode == KeyPress::returnKey ||
       keyCode == KeyPress::escapeKey ||
       keyCode == KeyPress::tabKey)
   {
       // Consume these keys to prevent OS clicks
       return true;
   }

   // For any other key, let the parent component handle it
   return Component::keyPressed(key);
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