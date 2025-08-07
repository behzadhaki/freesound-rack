/*
  ==============================================================================

    BookmarkViewerComponent.cpp - Updated with Collection Manager Integration,
    Search Functionality, and Bookmarks/Local Sounds Toggle

  ==============================================================================
*/

#include "BookmarkViewerComponent.h"
#include "PluginEditor.h"

//==============================================================================
// ToggleButton Implementation
//==============================================================================

BookmarkViewerComponent::ToggleButton::ToggleButton()
{
    setMouseCursor(MouseCursor::PointingHandCursor);
}

void BookmarkViewerComponent::ToggleButton::paint(Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background
    g.setColour(Colour(0xff2A2A2A));
    g.fillRoundedRectangle(bounds, 4.0f);

    // Border
    g.setColour(Colour(0xff404040));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    // Calculate button halves
    float halfWidth = bounds.getWidth() * 0.5f;
    leftBounds = bounds.removeFromLeft(halfWidth);
    rightBounds = bounds;

    // Draw active state background
    if (bookmarksActive)
    {
        g.setColour(Colour(0xff4A9EFF));
        g.fillRoundedRectangle(leftBounds.reduced(2.0f), 2.0f);
    }
    else
    {
        g.setColour(Colour(0xff4A9EFF));
        g.fillRoundedRectangle(rightBounds.reduced(2.0f), 2.0f);
    }

    // Draw text
    g.setFont(Font(10.0f));

    // Left text (Bookmarks)
    g.setColour(bookmarksActive ? Colours::white : Colours::lightgrey);
    g.drawText(leftText, leftBounds, Justification::centred);

    // Right text (Local Sounds)
    g.setColour(bookmarksActive ? Colours::lightgrey : Colours::white);
    g.drawText(rightText, rightBounds, Justification::centred);
}

void BookmarkViewerComponent::ToggleButton::resized()
{
    // Recalculate bounds
    repaint();
}

void BookmarkViewerComponent::ToggleButton::mouseUp(const MouseEvent& event)
{
    auto clickPos = event.getPosition().toFloat();
    bool clickedLeft = clickPos.x < (getWidth() * 0.5f);

    if (clickedLeft != bookmarksActive)
    {
        bookmarksActive = clickedLeft;
        repaint();

        if (onToggleChanged)
            onToggleChanged(bookmarksActive);
    }
}

void BookmarkViewerComponent::ToggleButton::setBookmarksMode(bool shouldBeBookmarks)
{
    if (bookmarksActive != shouldBeBookmarks)
    {
        bookmarksActive = shouldBeBookmarks;
        repaint();
    }
}

//==============================================================================
// BookmarkViewerComponent Implementation
//==============================================================================

BookmarkViewerComponent::BookmarkViewerComponent()
    : processor(nullptr)
{
    // Title label
    titleLabel.setText("Sample Browser", dontSendNotification);
    // titleLabel.setFont(Font(14.0f, Font::bold));
    // titleLabel.setColour(Label::textColourId, Colours::white);
    addAndMakeVisible(titleLabel);

    // Refresh button
    refreshButton.onClick = [this]() {
        if (isBookmarksMode)
            refreshBookmarks();
        else
            refreshAllSamples();
    };
    addAndMakeVisible(refreshButton);

    // Mode toggle
    modeToggle.onToggleChanged = [this](bool bookmarksMode) {
        onModeToggled(bookmarksMode);
    };
    addAndMakeVisible(modeToggle);

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

    clearSamplePads();
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
    int toggleHeight = 24; // Height for toggle button
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

    // === TOGGLE ROW ===
    auto toggleRow = bounds.removeFromTop(toggleHeight);
    int toggleWidth = 140; // Fixed width for toggle
    auto toggleBounds = toggleRow.removeFromLeft(toggleWidth);
    modeToggle.setBounds(toggleBounds);

    bounds.removeFromTop(5); // Small gap

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
    resultCountLabel.setFont(Font(8.0f));
    // resultCountLabel.setColour(Label::textColourId, Colours::lightgrey);
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

        // Initial load of data
        refreshBookmarks();
        refreshAllSamples();
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

    // Apply current search filter if in bookmarks mode
    if (isBookmarksMode)
    {
        performSearch();
    }
}

void BookmarkViewerComponent::refreshAllSamples()
{
    // Ensure we're on the message thread
    if (!MessageManager::getInstance()->isThisTheMessageThread())
    {
        MessageManager::callAsync([this]() {
            refreshAllSamples();
        });
        return;
    }

    if (!processor || !processor->getCollectionManager())
        return;

    // Get all local samples from collection manager
    allLocalSamples = processor->getCollectionManager()->getAllSamples();

    // Apply current search filter if in local sounds mode
    if (!isBookmarksMode)
    {
        performSearch();
    }
}

void BookmarkViewerComponent::onModeToggled(bool bookmarksMode)
{
    isBookmarksMode = bookmarksMode;

    // Update search placeholder text
    String placeholderText = bookmarksMode ?
        "Search bookmarked samples..." :
        "Search all local samples...";
    searchBox.setTextToShowWhenEmpty(placeholderText, Colours::grey);

    // Apply search to new data set
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
    filteredSamples.clear();

    // Choose the source data based on current mode
    const Array<SampleMetadata>& sourceData = isBookmarksMode ? currentBookmarks : allLocalSamples;

    if (currentSearchTerm.isEmpty())
    {
        // No search term - show all samples from current mode
        filteredSamples = sourceData;
        resultCountLabel.setVisible(false);
    }
    else
    {
        // Filter samples based on search term
        for (const auto& sample : sourceData)
        {
            if (matchesSearchTerm(sample, currentSearchTerm))
            {
                filteredSamples.add(sample);
            }
        }

        // Show result count
        updateResultCount();
        resultCountLabel.setVisible(true);
    }

    // Update the display with filtered results
    updateSamplePads();

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

    // Reset to show all samples from current mode
    const Array<SampleMetadata>& sourceData = isBookmarksMode ? currentBookmarks : allLocalSamples;
    filteredSamples = sourceData;
    resultCountLabel.setVisible(false);

    updateSamplePads();
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
    const Array<SampleMetadata>& sourceData = isBookmarksMode ? currentBookmarks : allLocalSamples;
    String modeText = isBookmarksMode ? "bookmarks" : "local samples";

    String countText = String(filteredSamples.size()) + " of " +
                      String(sourceData.size()) + " " + modeText;

    if (filteredSamples.size() == 0)
    {
        countText += " (no matches)";
    }
    else if (filteredSamples.size() == sourceData.size())
    {
        countText += " (showing all)";
    }
    else
    {
        countText += " (filtered)";
    }
    resultCountLabel.setText(countText, dontSendNotification);
}

void BookmarkViewerComponent::updateSamplePads()
{
    // Ensure we're on the message thread
    if (!MessageManager::getInstance()->isThisTheMessageThread())
    {
        MessageManager::callAsync([this]() {
            updateSamplePads();
        });
        return;
    }

    // Clear existing pads
    clearSamplePads();

    // Calculate total rows needed (use filtered samples)
    totalRows = filteredSamples.size();

    // Create pads for filtered samples
    createSamplePads();

    // Update scrollable area
    updateScrollableArea();
}

void BookmarkViewerComponent::createSamplePads()
{
    if (filteredSamples.isEmpty())
        return;

    const int padWidth = 190;
    const int padHeight = 100;
    const int padSpacing = 8;

    for (int i = 0; i < filteredSamples.size(); ++i)
    {
        const SampleMetadata& sample = filteredSamples[i];

        // Create SamplePad in Preview mode with unique index
        auto* pad = new SamplePad(1000 + i, SamplePad::PadMode::Preview);
        pad->setProcessor(processor);

        // Load the sample if file exists
        File sampleFile = processor->getCollectionManager()->getSampleFile(sample.freesoundId);
        if (sampleFile.existsAsFile())
        {
            pad->setSample(sampleFile, sample.originalName, sample.authorName,
                          sample.freesoundId, sample.licenseType, sample.searchQuery,
                          sample.tags, sample.description);
        }

        // Calculate position (single column)
        int x = 0;
        int y = i * (padHeight + padSpacing);

        pad->setBounds(x, y, padWidth, padHeight);

        bookmarkContainer.addAndMakeVisible(pad);
        bookmarkPads.add(pad);
        bookmarkPadMap[sample.freesoundId] = pad; // Map by freesound ID
    }
}

void BookmarkViewerComponent::clearSamplePads()
{
    bookmarkPads.clear();
    bookmarkContainer.removeAllChildren();
    bookmarkPadMap.clear();
}

void BookmarkViewerComponent::updateScrollableArea()
{
    const int padHeight = 100; // Match the padHeight from createSamplePads
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