/*
  ==============================================================================

    BookmarkViewerComponent.cpp - Updated with Preview Playback Tracking

  ==============================================================================
*/

#include "BookmarkViewerComponent.h"
#include "PluginEditor.h"

//==============================================================================
// BookmarkSamplePad Implementation
//==============================================================================

BookmarkSamplePad::BookmarkSamplePad(int index, const BookmarkInfo& bookmarkInfo)
    : SamplePad(index, false), bookmark(bookmarkInfo) // false = non-searchable
{
    padColour = defaultColour.withAlpha(0.2f);
}

BookmarkSamplePad::~BookmarkSamplePad()
{
    // Make sure to stop preview if component is destroyed while playing
    if (isPreviewPlaying)
    {
        stopPreviewPlayback();
    }
}

void BookmarkSamplePad::paint(Graphics& g)
{
    // Call parent paint first to draw everything normally
    SamplePad::paint(g);

    // Add preview playhead overlay if playing
    if (isPreviewPlaying && hasValidSample)
    {
        auto bounds = getLocalBounds();
        auto waveformBounds = bounds.reduced(3);
        waveformBounds.removeFromTop(18); // Space for top line badges
        waveformBounds.removeFromBottom(18); // Space for bottom line

        // Draw preview playhead (different color from main playhead)
        float x = waveformBounds.getX() + (previewPlayheadPosition * waveformBounds.getWidth());

        g.setColour(Colours::orange); // Orange for preview playhead vs red for main
        g.drawLine(x, waveformBounds.getY(), x, waveformBounds.getBottom(), 2.0f);
    }
}

void BookmarkSamplePad::setPreviewPlaying(bool playing)
{
    // Ensure GUI updates happen on the message thread
    MessageManager::callAsync([this, playing]()
    {
        isPreviewPlaying = playing;

        // Update pad color based on playing state
        if (playing)
        {
            padColour = defaultColour.withAlpha(0.5f);
        }
        else
        {
            padColour = defaultColour.withAlpha(0.2f);
            previewPlayheadPosition = 0.0f; // Reset playhead when stopped
        }

        repaint();
    });
}

void BookmarkSamplePad::setPreviewPlayheadPosition(float position)
{
    // Ensure GUI updates happen on the message thread
    MessageManager::callAsync([this, position]()
    {
        previewPlayheadPosition = jlimit(0.0f, 1.0f, position);
        repaint();
    });
}

void BookmarkSamplePad::mouseDown(const MouseEvent& event)
{

    // Reset flags
    mouseDownInWaveform = false;
    previewRequested = false;

    // Don't allow interaction while downloading
    if (isDownloading)
    {
        DBG("  - Download in progress, ignoring");
        return;
    }

    // Check if clicked on any badge FIRST
    Badge* clickedBadge = findBadgeAtPosition(event.getPosition());
    if (clickedBadge && clickedBadge->onClick)
    {
        clickedBadge->onClick();
        return;
    }

    // If no valid sample and didn't click a badge, return
    if (!hasValidSample)
    {
        DBG("  - No valid sample");
        return;
    }

    // Calculate waveform bounds
    auto bounds = getLocalBounds();
    auto waveformBounds = bounds.reduced(8);
    waveformBounds.removeFromTop(18); // Space for top line badges
    waveformBounds.removeFromBottom(18); // Space for bottom line

    if (waveformBounds.contains(event.getPosition()))
    {
        mouseDownInWaveform = true;

        // Start preview immediately and synchronously
        startPreviewPlayback();
        return;
    }
    else
    {
        // Handle potential drag preparation for other areas
        // Call parent for badge dragging etc.
        // SamplePad::mouseDown(event);  // Commented out to avoid conflicts
    }
}

void BookmarkSamplePad::mouseDrag(const MouseEvent& event)
{
    // If we started preview in waveform, check if we're still in bounds
    if (mouseDownInWaveform && previewRequested)
    {
        auto bounds = getLocalBounds();
        auto waveformBounds = bounds.reduced(8);
        waveformBounds.removeFromTop(18);
        waveformBounds.removeFromBottom(18);

        // If mouse moves outside waveform area while dragging, stop preview
        if (!waveformBounds.contains(event.getPosition()))
        {
            stopPreviewPlayback();
            mouseDownInWaveform = false;
            return;
        }
    }
    else if (!mouseDownInWaveform)
    {
        // For non-preview drags (like copy badge), call parent implementation
        // Check if drag started on a badge that supports dragging
        Badge* draggedBadge = findBadgeAtPosition(event.getMouseDownPosition());
        if (draggedBadge && draggedBadge->onDrag)
        {
            draggedBadge->onDrag(event);
            return;
        }

        // Handle potential sample copying drag from edge areas
        if (hasValidSample && event.getDistanceFromDragStart() > 10)
        {
            performEnhancedDragDrop();
        }
    }
}

void BookmarkSamplePad::mouseUp(const MouseEvent& event)
{

    // Always stop preview when mouse is released, regardless of where
    if (previewRequested || isPreviewPlaying)
    {
        stopPreviewPlayback();
    }

    // Reset state
    mouseDownInWaveform = false;
    previewRequested = false;

    // Reset cursor if it was changed
    setMouseCursor(MouseCursor::NormalCursor);
}

void BookmarkSamplePad::startPreviewPlayback()
{

    if (!hasValidSample)
    {
        DBG("  - No valid sample, aborting");
        return;
    }

    if (!audioFile.existsAsFile())
    {
        DBG("  - Audio file doesn't exist: " + audioFile.getFullPathName());
        return;
    }

    if (!processor)
    {
        DBG("  - No processor, aborting");
        return;
    }

    if (previewRequested)
    {
        DBG("  - Preview already requested, ignoring");
        return;
    }

    previewRequested = true;

    // Load the sample into the preview sampler
    processor->loadPreviewSample(audioFile, freesoundId);

    // Small delay to ensure sample is loaded before playing
    Timer::callAfterDelay(50, [this]() {
        if (previewRequested && processor) // Check if still valid
        {
            processor->playPreviewSample();

            // Change cursor to indicate preview mode
            setMouseCursor(MouseCursor::PointingHandCursor);
        }
    });
}

void BookmarkSamplePad::stopPreviewPlayback()
{

    if (!previewRequested && !isPreviewPlaying)
    {
        DBG("  - No preview to stop");
        return;
    }

    if (processor)
    {
        processor->stopPreviewSample();
    }

    previewRequested = false;

    // Reset cursor
    setMouseCursor(MouseCursor::NormalCursor);
}

// Also add these debug methods to help troubleshoot:

void BookmarkSamplePad::mouseEnter(const MouseEvent& event)
{
    SamplePad::mouseEnter(event);
}

void BookmarkSamplePad::mouseExit(const MouseEvent& event)
{

    // Stop preview if mouse exits while playing
    if (previewRequested || isPreviewPlaying)
    {
        stopPreviewPlayback();
    }

    mouseDownInWaveform = false;
    previewRequested = false;

    SamplePad::mouseExit(event);
}

//==============================================================================
// BookmarkViewerComponent Implementation
//==============================================================================

BookmarkViewerComponent::BookmarkViewerComponent()
    : processor(nullptr)
{
    // Title label
    titleLabel.setText("Bookmarks", dontSendNotification);
    addAndMakeVisible(titleLabel);

    // Refresh button
    refreshButton.onClick = [this]() {
        refreshBookmarks();
    };
    addAndMakeVisible(refreshButton);

    // Viewport for scrolling
    bookmarkViewport.setViewedComponent(&bookmarkContainer, false);
    bookmarkViewport.setScrollBarsShown(true, false); // Vertical scrollbar only
    addAndMakeVisible(bookmarkViewport);
}

BookmarkViewerComponent::~BookmarkViewerComponent()
{
    // Remove preview playback listener
    if (processor)
    {
        processor->removePreviewPlaybackListener(this);
    }

    clearBookmarkPads();
}

void BookmarkViewerComponent::paint(Graphics& g)
{
    // Dark background
    g.setColour(Colour(0xff1A1A1A));
    g.fillAll();

    // Subtle border
    g.setColour(Colour(0xff404040).withAlpha(0.3f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2), 6.0f, 1.0f);
}

void BookmarkViewerComponent::resized()
{
    auto bounds = getLocalBounds().reduced(8);

    int textHeight = 18; // Height for title and refresh button
    int refreshButtonWidth = 17; // Height for refresh button
    int gapHeight = 8; // Gap

    // Title takes most of the header
    titleLabel.setBounds(bounds.removeFromTop(textHeight));

    bounds.removeFromTop(gapHeight);

    // Refresh button on the right
    refreshButton.setBounds(bounds.removeFromLeft(refreshButtonWidth));

    bounds.removeFromLeft(6);
    // Viewport takes the rest
    bookmarkViewport.setBounds(bounds);

    // Update the scrollable area
    updateScrollableArea();
}

void BookmarkViewerComponent::setProcessor(FreesoundAdvancedSamplerAudioProcessor* p)
{
    // Remove old listener if exists
    if (processor)
    {
        processor->removePreviewPlaybackListener(this);
    }

    processor = p;

    if (processor)
    {
        // Add preview playback listener
        processor->addPreviewPlaybackListener(this);

        // Initial load of bookmarks
        refreshBookmarks();
    }
}

void BookmarkViewerComponent::refreshBookmarks()
{
    if (!processor)
        return;

    // Get all bookmarks from the bookmark manager
    currentBookmarks = processor->getBookmarkManager().getAllBookmarks();

    // Sort by bookmarked date (newest first)
    std::sort(currentBookmarks.begin(), currentBookmarks.end(),
              [](const BookmarkInfo& a, const BookmarkInfo& b) {
                  return a.bookmarkedAt > b.bookmarkedAt;
              });

    // Update the display
    updateBookmarkPads();
}

void BookmarkViewerComponent::updateBookmarkPads()
{
    // Clear existing pads
    clearBookmarkPads();

    // Calculate total rows needed
    totalRows = currentBookmarks.size();

    // Create pads for bookmarks
    createBookmarkPads();

    // iterate through pads and add them to the container
    for (auto* pad : bookmarkPads)
    {
        bookmarkContainer.addAndMakeVisible(pad);
        bookmarkPadMap[pad->getFreesoundId()] = pad; // Map by freesound ID
    }

    // Update scrollable area
    updateScrollableArea();
}

void BookmarkViewerComponent::createBookmarkPads()
{
    if (currentBookmarks.isEmpty())
        return;

    const int padWidth = 180;
    const int padHeight = 80;
    const int padSpacing = 8;

    for (int i = 0; i < currentBookmarks.size(); ++i)
    {
        const BookmarkInfo& bookmark = currentBookmarks[i];

        // FIXED: Use unique index to avoid conflicts with main grid
        auto* pad = new BookmarkSamplePad(1000 + i, bookmark);
        pad->setProcessor(processor);

        // Load the bookmark sample if file exists
        File sampleFile = processor->getPresetManager().getSampleFile(bookmark.freesoundId);
        if (sampleFile.existsAsFile())
        {
            pad->setSample(sampleFile, bookmark.sampleName, bookmark.authorName,
                          bookmark.freesoundId, bookmark.licenseType, bookmark.searchQuery);
        }

        // Calculate position (single column)
        int x = 0;
        int y = i * (padHeight + padSpacing);

        pad->setBounds(x, y, padWidth, padHeight);

        bookmarkContainer.addAndMakeVisible(pad);
        bookmarkPads.add(pad);
    }
}

void BookmarkViewerComponent::clearBookmarkPads()
{
    bookmarkPads.clear();
    bookmarkContainer.removeAllChildren();

    // Clear the map as well
    bookmarkPadMap.clear();
}

void BookmarkViewerComponent::updateScrollableArea()
{
    const int padHeight = 100; // Match the padHeight from createBookmarkPads
    const int padSpacing = 4;
    const int rowHeight = padHeight + padSpacing;

    // Calculate total height needed
    int totalHeight = totalRows * rowHeight;

    // Set container size
    bookmarkContainer.setSize(bookmarkViewport.getWidth() - 20, // Account for scrollbar
                             jmax(totalHeight, bookmarkViewport.getHeight()));
}

void BookmarkViewerComponent::loadBookmarkIntoSampleGrid(const BookmarkInfo& bookmark)
{
    // This method is no longer used since we removed the click-to-load functionality
    // Users now drag from the copy badge to load samples
}

void BookmarkViewerComponent::scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart)
{
    // The viewport handles scrolling automatically
}

//==============================================================================
// NEW: PreviewPlaybackListener Implementation
//==============================================================================

void BookmarkViewerComponent::previewStarted(const String& freesoundId)
{
    if (auto* pad = findPadByFreesoundId(freesoundId))
    {
        pad->setPreviewPlaying(true);
    }
}

void BookmarkViewerComponent::previewStopped(const String& freesoundId)
{
    if (auto* pad = findPadByFreesoundId(freesoundId))
    {
        pad->setPreviewPlaying(false);
    }
}

void BookmarkViewerComponent::previewPlayheadPositionChanged(const String& freesoundId, float position)
{
    if (auto* pad = findPadByFreesoundId(freesoundId))
    {
        pad->setPreviewPlayheadPosition(position);
    }
}

BookmarkSamplePad* BookmarkViewerComponent::findPadByFreesoundId(const String& freesoundId)
{
    return bookmarkPadMap.count(freesoundId) > 0 ? bookmarkPadMap[freesoundId] : nullptr;
}