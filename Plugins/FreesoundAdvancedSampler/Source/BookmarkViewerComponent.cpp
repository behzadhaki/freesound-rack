/*
  ==============================================================================

    BookmarkViewerComponent.cpp - Updated with Collection Manager Integration
    and Search Functionality

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
    titleLabel.setFont(Font(14.0f, Font::bold));
    titleLabel.setColour(Label::textColourId, Colours::white);
    addAndMakeVisible(titleLabel);

    // Refresh button
    refreshButton.onClick = [this]() {
        refreshBookmarks();
    };
    addAndMakeVisible(refreshButton);

    // Setup search components
    setupSearchComponents();

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
    g.setColour(Colour(0xff1A1A1A).withAlpha(0.9f));
    g.fillAll();
}

void BookmarkViewerComponent::resized()
{
    auto bounds = getLocalBounds().reduced(3);

    int textHeight = 18; // Height for title and refresh button
    int refreshButtonWidth = 17; // Width for refresh button
    int gapHeight = 8; // Gap
    int searchRowHeight = 20; // Height for search components

    // === TOP ROW: Title and Refresh Button ===
    auto topLine = bounds.removeFromTop(textHeight);
    auto titleBounds = topLine;
    titleBounds.removeFromRight(refreshButtonWidth + 5); // Leave space for refresh button
    titleLabel.setBounds(titleBounds);

    auto refreshBounds = Rectangle<int>(bounds.getTopLeft().x + refreshButtonWidth, topLine.getY(),
                                       refreshButtonWidth, textHeight);
    refreshButton.setBounds(refreshBounds);

    bounds.removeFromTop(gapHeight);

    // === SEARCH ROW ===
    auto searchRow = bounds.removeFromTop(searchRowHeight);

    // Clear button (fixed width on right)
    auto clearButtonBounds = searchRow.removeFromRight(20);
    clearSearchButton.setBounds(clearButtonBounds);

    // Small gap
    searchRow.removeFromRight(3);

    // Search box (remaining space)
    searchBox.setBounds(searchRow);

    bounds.removeFromTop(5); // Small gap

    // === RESULT COUNT ROW (only if searching) ===
    if (currentSearchTerm.isNotEmpty())
    {
        auto countRow = bounds.removeFromTop(16);
        resultCountLabel.setBounds(countRow);
        bounds.removeFromTop(3);
    }

    // Update the scrollable area
    updateScrollableArea();

    // === VIEWPORT: Rest of space ===
    bookmarkViewport.setBounds(bounds);
}

void BookmarkViewerComponent::setupSearchComponents()
{
    // Search box
    searchBox.setMultiLine(false);
    searchBox.setReturnKeyStartsNewLine(false);
    searchBox.setScrollbarsShown(false);
    searchBox.setPopupMenuEnabled(true);
    searchBox.setFont(Font(11.0f));
    searchBox.setColour(TextEditor::backgroundColourId, Colour(0xff2A2A2A));
    searchBox.setColour(TextEditor::textColourId, Colours::white);
    searchBox.setColour(TextEditor::highlightColourId, Colour(0xff4A9EFF));
    searchBox.setColour(TextEditor::outlineColourId, Colour(0xff404040));
    searchBox.setColour(TextEditor::focusedOutlineColourId, Colour(0xff404040));
    searchBox.setTextToShowWhenEmpty("Search all fields: tags, description, name, author, license...", Colours::grey);

    // Thread-safe real-time search as user types
    searchBox.onTextChange = [this]() {
        // Use a weak reference to avoid crashes if component is destroyed
        Component::SafePointer<BookmarkViewerComponent> safeThis(this);

        // Debounce the search to avoid excessive updates
        Timer::callAfterDelay(150, [safeThis]() {
            if (safeThis != nullptr)
            {
                safeThis->performSearch();
            }
        });
    };

    // Also search on return key (immediate)
    searchBox.onReturnKey = [this]() {
        performSearch();
    };

    addAndMakeVisible(searchBox);

    // Clear search button
    clearSearchButton.onClick = [this]() {
        clearSearch();
    };
    addAndMakeVisible(clearSearchButton);

    // Result count label
    resultCountLabel.setFont(Font(10.0f));
    resultCountLabel.setColour(Label::textColourId, Colours::lightgrey);
    resultCountLabel.setJustificationType(Justification::centredLeft);
    addAndMakeVisible(resultCountLabel);
    resultCountLabel.setVisible(false); // Hidden initially
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
    // Ensure we're on the message thread
    if (!MessageManager::getInstance()->isThisTheMessageThread())
    {
        MessageManager::callAsync([this]() {
            refreshBookmarks();
        });
        return;
    }

    if (!processor || !processor->getCollectionManager())
        return;

    // Get bookmarked samples directly from collection manager
    currentBookmarks = processor->getCollectionManager()->getBookmarkedSamples();

    // Apply current search filter
    performSearch();
}

void BookmarkViewerComponent::performSearch()
{
    // Ensure we're on the message thread
    if (!MessageManager::getInstance()->isThisTheMessageThread())
    {
        MessageManager::callAsync([this]() {
            performSearch();
        });
        return;
    }

    currentSearchTerm = searchBox.getText().trim();

    // Clear filtered results
    filteredBookmarks.clear();

    if (currentSearchTerm.isEmpty())
    {
        // No search term - show all bookmarks
        filteredBookmarks = currentBookmarks;
        resultCountLabel.setVisible(false);
    }
    else
    {
        // Filter bookmarks based on search term
        for (const auto& bookmark : currentBookmarks)
        {
            if (matchesSearchTerm(bookmark, currentSearchTerm))
            {
                filteredBookmarks.add(bookmark);
            }
        }

        // Show result count
        updateResultCount();
        resultCountLabel.setVisible(true);
    }

    // Update the display with filtered results
    updateBookmarkPads();

    // Trigger layout update
    resized();
}

void BookmarkViewerComponent::clearSearch()
{
    // Ensure we're on the message thread
    if (!MessageManager::getInstance()->isThisTheMessageThread())
    {
        MessageManager::callAsync([this]() {
            clearSearch();
        });
        return;
    }

    searchBox.clear();
    currentSearchTerm = "";
    filteredBookmarks = currentBookmarks;
    resultCountLabel.setVisible(false);

    updateBookmarkPads();
    resized(); // Update layout
}

bool BookmarkViewerComponent::matchesSearchTerm(const SampleMetadata& sample, const String& searchTerm) const
{
    // Convert search term to lowercase for case-insensitive search
    String lowerSearchTerm = searchTerm.toLowerCase();

    // === CORE METADATA FIELDS ===

    // Search in original name
    if (sample.originalName.toLowerCase().contains(lowerSearchTerm))
        return true;

    // Search in author name
    if (sample.authorName.toLowerCase().contains(lowerSearchTerm))
        return true;

    // Search in description
    if (sample.description.toLowerCase().contains(lowerSearchTerm))
        return true;

    // Search in tags
    if (sample.tags.toLowerCase().contains(lowerSearchTerm))
        return true;

    // Search in search query (the original search term used to find this sample)
    if (sample.searchQuery.toLowerCase().contains(lowerSearchTerm))
        return true;

    // === TECHNICAL METADATA ===

    // Search in license type
    if (sample.licenseType.toLowerCase().contains(lowerSearchTerm))
        return true;

    // Search in file name
    if (sample.fileName.toLowerCase().contains(lowerSearchTerm))
        return true;

    // Search in Freesound URL
    if (sample.freesoundUrl.toLowerCase().contains(lowerSearchTerm))
        return true;

    // Search in Freesound ID (exact match or partial)
    if (sample.freesoundId.toLowerCase().contains(lowerSearchTerm))
        return true;

    // === DATE/TIME FIELDS ===

    // Search in download date
    if (sample.downloadedAt.toLowerCase().contains(lowerSearchTerm))
        return true;

    // Search in bookmark date
    if (sample.bookmarkedAt.toLowerCase().contains(lowerSearchTerm))
        return true;

    // === NUMERIC FIELDS (convert to searchable strings) ===

    // Search in duration (convert seconds to searchable format)
    if (sample.duration > 0.0)
    {
        String durationStr = String(sample.duration, 1) + "s"; // e.g., "3.2s"
        if (durationStr.toLowerCase().contains(lowerSearchTerm))
            return true;

        // Also search formatted duration (e.g., "1m 30s")
        int minutes = (int)(sample.duration / 60);
        int seconds = (int)(sample.duration) % 60;
        if (minutes > 0)
        {
            String formattedDuration = String(minutes) + "m " + String(seconds) + "s";
            if (formattedDuration.toLowerCase().contains(lowerSearchTerm))
                return true;
        }
    }

    // Search in file size (convert bytes to searchable format)
    if (sample.fileSize > 0)
    {
        String fileSizeStr;
        if (sample.fileSize > 1024 * 1024)
            fileSizeStr = String(sample.fileSize / (1024 * 1024)) + "mb";
        else if (sample.fileSize > 1024)
            fileSizeStr = String(sample.fileSize / 1024) + "kb";
        else
            fileSizeStr = String(sample.fileSize) + "b";

        if (fileSizeStr.toLowerCase().contains(lowerSearchTerm))
            return true;
    }

    // === SPECIAL SEARCH TERMS ===

    // Search for license categories
    String lowerLicense = sample.licenseType.toLowerCase();
    if (lowerSearchTerm == "cc" && lowerLicense.contains("creative"))
        return true;
    if (lowerSearchTerm == "cc0" && lowerLicense.contains("cc0"))
        return true;
    if (lowerSearchTerm == "by" && lowerLicense.contains("by"))
        return true;
    if (lowerSearchTerm == "sa" && lowerLicense.contains("sa"))
        return true;
    if (lowerSearchTerm == "nc" && lowerLicense.contains("nc"))
        return true;
    if (lowerSearchTerm == "public" && lowerLicense.contains("public"))
        return true;

    // Search for duration ranges
    if (lowerSearchTerm == "short" && sample.duration < 5.0)
        return true;
    if (lowerSearchTerm == "medium" && sample.duration >= 5.0 && sample.duration < 30.0)
        return true;
    if (lowerSearchTerm == "long" && sample.duration >= 30.0)
        return true;

    // Search for file size ranges
    if (lowerSearchTerm == "small" && sample.fileSize < 100 * 1024) // < 100KB
        return true;
    if (lowerSearchTerm == "large" && sample.fileSize > 1024 * 1024) // > 1MB
        return true;

    return false;
}

void BookmarkViewerComponent::updateResultCount()
{
    String countText = String(filteredBookmarks.size()) + " of " +
                      String(currentBookmarks.size()) + " bookmarks";

    if (filteredBookmarks.size() == 0)
    {
        resultCountLabel.setColour(Label::textColourId, Colours::orange);
        countText += " (no matches)";
    }
    else if (filteredBookmarks.size() == currentBookmarks.size())
    {
        resultCountLabel.setColour(Label::textColourId, Colours::lightgreen);
        countText += " (showing all)";
    }
    else
    {
        resultCountLabel.setColour(Label::textColourId, Colours::lightblue);
        countText += " (filtered)";
    }

    resultCountLabel.setText(countText, dontSendNotification);
}

void BookmarkViewerComponent::updateBookmarkPads()
{
    // Ensure we're on the message thread
    if (!MessageManager::getInstance()->isThisTheMessageThread())
    {
        MessageManager::callAsync([this]() {
            updateBookmarkPads();
        });
        return;
    }

    // Clear existing pads
    clearBookmarkPads();

    // Calculate total rows needed (use filtered bookmarks)
    totalRows = filteredBookmarks.size();

    // Create pads for filtered bookmarks
    createBookmarkPads();

    // Update scrollable area
    updateScrollableArea();
}

void BookmarkViewerComponent::createBookmarkPads()
{
    if (filteredBookmarks.isEmpty())
        return;

    const int padWidth = 190;
    const int padHeight = 100;
    const int padSpacing = 8;

    for (int i = 0; i < filteredBookmarks.size(); ++i)
    {
        const SampleMetadata& bookmark = filteredBookmarks[i];

        // Create SamplePad in Preview mode with unique index
        auto* pad = new SamplePad(1000 + i, SamplePad::PadMode::Preview);
        pad->setProcessor(processor);

        // Load the bookmark sample if file exists
        File sampleFile = processor->getCollectionManager()->getSampleFile(bookmark.freesoundId);
        if (sampleFile.existsAsFile())
        {
            pad->setSample(sampleFile, bookmark.originalName, bookmark.authorName,
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

    // Calculate total height needed (based on filtered results)
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
// PreviewPlaybackListener Implementation (Thread-Safe)
//==============================================================================

void BookmarkViewerComponent::previewStarted(const String& freesoundId)
{
    // Ensure UI updates happen on message thread
    MessageManager::callAsync([this, freesoundId]() {
        if (auto* pad = findPadByFreesoundId(freesoundId))
        {
            pad->setPreviewPlaying(true);
        }
    });
}

void BookmarkViewerComponent::previewStopped(const String& freesoundId)
{
    // Ensure UI updates happen on message thread
    MessageManager::callAsync([this, freesoundId]() {
        if (auto* pad = findPadByFreesoundId(freesoundId))
        {
            pad->setPreviewPlaying(false);
        }
    });
}

void BookmarkViewerComponent::previewPlayheadPositionChanged(const String& freesoundId, float position)
{
    // Ensure UI updates happen on message thread
    MessageManager::callAsync([this, freesoundId, position]() {
        if (auto* pad = findPadByFreesoundId(freesoundId))
        {
            pad->setPreviewPlayheadPosition(position);
        }
    });
}

SamplePad* BookmarkViewerComponent::findPadByFreesoundId(const String& freesoundId)
{
    return bookmarkPadMap.count(freesoundId) > 0 ? bookmarkPadMap[freesoundId] : nullptr;
}