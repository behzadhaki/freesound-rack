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
}

BookmarkSamplePad::~BookmarkSamplePad()
{
}

void BookmarkSamplePad::mouseDown(const MouseEvent& event)
{
    // Check if this is a click on the waveform area (not on badges)
    auto bounds = getLocalBounds();
    auto waveformBounds = bounds.reduced(8);
    waveformBounds.removeFromTop(16); // Space for top line badges
    waveformBounds.removeFromBottom(16); // Space for bottom line

    if (waveformBounds.contains(event.getPosition()))
    {
        // This is a click on the waveform - trigger the bookmark load
        if (onBookmarkClicked)
            onBookmarkClicked(bookmark);
        return;
    }

    // For other areas, call the parent implementation (handles badges, etc.)
    SamplePad::mouseDown(event);
}

//==============================================================================
// BookmarkViewerComponent Implementation
//==============================================================================

BookmarkViewerComponent::BookmarkViewerComponent()
    : processor(nullptr)
{
    // Title label
    titleLabel.setText("Bookmarks", dontSendNotification);
    titleLabel.setFont(Font(14.0f, Font::bold));
    titleLabel.setJustificationType(Justification::centredLeft);
    titleLabel.setColour(Label::textColourId, Colours::white);
    addAndMakeVisible(titleLabel);

    // Refresh button
    refreshButton.onClick = [this]() {
        refreshBookmarks();
    };
    addAndMakeVisible(refreshButton);

    // Viewport for scrolling
    bookmarkViewport.setViewedComponent(&bookmarkContainer, false);
    bookmarkViewport.setScrollBarsShown(true, false); // Vertical scrollbar only
    bookmarkViewport.setColour(ScrollBar::backgroundColourId, Colour(0xff2A2A2A));
    bookmarkViewport.setColour(ScrollBar::thumbColourId, Colour(0xff555555));
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

    // Top header area
    auto headerBounds = bounds.removeFromTop(30);

    // Title takes most of the header
    auto titleBounds = headerBounds.removeFromLeft(headerBounds.getWidth() - 35);
    titleLabel.setBounds(titleBounds);

    // Refresh button on the right
    headerBounds.removeFromLeft(5); // Small spacing
    refreshButton.setBounds(headerBounds.withWidth(30).withHeight(24));

    bounds.removeFromTop(8); // Spacing after header

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
    totalRows = (currentBookmarks.size() + PADS_PER_ROW - 1) / PADS_PER_ROW;

    // Create pads for bookmarks
    createBookmarkPads();

    // Update scrollable area
    updateScrollableArea();
}

void BookmarkViewerComponent::createBookmarkPads()
{
    if (currentBookmarks.isEmpty())
        return;

    const int padWidth = 80;
    const int padHeight = 80;
    const int padSpacing = 4;

    int currentRow = 0;
    int currentCol = 0;

    for (int i = 0; i < currentBookmarks.size(); ++i)
    {
        const BookmarkInfo& bookmark = currentBookmarks[i];

        // Create a custom bookmark sample pad
        auto* pad = new BookmarkSamplePad(i, bookmark);
        pad->setProcessor(processor);

        // Load the bookmark sample if file exists
        File sampleFile = processor->getPresetManager().getSampleFile(bookmark.freesoundId);
        if (sampleFile.existsAsFile())
        {
            pad->setSample(sampleFile, bookmark.sampleName, bookmark.authorName,
                          bookmark.freesoundId, bookmark.licenseType, bookmark.searchQuery);
        }

        // Calculate position
        int x = currentCol * (padWidth + padSpacing);
        int y = currentRow * (padHeight + padSpacing);

        pad->setBounds(x, y, padWidth, padHeight);

        // Set up the click callback
        pad->onBookmarkClicked = [this](const BookmarkInfo& clickedBookmark) {
            loadBookmarkIntoSampleGrid(clickedBookmark);
        };

        bookmarkContainer.addAndMakeVisible(pad);
        bookmarkPads.add(pad);

        // Move to next position
        currentCol++;
        if (currentCol >= PADS_PER_ROW)
        {
            currentCol = 0;
            currentRow++;
        }
    }
}

void BookmarkViewerComponent::clearBookmarkPads()
{
    bookmarkPads.clear();
    bookmarkContainer.removeAllChildren();
}

void BookmarkViewerComponent::updateScrollableArea()
{
    const int padHeight = 80;
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
    if (!processor)
        return;

    // Get the editor to access the sample grid
    if (auto* editor = dynamic_cast<FreesoundAdvancedSamplerAudioProcessorEditor*>(processor->getActiveEditor()))
    {
        auto& sampleGrid = editor->getSampleGridComponent();

        // Find the first empty pad
        int targetPadIndex = -1;

        for (int i = 0; i < 16; ++i)
        {
            auto padInfo = sampleGrid.getPadInfo(i);
            if (!padInfo.hasValidSample)
            {
                targetPadIndex = i;
                break;
            }
        }

        if (targetPadIndex == -1)
        {
            AlertWindow::showMessageBoxAsync(
                AlertWindow::QuestionIcon,
                "All Pads Full",
                "All sample pads are occupied. Please clear a pad first, then try again.",
                "OK"
            );
            return;
        }

        // Check if sample file exists
        File sampleFile = processor->getPresetManager().getSampleFile(bookmark.freesoundId);
        if (!sampleFile.existsAsFile())
        {
            AlertWindow::showMessageBoxAsync(
                AlertWindow::WarningIcon,
                "Sample File Missing",
                "The sample file for \"" + bookmark.sampleName + "\" is missing from the resources folder.",
                "OK"
            );
            return;
        }

        // Create FSSound object
        FSSound sound;
        sound.id = bookmark.freesoundId;
        sound.name = bookmark.sampleName;
        sound.user = bookmark.authorName;
        sound.license = bookmark.licenseType;
        sound.duration = bookmark.duration;
        sound.filesize = bookmark.fileSize;

        // Access the sample pads directly from the grid
        if (targetPadIndex >= 0 && targetPadIndex < 16)
        {
            // Get the target pad and load the sample
            sampleGrid.samplePads[targetPadIndex]->setSample(sampleFile, bookmark.sampleName, bookmark.authorName,
                                                           bookmark.freesoundId, bookmark.licenseType, bookmark.searchQuery);

            // Update processor arrays
            auto& currentSounds = processor->getCurrentSoundsArrayReference();
            auto& soundsData = processor->getDataReference();

            // Ensure arrays are large enough
            while (currentSounds.size() <= targetPadIndex)
                currentSounds.add(FSSound());
            while (soundsData.size() <= targetPadIndex)
            {
                StringArray emptyData;
                emptyData.add(""); emptyData.add(""); emptyData.add(""); emptyData.add("");
                soundsData.push_back(emptyData);
            }

            // Update arrays
            currentSounds.set(targetPadIndex, sound);

            StringArray soundData;
            soundData.add(bookmark.sampleName);
            soundData.add(bookmark.authorName);
            soundData.add(bookmark.licenseType);
            soundData.add(bookmark.searchQuery);
            soundsData[targetPadIndex] = soundData;

            // Rebuild sampler
            processor->setSources();

            // Show confirmation
            AlertWindow::showMessageBoxAsync(
                AlertWindow::InfoIcon,
                "Sample Loaded",
                "\"" + bookmark.sampleName + "\" loaded into pad " + String(targetPadIndex + 1) + ".",
                "OK"
            );
        }
    }
}

void BookmarkViewerComponent::scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart)
{
    // The viewport handles scrolling automatically
}