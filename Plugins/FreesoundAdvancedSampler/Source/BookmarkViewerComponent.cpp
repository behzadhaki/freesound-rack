/*
  ==============================================================================

    BookmarkViewerComponent.cpp
    Created: Bookmark viewer component with scrollable sample pads
    Author: Generated

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

void BookmarkSamplePad::mouseDown(const MouseEvent& event)
{
    // Don't allow interaction while downloading
    if (isDownloading)
        return;

    // Check if clicked on any badge
    Badge* clickedBadge = findBadgeAtPosition(event.getPosition());
    if (clickedBadge && clickedBadge->onClick)
    {
        clickedBadge->onClick();
        return;
    }

    // If no valid sample and didn't click a badge, return
    if (!hasValidSample)
        return;

    // Check if clicked in waveform area - for preview playback
    auto bounds = getLocalBounds();
    auto waveformBounds = bounds.reduced(8);
    waveformBounds.removeFromTop(18); // Space for top line badges
    waveformBounds.removeFromBottom(18); // Space for bottom line

    if (waveformBounds.contains(event.getPosition()))
    {
        // Start preview playback (will play as long as mouse is held down)
        startPreviewPlayback();
        return;
    }

    // If clicked on edge areas (outside waveform but not on badges), handle edge dragging preparation
    // This will be handled in mouseDrag if the drag distance threshold is met
}

void BookmarkSamplePad::mouseDrag(const MouseEvent& event)
{
    // Check if we started preview in waveform area
    auto bounds = getLocalBounds();
    auto waveformBounds = bounds.reduced(8);
    waveformBounds.removeFromTop(18);
    waveformBounds.removeFromBottom(18);

    // If mouse moves outside waveform area while dragging, stop preview
    if (isPreviewPlaying && !waveformBounds.contains(event.getPosition()))
    {
        stopPreviewPlayback();
        return;
    }

    // For other drag operations (like copy badge), call parent implementation
    if (!isPreviewPlaying)
    {
        SamplePad::mouseDrag(event);
    }
}

void BookmarkSamplePad::mouseUp(const MouseEvent& event)
{
    // Always stop preview when mouse is released
    if (isPreviewPlaying)
    {
        stopPreviewPlayback();
    }

    // Reset cursor if it was changed
    setMouseCursor(MouseCursor::NormalCursor);
}

void BookmarkSamplePad::startPreviewPlayback()
{
    if (!hasValidSample || !audioFile.existsAsFile() || !processor || isPreviewPlaying)
        return;

    // Load the sample into the preview sampler
    processor->loadPreviewSample(audioFile, freesoundId);

    // Play the preview sample
    processor->playPreviewSample();

    isPreviewPlaying = true;

    padColour = defaultColour.withAlpha(0.5f);

    repaint();
    // Optional: Change cursor to indicate preview mode
    setMouseCursor(MouseCursor::PointingHandCursor);

}

void BookmarkSamplePad::stopPreviewPlayback()
{
    if (!isPreviewPlaying || !processor)
        return;

    // Stop the preview sample
    processor->stopPreviewSample();

    isPreviewPlaying = false;

    padColour = defaultColour.withAlpha(0.2f);

    repaint();
    // Reset cursor
    setMouseCursor(MouseCursor::NormalCursor);

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
    processor = p;

    if (processor)
    {
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