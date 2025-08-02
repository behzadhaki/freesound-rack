/*
  ==============================================================================

    SampleGridComponent.cpp
    Created: New grid component for displaying samples in 4x4 grid
    Author: Generated
    Modified: Added drag-and-drop swap functionality and shuffle button

  ==============================================================================
*/

#include "SampleGridComponent.h"

//==============================================================================
// SamplePad Implementation
//==============================================================================

// In SamplePad constructor:
SamplePad::SamplePad(int index)
    : padIndex(index)
    , processor(nullptr)
    , audioThumbnailCache(5)
    , audioThumbnail(512, formatManager, audioThumbnailCache)
    , freesoundId(String())
    , licenseType(String())
    , padQuery(String())
    , playheadPosition(0.0f)
    , isPlaying(false)
    , hasValidSample(false)
    , isDragHover(false)
    , currentDownloadProgress(0.0)
{
    formatManager.registerBasicFormats();

    // Generate a unique color for each pad
    float hue = (float)padIndex / 16.0f;
    padColour = Colour::fromHSV(hue, 0.3f, 0.8f, 1.0f);

    // Set up query text box (existing code)
    queryTextBox.setMultiLine(false);
    queryTextBox.setReturnKeyStartsNewLine(false);
    queryTextBox.setReadOnly(false);
    queryTextBox.setScrollbarsShown(false);
    queryTextBox.setCaretVisible(true);
    queryTextBox.setPopupMenuEnabled(true);
    queryTextBox.setColour(TextEditor::backgroundColourId, Colours::white.withAlpha(0.8f));
    queryTextBox.setColour(TextEditor::textColourId, Colours::black);
    queryTextBox.setColour(TextEditor::highlightColourId, Colours::blue.withAlpha(0.3f));
    queryTextBox.setColour(TextEditor::outlineColourId, Colours::grey);
    queryTextBox.setFont(Font(9.0f));
    queryTextBox.onReturnKey = [this]() {
        String query = queryTextBox.getText().trim();
        if (query.isNotEmpty())
        {
            if (auto* gridComponent = findParentComponentOfClass<SampleGridComponent>())
            {
                gridComponent->searchForSinglePadWithQuery(padIndex, query);
            }
        }
    };
    addAndMakeVisible(queryTextBox);

    // DON'T create progress components in constructor - create them on demand
}

SamplePad::~SamplePad()
{
    stopTimer(); // Stop any running timers
    cleanupProgressComponents(); // Clean up progress components
}

void SamplePad::resized()
{
    auto bounds = getLocalBounds();

    // Position query text box or progress components at the bottom
    auto bottomBounds = bounds.reduced(3);
    int bottomHeight = 14;
    int dragBadgeWidth = 30;
    int searchBadgeWidth = 35;
    int badgeSpacing = 3;

    bottomBounds = bottomBounds.removeFromBottom(bottomHeight);

    if (isDownloading && progressBar && progressLabel)
    {
        // Show progress bar and label instead of query text box
        auto progressArea = bottomBounds.reduced(2);

        // Split area: progress bar on top, label below
        auto labelBounds = progressArea.removeFromBottom(8);
        progressLabel->setBounds(labelBounds);
        progressBar->setBounds(progressArea);
    }
    else if (!isDownloading)
    {
        // Normal layout with query text box
        bottomBounds.removeFromLeft(dragBadgeWidth + badgeSpacing);
        bottomBounds.removeFromRight(searchBadgeWidth + badgeSpacing);
        queryTextBox.setBounds(bottomBounds);
    }
}


void SamplePad::paint(Graphics& g)
{
    auto bounds = getLocalBounds();

    // Modern dark background with subtle gradients (existing code stays the same)
    if (isDragHover)
    {
        g.setColour(Colour(0x8000D9FF).withAlpha(0.6f));
    }
    else if (isPlaying)
    {
        g.setGradientFill(ColourGradient(
            padColour.brighter(0.4f), bounds.getCentre().toFloat(),
            padColour.darker(0.2f), bounds.getBottomRight().toFloat(), false));
    }
    else if (isDownloading)
    {
        g.setGradientFill(ColourGradient(
            Colour(0x8000D9FF).withAlpha(0.3f), bounds.getTopLeft().toFloat(),
            Colour(0x800099CC).withAlpha(0.3f), bounds.getBottomRight().toFloat(), false));
    }
    else if (hasValidSample)
    {
        g.setGradientFill(ColourGradient(
            padColour.withAlpha(0.3f), bounds.getTopLeft().toFloat(),
            padColour.darker(0.3f).withAlpha(0.3f), bounds.getBottomRight().toFloat(), false));
    }
    else
    {
        g.setColour(Colour(0x801A1A1A));
    }

    g.fillRoundedRectangle(bounds.toFloat(), 6.0f);

    // Modern border styling (existing code stays the same)
    if (isDragHover)
    {
        g.setColour(Colour(0x8000D9FF));
        g.drawRoundedRectangle(bounds.toFloat().reduced(1), 6.0f, 2.5f);
    }
    else if (isDownloading)
    {
        g.setColour(Colour(0x8000D9FF));
        g.drawRoundedRectangle(bounds.toFloat().reduced(1), 6.0f, 2.0f);
    }
    else if (isPlaying)
    {
        g.setColour(Colours::white);
        g.drawRoundedRectangle(bounds.toFloat().reduced(1), 6.0f, 2.0f);
    }
    else if (hasValidSample)
    {
        g.setColour(Colour(0x80404040));
        g.drawRoundedRectangle(bounds.toFloat().reduced(1), 6.0f, 1.0f);
    }
    else
    {
        g.setColour(Colour(0x802A2A2A));
        g.drawRoundedRectangle(bounds.toFloat().reduced(1), 6.0f, 1.0f);
    }

    // === TOP LINE: MIDI/Keyboard number + Web badge + License badge ===

    // MIDI Note and Keyboard Key display (UPDATED)
    g.setColour(Colours::white);
    g.setFont(Font(8.0f, Font::bold));
    auto numberBounds = bounds.reduced(4);
    auto numberRect = numberBounds.removeFromTop(14).removeFromLeft(24); // Slightly wider for MIDI info

    // Small dark background for MIDI/keyboard info
    g.setColour(Colour(0x80000000).withAlpha(0.6f));
    g.fillRoundedRectangle(numberRect.toFloat(), 2.0f);
    g.setColour(Colours::white);

    // Calculate MIDI note number (pad 0 = MIDI note 36, pad 1 = note 37, etc.)
    int midiNote = padIndex + 36;

    // Get keyboard key for this pad
    String keyboardKey = getKeyboardKeyForPad(padIndex);

    // Format display text: "MIDI/Key" format
    String displayText = String(midiNote) + " | " + keyboardKey;

    g.drawText(displayText, numberRect, Justification::centred);

    // Web badge and License badge in top-right area (existing code stays the same)
    if (hasValidSample)
    {
        g.setFont(Font(8.0f, Font::bold));

        // Start from top-right and work leftward
        auto topRightBounds = bounds.reduced(4);
        topRightBounds = topRightBounds.removeFromTop(14);

        // DEL badge (rightmost) - Darker red styling
        if (hasValidSample)
        {
            int delBadgeWidth = 28;
            auto delBounds = topRightBounds.removeFromRight(delBadgeWidth);

            g.setGradientFill(ColourGradient(
                Colour(0x80C62828), delBounds.getTopLeft().toFloat(),
                Colour(0x808E0000), delBounds.getBottomRight().toFloat(), false));
            g.fillRoundedRectangle(delBounds.toFloat(), 3.0f);

            g.setColour(Colours::white.withAlpha(0.9f));
            g.drawText("DEL", delBounds, Justification::centred);

            topRightBounds.removeFromRight(3);
        }

        // License badge (middle) - Darker amber styling
        if (licenseType.isNotEmpty())
        {
            String shortLicense = getLicenseShortName(licenseType);
            int licenseBadgeWidth = 32;
            auto licenseBounds = topRightBounds.removeFromRight(licenseBadgeWidth);

            g.setGradientFill(ColourGradient(
                Colour(0x80EF6C00), licenseBounds.getTopLeft().toFloat(),
                Colour(0x80BF360C), licenseBounds.getBottomRight().toFloat(), false));
            g.fillRoundedRectangle(licenseBounds.toFloat(), 3.0f);

            g.setColour(Colours::white.withAlpha(0.9f));
            g.drawText(shortLicense, licenseBounds, Justification::centred);

            topRightBounds.removeFromRight(3);
        }

        // Web badge (leftmost) - Darker blue styling
        if (freesoundId.isNotEmpty())
        {
            int webBadgeWidth = 28;
            auto webBounds = topRightBounds.removeFromRight(webBadgeWidth);

            g.setGradientFill(ColourGradient(
                Colour(0x800277BD), webBounds.getTopLeft().toFloat(),
                Colour(0x8001579B), webBounds.getBottomRight().toFloat(), false));
            g.fillRoundedRectangle(webBounds.toFloat(), 3.0f);

            g.setColour(Colours::white.withAlpha(0.9f));
            g.drawText("Web", webBounds, Justification::centred);
        }
    }

    // === MIDDLE: Waveform and sample text === (existing code stays the same)

    if (hasValidSample)
    {
        // Adjust waveform bounds to account for top and bottom badges
        auto waveformBounds = bounds.reduced(8);
        waveformBounds.removeFromTop(18); // Space for top line badges
        waveformBounds.removeFromBottom(18); // Space for bottom line

        // Draw waveform with modern styling
        drawWaveform(g, waveformBounds);

        // Draw playhead if playing
        if (isPlaying)
        {
            drawPlayhead(g, waveformBounds);
        }

        // Draw filename and author with modern white text
        g.setColour(Colours::white);
        g.setFont(Font(8.5f, Font::bold));

        // Format filename to max 10 characters with ellipsis if needed
        String displayName = sampleName;
        if (displayName.length() > 10)
        {
            displayName = displayName.substring(0, 7) + "...";
        }

        // Format author name
        String displayAuthor = authorName;
        if (displayAuthor.length() > 10)
        {
            displayAuthor = displayAuthor.substring(0, 7) + "...";
        }

        // Create the full text string
        String displayText = displayName + " by " + displayAuthor;

        // Position at bottom of waveform area with dark background
        auto filenameBounds = waveformBounds.removeFromBottom(14);
        g.setColour(Colour(0x80000000).withAlpha(0.7f));
        g.fillRoundedRectangle(filenameBounds.toFloat(), 2.0f);
        g.setColour(Colours::white);
        g.drawText(displayText, filenameBounds, Justification::centred, true);
    }

    // === BOTTOM LINE: Drag badge + Query text box + Search badge === (existing code stays the same)

    // Drag badge in bottom-left corner - Modern green styling
    if (hasValidSample)
    {
        g.setFont(Font(8.0f, Font::bold));

        auto dragBounds = bounds.reduced(4);
        int badgeWidth = 28;
        int badgeHeight = 14;
        dragBounds = dragBounds.removeFromBottom(badgeHeight).removeFromLeft(badgeWidth);

        g.setGradientFill(ColourGradient(
            Colour(0x804ECDC4), dragBounds.getTopLeft().toFloat(),
            Colour(0x8026A69A), dragBounds.getBottomRight().toFloat(), false));
        g.fillRoundedRectangle(dragBounds.toFloat(), 3.0f);

        g.setColour(Colours::white);
        g.drawText("Drag", dragBounds, Justification::centred);
    }

    // Search badge in bottom-right corner (always visible when not downloading)
    if (!isDownloading)
    {
        g.setFont(Font(8.0f, Font::bold));

        auto searchBounds = bounds.reduced(4);
        int badgeWidth = 32;
        int badgeHeight = 14;
        searchBounds = searchBounds.removeFromBottom(badgeHeight).removeFromRight(badgeWidth);

        g.setGradientFill(ColourGradient(
            Colour(0x809C88FF), searchBounds.getTopLeft().toFloat(),
            Colour(0x807B68EE), searchBounds.getBottomRight().toFloat(), false));
        g.fillRoundedRectangle(searchBounds.toFloat(), 3.0f);

        g.setColour(Colours::white);
        g.drawText("Search", searchBounds, Justification::centred);
    }

    // Empty pad text (only show when not downloading and no sample)
    if (!hasValidSample && !isDownloading)
    {
        g.setColour(Colour(0x80666666));
        g.setFont(Font(12.0f, Font::bold));
        auto emptyBounds = bounds.reduced(8);
        emptyBounds.removeFromTop(18);
        emptyBounds.removeFromBottom(18);
        g.drawText("Empty", emptyBounds, Justification::centred);
    }
}

String SamplePad::getKeyboardKeyForPad(int padIndex) const
{
    // Map pad indices to keyboard keys (matching the keyPressed mapping in PluginEditor)
    // Grid layout (bottom row to top row):
    // Bottom row (pads 0-3): z x c v
    // Second row (pads 4-7): a s d f
    // Third row (pads 8-11): q w e r
    // Top row (pads 12-15): 1 2 3 4

    switch (padIndex)
    {
        // Bottom row (pads 0-3)
        case 0: return "Z";
        case 1: return "X";
        case 2: return "C";
        case 3: return "V";

        // Second row (pads 4-7)
        case 4: return "A";
        case 5: return "S";
        case 6: return "D";
        case 7: return "F";

        // Third row (pads 8-11)
        case 8: return "Q";
        case 9: return "W";
        case 10: return "E";
        case 11: return "R";

        // Top row (pads 12-15)
        case 12: return "1";
        case 13: return "2";
        case 14: return "3";
        case 15: return "4";

        default: return "?";
    }
}


void SamplePad::mouseDown(const MouseEvent& event)
{
    // Don't allow interaction while downloading
    if (isDownloading)
        return;

    // Check if clicked on search badge (bottom-right) - ALWAYS available when not downloading
    {
        auto bounds = getLocalBounds();
        auto searchBounds = bounds.reduced(3);
        int badgeWidth = 35;
        int badgeHeight = 12;
        searchBounds = searchBounds.removeFromBottom(badgeHeight).removeFromRight(badgeWidth);

        if (searchBounds.contains(event.getPosition()))
        {
            String searchQuery = queryTextBox.getText().trim();
            if (searchQuery.isEmpty() && processor)
            {
                searchQuery = processor->getQuery();
            }

            if (searchQuery.isNotEmpty())
            {
                queryTextBox.setText(searchQuery);

                if (auto* gridComponent = findParentComponentOfClass<SampleGridComponent>())
                {
                    gridComponent->searchForSinglePadWithQuery(padIndex, searchQuery);
                }
            }
            else
            {
                AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
                    "Empty Query",
                    "Please enter a search term in the pad's text box or the main search box.");
            }
            return;
        }
    }

    auto bounds = getLocalBounds();
    auto topRightBounds = bounds.reduced(3);
    topRightBounds = topRightBounds.removeFromTop(12);

   if (!hasValidSample)
       return; // Don't process other clicks for empty pads

    // Check DEL badge (rightmost)
    if (hasValidSample)
    {
        int delBadgeWidth = 28;
        auto delBounds = topRightBounds.removeFromRight(delBadgeWidth);

        if (delBounds.contains(event.getPosition()))
        {
            handleDeleteClick();
            return;
        }
        topRightBounds.removeFromRight(3);
    }

    // Check License badge (middle)
    if (licenseType.isNotEmpty())
    {
        int licenseBadgeWidth = 32;
        auto licenseBounds = topRightBounds.removeFromRight(licenseBadgeWidth);

        if (licenseBounds.contains(event.getPosition()))
        {
            URL(licenseType).launchInDefaultBrowser();
            return;
        }
        topRightBounds.removeFromRight(3);
    }

    // Check Web badge (leftmost)
    if (freesoundId.isNotEmpty())
    {
        int webBadgeWidth = 28;
        auto webBounds = topRightBounds.removeFromRight(webBadgeWidth);

        if (webBounds.contains(event.getPosition()))
        {
            String freesoundUrl = "https://freesound.org/s/" + freesoundId + "/";
            URL(freesoundUrl).launchInDefaultBrowser();
            return;
        }
    }

   // Check if clicked in waveform area - for manual triggering
   bounds = getLocalBounds();
   auto waveformBounds = bounds.reduced(8);
   waveformBounds.removeFromTop(16); // Space for top line badges
   waveformBounds.removeFromBottom(16); // Space for bottom line

   if (waveformBounds.contains(event.getPosition()))
   {
       // Play the sample
       if (processor)
       {
           int noteNumber = padIndex + 36;
           processor->addToMidiBuffer(noteNumber);
       }
       return;
   }

   // If clicked on edge areas (outside waveform but not on badges), do nothing here
   // mouseDrag will handle edge dragging for swapping
}

void SamplePad::handleDeleteClick()
{
    if (!hasValidSample)
        return;

    // Confirm with user before deleting
    AlertWindow::showOkCancelBox(
        AlertWindow::QuestionIcon,
        "Delete Sample",
        "Are you sure you want to delete this sample?",
        "Yes", "No",
        nullptr,
        ModalCallbackFunction::create([this](int result) {
            if (result == 1) // User clicked "Yes"
            {
                // Clear this pad
                clearSample();

                // Notify parent grid component
                if (auto* gridComponent = findParentComponentOfClass<SampleGridComponent>())
                {
                    // Update processor arrays if available
                    if (processor)
                    {
                        // Clear this pad in processor arrays
                        auto& sounds = processor->getCurrentSoundsArrayReference();
                        auto& data = processor->getDataReference();

                        if (padIndex < sounds.size())
                            sounds.set(padIndex, FSSound());

                        if (padIndex < data.size())
                            data[padIndex] = StringArray();

                        processor->setSources(); // Rebuild sampler
                    }

                    gridComponent->updateJsonMetadata();
                }
            }
        })
    );
}

void SamplePad::mouseDrag(const MouseEvent& event)
{
    if (!hasValidSample)
        return;

    auto bounds = getLocalBounds();

    // Check if drag started on the drag badge (bottom-left) - for file export
    {
        auto dragBounds = bounds.reduced(3);
        int badgeWidth = 30;
        int badgeHeight = 12;
        dragBounds = dragBounds.removeFromBottom(badgeHeight).removeFromLeft(badgeWidth);

        if (dragBounds.contains(event.getMouseDownPosition()))
        {
            // Start external drag operation for file export
            if (event.getDistanceFromDragStart() > 10 && audioFile.existsAsFile())
            {
                StringArray filePaths;
                filePaths.add(audioFile.getFullPathName());
                performExternalDragDropOfFiles(filePaths, false);
            }
            return;
        }
    }

    // Check if drag started in waveform area - no dragging allowed here
    auto waveformBounds = bounds.reduced(8, 20);
    if (waveformBounds.contains(event.getMouseDownPosition()))
    {
        // No dragging from waveform area - only triggering
        return;
    }

    // Check if drag started on web or license badges - no dragging
    if (freesoundId.isNotEmpty())
    {
        auto webBounds = bounds.reduced(3);
        int badgeWidth = 30;
        int badgeHeight = 12;
        webBounds = webBounds.removeFromTop(badgeHeight).removeFromLeft(badgeWidth);

        if (webBounds.contains(event.getMouseDownPosition()))
            return;
    }

    if (licenseType.isNotEmpty())
    {
        auto licenseBounds = bounds.reduced(3);
        int badgeWidth = 35;
        int badgeHeight = 12;
        licenseBounds = licenseBounds.removeFromTop(badgeHeight).removeFromRight(badgeWidth);

        if (licenseBounds.contains(event.getMouseDownPosition()))
            return;
    }

    // If we get here, drag started in edge area - start swapping drag
    if (event.getDistanceFromDragStart() > 10)
    {
        juce::DynamicObject::Ptr dragObject = new juce::DynamicObject();
        dragObject->setProperty("type", "samplePad");
        dragObject->setProperty("sourcePadIndex", padIndex);

        var dragData(dragObject.get());

        // Use the parent grid's drag container
        if (auto* gridComponent = findParentComponentOfClass<SampleGridComponent>())
        {
            gridComponent->startDragging(dragData, this);
        }
    }
}

// DragAndDropTarget implementation
bool SamplePad::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
    // Only accept drags from other sample pads
    if (dragSourceDetails.description.hasProperty("type") &&
        dragSourceDetails.description.getProperty("type", "") == "samplePad")
    {
        // Don't allow dropping on self
        int sourcePadIndex = dragSourceDetails.description.getProperty("sourcePadIndex", -1);
        return sourcePadIndex != padIndex;
    }
    return false;
}

void SamplePad::itemDragEnter(const SourceDetails& dragSourceDetails)
{
    isDragHover = true;
    repaint();
}

void SamplePad::itemDragExit(const SourceDetails& dragSourceDetails)
{
    isDragHover = false;
    repaint();
}

void SamplePad::itemDropped(const SourceDetails& dragSourceDetails)
{
    isDragHover = false;
    repaint();

    if (dragSourceDetails.description.hasProperty("type") &&
        dragSourceDetails.description.getProperty("type", "") == "samplePad")
    {
        int sourcePadIndex = dragSourceDetails.description.getProperty("sourcePadIndex", -1);

        // Notify parent grid component to perform the swap
        if (auto* gridComponent = findParentComponentOfClass<SampleGridComponent>())
        {
            gridComponent->swapSamples(sourcePadIndex, padIndex);
        }
    }
}

void SamplePad::loadWaveform()
{
    if (audioFile.existsAsFile())
    {
        // Clear existing thumbnail first
        audioThumbnail.clear();

        // Create new file input source and set it
        auto* fileSource = new FileInputSource(audioFile);
        audioThumbnail.setSource(fileSource);

        // Force a repaint after a brief delay to ensure thumbnail is loaded
        Timer::callAfterDelay(100, [this]() {
            repaint();
        });
    }
}

void SamplePad::drawWaveform(Graphics& g, Rectangle<int> bounds)
{
    if (!hasValidSample || audioThumbnail.getTotalLength() == 0.0)
        return;

    g.setColour(Colours::black.withAlpha(0.3f));
    g.fillRect(bounds);

    g.setColour(Colours::white.withAlpha(0.8f));
    audioThumbnail.drawChannels(g, bounds, 0.0, audioThumbnail.getTotalLength(), 1.0f);
}

void SamplePad::drawPlayhead(Graphics& g, Rectangle<int> bounds)
{
    if (!isPlaying || !hasValidSample)
        return;

    float x = bounds.getX() + (playheadPosition * bounds.getWidth());

    g.setColour(Colours::red);
    g.drawLine(x, bounds.getY(), x, bounds.getBottom(), 2.0f);
}

void SamplePad::setPlayheadPosition(float position)
{
    // Ensure GUI updates happen on the message thread
    MessageManager::callAsync([this, position]()
    {
        playheadPosition = jlimit(0.0f, 1.0f, position);
        repaint();
    });
}

void SamplePad::setIsPlaying(bool playing)
{
    // Ensure GUI updates happen on the message thread
    MessageManager::callAsync([this, playing]()
    {
        isPlaying = playing;
        repaint();
    });
}

void SamplePad::setProcessor(FreesoundAdvancedSamplerAudioProcessor* p)
{
    processor = p;
}

String SamplePad::getLicenseShortName(const String& license) const
{
    // Parse the actual Creative Commons URLs to determine license type
    String lowerLicense = license.toLowerCase();

    if (lowerLicense.contains("creativecommons.org/publicdomain/zero") ||
        lowerLicense.contains("cc0") ||
        lowerLicense.contains("publicdomain/zero"))
        return "cc0";
    else if (lowerLicense.contains("creativecommons.org/licenses/by-nc") ||
             lowerLicense.contains("by-nc"))
        return "by-nc";
    else if (lowerLicense.contains("creativecommons.org/licenses/by/") ||
             (lowerLicense.contains("creativecommons.org/licenses/by") && !lowerLicense.contains("-nc")))
        return "by";
    else if (lowerLicense.contains("sampling+"))
        return "by-nc"; // Sampling+ is treated as by-nc according to Freesound
    else
        return "by-nc"; // Default to most restrictive for unknown licenses
}

void SamplePad::setSample(const File& audioFile, const String& name, const String& author, String fsId, String license, String query)
{
    sampleName = name;
    authorName = author;
    freesoundId = fsId;
    licenseType = license;
    padQuery = query;

    // CRITICAL: Update query text box with the loaded query
    queryTextBox.setText(query, dontSendNotification);

    // Update audio file and waveform
    this->audioFile = audioFile;
    hasValidSample = audioFile.existsAsFile();

    if (hasValidSample)
    {
        loadWaveform();
    }
    else
    {
        audioThumbnail.clear();
    }

    resized(); // Recalculate layout
    repaint();

    // DBG("SamplePad::setSample - Set query '" + query + "' on pad " + String(padIndex));
}

void SamplePad::clearSample()
{
    audioFile = File();
    sampleName = "";
    authorName = "";
    freesoundId = String();
    licenseType = String();
    padQuery = String();
    hasValidSample = false;
    isPlaying = false;
    playheadPosition = 0.0f;
    audioThumbnail.clear();
    queryTextBox.setText("", dontSendNotification);

    resized();
    repaint();
}

void SamplePad::setQuery(const String& query)
{
    padQuery = query;
    queryTextBox.setText(query, dontSendNotification);
}

String SamplePad::getQuery() const
{
    return queryTextBox.getText().trim();
}

bool SamplePad::hasQuery() const
{
    return queryTextBox.getText().trim().isNotEmpty();
}

SamplePad::SampleInfo SamplePad::getSampleInfo() const
{
    SampleInfo info;
    info.audioFile = audioFile;
    info.sampleName = sampleName;
    info.authorName = authorName;
    info.freesoundId = freesoundId;
    info.licenseType = licenseType;
    info.query = getQuery(); // Get current query from text box
    info.hasValidSample = hasValidSample;
    info.padIndex = 0; // Will be set by SampleGridComponent
    return info;
}

void SamplePad::startDownloadProgress()
{
    // Stop any existing timer first
    stopTimer();

    isDownloading = true;
    pendingCleanup = false;
    currentDownloadProgress = 0.0;

    // Create progress components on demand
    if (!progressBar)
    {
        progressBar = std::make_unique<ProgressBar>(currentDownloadProgress);
        progressBar->setColour(ProgressBar::backgroundColourId, Colour(0x80404040));
        progressBar->setColour(ProgressBar::foregroundColourId, Colour(0x8000D9FF));
    }

    if (!progressLabel)
    {
        progressLabel = std::make_unique<Label>();
        progressLabel->setFont(Font(8.0f, Font::bold));
        progressLabel->setColour(Label::textColourId, Colours::white);
        progressLabel->setJustificationType(Justification::centred);
    }

    // Add components if not already added
    if (!progressBar->getParentComponent())
        addAndMakeVisible(*progressBar);
    if (!progressLabel->getParentComponent())
        addAndMakeVisible(*progressLabel);

    // Hide query text box and show progress components
    queryTextBox.setVisible(false);
    progressBar->setVisible(true);
    progressLabel->setVisible(true);

    progressLabel->setText("Downloading...", dontSendNotification);
    resized(); // Recalculate layout
    repaint();
}

void SamplePad::updateDownloadProgress(double progress, const String& status)
{
    if (!isDownloading || !progressBar || !progressLabel)
        return;

    // Use WeakReference for safe async calls
    Component::SafePointer<SamplePad> safeThis(this);

    MessageManager::callAsync([safeThis, progress, status]()
    {
        if (safeThis == nullptr || !safeThis->isDownloading)
            return;

        safeThis->currentDownloadProgress = jlimit(0.0, 1.0, progress);

        String progressText = status.isEmpty() ?
            "Downloading... " + String((int)(progress * 100)) + "%" :
            status;

        if (safeThis->progressLabel)
        {
            safeThis->progressLabel->setText(progressText, dontSendNotification);
        }

        if (safeThis->progressBar)
        {
            safeThis->progressBar->repaint();
        }
    });
}

void SamplePad::finishDownloadProgress(bool success, const String& message)
{
    if (!isDownloading || !progressBar || !progressLabel)
        return;

    Component::SafePointer<SamplePad> safeThis(this);

    MessageManager::callAsync([safeThis, success, message]()
    {
        if (safeThis == nullptr || !safeThis->isDownloading)
            return;

        if (success)
        {
            if (safeThis->progressLabel)
                safeThis->progressLabel->setText("Complete!", dontSendNotification);
            safeThis->currentDownloadProgress = 1.0;
            if (safeThis->progressBar)
                safeThis->progressBar->repaint();

            // Use timer for cleanup instead of Timer::callAfterDelay
            safeThis->pendingCleanup = true;
            safeThis->startTimer(1000); // 1 second delay
        }
        else
        {
            if (safeThis->progressLabel)
            {
                safeThis->progressLabel->setText(message.isEmpty() ? "Failed!" : message, dontSendNotification);
                safeThis->progressLabel->setColour(Label::textColourId, Colours::red);
            }

            // Use timer for cleanup instead of Timer::callAfterDelay
            safeThis->pendingCleanup = true;
            safeThis->startTimer(2000); // 2 second delay for errors
        }
    });
}

void SamplePad::timerCallback()
{
    if (pendingCleanup)
    {
        stopTimer();
        cleanupProgressComponents();
    }
}

void SamplePad::cleanupProgressComponents()
{
    isDownloading = false;
    pendingCleanup = false;

    // Hide and remove progress components
    if (progressBar)
    {
        progressBar->setVisible(false);
        removeChildComponent(progressBar.get());
    }

    if (progressLabel)
    {
        progressLabel->setVisible(false);
        progressLabel->setColour(Label::textColourId, Colours::white); // Reset color
        removeChildComponent(progressLabel.get());
    }

    // Show query text box
    queryTextBox.setVisible(true);

    // Clean up progress components completely
    progressBar.reset();
    progressLabel.reset();

    resized();
    repaint();
}



//==============================================================================
// SampleGridComponent Implementation
//==============================================================================

SampleGridComponent::SampleGridComponent()
    : processor(nullptr)
{

    // Create and add sample pads
    for (int i = 0; i < TOTAL_PADS; ++i)
    {
        samplePads[i] = std::make_unique<SamplePad>(i);
        addAndMakeVisible(*samplePads[i]);
    }

    // Set up shuffle button
    shuffleButton.onClick = [this]() {
        shuffleSamples();
    };
    addAndMakeVisible(shuffleButton);

    // Set up clear all button
    clearAllButton.onClick = [this]() {
        // Confirm with user before clearing
        AlertWindow::showOkCancelBox(
            AlertWindow::QuestionIcon,
            "Clear All Pads",
            "Are you sure you want to clear all pads? This cannot be undone.",
            "Yes", "No",
            nullptr,
            ModalCallbackFunction::create([this](int result) {
                if (result == 1) { // User clicked "Yes"
                    clearAllPads();
                }
            })
        );
    };
    addAndMakeVisible(clearAllButton);
}

SampleGridComponent::~SampleGridComponent()
{
    stopTimer();

    // Clean up any active downloads
    cleanupSingleDownload();

    if (processor)
    {
        processor->removePlaybackListener(this);
    }
}

void SampleGridComponent::paint(Graphics& g)
{
    // Dark grid background
    g.setColour(Colour(0x800F0F0F));
    g.fillAll();

    // Add subtle grid lines for visual organization
    g.setColour(Colour(0x802A2A2A).withAlpha(0.3f));

    auto bounds = getLocalBounds();
    int padding = 4;
    int bottomControlsHeight = 30;
    bounds.removeFromBottom(bottomControlsHeight + padding);

    int totalPadding = padding * (GRID_SIZE + 1);
    int padWidth = (bounds.getWidth() - totalPadding) / GRID_SIZE;
    int padHeight = (bounds.getHeight() - totalPadding) / GRID_SIZE;

    // Draw vertical grid lines
    for (int col = 0; col <= GRID_SIZE; ++col)
    {
        int x = padding + col * (padWidth + padding) - (padding / 2);
        g.drawLine(x, bounds.getY(), x, bounds.getBottom(), 1.0f);
    }

    // Draw horizontal grid lines
    for (int row = 0; row <= GRID_SIZE; ++row)
    {
        int y = padding + row * (padHeight + padding) - (padding / 2);
        g.drawLine(bounds.getX(), y, bounds.getRight(), y, 1.0f);
    }
}

void SampleGridComponent::resized()
{
    auto bounds = getLocalBounds();
    int padding = 4;
    int bottomControlsHeight = 30;
    int buttonWidth = 60;
    int spacing = 5;

    // Reserve space for bottom controls
    auto bottomArea = bounds.removeFromBottom(bottomControlsHeight + padding);

    // Position controls in bottom area
    auto controlsBounds = bottomArea.reduced(padding);
    shuffleButton.setBounds(controlsBounds.removeFromLeft(buttonWidth));
    controlsBounds.removeFromLeft(spacing);

    clearAllButton.setBounds(controlsBounds.removeFromRight(buttonWidth));
    // controlsBounds.removeFromLeft(spacing);

    // Calculate grid layout in remaining space
    int totalPadding = padding * (GRID_SIZE + 1);
    int padWidth = (bounds.getWidth() - totalPadding) / GRID_SIZE;
    int padHeight = (bounds.getHeight() - totalPadding) / GRID_SIZE;

    for (int row = 0; row < GRID_SIZE; ++row)
    {
        for (int col = 0; col < GRID_SIZE; ++col)
        {
            // Calculate pad index starting from bottom left
            // Bottom row (row 3) = pads 0-3, next row up (row 2) = pads 4-7, etc.
            int padIndex = (GRID_SIZE - 1 - row) * GRID_SIZE + col;

            int x = padding + col * (padWidth + padding);
            int y = padding + row * (padHeight + padding);

            samplePads[padIndex]->setBounds(x, y, padWidth, padHeight);
        }
    }
}

void SampleGridComponent::setProcessor(FreesoundAdvancedSamplerAudioProcessor* p)
{
    if (processor)
    {
        processor->removePlaybackListener(this);
    }

    processor = p;

    if (processor)
    {
        processor->addPlaybackListener(this);
    }

    // Set processor for all pads
    for (auto& pad : samplePads)
    {
        pad->setProcessor(processor);
    }
}

void SampleGridComponent::updateSamples(const Array<FSSound>& sounds, const std::vector<StringArray>& soundInfo)
{
    // Always clear samples first
    clearSamples();

    if (!processor)
        return;

    File downloadDir = processor->getCurrentDownloadLocation();

    // DBG("SampleGridComponent::updateSamples called with " + String(sounds.size()) + " sounds");

    // For preset loading, always use the provided arrays directly
    if (!sounds.isEmpty())
    {
        // DBG("Using provided arrays for grid update");
        loadSamplesFromArrays(sounds, soundInfo, downloadDir);
    }

    // Force repaint all pads
    for (auto& pad : samplePads)
    {
        pad->repaint();
    }

    // DBG("Grid visual update complete");
}

void SampleGridComponent::loadSamplesFromJson(const File& metadataFile)
{
    FileInputStream inputStream(metadataFile);
    if (!inputStream.openedOk())
        return;

    String jsonText = inputStream.readEntireStreamAsString();
    var parsedJson = juce::JSON::parse(jsonText);

    if (!parsedJson.isObject())
        return;

    var samplesArray = parsedJson.getProperty("samples", var());
    if (!samplesArray.isArray())
        return;

    File downloadDir = metadataFile.getParentDirectory();
    int numSamples = jmin(TOTAL_PADS, (int)samplesArray.size());

    for (int i = 0; i < numSamples; ++i)
    {
        var sample = samplesArray[i];
        if (!sample.isObject())
            continue;

        String fileName = sample.getProperty("file_name", "");
        String sampleName = sample.getProperty("original_name", "Sample " + String(i + 1));
        String authorName = sample.getProperty("author", "Unknown");
        String license = sample.getProperty("license", "Unknown");
        String freesoundId = sample.getProperty("freesound_id", "");

        if (fileName.isNotEmpty())
        {
            File audioFile = downloadDir.getChildFile(fileName);
            if (audioFile.existsAsFile())
            {
                samplePads[i]->setSample(audioFile, sampleName, authorName, freesoundId, license);
            }
        }
    }
}

void SampleGridComponent::loadSamplesFromArrays(const Array<FSSound>& sounds, const std::vector<StringArray>& soundInfo, const File& downloadDir)
{
    int numSamples = jmin(TOTAL_PADS, sounds.size());

    // DBG("Loading " + String(numSamples) + " samples to grid from arrays");

    for (int i = 0; i < numSamples; ++i)
    {
        const FSSound& sound = sounds[i];

        // Skip empty sounds
        if (sound.id.isEmpty())
        {
            samplePads[i]->clearSample();
            continue;
        }

        String sampleName = (i < soundInfo.size() && soundInfo[i].size() > 0) ? soundInfo[i][0] : sound.name;
        String authorName = (i < soundInfo.size() && soundInfo[i].size() > 1) ? soundInfo[i][1] : sound.user;
        String license = (i < soundInfo.size() && soundInfo[i].size() > 2) ? soundInfo[i][2] : sound.license;
        String sampleQuery = (i < soundInfo.size() && soundInfo[i].size() > 3) ? soundInfo[i][3] : "";  // Get query from 4th element
        String freesoundId = sound.id;

        String expectedFilename = "FS_ID_" + freesoundId + ".ogg";
        File audioFile = downloadDir.getChildFile(expectedFilename);

        // DBG("Pad " + String(i) + ": " + sampleName + " (ID: " + freesoundId + ") with query: '" + sampleQuery + "'");
        // DBG("  Looking for file: " + audioFile.getFullPathName());
        // DBG("  File exists: " + String(audioFile.existsAsFile() ? "YES" : "NO"));

        if (audioFile.existsAsFile())
        {
            // IMPORTANT: Pass the query to setSample so it appears in the text box
            samplePads[i]->setSample(audioFile, sampleName, authorName, freesoundId, license, sampleQuery);
            // DBG("  Successfully set sample on pad " + String(i) + " with query: '" + sampleQuery + "'");
        }
        else
        {
            samplePads[i]->clearSample();
            // DBG("  Cleared pad " + String(i) + " - file not found");
        }
    }

    // Clear any remaining pads
    for (int i = numSamples; i < TOTAL_PADS; ++i)
    {
        samplePads[i]->clearSample();
    }

    startTimer(200);
    // DBG("Grid update complete");
}

void SampleGridComponent::clearSamples()
{
    for (auto& pad : samplePads)
    {
        pad->clearSample();

    }

    for (auto& pad : samplePads)
    {
        pad->clearSample();
        pad->setQuery(""); // Also clear any individual pad queries
    }
}

void SampleGridComponent::swapSamples(int sourcePadIndex, int targetPadIndex)
{
    if (sourcePadIndex < 0 || sourcePadIndex >= TOTAL_PADS ||
        targetPadIndex < 0 || targetPadIndex >= TOTAL_PADS ||
        sourcePadIndex == targetPadIndex)
        return;

    // Swap visual pad objects
    std::swap(samplePads[sourcePadIndex], samplePads[targetPadIndex]);

    // Swap their bounds to keep layout consistent
    auto sourceBounds = samplePads[sourcePadIndex]->getBounds();
    auto targetBounds = samplePads[targetPadIndex]->getBounds();
    samplePads[sourcePadIndex]->setBounds(targetBounds);
    samplePads[targetPadIndex]->setBounds(sourceBounds);

    // Update padIndex of each pad so MIDI note triggering is correct
    samplePads[sourcePadIndex]->setPadIndex(sourcePadIndex);
    samplePads[targetPadIndex]->setPadIndex(targetPadIndex);

    // Update processor-side arrays to keep logic in sync
    if (processor != nullptr)
    {
        // Swap current sound data
        auto& sounds = processor->getCurrentSoundsArrayReference();
        std::swap(sounds.getReference(sourcePadIndex), sounds.getReference(targetPadIndex));

        // Swap associated metadata (author, license, etc.)
        auto& metadata = processor->getDataReference();
        std::swap(metadata[sourcePadIndex], metadata[targetPadIndex]);

        // Rebuild the sampler to apply updated MIDI mappings
        processor->setSources();
    }

    // Repaint both pads to reflect new visuals
    samplePads[sourcePadIndex]->repaint();
    samplePads[targetPadIndex]->repaint();
}

void SampleGridComponent::shuffleSamples()
{
    if (!processor)
        return;

    // Collect all current sample info
    Array<SamplePad::SampleInfo> allSamples;
    for (int i = 0; i < TOTAL_PADS; ++i)
    {
        auto sampleInfo = samplePads[i]->getSampleInfo();
        if (sampleInfo.hasValidSample)
        {
            allSamples.add(sampleInfo);
        }
    }

    // If we have no samples or only one sample, nothing to shuffle
    if (allSamples.size() <= 1)
        return;

    // Clear all pads first
    for (auto& pad : samplePads)
    {
        pad->clearSample();
    }

    // Shuffle the samples array using JUCE's Random class
    Random random;
    for (int i = allSamples.size() - 1; i > 0; --i)
    {
        int j = random.nextInt(i + 1);
        allSamples.swap(i, j);
    }

    // Redistribute shuffled samples to pads
    for (int i = 0; i < allSamples.size() && i < TOTAL_PADS; ++i)
    {
        const auto& sample = allSamples[i];
        samplePads[i]->setSample(sample.audioFile, sample.sampleName,
                                sample.authorName, sample.freesoundId, sample.licenseType);
    }

    // Update processor arrays to match new order
    updateProcessorArraysFromGrid();

    // Reload the sampler with new pad order
    if (processor)
    {
        processor->setSources();
        // REMOVED: processor->updateReadmeFile();
    }

    // DBG("Samples shuffled successfully");
}

void SampleGridComponent::updateProcessorArraysFromGrid()
{
    if (!processor)
        return;

    // Clear the processor's current arrays
    auto& currentSounds = processor->getCurrentSoundsArrayReference();
    auto& soundsData = processor->getDataReference();

    currentSounds.clear();
    soundsData.clear();

    // Rebuild arrays based on current grid order
    for (int i = 0; i < TOTAL_PADS; ++i)
    {
        auto padInfo = samplePads[i]->getSampleInfo();

        if (padInfo.hasValidSample)
        {
            // Create FSSound object from pad info (no API changes)
            FSSound sound;
            sound.id = padInfo.freesoundId;
            sound.name = padInfo.sampleName;
            sound.user = padInfo.authorName;
            sound.license = padInfo.licenseType;
            // Note: Can't store query in FSSound, so we rely on soundsData

            // Try to get duration and file size from existing data or set defaults
            sound.duration = 0.5;
            sound.filesize = 50000;

            // If we can find this sound in the existing arrays, preserve its metadata
            Array<FSSound> existingSounds = processor->getCurrentSounds();
            for (const auto& existingSound : existingSounds)
            {
                if (existingSound.id == sound.id)
                {
                    sound.duration = existingSound.duration;
                    sound.filesize = existingSound.filesize;
                    break;
                }
            }

            currentSounds.add(sound);

            // Create sound info including query as 4th element
            StringArray soundData;
            soundData.add(padInfo.sampleName);
            soundData.add(padInfo.authorName);
            soundData.add(padInfo.licenseType);
            soundData.add(padInfo.query);  // Store query in 4th position
            soundsData.push_back(soundData);
        }
    }

    // DBG("Updated processor arrays from grid - now has " + String(currentSounds.size()) + " sounds");
}

void SampleGridComponent::updateJsonMetadata()
{
    if (!processor)
        return;

    File downloadDir = processor->getCurrentDownloadLocation();
    File metadataFile = downloadDir.getChildFile("metadata.json");

    if (!metadataFile.existsAsFile())
        return;

    // Read existing JSON
    FileInputStream inputStream(metadataFile);
    if (!inputStream.openedOk())
        return;

    String jsonText = inputStream.readEntireStreamAsString();
    var parsedJson = juce::JSON::parse(jsonText);

    if (!parsedJson.isObject())
        return;

    // Get the root object
    auto* rootObject = parsedJson.getDynamicObject();
    if (!rootObject)
        return;

    // Create new samples array based on current pad order
    juce::Array<juce::var> newSamplesArray;

    for (int i = 0; i < TOTAL_PADS; ++i)
    {
        auto padInfo = samplePads[i]->getSampleInfo();

        if (padInfo.hasValidSample)
        {
            juce::DynamicObject::Ptr sample = new juce::DynamicObject();

            // Extract filename from full path
            String fileName = padInfo.audioFile.getFileName();

            sample->setProperty("file_name", fileName);
            sample->setProperty("original_name", padInfo.sampleName);
            sample->setProperty("freesound_id", padInfo.freesoundId);
            sample->setProperty("search_query", processor->getQuery());
            sample->setProperty("author", padInfo.authorName);
            sample->setProperty("license", padInfo.licenseType);

            // We need to get duration and file size from the original JSON
            // Find the original entry by freesound_id
            var originalSamplesArray = parsedJson.getProperty("samples", var());
            if (originalSamplesArray.isArray())
            {
                for (int j = 0; j < originalSamplesArray.size(); ++j)
                {
                    var originalSample = originalSamplesArray[j];
                    if (originalSample.isObject())
                    {
                        String originalId = originalSample.getProperty("freesound_id", "");
                        if (originalId == padInfo.freesoundId)
                        {
                            // Copy over the missing properties
                            sample->setProperty("duration", originalSample.getProperty("duration", 0.0));
                            sample->setProperty("file_size", originalSample.getProperty("file_size", 0));
                            sample->setProperty("downloaded_at", originalSample.getProperty("downloaded_at", ""));
                            sample->setProperty("freesound_url", "https://freesound.org/s/" + padInfo.freesoundId + "/");
                            break;
                        }
                    }
                }
            }

            sample->setProperty("original_pad_index", i + 1); // 1-based for user display

            newSamplesArray.add(juce::var(sample.get()));
        }
    }

    // Update the JSON with new samples array
    rootObject->setProperty("samples", newSamplesArray);

    // Update session info
    var sessionInfo = parsedJson.getProperty("session_info", var());
    if (sessionInfo.isObject())
    {
        if (auto* sessionObj = sessionInfo.getDynamicObject())
        {
            sessionObj->setProperty("total_samples", newSamplesArray.size());
        }
    }

    // Write back to file
    String newJsonString = juce::JSON::toString(parsedJson, true);
    metadataFile.replaceWithText(newJsonString);
}

void SampleGridComponent::noteStarted(int noteNumber, float velocity)
{
    // Convert MIDI note back to pad index
    int padIndex = noteNumber - 36; // Note 36 = pad 0, Note 37 = pad 1, etc.
    if (padIndex >= 0 && padIndex < TOTAL_PADS)
    {
        // These methods now handle message thread dispatching internally
        samplePads[padIndex]->setIsPlaying(true);
        samplePads[padIndex]->setPlayheadPosition(0.0f);
    }
}

void SampleGridComponent::noteStopped(int noteNumber)
{
    // Convert MIDI note back to pad index
    int padIndex = noteNumber - 36;
    if (padIndex >= 0 && padIndex < TOTAL_PADS)
    {
        // This method now handles message thread dispatching internally
        samplePads[padIndex]->setIsPlaying(false);
    }
}

void SampleGridComponent::playheadPositionChanged(int noteNumber, float position)
{
    // Convert MIDI note back to pad index
    int padIndex = noteNumber - 36;
    if (padIndex >= 0 && padIndex < TOTAL_PADS)
    {
        // This method now handles message thread dispatching internally
        samplePads[padIndex]->setPlayheadPosition(position);
    }
}

Array<SamplePad::SampleInfo> SampleGridComponent::getAllSampleInfo() const
{
    Array<SamplePad::SampleInfo> allSamples;

    for (int i = 0; i < samplePads.size(); ++i)
    {
        SamplePad::SampleInfo info = samplePads[i]->getSampleInfo();
        if (info.hasValidSample)
        {
            // Use 0-based index to match processor expectations
            info.padIndex = i; // Changed from i + 1 to i
            allSamples.add(info);
        }
    }

    return allSamples;
}

Array<FSSound> SampleGridComponent::searchSingleSound(const String& query)
{
    FreesoundClient client(FREESOUND_API_KEY);
    SoundList list = client.textSearch(query, "duration:[0 TO 0.5]", "score", 1, 1, 10000, "id,name,username,license,previews");
    Array<FSSound> sounds = list.toArrayOfSounds();

    if (sounds.isEmpty())
        return sounds;

    // Randomize and pick one
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(sounds.begin(), sounds.end(), g);

    // Return just the first one
    Array<FSSound> result;
    result.add(sounds[0]);
    return result;
}

void SampleGridComponent::loadSingleSample(int padIndex, const FSSound& sound, const File& audioFile)
{
    // Update the pad visually
    samplePads[padIndex]->setSample(audioFile, sound.name, sound.user, sound.id, sound.license);

    // Update processor's internal arrays
    updateSinglePadInProcessor(padIndex, sound);

    // Reload the sampler to include the new sample
    if (processor)
    {
        processor->setSources();
        processor->updateReadmeFile();
    }

    // Update JSON metadata
    updateJsonMetadata();
}

void SampleGridComponent::downloadSingleSample(int padIndex, const FSSound& sound)
{
    // Create a download manager for single file
    singlePadDownloadManager = std::make_unique<AudioDownloadManager>();
    currentDownloadPadIndex = padIndex;
    currentDownloadSound = sound;

    // Use a simple callback approach instead of inheritance
    // We'll check for completion in a timer or use the processor's download system

    // Start download
    Array<FSSound> singleSoundArray;
    singleSoundArray.add(sound);

    File samplesFolder = processor->getCurrentDownloadLocation();

    // Use a timer to check for completion
    startTimer(500); // Check every 500ms
    singlePadDownloadManager->startDownloads(singleSoundArray, samplesFolder, processor->getQuery());

}

void SampleGridComponent::updateSinglePadInProcessor(int padIndex, const FSSound& sound)
{
    if (!processor)
        return;

    auto& currentSounds = processor->getCurrentSoundsArrayReference();
    auto& soundsData = processor->getDataReference();

    // Ensure arrays are large enough
    while (currentSounds.size() <= padIndex)
    {
        FSSound emptySound;
        currentSounds.add(emptySound);
    }

    while (soundsData.size() <= padIndex)
    {
        StringArray emptyData;
        emptyData.add(""); // name
        emptyData.add(""); // author
        emptyData.add(""); // license
        emptyData.add(""); // query - 4th element
        soundsData.push_back(emptyData);
    }

    // Update the specific pad
    currentSounds.set(padIndex, sound);

    // Get the query from the pad's UI since we can't store it in FSSound
    String query = "";
    if (padIndex < TOTAL_PADS && samplePads[padIndex])
    {
        query = samplePads[padIndex]->getQuery();
    }

    StringArray soundData;
    soundData.add(sound.name);
    soundData.add(sound.user);
    soundData.add(sound.license);
    soundData.add(query);  // Store query in 4th position
    soundsData[padIndex] = soundData;
}

void SampleGridComponent::handleMasterSearch(const Array<FSSound>& sounds, const std::vector<StringArray>& soundInfo, const String& masterQuery)
{
    if (!processor)
        return;

    File downloadDir = processor->getCurrentDownloadLocation();

    // Find pads with empty queries
    Array<int> emptyQueryPads;
    for (int i = 0; i < TOTAL_PADS; ++i)
    {
        if (!samplePads[i]->hasQuery())
        {
            emptyQueryPads.add(i);
        }
    }

    if (emptyQueryPads.isEmpty())
    {
        AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon,
            "No Empty Pads",
            "All pads already have individual search queries. Clear pad queries to use master search.");
        return;
    }

    // Distribute sounds to empty query pads
    int soundIndex = 0;
    for (int padIndex : emptyQueryPads)
    {
        if (soundIndex >= sounds.size())
            break;

        const FSSound& sound = sounds[soundIndex];

        // Create filename using ID-based naming scheme
        String expectedFilename = "FS_ID_" + sound.id + ".ogg";
        File audioFile = downloadDir.getChildFile(expectedFilename);

        // Get sample info
        String sampleName = (soundIndex < soundInfo.size() && soundInfo[soundIndex].size() > 0) ? soundInfo[soundIndex][0] : sound.name;
        String authorName = (soundIndex < soundInfo.size() && soundInfo[soundIndex].size() > 1) ? soundInfo[soundIndex][1] : sound.user;
        String license = (soundIndex < soundInfo.size() && soundInfo[soundIndex].size() > 2) ? soundInfo[soundIndex][2] : sound.license;

        if (audioFile.existsAsFile())
        {
            // Set the master query in the pad's text box
            samplePads[padIndex]->setSample(audioFile, sampleName, authorName, sound.id, license, masterQuery);
        }

        soundIndex++;
    }

    // Update processor arrays to reflect the new state
    updateProcessorArraysFromGrid();

    // Reload the sampler
    if (processor)
    {
        processor->setSources();
    }

    // DBG("Master search applied to " + String(emptyQueryPads.size()) + " pads with master query: " + masterQuery);
}

void SampleGridComponent::searchForSinglePadWithQuery(int padIndex, const String& query)
{
    if (!processor || padIndex < 0 || padIndex >= TOTAL_PADS)
        return;

    if (query.isEmpty())
    {
        AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
            "Empty Query",
            "Please enter a search term.");
        return;
    }

    // Check if this pad is already downloading
    if (currentDownloadPadIndex == padIndex && singlePadDownloadManager && singlePadDownloadManager->isThreadRunning())
    {
        AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon,
            "Download In Progress",
            "This pad is already downloading. Please wait for it to complete.");
        return;
    }

    // Search for a single sound with the specific query
    Array<FSSound> searchResults = searchSingleSound(query);

    if (searchResults.isEmpty())
    {
        AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
            "No Results",
            "No sounds found for the query: " + query);
        return;
    }

    // Get the first (random) result
    FSSound newSound = searchResults[0];

    // Create the filename
    String fileName = "FS_ID_" + newSound.id + ".ogg";
    File samplesFolder = processor->getCurrentDownloadLocation();
    File audioFile = samplesFolder.getChildFile(fileName);

    // Check if we already have this sample downloaded
    if (audioFile.existsAsFile())
    {
        // Use existing file
        loadSingleSampleWithQuery(padIndex, newSound, audioFile, query);
    }
    else
    {
        // Need to download it
        downloadSingleSampleWithQuery(padIndex, newSound, query);
    }
}

void SampleGridComponent::loadSingleSampleWithQuery(int padIndex, const FSSound& sound, const File& audioFile, const String& query)
{
    // Update the pad visually with the query
    samplePads[padIndex]->setSample(audioFile, sound.name, sound.user, sound.id, sound.license, query);

    // Update processor's internal arrays
    updateSinglePadInProcessor(padIndex, sound);

    // Reload the sampler to include the new sample
    if (processor)
    {
        processor->setSources();
        processor->updateReadmeFile();
    }

    // Update JSON metadata
    updateJsonMetadata();
}

void SampleGridComponent::downloadSingleSampleWithQuery(int padIndex, const FSSound& sound, const String& query)
{
    // Store the query for later use
    currentDownloadQuery = query;

    // Clean up any existing download manager first
    if (singlePadDownloadManager)
    {
        singlePadDownloadManager->stopThread(1000);
        singlePadDownloadManager.reset();
    }

    // Create a download manager for single file
    singlePadDownloadManager = std::make_unique<AudioDownloadManager>();
    // DON'T add listener to avoid double-listening conflicts

    currentDownloadPadIndex = padIndex;
    currentDownloadSound = sound;

    // Start progress display on the pad
    if (padIndex >= 0 && padIndex < TOTAL_PADS)
    {
        samplePads[padIndex]->startDownloadProgress();
    }

    // Start download
    Array<FSSound> singleSoundArray;
    singleSoundArray.add(sound);

    File samplesFolder = processor->getCurrentDownloadLocation();

    // Use timer-based checking instead of listener to avoid conflicts
    startTimer(250); // Check every 250ms

    singlePadDownloadManager->startDownloads(singleSoundArray, samplesFolder, query);
}

void SampleGridComponent::cleanupSingleDownload()
{
    // Clean up download manager safely
    if (singlePadDownloadManager)
    {
        // Stop thread with timeout
        if (singlePadDownloadManager->isThreadRunning())
        {
            singlePadDownloadManager->stopThread(2000);
        }
        singlePadDownloadManager.reset();
    }

    currentDownloadPadIndex = -1;
    currentDownloadQuery = "";
}

void SampleGridComponent::timerCallback()
{
    // Check if we're handling single pad downloads
    if (singlePadDownloadManager && currentDownloadPadIndex >= 0)
    {
        String fileName = "FS_ID_" + currentDownloadSound.id + ".ogg";
        File samplesFolder = processor->getCurrentDownloadLocation();
        File audioFile = samplesFolder.getChildFile(fileName);

        // Check if download is still running
        bool downloadRunning = singlePadDownloadManager->isThreadRunning();

        if (audioFile.existsAsFile())
        {
            // Download completed successfully
            stopTimer();

            // Load the sample
            loadSingleSampleWithQuery(currentDownloadPadIndex, currentDownloadSound, audioFile, currentDownloadQuery);

            // Finish progress with success - use SafePointer
            if (currentDownloadPadIndex < TOTAL_PADS && samplePads[currentDownloadPadIndex])
            {
                samplePads[currentDownloadPadIndex]->finishDownloadProgress(true, "Complete!");
            }

            // Clean up safely
            cleanupSingleDownload();
        }
        else if (!downloadRunning)
        {
            // Download thread finished but no file - failed
            stopTimer();

            // Finish progress with error - use SafePointer
            if (currentDownloadPadIndex < TOTAL_PADS && samplePads[currentDownloadPadIndex])
            {
                samplePads[currentDownloadPadIndex]->finishDownloadProgress(false, "Download failed!");
            }

            // Clean up safely
            cleanupSingleDownload();
        }
        else
        {
            // Still downloading - update progress if possible
            // Simple progress estimation based on time (fallback)
            if (currentDownloadPadIndex < TOTAL_PADS && samplePads[currentDownloadPadIndex])
            {
                // You could implement a simple time-based progress here if needed
                // For now, just ensure the progress bar is still showing
            }
        }
    }
    else
    {
        // This is the delayed repaint from loadSamplesFromArrays
        stopTimer();

        // Safely repaint all pads
        for (auto& pad : samplePads)
        {
            if (pad && pad->isShowing() &&
                pad->getLocalBounds().getWidth() > 0 &&
                pad->getLocalBounds().getHeight() > 0)
            {
                pad->repaint();
            }
        }
    }
}

SamplePad::SampleInfo SampleGridComponent::getPadInfo(int padIndex) const
{
    if (padIndex >= 0 && padIndex < TOTAL_PADS)
    {
        return samplePads[padIndex]->getSampleInfo();
    }

    // Return empty info for invalid indices
    SamplePad::SampleInfo emptyInfo;
    emptyInfo.hasValidSample = false;
    return emptyInfo;
}

void SampleGridComponent::mouseDown(const MouseEvent& event)
{
    // Let normal mouse handling continue
    Component::mouseDown(event);
}

void SampleGridComponent::clearAllPads()
{
    // Clear all pads visually
    clearSamples();

    // Clear processor arrays if available
    if (processor)
    {
        processor->getCurrentSoundsArrayReference().clear();
        processor->getDataReference().clear();
        processor->setSources(); // Rebuild sampler with empty state
    }

    // Update any metadata
    updateJsonMetadata();
}

// Helper function to get query for a specific pad from soundsArray:
String SampleGridComponent::getQueryForPad(int padIndex) const
{
    if (!processor || padIndex < 0 || padIndex >= TOTAL_PADS)
        return "";

    const auto& soundsData = processor->getDataReference();

    if (padIndex < soundsData.size() && soundsData[padIndex].size() > 3)
    {
        return soundsData[padIndex][3];  // Query is in 4th position
    }

    return "";
}

// Helper function to set query for a specific pad in soundsArray:
void SampleGridComponent::setQueryForPad(int padIndex, const String& query)
{
    if (!processor || padIndex < 0 || padIndex >= TOTAL_PADS)
        return;

    auto& soundsData = processor->getDataReference();

    // Ensure array is large enough
    while (soundsData.size() <= padIndex)
    {
        StringArray emptyData;
        emptyData.add(""); // name
        emptyData.add(""); // author
        emptyData.add(""); // license
        emptyData.add(""); // query
        soundsData.push_back(emptyData);
    }

    // Ensure this entry has 4 elements
    while (soundsData[padIndex].size() < 4)
    {
        soundsData[padIndex].add("");
    }

    soundsData[padIndex].set(3, query);  // Set query in 4th position
}


//==============================================================================
// SampleDragArea Implementation
//==============================================================================

SampleDragArea::SampleDragArea()
    : sampleGrid(nullptr)
    , isDragging(false)
{
    setSize(80, 40);
}

SampleDragArea::~SampleDragArea()
{
}

void SampleDragArea::paint(Graphics& g)
{
    auto bounds = getLocalBounds();

    // Modern dark drag area styling
    if (isDragging)
    {
        g.setGradientFill(ColourGradient(
            Colour(0x8000D9FF).withAlpha(0.5f), bounds.getTopLeft().toFloat(),
            Colour(0x800099CC).withAlpha(0.5f), bounds.getBottomRight().toFloat(), false));
    }
    else
    {
        g.setColour(Colour(0x802A2A2A).withAlpha(0.8f));
    }
    g.fillRoundedRectangle(bounds.toFloat(), 6.0f);

    // Modern border
    g.setColour(isDragging ? Colour(0x8000D9FF) : Colour(0x80404040));
    g.drawRoundedRectangle(bounds.toFloat().reduced(1), 6.0f, 1.5f);

    // Modern icon and text styling
    g.setColour(Colours::white);
    g.setFont(Font(10.0f, Font::bold));

    // Draw a modern drag icon (three horizontal lines)
    auto iconBounds = bounds.removeFromLeft(20);
    int lineY = iconBounds.getCentreY() - 6;
    for (int i = 0; i < 3; ++i)
    {
        g.fillRoundedRectangle(iconBounds.getX() + 4, lineY + i * 4, 12, 2, 1.0f);
    }

    // Draw text
    g.drawText("Drag\nSamples", bounds, Justification::centred);
}

void SampleDragArea::resized()
{
}

void SampleDragArea::mouseDown(const MouseEvent& event)
{
    if (!sampleGrid)
        return;

    // Check if we have any samples to drag
    auto samples = sampleGrid->getAllSampleInfo();
    if (samples.isEmpty())
        return;

    isDragging = true;
    repaint();
}

void SampleDragArea::mouseDrag(const MouseEvent& event)
{
    if (!sampleGrid || !isDragging)
        return;

    auto samples = sampleGrid->getAllSampleInfo();
    if (samples.isEmpty())
        return;

    // Create StringArray of file paths for drag operation
    StringArray filePaths;

    for (const auto& sample : samples)
    {
        if (sample.hasValidSample && sample.audioFile.existsAsFile())
        {
            // Reference the existing file directly from the subfolder
            filePaths.add(sample.audioFile.getFullPathName());
        }
    }

    if (!filePaths.isEmpty())
    {
        // Start the drag operation with existing files
        performExternalDragDropOfFiles(filePaths, false); // false = don't allow moving
    }

    isDragging = false;
    repaint();
}

void SampleDragArea::setSampleGridComponent(SampleGridComponent* gridComponent)
{
    sampleGrid = gridComponent;
}

//==============================================================================
// DirectoryOpenButton Implementation
//==============================================================================

DirectoryOpenButton::DirectoryOpenButton()
    : processor(nullptr)
{
    setButtonText("View Files");
    setSize(60, 40);

    // Modern dark button styling
    setColour(TextButton::buttonColourId, Colour(0x80404040));
    setColour(TextButton::buttonOnColourId, Colour(0x80FFB347));
    setColour(TextButton::textColourOffId, Colours::white);
    setColour(TextButton::textColourOnId, Colours::black);

    // Set up the click callback using lambda
    onClick = [this]() {
        if (!processor)
            return;

        File currentFolder = processor->getCurrentDownloadLocation();

        if (currentFolder.exists())
        {
            currentFolder.revealToUser();
        }
        else
        {
            // If current session folder doesn't exist, open the main download directory
            File mainFolder = processor->tmpDownloadLocation;
            if (mainFolder.exists())
            {
                mainFolder.revealToUser();
            }
            else
            {
                // Create and open the main directory if it doesn't exist
                mainFolder.createDirectory();
                mainFolder.revealToUser();
            }
        }
    };
}

DirectoryOpenButton::~DirectoryOpenButton()
{
}

void DirectoryOpenButton::paint(Graphics &g) {

    // use same styling as SampleDragArea
    auto bounds = getLocalBounds();

    // highlight same as SampleDragArea when hovered
    if (isMouseOver())
    {
        // g.setGradientFill(ColourGradient(
        //    Colour(0x8000D9FF).withAlpha(0.5f), bounds.getTopLeft().toFloat(),
        //    Colour(0x800099CC).withAlpha(0.5f), bounds.getBottomRight().toFloat(), false));
        g.setColour(Colour(0x8000D9FF).withAlpha(0.3f));
        g.fillRoundedRectangle(bounds.toFloat(), 6.0f);
    }
    else
    {
        g.setColour(Colour(0x80404040));
        g.fillRoundedRectangle(bounds.toFloat(), 6.0f);
    }

    g.setColour(Colours::white);
    g.setFont(Font(10.0f, Font::bold));
    g.drawText("View Resources", bounds, Justification::centred);


}

void DirectoryOpenButton::setProcessor(FreesoundAdvancedSamplerAudioProcessor* p)
{
    processor = p;
}


