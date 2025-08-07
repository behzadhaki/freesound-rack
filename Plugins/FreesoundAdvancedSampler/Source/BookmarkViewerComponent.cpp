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

    // Background using consistent plugin colors
    g.setColour(Colour(0xff2A2A2A)); // textEditorBackgroundColour from CustomLookAndFeel
    g.fillRoundedRectangle(bounds, 4.0f);

    // Border using consistent outline color
    g.setColour(Colour(0xff404040)); // alertOutlineColour from CustomLookAndFeel
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    // Calculate button halves
    float halfWidth = bounds.getWidth() * 0.5f;
    leftBounds = bounds.removeFromLeft(halfWidth);
    rightBounds = bounds;

    // Draw active state background using pluginChoiceColour
    Colour activeColour = pluginChoiceColour.withAlpha(0.8f);

    if (bookmarksActive)
    {
        // Create gradient for active state like in CustomLookAndFeel
        ColourGradient gradient(
            activeColour.brighter(0.1f), leftBounds.getTopLeft(),
            activeColour.darker(0.1f), leftBounds.getBottomRight(), false);
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(leftBounds.reduced(2.0f), 2.0f);
    }
    else
    {
        // Create gradient for active state like in CustomLookAndFeel
        ColourGradient gradient(
            activeColour.brighter(0.1f), rightBounds.getTopLeft(),
            activeColour.darker(0.1f), rightBounds.getBottomRight(), false);
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(rightBounds.reduced(2.0f), 2.0f);
    }

    // Draw text using consistent font and colors
    g.setFont(Font(10.0f));

    // Left text (Bookmarks) - use white for active, lightgrey for inactive
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
    titleLabel.setFont(Font(14.0f, Font::bold));
    titleLabel.setColour(Label::textColourId, Colours::white);
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
    // Dark background consistent with plugin theme
    g.setColour(Colour(0xff1A1A1A).withAlpha(0.9f)); // alertBackgroundColour from CustomLookAndFeel
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
    // Search box using consistent styling from CustomLookAndFeel
    searchBox.setMultiLine(false);
    searchBox.setReturnKeyStartsNewLine(false);
    searchBox.setScrollbarsShown(false);
    searchBox.setPopupMenuEnabled(true);
    searchBox.setFont(Font(11.0f));
    // Use consistent colors from CustomLookAndFeel
    searchBox.setColour(TextEditor::backgroundColourId, Colour(0xff2A2A2A)); // textEditorBackgroundColour
    searchBox.setColour(TextEditor::textColourId, Colours::white); // textEditorTextColour
    searchBox.setColour(TextEditor::highlightColourId, pluginChoiceColour.withAlpha(0.6f));
    searchBox.setColour(TextEditor::outlineColourId, Colour(0xff404040)); // textEditorOutlineColour
    searchBox.setColour(TextEditor::focusedOutlineColourId, Colour(0xff606060)); // textEditorFocusedOutlineColour
    searchBox.setTextToShowWhenEmpty("Search: name, author, tags, description, license, duration, file size, dates...", Colours::grey);

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

    // Clear search button - already using StyledButton from CustomButtonStyle
    clearSearchButton.onClick = [this]() {
        clearSearch();
    };
    addAndMakeVisible(clearSearchButton);

    // Result count label using consistent styling
    resultCountLabel.setFont(Font(10.0f));
    resultCountLabel.setColour(Label::textColourId, Colours::lightgrey); // consistent with CustomLookAndFeel labelTextColour
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

    // Update search placeholder text based on mode
    String placeholderText = bookmarksMode ?
        "Search bookmarks: name, author, tags, license, duration, size..." :
        "Search all samples: name, author, tags, license, duration, size...";
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
    if (searchTerm.trim().isEmpty())
        return true;

    // Split search term into individual words for multi-word search
    StringArray searchWords = StringArray::fromTokens(searchTerm.toLowerCase(), " ", "");
    searchWords.removeEmptyStrings();

    if (searchWords.isEmpty())
        return true;

    // Count how many search words are found
    int matchedWords = 0;

    // Create searchable content from all fields
    StringArray searchableFields;

    // === CORE METADATA FIELDS ===
    searchableFields.add(sample.originalName.toLowerCase());
    searchableFields.add(sample.authorName.toLowerCase());
    searchableFields.add(sample.description.toLowerCase());
    searchableFields.add(sample.tags.toLowerCase());
    searchableFields.add(sample.searchQuery.toLowerCase());

    // === TECHNICAL METADATA ===
    searchableFields.add(sample.licenseType.toLowerCase());
    searchableFields.add(sample.fileName.toLowerCase());
    searchableFields.add(sample.freesoundUrl.toLowerCase());
    searchableFields.add(sample.freesoundId.toLowerCase());

    // === DATE/TIME FIELDS ===
    searchableFields.add(sample.downloadedAt.toLowerCase());
    searchableFields.add(sample.bookmarkedAt.toLowerCase());
    searchableFields.add(sample.lastModifiedAt.toLowerCase());

    // === DERIVED/COMPUTED FIELDS ===

    // Add file extension using string methods instead of File constructor
    String fileExtension;
    int dotIndex = sample.fileName.lastIndexOf(".");
    if (dotIndex >= 0 && dotIndex < sample.fileName.length() - 1)
    {
        fileExtension = sample.fileName.substring(dotIndex + 1).toLowerCase();
        searchableFields.add(fileExtension);
        searchableFields.add("." + fileExtension); // Include version with dot
    }

    // Duration in various formats
    if (sample.duration > 0.0)
    {
        // Raw seconds with decimal
        searchableFields.add(String(sample.duration, 2) + "s");
        searchableFields.add(String(sample.duration, 1) + "s");
        searchableFields.add(String((int)sample.duration) + "s");

        // Formatted duration (e.g., "1m 30s")
        int minutes = (int)(sample.duration / 60);
        int seconds = (int)(sample.duration) % 60;
        if (minutes > 0)
        {
            searchableFields.add(String(minutes) + "m " + String(seconds) + "s");
            searchableFields.add(String(minutes) + "min " + String(seconds) + "sec");
            searchableFields.add(String(minutes) + ":" + String(seconds).paddedLeft('0', 2));
        }
        else
        {
            searchableFields.add(String(seconds) + "sec");
        }

        // Duration in milliseconds (for precise searches)
        int milliseconds = (int)(sample.duration * 1000);
        searchableFields.add(String(milliseconds) + "ms");
    }

    // File size in various formats
    if (sample.fileSize > 0)
    {
        // Bytes
        searchableFields.add(String(sample.fileSize) + "b");
        searchableFields.add(String(sample.fileSize) + "bytes");

        // Kilobytes
        if (sample.fileSize > 1024)
        {
            searchableFields.add(String(sample.fileSize / 1024) + "kb");
            searchableFields.add(String(sample.fileSize / 1024) + "kilobytes");
        }

        // Megabytes
        if (sample.fileSize > 1024 * 1024)
        {
            float mb = sample.fileSize / (1024.0f * 1024.0f);
            searchableFields.add(String(mb, 1) + "mb");
            searchableFields.add(String(mb, 2) + "mb");
            searchableFields.add(String((int)mb) + "mb");
            searchableFields.add(String(mb, 1) + "megabytes");
        }
    }

    // Sample rate (if available in future - placeholder for now)
    // Note: SampleMetadata doesn't currently have sample rate, but adding placeholder
    // searchableFields.add(String(sample.sampleRate) + "hz");
    // searchableFields.add(String(sample.sampleRate / 1000) + "khz");

    // Bit depth (if available in future - placeholder)
    // searchableFields.add(String(sample.bitDepth) + "bit");

    // === SPECIAL SEARCH TERMS AND CATEGORIES ===

    // License categories and short forms (using SamplePad's logic)
    String lowerLicense = sample.licenseType.toLowerCase();

    // Add full license text
    if (lowerLicense.contains("creative"))
        searchableFields.add("creative commons");
    if (lowerLicense.contains("cc"))
        searchableFields.add("cc");
    if (lowerLicense.contains("by"))
        searchableFields.add("attribution");
    if (lowerLicense.contains("sa"))
        searchableFields.add("share-alike");
    if (lowerLicense.contains("nc"))
        searchableFields.add("non-commercial");
    if (lowerLicense.contains("nd"))
        searchableFields.add("no-derivatives");
    if (lowerLicense.contains("public"))
        searchableFields.add("public domain");

    // Add license short forms (based on SamplePad's getLicenseShortName logic)
    String licenseShortForm = getLicenseShortForm(lowerLicense);
    if (licenseShortForm.isNotEmpty())
    {
        searchableFields.add(licenseShortForm.toLowerCase());

        // Also add without version numbers for easier searching
        String baseForm = licenseShortForm.upToFirstOccurrenceOf(" ", false, false);
        if (baseForm != licenseShortForm)
            searchableFields.add(baseForm.toLowerCase());
    }

    // Duration categories
    if (sample.duration < 1.0)
        searchableFields.add("very short");
    if (sample.duration < 5.0)
        searchableFields.add("short");
    else if (sample.duration >= 5.0 && sample.duration < 30.0)
        searchableFields.add("medium");
    else if (sample.duration >= 30.0 && sample.duration < 120.0)
        searchableFields.add("long");
    else if (sample.duration >= 120.0)
        searchableFields.add("very long");

    // File size categories
    if (sample.fileSize > 0)
    {
        if (sample.fileSize < 50 * 1024) // < 50KB
            searchableFields.add("tiny");
        if (sample.fileSize < 200 * 1024) // < 200KB
            searchableFields.add("small");
        else if (sample.fileSize < 1024 * 1024) // < 1MB
            searchableFields.add("medium size");
        else if (sample.fileSize < 5 * 1024 * 1024) // < 5MB
            searchableFields.add("large");
        else
            searchableFields.add("very large");
    }

    // Bookmark status
    if (sample.isBookmarked)
        searchableFields.add("bookmarked");
    else
        searchableFields.add("not bookmarked");

    // Date-based searches (extract year, month from dates)
    auto extractDateTerms = [&searchableFields](const String& dateStr) {
        if (dateStr.isNotEmpty())
        {
            // Try to extract year (look for 4-digit numbers)
            for (int i = 0; i <= dateStr.length() - 4; ++i)
            {
                String yearCandidate = dateStr.substring(i, i + 4);
                if (yearCandidate.containsOnly("0123456789"))
                {
                    int year = yearCandidate.getIntValue();
                    if (year >= 2000 && year <= 2030) // Reasonable range
                    {
                        searchableFields.add(String(year));
                        searchableFields.add("year " + String(year));
                    }
                }
            }

            // Add common date patterns
            if (dateStr.contains("2024"))
                searchableFields.add("this year");
            if (dateStr.contains("jan"))
                searchableFields.add("january");
            if (dateStr.contains("feb"))
                searchableFields.add("february");
            // Add more month mappings as needed...
        }
    };

    extractDateTerms(sample.downloadedAt.toLowerCase());
    extractDateTerms(sample.bookmarkedAt.toLowerCase());

    // === MULTI-WORD SEARCH LOGIC ===

    for (const String& searchWord : searchWords)
    {
        if (searchWord.length() < 2) // Skip very short words
            continue;

        bool wordFound = false;

        // Check if this search word appears in any field
        for (const String& field : searchableFields)
        {
            if (field.contains(searchWord))
            {
                wordFound = true;
                break;
            }
        }

        if (wordFound)
            matchedWords++;
    }

    // Require all words to be found for a match
    // This implements AND logic: "drum kick" requires both "drum" AND "kick" to be present
    return matchedWords == searchWords.size();
}

void BookmarkViewerComponent::updateResultCount()
{
    const Array<SampleMetadata>& sourceData = isBookmarksMode ? currentBookmarks : allLocalSamples;
    String modeText = isBookmarksMode ? "bookmarks" : "local samples";

    String countText = String(filteredSamples.size()) + " of " +
                      String(sourceData.size()) + " " + modeText;

    // Use consistent colors from plugin theme
    if (filteredSamples.size() == 0)
    {
        resultCountLabel.setColour(Label::textColourId, Colour(0xFFFF6B47)); // Warning orange
        countText += " (no matches)";
    }
    else if (filteredSamples.size() == sourceData.size())
    {
        resultCountLabel.setColour(Label::textColourId, Colour(0xFF4CAF50)); // Success green
        countText += " (showing all)";
    }
    else
    {
        resultCountLabel.setColour(Label::textColourId, pluginChoiceColour.brighter(0.3f)); // Plugin theme color
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

String BookmarkViewerComponent::getLicenseShortForm(const String& lowerLicense) const
{
    String licenseVer = "";

    // Extract version number (e.g., "/4.0" -> " 4.0")
    try
    {
        for (int i = 0; i < lowerLicense.length() - 4; ++i)
        {
            if (lowerLicense[i] == '/' &&
                lowerLicense[i + 1] >= '0' && lowerLicense[i + 1] <= '9' &&
                lowerLicense[i + 2] == '.' &&
                lowerLicense[i + 3] >= '0' && lowerLicense[i + 3] <= '9')
            {
                String version = "";
                version += lowerLicense[i + 1];
                version += '.';
                version += lowerLicense[i + 3];

                int j = i + 4;
                while (j < lowerLicense.length() &&
                       lowerLicense[j] >= '0' && lowerLicense[j] <= '9')
                {
                    version += lowerLicense[j];
                    j++;
                }

                licenseVer = " " + version;
                break;
            }
        }
    }
    catch (...)
    {
        licenseVer = "";
    }

    // Determine license short form
    try
    {
        if (lowerLicense.contains("creativecommons.org/publicdomain/zero") ||
            lowerLicense.contains("cc0") ||
            lowerLicense.contains("publicdomain/zero"))
            return "CC0" + licenseVer;
        else if (lowerLicense.contains("creativecommons.org/licenses/by-nc-sa") ||
                 lowerLicense.contains("by-nc-sa"))
            return "BY-NC-SA" + licenseVer;
        else if (lowerLicense.contains("creativecommons.org/licenses/by-nc-nd") ||
                 lowerLicense.contains("by-nc-nd"))
            return "BY-NC-ND" + licenseVer;
        else if (lowerLicense.contains("creativecommons.org/licenses/by-sa") ||
                 lowerLicense.contains("by-sa"))
            return "BY-SA" + licenseVer;
        else if (lowerLicense.contains("creativecommons.org/licenses/by-nd") ||
                 lowerLicense.contains("by-nd"))
            return "BY-ND" + licenseVer;
        else if (lowerLicense.contains("creativecommons.org/licenses/by-nc") ||
                 lowerLicense.contains("by-nc"))
            return "BY-NC" + licenseVer;
        else if (lowerLicense.contains("creativecommons.org/licenses/by/") ||
                 (lowerLicense.contains("creativecommons.org/licenses/by") && !lowerLicense.contains("-nc")))
            return "BY" + licenseVer;
        else if (lowerLicense.contains("sampling+"))
            return "S+" + licenseVer;
        else if (lowerLicense.contains("sampling"))
            return "S" + licenseVer;
        else if (lowerLicense.contains("public domain") || lowerLicense.contains("publicdomain"))
            return "PD";
        else if (lowerLicense.contains("all rights reserved"))
            return "ARR";
        else
            return "???";
    }
    catch (...)
    {
        return "???";
    }
}