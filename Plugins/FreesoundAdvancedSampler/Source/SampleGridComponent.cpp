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
{
    formatManager.registerBasicFormats();

    // Generate a unique color for each pad
    float hue = (float)padIndex / 16.0f;
    padColour = Colour::fromHSV(hue, 0.3f, 0.8f, 1.0f);

    // Set up query text box
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
        // When user presses enter, trigger search with this pad's query
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
}

SamplePad::~SamplePad()
{
}

void SamplePad::resized()
{
    auto bounds = getLocalBounds();

    // Position query text box at the bottom, between drag and search badges
    auto queryBounds = bounds.reduced(3);
    int queryHeight = 14;
    int dragBadgeWidth = 30; // Width of drag badge
    int searchBadgeWidth = 35; // Width of search badge
    int badgeSpacing = 3; // Small spacing between elements

    // Bottom line: Drag badge + Query text box + Search badge
    queryBounds = queryBounds.removeFromBottom(queryHeight);
    queryBounds.removeFromLeft(dragBadgeWidth + badgeSpacing); // Space for drag badge + spacing
    queryBounds.removeFromRight(searchBadgeWidth + badgeSpacing); // Space for search badge + spacing

    queryTextBox.setBounds(queryBounds);
}

void SamplePad::paint(Graphics& g)
{
    auto bounds = getLocalBounds();

    // Modern dark background with subtle gradients
    if (isDragHover)
    {
        // Bright accent when dragging over
        g.setColour(Colour(0x8000D9FF).withAlpha(0.6f)); // Bright cyan for drag hover
    }
    else if (isPlaying)
    {
        // Glowing effect when playing
        g.setGradientFill(ColourGradient(
            padColour.brighter(0.4f), bounds.getCentre().toFloat(),
            padColour.darker(0.2f), bounds.getBottomRight().toFloat(), false));
    }
    else if (hasValidSample)
    {
        // Subtle gradient for active pads
        g.setGradientFill(ColourGradient(
            padColour.withAlpha(0.3f), bounds.getTopLeft().toFloat(),
            padColour.darker(0.3f).withAlpha(0.3f), bounds.getBottomRight().toFloat(), false));
    }
    else
    {
        // Dark grey for empty pads
        g.setColour(Colour(0x801A1A1A)); // Very dark grey
    }

    g.fillRoundedRectangle(bounds.toFloat(), 6.0f); // Slightly more rounded corners

    // Modern border styling
    if (isDragHover)
    {
        g.setColour(Colour(0x8000D9FF)); // Bright cyan border for drag hover
        g.drawRoundedRectangle(bounds.toFloat().reduced(1), 6.0f, 2.5f);
    }
    else if (isPlaying)
    {
        g.setColour(Colours::white);
        g.drawRoundedRectangle(bounds.toFloat().reduced(1), 6.0f, 2.0f);
    }
    else if (hasValidSample)
    {
        g.setColour(Colour(0x80404040)); // Medium grey border
        g.drawRoundedRectangle(bounds.toFloat().reduced(1), 6.0f, 1.0f);
    }
    else
    {
        g.setColour(Colour(0x802A2A2A)); // Dark grey border for empty pads
        g.drawRoundedRectangle(bounds.toFloat().reduced(1), 6.0f, 1.0f);
    }

    // === TOP LINE: Pad number + Web badge + License badge ===

    // Pad number with modern styling
    g.setColour(Colours::white);
    g.setFont(Font(9.0f, Font::bold));
    auto numberBounds = bounds.reduced(4);
    auto numberRect = numberBounds.removeFromTop(14).removeFromLeft(18);

    // Small dark background for pad number
    g.setColour(Colour(0x80000000).withAlpha(0.6f));
    g.fillRoundedRectangle(numberRect.toFloat(), 2.0f);
    g.setColour(Colours::white);
    g.drawText(String(padIndex + 1), numberRect, Justification::centred);

    // Web badge and License badge in top-right area
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

            // Darker red gradient
            g.setGradientFill(ColourGradient(
                Colour(0x80C62828), delBounds.getTopLeft().toFloat(),  // Darker red
                Colour(0x808E0000), delBounds.getBottomRight().toFloat(), false));  // Deep red
            g.fillRoundedRectangle(delBounds.toFloat(), 3.0f);

            // Soft white text
            g.setColour(Colours::white.withAlpha(0.9f));
            g.drawText("DEL", delBounds, Justification::centred);

            // Add small spacing between badges
            topRightBounds.removeFromRight(3);
        }

        // License badge (middle) - Darker amber styling
        if (licenseType.isNotEmpty())
        {
            String shortLicense = getLicenseShortName(licenseType);
            int licenseBadgeWidth = 32;
            auto licenseBounds = topRightBounds.removeFromRight(licenseBadgeWidth);

            // Darker amber gradient
            g.setGradientFill(ColourGradient(
                Colour(0x80EF6C00), licenseBounds.getTopLeft().toFloat(),  // Dark orange
                Colour(0x80BF360C), licenseBounds.getBottomRight().toFloat(), false));  // Deep orange
            g.fillRoundedRectangle(licenseBounds.toFloat(), 3.0f);

            // Soft white text
            g.setColour(Colours::white.withAlpha(0.9f));
            g.drawText(shortLicense, licenseBounds, Justification::centred);

            // Add small spacing between badges
            topRightBounds.removeFromRight(3);
        }

        // Web badge (leftmost) - Darker blue styling
        if (freesoundId.isNotEmpty())
        {
            int webBadgeWidth = 28;
            auto webBounds = topRightBounds.removeFromRight(webBadgeWidth);

            // Darker blue gradient
            g.setGradientFill(ColourGradient(
                Colour(0x800277BD), webBounds.getTopLeft().toFloat(),  // Dark blue
                Colour(0x8001579B), webBounds.getBottomRight().toFloat(), false));  // Deep blue
            g.fillRoundedRectangle(webBounds.toFloat(), 3.0f);

            // Soft white text
            g.setColour(Colours::white.withAlpha(0.9f));
            g.drawText("Web", webBounds, Justification::centred);
        }
    }

    // === MIDDLE: Waveform and sample text ===

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

    // === BOTTOM LINE: Drag badge + Query text box + Search badge ===

    // Drag badge in bottom-left corner - Modern green styling
    if (hasValidSample)
    {
        g.setFont(Font(8.0f, Font::bold));

        // Create drag badge in bottom-left corner
        auto dragBounds = bounds.reduced(4);
        int badgeWidth = 28;
        int badgeHeight = 14;
        dragBounds = dragBounds.removeFromBottom(badgeHeight).removeFromLeft(badgeWidth);

        // Modern green gradient
        g.setGradientFill(ColourGradient(
            Colour(0x804ECDC4), dragBounds.getTopLeft().toFloat(),
            Colour(0x8026A69A), dragBounds.getBottomRight().toFloat(), false));
        g.fillRoundedRectangle(dragBounds.toFloat(), 3.0f);

        // White text
        g.setColour(Colours::white);
        g.drawText("Drag", dragBounds, Justification::centred);
    }

    // Search badge in bottom-right corner - Modern purple styling (always visible)
    {
        g.setFont(Font(8.0f, Font::bold));

        // Create search badge in bottom-right corner
        auto searchBounds = bounds.reduced(4);
        int badgeWidth = 32;
        int badgeHeight = 14;
        searchBounds = searchBounds.removeFromBottom(badgeHeight).removeFromRight(badgeWidth);

        // Modern purple gradient
        g.setGradientFill(ColourGradient(
            Colour(0x809C88FF), searchBounds.getTopLeft().toFloat(),
            Colour(0x807B68EE), searchBounds.getBottomRight().toFloat(), false));
        g.fillRoundedRectangle(searchBounds.toFloat(), 3.0f);

        // White text
        g.setColour(Colours::white);
        g.drawText("Search", searchBounds, Justification::centred);
    }

    // Empty pad text (centered, avoiding all badges)
    if (!hasValidSample)
    {
        g.setColour(Colour(0x80666666)); // Medium grey for empty text
        g.setFont(Font(12.0f, Font::bold));
        auto emptyBounds = bounds.reduced(8);
        emptyBounds.removeFromTop(18); // Space for top line
        emptyBounds.removeFromBottom(18); // Space for bottom line
        g.drawText("Empty", emptyBounds, Justification::centred);
    }
}

void SamplePad::mouseDown(const MouseEvent& event)
{
    // Check if clicked on search badge (bottom-right) - ALWAYS available
    {
       auto bounds = getLocalBounds();
       auto searchBounds = bounds.reduced(3);
       int badgeWidth = 35;
       int badgeHeight = 12;
       searchBounds = searchBounds.removeFromBottom(badgeHeight).removeFromRight(badgeWidth);

       if (searchBounds.contains(event.getPosition()))
       {
           // Use the pad's individual query if it has one, otherwise use master query
           String searchQuery = queryTextBox.getText().trim();
           if (searchQuery.isEmpty() && processor)
           {
               searchQuery = processor->getQuery();
           }

           if (searchQuery.isNotEmpty())
           {
               // Update the text box with the query being used
               queryTextBox.setText(searchQuery);

               // Trigger single pad search
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

    // Update query text box
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
    shuffleButton.setButtonText("Shuffle");
    shuffleButton.onClick = [this]() {
        shuffleSamples();
    };
    addAndMakeVisible(shuffleButton);

    // Set up clear all button
    clearAllButton.setButtonText("Clear All");
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

    DBG("SampleGridComponent::updateSamples called with " + String(sounds.size()) + " sounds");

    // For preset loading, always use the provided arrays directly
    // This ensures the visual grid matches exactly what the sampler loaded
    if (!sounds.isEmpty())
    {
        DBG("Using provided arrays for grid update");
        loadSamplesFromArrays(sounds, soundInfo, downloadDir);
    }
    else
    {
        // Only try JSON if no sounds were provided (fallback for edge cases)
        File metadataFile = downloadDir.getChildFile("metadata.json");
        if (metadataFile.existsAsFile())
        {
            DBG("No sounds provided, trying JSON metadata");
            loadSamplesFromJson(metadataFile);
        }
        else
        {
            DBG("No sounds or metadata available");
        }
    }

    // Force repaint all pads
    for (auto& pad : samplePads)
    {
        pad->repaint();
    }

    DBG("Grid visual update complete");
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

    DBG("Loading " + String(numSamples) + " samples to grid from arrays");

    for (int i = 0; i < numSamples; ++i)
    {
        const FSSound& sound = sounds[i];

        String sampleName = (i < soundInfo.size() && soundInfo[i].size() > 0) ? soundInfo[i][0] : sound.name;
        String authorName = (i < soundInfo.size() && soundInfo[i].size() > 1) ? soundInfo[i][1] : sound.user;
        String license = (i < soundInfo.size() && soundInfo[i].size() > 2) ? soundInfo[i][2] : sound.license;
        String freesoundId = sound.id;

        // Create filename using ID-based naming scheme: FS_ID_XXXX.ogg
        String expectedFilename = "FS_ID_" + freesoundId + ".ogg";
        File audioFile = downloadDir.getChildFile(expectedFilename);

        DBG("Pad " + String(i) + ": " + sampleName + " (ID: " + freesoundId + ")");
        DBG("  Looking for file: " + audioFile.getFullPathName());
        DBG("  File exists: " + String(audioFile.existsAsFile() ? "YES" : "NO"));

        // Load the sample whether file exists or not (for consistent pad mapping)
        if (audioFile.existsAsFile())
        {
            samplePads[i]->setSample(audioFile, sampleName, authorName, freesoundId, license);
            DBG("  Successfully set sample on pad " + String(i));
        }
        else
        {
            // Clear the pad if file doesn't exist
            samplePads[i]->clearSample();
            DBG("  Cleared pad " + String(i) + " - file not found");
        }
    }

    // Clear any remaining pads
    for (int i = numSamples; i < TOTAL_PADS; ++i)
    {
        samplePads[i]->clearSample();
    }

    // FIXED: Use a member timer instead of Timer::callAfterDelay to ensure proper cleanup
    // Start the delayed repaint timer
    startTimer(200); // This will call timerCallback after 200ms

    DBG("Grid update complete");
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

    // Update processor arrays and metadata to match new order
    updateProcessorArraysFromGrid();
    updateJsonMetadata();

    // Reload the sampler with new pad order
    if (processor)
    {
        processor->setSources();
        processor->updateReadmeFile();
    }

    DBG("Samples shuffled successfully");
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
            // Create FSSound object from pad info
            FSSound sound;
            sound.id = padInfo.freesoundId;
            sound.name = padInfo.sampleName;
            sound.user = padInfo.authorName;
            sound.license = padInfo.licenseType;

            // Try to get duration and file size from existing data or set defaults
            sound.duration = 0.5; // Default duration
            sound.filesize = 50000; // Default file size

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

            // Create sound info for legacy compatibility
            StringArray soundData;
            soundData.add(padInfo.sampleName);
            soundData.add(padInfo.authorName);
            soundData.add(padInfo.licenseType);
            soundsData.push_back(soundData);
        }
    }

    DBG("Updated processor arrays from grid - now has " + String(currentSounds.size()) + " sounds");
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
    SoundList list = client.textSearch(query, "duration:[0 TO 0.5]", "score", 1, -1, 50, "id,name,username,license,previews");
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
        soundsData.push_back(emptyData);
    }

    // Update the specific pad
    currentSounds.set(padIndex, sound);

    StringArray soundData;
    soundData.add(sound.name);
    soundData.add(sound.user);
    soundData.add(sound.license);
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

        // Set the master query in the pad's text box
        samplePads[padIndex]->setQuery(masterQuery);

        // Create filename using ID-based naming scheme
        String expectedFilename = "FS_ID_" + sound.id + ".ogg";
        File audioFile = downloadDir.getChildFile(expectedFilename);

        // Get sample info
        String sampleName = (soundIndex < soundInfo.size() && soundInfo[soundIndex].size() > 0) ? soundInfo[soundIndex][0] : sound.name;
        String authorName = (soundIndex < soundInfo.size() && soundInfo[soundIndex].size() > 1) ? soundInfo[soundIndex][1] : sound.user;
        String license = (soundIndex < soundInfo.size() && soundInfo[soundIndex].size() > 2) ? soundInfo[soundIndex][2] : sound.license;

        if (audioFile.existsAsFile())
        {
            samplePads[padIndex]->setSample(audioFile, sampleName, authorName, sound.id, license, masterQuery);
        }

        soundIndex++;
    }

    // Update processor arrays to reflect the new state
    updateProcessorArraysFromGrid();

    // Update JSON metadata
    updateJsonMetadata();

    // Reload the sampler
    if (processor)
    {
        processor->setSources();
        processor->updateReadmeFile();
    }

    DBG("Master search applied to " + String(emptyQueryPads.size()) + " pads with empty queries");
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

    // Create a download manager for single file
    singlePadDownloadManager = std::make_unique<AudioDownloadManager>();
    currentDownloadPadIndex = padIndex;
    currentDownloadSound = sound;

    // Use a timer to check for completion
    startTimer(500); // Check every 500ms

    // Start download
    Array<FSSound> singleSoundArray;
    singleSoundArray.add(sound);

    File samplesFolder = processor->getCurrentDownloadLocation();
    singlePadDownloadManager->startDownloads(singleSoundArray, samplesFolder, query);

}

void SampleGridComponent::timerCallback()
{
    // Check if we're handling single pad downloads
    if (singlePadDownloadManager && currentDownloadPadIndex >= 0)
    {
        // Existing single pad download logic...
        String fileName = "FS_ID_" + currentDownloadSound.id + ".ogg";
        File samplesFolder = processor->getCurrentDownloadLocation();
        File audioFile = samplesFolder.getChildFile(fileName);

        if (audioFile.existsAsFile())
        {
            // Download completed successfully
            stopTimer();
            loadSingleSampleWithQuery(currentDownloadPadIndex, currentDownloadSound, audioFile, currentDownloadQuery);

            // Clean up
            singlePadDownloadManager.reset();
            currentDownloadPadIndex = -1;
            currentDownloadQuery = "";
        }
        else if (!singlePadDownloadManager->isThreadRunning())
        {
            // Download thread finished but no file - failed
            stopTimer();

            AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
                "Download Failed",
                "Failed to download sample for pad " + String(currentDownloadPadIndex + 1));

            // Clean up
            singlePadDownloadManager.reset();
            currentDownloadPadIndex = -1;
            currentDownloadQuery = "";
        }
    }
    else
    {
        // This is the delayed repaint from loadSamplesFromArrays
        stopTimer(); // Stop the timer since this is a one-time repaint

        // Safely repaint all pads, checking if they're still valid
        for (auto& pad : samplePads)
        {
            if (pad && pad->isShowing() && pad->getLocalBounds().getWidth() > 0 && pad->getLocalBounds().getHeight() > 0)
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
        g.setGradientFill(ColourGradient(
            Colour(0x8000D9FF).withAlpha(0.5f), bounds.getTopLeft().toFloat(),
            Colour(0x800099CC).withAlpha(0.5f), bounds.getBottomRight().toFloat(), false));
        g.fillRoundedRectangle(bounds.toFloat(), 6.0f);
    }
    else
    {
        g.setColour(Colour(0x80404040));
        g.fillRoundedRectangle(bounds.toFloat(), 6.0f);
    }


    g.setColour(Colour(0x802A2A2A).withAlpha(0.8f));
    g.fillRoundedRectangle(bounds.toFloat(), 6.0f);
    g.setColour(Colour(0x80404040));
    g.drawRoundedRectangle(bounds.toFloat().reduced(1), 6.0f, 1.5f);
    g.setColour(Colours::white);
    g.setFont(Font(10.0f, Font::bold));
    g.drawText("View Resources", bounds, Justification::centred);


}



void DirectoryOpenButton::setProcessor(FreesoundAdvancedSamplerAudioProcessor* p)
{
    processor = p;
}