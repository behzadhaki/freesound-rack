/*
  ==============================================================================

    BookmarkViewerComponent.cpp - Updated with Preview Playback Tracking

  ==============================================================================
*/

#include "BookmarkViewerComponent.h"
#include "PluginEditor.h"

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
}

void BookmarkViewerComponent::resized()
{
    auto bounds = getLocalBounds().reduced(3);

    int textHeight = 18; // Height for title and refresh button
    int refreshButtonWidth = 17; // Width for refresh button
    int gapHeight = 8; // Gap

    // Title takes most of the header
    auto topLine = bounds.removeFromTop(textHeight);
    auto topLine2 = getLocalBounds().reduced(3).removeFromTop(textHeight);
    titleLabel.setBounds(topLine);
    refreshButton.setBounds(topLine2.removeFromLeft(refreshButtonWidth));


    bounds.removeFromTop(gapHeight);

    // Update the scrollable area
    updateScrollableArea();

    // Viewport takes the rest
    bookmarkViewport.setBounds(bounds);
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

    // Update scrollable area
    updateScrollableArea();
}

void BookmarkViewerComponent::createBookmarkPads()
{
    if (currentBookmarks.isEmpty())
        return;

    const int padWidth = 190;
    const int padHeight = 100;
    const int padSpacing = 8;

    for (int i = 0; i < currentBookmarks.size(); ++i)
    {
        const BookmarkInfo& bookmark = currentBookmarks[i];

        // Create SamplePad in Preview mode with unique index
        auto* pad = new SamplePad(1000 + i, SamplePad::PadMode::Preview);
        pad->setProcessor(processor);

        // Load the bookmark sample if file exists
        File sampleFile = processor->getPresetManager().getSampleFile(bookmark.freesoundId);
        if (sampleFile.existsAsFile())
        {
            pad->setSample(sampleFile, bookmark.sampleName, bookmark.authorName,
                          bookmark.freesoundId, bookmark.licenseType, bookmark.searchQuery,
                          bookmark.tags, bookmark.description);
        }

        // Calculate position (single column)
        int x = 0;
        int y = i * (padHeight + padSpacing);

        pad->setBounds(x, y, padWidth, padHeight);

        bookmarkContainer.addAndMakeVisible(pad);
        bookmarkPads.add(pad);
        bookmarkPadMap[bookmark.freesoundId] = pad; // Map by freesound ID
    }
}

void BookmarkViewerComponent::clearBookmarkPads()
{
    bookmarkPads.clear();
    bookmarkContainer.removeAllChildren();
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
    bookmarkContainer.setSize(bookmarkViewport.getWidth() - 10, // Account for scrollbar
                             jmax(totalHeight, bookmarkViewport.getHeight()));
}

void BookmarkViewerComponent::scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart)
{
    // The viewport handles scrolling automatically
}

//==============================================================================
// PreviewPlaybackListener Implementation
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

SamplePad* BookmarkViewerComponent::findPadByFreesoundId(const String& freesoundId)
{
    return bookmarkPadMap.count(freesoundId) > 0 ? bookmarkPadMap[freesoundId] : nullptr;
}