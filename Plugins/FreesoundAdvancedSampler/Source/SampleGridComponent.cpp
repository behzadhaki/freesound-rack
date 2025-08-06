/*
  ==============================================================================

    SampleGridComponent.cpp
    Created: New grid component for displaying samples in 4x4 grid
    Author: Generated
    Modified: Added drag-and-drop swap functionality and shuffle button

  ==============================================================================
*/

#include "SampleGridComponent.h"
#include "PluginEditor.h"
#include "BadgeSVGs.h"

//==============================================================================
// SamplePad Complete Implementation
//==============================================================================

SamplePad::SamplePad(int index, PadMode mode)
    : padIndex(index)
    , padMode(mode)
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

    // Set default colour for preview mode
    if (padMode == PadMode::Preview)
    {
        padColour = defaultColour.withAlpha(0.2f);
    }

    // Set up query text box based on mode
    queryTextBox.setMultiLine(false);
    queryTextBox.setReturnKeyStartsNewLine(false);
    queryTextBox.setReadOnly(padMode != PadMode::Normal); // Only normal mode is editable
    queryTextBox.setScrollbarsShown(false);
    queryTextBox.setCaretVisible(padMode == PadMode::Normal);
    queryTextBox.setPopupMenuEnabled(padMode == PadMode::Normal);

    // CRITICAL: Set mouse interaction settings to prevent unwanted caret
    queryTextBox.setInterceptsMouseClicks(true, false); // Intercept clicks but not child clicks
    queryTextBox.setClicksOutsideDismissVirtualKeyboard(true);

    // Override the TextEditor's mouse handling to only accept clicks within its bounds
    queryTextBox.setMouseClickGrabsKeyboardFocus(padMode == PadMode::Normal);

    // CONSISTENT styling for all states - dark background with white text
    queryTextBox.setColour(TextEditor::backgroundColourId, Colour(0xff2A2A2A).withAlpha(0.5f));
    queryTextBox.setColour(TextEditor::textColourId, Colours::lightgrey);
    queryTextBox.setColour(TextEditor::highlightColourId, padColour.brighter(0.5f));
    queryTextBox.setColour(TextEditor::highlightedTextColourId, Colour(0xff2A2A2A));
    queryTextBox.setColour(TextEditor::outlineColourId, Colour(0xff404040).withAlpha(0.0f));
    queryTextBox.setColour(TextEditor::focusedOutlineColourId, Colour(0xff606060).withAlpha(0.0f));
    queryTextBox.setFont(Font(11.0f));
    queryTextBox.setJustification(Justification::centredLeft);

    // Only add search functionality if in normal mode
    if (padMode == PadMode::Normal)
    {
        queryTextBox.onReturnKey = [this]() {
            String query = queryTextBox.getText().trim();
            if (query.isNotEmpty() && !connectedToMaster)
            {
                if (auto* gridComponent = findParentComponentOfClass<SampleGridComponent>())
                {
                    gridComponent->searchForSinglePadWithQuery(padIndex, query);
                }
            }
        };
    }

    addAndMakeVisible(queryTextBox);
}

SamplePad::~SamplePad()
{
    stopTimer();
    cleanupProgressComponents();

    // Stop any preview playback if in preview mode
    if (padMode == PadMode::Preview && (isPreviewPlaying || previewRequested))
    {
        stopPreviewPlayback();
    }
}

void SamplePad::paint(Graphics& g)
{
    auto bounds = getLocalBounds();

    // === BACKGROUND WITH MODE-SPECIFIC STYLING ===
    if (isDragHover || isCopyDragHover)
    {
        g.setColour(padColour.withAlpha(0.3f));
    }
    else if (isPlaying)
    {
        g.setColour(padColour.withAlpha(0.6f));
    }
    else if (isDownloading)
    {
        g.setColour(Colours::grey.withAlpha(0.5f));
    }
    else if (connectedToMaster)
    {
        // Special styling for master-connected pads
        g.setGradientFill(ColourGradient(
            pluginChoiceColour.withAlpha(0.2f), bounds.getTopLeft().toFloat(),
            pluginChoiceColour.withAlpha(0.2f), bounds.getBottomRight().toFloat(), false));
    }
    else
    {
        // Mode-specific background colors
        switch (padMode)
        {
            case PadMode::Normal:
                if (hasValidSample)
                    g.setColour(padColour.withAlpha(0.15f));
                else
                    g.setColour(Colour(0x801A1A1A));
                break;

            case PadMode::NonSearchable:
                if (hasValidSample)
                    g.setColour(padColour);
                else
                    g.setColour(Colour(0x801A1A1A));
                break;

            case PadMode::Preview:
                // Preview mode uses the current pad colour (updated by preview state)
                g.setColour(padColour);
                break;
        }
    }

    g.fillRoundedRectangle(bounds.toFloat(), 6.0f);

    // Modern border styling
    if (isDragHover || isCopyDragHover)
    {
        g.setColour(padColour.withAlpha(0.3f));
        g.drawRoundedRectangle(bounds.toFloat().reduced(1), 6.0f, 2.5f);
    }
    else if (isDownloading)
    {
        g.setColour(pluginChoiceColour);
        g.drawRoundedRectangle(bounds.toFloat().reduced(1), 6.0f, 1.0f);
    }
    else if (isPlaying)
    {
        g.setColour(Colours::white.withAlpha(0.0f));
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

    // Initialize and paint badges
    initializeBadges();
    layoutBadges();
    paintBadges(g);

    // === MIDDLE: Waveform and sample text ===
    if (hasValidSample)
    {
        // Adjust waveform bounds to account for top and bottom badges
        auto waveformBounds = bounds.reduced(3);
        waveformBounds.removeFromTop(18); // Space for top line badges
        waveformBounds.removeFromBottom(18); // Space for bottom line

        // Draw waveform with modern styling
        drawWaveform(g, waveformBounds);

        // Draw playhead if playing (main playback)
        if (isPlaying)
        {
            drawPlayhead(g, waveformBounds);
        }

        // Draw preview playhead if in preview mode and preview is playing
        if (padMode == PadMode::Preview && isPreviewPlaying)
        {
            drawPreviewPlayhead(g, waveformBounds);
        }

        // Overlay MIDI/Keyboard info at top-right of waveform (ONLY for normal mode)
        if (padMode == PadMode::Normal)
        {
            g.setColour(padColour);
            g.setFont(Font(11.0f));

            // Create MIDI info display
            int midiNote = padIndex + 36;
            String keyboardKey = getKeyboardKeyForPad(padIndex);
            String displayText = String(CharPointer_UTF8("\xF0\x9F\x8E\xB9")) + String(midiNote) + " | " +
                               String(CharPointer_UTF8("\xE2\x8C\xA8\xEF\xB8\x8F")) + " " + keyboardKey;

            // Position at top-right corner of waveform area
            auto midiInfoBounds = Rectangle<int>(waveformBounds.getRight() - 60, waveformBounds.getY(), 60, 14);
            if (!isPlaying) {
                g.setColour(Colour(0x80000000).withAlpha(0.0f));
                g.fillRect(midiInfoBounds);
            }
            g.setColour(Colours::white);
            g.drawText(displayText, midiInfoBounds, Justification::centred);
        }

        // Draw Freesound ID at bottom-right of waveform
        g.setColour(padColour);
        g.setFont(Font(10.0f));

        String displayText = "FS ID: " + freesoundId;
        if (displayText.length() > 20)
        {
            displayText = displayText.substring(0, 17) + "...";
        }

        // Position at bottom-right of waveform area
        auto idBounds = waveformBounds.removeFromBottom(14);
        idBounds = Rectangle<int>(idBounds.getRight() - 60, idBounds.getY(), 60, 14);

        if (!isPlaying && !isPreviewPlaying) {
            g.setColour(Colour(0x80000000).withAlpha(0.0f));
            g.fillRect(idBounds);
        }
        g.setColour(!isPreviewMode()? padColour : Colours::white);
        g.drawText(displayText, idBounds, Justification::centredRight, true);
    }

    // Empty pad text (only show when not downloading and no sample)
    if (!hasValidSample && !isDownloading)
    {
        g.setColour(Colour(0x80666666));
        g.setFont(Font(10.0f, Font::bold));
        auto emptyBounds = bounds.reduced(8);
        emptyBounds.removeFromTop(18);
        emptyBounds.removeFromBottom(18);
        g.drawText("Empty", emptyBounds, Justification::centred);
    }

    if (hasValidSample) {
        if (isPlaying)
            queryTextBox.setColour(TextEditor::backgroundColourId, padColour.withAlpha(0.0f));
        else
            queryTextBox.setColour(TextEditor::backgroundColourId, padColour.withAlpha(0.1f));
    }
}

void SamplePad::resized()
{
    auto bounds = getLocalBounds();

    // Position query text box or progress components at the bottom
    auto bottomBounds = bounds.reduced(3);
    int bottomHeight = 18;
    bottomBounds = bottomBounds.removeFromBottom(bottomHeight);

    if (isDownloading && progressBar && progressLabel)
    {
        // Show progress bar and label instead of query text box
        auto progressArea = bottomBounds.reduced(2);
        auto labelBounds = progressArea.removeFromBottom(8);
        progressLabel->setBounds(labelBounds);
        progressArea.removeFromLeft(42);
        progressBar->setBounds(progressArea);
    }
    else if (!isDownloading)
    {
        // Calculate badge widths manually (since badges aren't initialized yet)
        int leftBadgesWidth = 0;
        int spacing = 3;

        // Calculate bottom-left badges width (.wav and search when sample exists)
        if (hasValidSample) {
            leftBadgesWidth += 20 + spacing; // .wav
            if (isSearchable()) leftBadgesWidth += 20 + spacing; // search icon
        } else {
            if (isSearchable()) leftBadgesWidth += 20 + spacing; // search icon only
        }

        // Position query text box in the remaining space between badges
        auto textBoxBounds = bottomBounds;
        if (leftBadgesWidth > 0) {
            textBoxBounds.removeFromLeft(leftBadgesWidth);
        }
        queryTextBox.setJustification(Justification::topLeft);
        queryTextBox.setBounds(textBoxBounds.reduced(2));
    }
}

void SamplePad::mouseDown(const MouseEvent& event)
{
    // Don't allow interaction while downloading
    if (isDownloading)
        return;

    // Store the current keyboard focus component before we do anything
    Component* currentKeyboardFocus = Component::getCurrentlyFocusedComponent();

    // Check if the click is specifically on the query text box area
    if (queryTextBox.getBounds().contains(event.getPosition()) && padMode == PadMode::Normal)
    {
        // Allow the TextEditor to handle this click naturally
        queryTextBox.grabKeyboardFocus();
        return;
    }

    // For all other clicks, we need to be careful about keyboard focus
    // We want to remove focus from TextEditor but preserve main editor focus
    if (queryTextBox.hasKeyboardFocus(true))
    {
        queryTextBox.giveAwayKeyboardFocus();

        // If the main editor (or plugin editor) had focus before, restore it
        if (currentKeyboardFocus && currentKeyboardFocus != &queryTextBox)
        {
            currentKeyboardFocus->grabKeyboardFocus();
        }
        else
        {
            // Find and focus the main plugin editor for keyboard sample triggering
            if (auto* pluginEditor = findParentComponentOfClass<FreesoundAdvancedSamplerAudioProcessorEditor>())
            {
                pluginEditor->grabKeyboardFocus();
            }
            else if (auto* gridComponent = findParentComponentOfClass<SampleGridComponent>())
            {
                // Alternatively, focus the grid component if it handles keyboard
                gridComponent->grabKeyboardFocus();
            }
        }
    }

    // Reset preview state flags
    if (padMode == PadMode::Preview)
    {
        mouseDownInWaveform = false;
        previewRequested = false;
    }

    // Check if clicked on any badge
    Badge* clickedBadge = findBadgeAtPosition(event.getPosition());
    if (clickedBadge && clickedBadge->onClick)
    {
        clickedBadge->onClick(event);  // Pass the event
        return;
    }

    // If no valid sample and didn't click a badge, return
    if (!hasValidSample)
        return;

    // Check if clicked in waveform area
    auto bounds = getLocalBounds();
    auto waveformBounds = bounds.reduced(8);
    waveformBounds.removeFromTop(18);
    waveformBounds.removeFromBottom(18);

    if (waveformBounds.contains(event.getPosition()))
    {
        if (padMode == PadMode::Preview)
        {
            // Preview mode: Start preview playback with pointing hand cursor
            mouseDownInWaveform = true;
            startPreviewPlayback();
        }
        else if (padMode == PadMode::Normal || padMode == PadMode::NonSearchable)
        {
            // Normal/NonSearchable mode: Trigger sample playback with play cursor
            setMouseCursor(MouseCursor::PointingHandCursor);

            if (processor)
            {
                int noteNumber = padIndex + 36;
                processor->addNoteOnToMidiBuffer(noteNumber);
            }
        }
        return;
    }

    // Edge area interactions - different cursors for different modes
    if (padMode == PadMode::Normal)
    {
        setMouseCursor(MouseCursor::DraggingHandCursor);
    }
    else if (padMode == PadMode::NonSearchable)
    {
        setMouseCursor(MouseCursor::CopyingCursor);
    }
}

void SamplePad::mouseUp(const MouseEvent& event)
{
    // Preview mode: Always stop preview when mouse is released
    if (padMode == PadMode::Preview && (previewRequested || isPreviewPlaying))
    {
        stopPreviewPlayback();
        mouseDownInWaveform = false;
        previewRequested = false;
    }

    // Normal/NonSearchable modes: Stop playback when mouse is released
    if (padMode != PadMode::Preview && isPlaying && processor)
    {
        int noteNumber = padIndex + 36;
        processor->addNoteOffToMidiBuffer(noteNumber);
        setIsPlaying(false);
        processorSampleRate = processor->getSampleRate();
    }

    // Reset cursor
    setMouseCursor(MouseCursor::NormalCursor);
}

void SamplePad::mouseDrag(const MouseEvent& event)
{
    if (!hasValidSample)
        return;

    // Preview mode: Handle preview drag behavior
    if (padMode == PadMode::Preview)
    {
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
            // Handle potential sample copying drag from edge areas
            Badge* draggedBadge = findBadgeAtPosition(event.getMouseDownPosition());
            if (draggedBadge && draggedBadge->onDrag)
            {
                draggedBadge->onDrag(event);
                return;
            }

            if (event.getDistanceFromDragStart() > 10)
            {
                performEnhancedDragDrop();
            }
        }
        return;
    }

    // Normal/NonSearchable modes: Handle badge dragging and pad swapping
    Badge* draggedBadge = findBadgeAtPosition(event.getMouseDownPosition());
    if (draggedBadge && draggedBadge->onDrag)
    {
        draggedBadge->onDrag(event);
        return;
    }

    // Check if drag started in waveform area - no dragging allowed here
    auto bounds = getLocalBounds();
    auto waveformBounds = bounds.reduced(8);
    waveformBounds.removeFromTop(18);
    waveformBounds.removeFromBottom(18);

    if (waveformBounds.contains(event.getMouseDownPosition()))
    {
        return; // No dragging from waveform area
    }

    // Edge area dragging for pad swapping (Normal mode only)
    if (padMode == PadMode::Normal && event.getDistanceFromDragStart() > 10)
    {
        juce::DynamicObject::Ptr dragObject = new juce::DynamicObject();
        dragObject->setProperty("type", "samplePad");
        dragObject->setProperty("sourcePadIndex", padIndex);

        var dragData(dragObject.get());

        if (auto* gridComponent = findParentComponentOfClass<SampleGridComponent>())
        {
            gridComponent->startDragging(dragData, this);
        }
    }
}

void SamplePad::mouseDoubleClick(const MouseEvent& event)
{
    if (!hasValidSample)
        return;

    // Check if double-clicked on any badge
    Badge* clickedBadge = findBadgeAtPosition(event.getPosition());
    if (clickedBadge && clickedBadge->onDoubleClick)
    {
        clickedBadge->onDoubleClick();
        return;
    }
}

void SamplePad::mouseEnter(const MouseEvent& event)
{
    Component::mouseEnter(event);
}

void SamplePad::mouseExit(const MouseEvent& event)
{
    // Preview mode: Stop preview if mouse exits while playing
    if (padMode == PadMode::Preview && (previewRequested || isPreviewPlaying))
    {
        stopPreviewPlayback();
        mouseDownInWaveform = false;
        previewRequested = false;
    }

    Component::mouseExit(event);
}

void SamplePad::timerCallback()
{
    if (pendingCleanup)
    {
        stopTimer();
        cleanupProgressComponents();
    }
}

// DragAndDropTarget implementation
bool SamplePad::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
    // Only accept drags in Normal mode
    if (padMode != PadMode::Normal)
        return false;

    // Accept pad swaps
    if (dragSourceDetails.description.hasProperty("type") &&
        dragSourceDetails.description.getProperty("type", "") == "samplePad")
    {
        // Don't allow dropping on self
        int sourcePadIndex = dragSourceDetails.description.getProperty("sourcePadIndex", -1);
        return sourcePadIndex != padIndex;
    }

    // Accept enhanced sampler data (copy operations)
    if (dragSourceDetails.description.hasProperty("mime_type") &&
        dragSourceDetails.description.getProperty("mime_type", "") == FREESOUND_SAMPLER_MIME_TYPE)
    {
        return true;
    }

    return false;
}

void SamplePad::itemDragEnter(const SourceDetails& dragSourceDetails)
{
    if (dragSourceDetails.description.hasProperty("type") &&
        dragSourceDetails.description.getProperty("type", "") == "samplePad")
    {
        isDragHover = true;
    }
    else if (dragSourceDetails.description.hasProperty("mime_type") &&
             dragSourceDetails.description.getProperty("mime_type", "") == FREESOUND_SAMPLER_MIME_TYPE)
    {
        isCopyDragHover = true;
    }
    repaint();
}

void SamplePad::itemDragExit(const SourceDetails& dragSourceDetails)
{
    isDragHover = false;
    isCopyDragHover = false;
    repaint();
}

void SamplePad::itemDropped(const SourceDetails& dragSourceDetails)
{
    isDragHover = false;
    isCopyDragHover = false;
    repaint();

    // Handle pad swaps
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
    // Handle enhanced sampler data drops (copy operations)
    else if (dragSourceDetails.description.hasProperty("mime_type") &&
             dragSourceDetails.description.getProperty("mime_type", "") == FREESOUND_SAMPLER_MIME_TYPE)
    {
        if (auto* gridComponent = findParentComponentOfClass<SampleGridComponent>())
        {
            String jsonMetadata = dragSourceDetails.description.getProperty("metadata", "");
            var filesVar = dragSourceDetails.description.getProperty("files", var());

            StringArray filePaths;
            if (filesVar.isArray())
            {
                Array<var>* filesArray = filesVar.getArray();
                for (const auto& fileVar : *filesArray)
                {
                    filePaths.add(fileVar.toString());
                }
            }
            else if (filesVar.isString())
            {
                filePaths.add(filesVar.toString());
            }

            gridComponent->handleEnhancedDrop(jsonMetadata, filePaths, padIndex);
        }
    }
}

void SamplePad::setSample(const File& audioFile, const String& name, const String& author,
                         String fsId, String license, String query, String fsTags, String fsDescription)
{
    sampleName = name;
    authorName = author;
    freesoundId = fsId;
    licenseType = license;
    padQuery = query;
    tags = fsTags;
    description = fsDescription;

    // Update query text box with the sample's query (but don't override if connected to master)
    if (!connectedToMaster)
    {
        queryTextBox.setText(query, dontSendNotification);
    }

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

    resized();
    repaint();
}

void SamplePad::setPlayheadPosition(float position)
{
    // Adjust position using fileSourceSampleRate and processorSampleRate
    position = (position * fileSourceSampleRate) / processorSampleRate;

    MessageManager::callAsync([this, position]()
    {
        playheadPosition = jlimit(0.0f, 1.0f, position);
        repaint();
    });
}

void SamplePad::setIsPlaying(bool playing)
{
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

void SamplePad::setQuery(const String& query, bool dontUpdatePadInfoQuery)
{
    if (!dontUpdatePadInfoQuery)
        padQuery = query;

    queryTextBox.setText(query, dontSendNotification);
}

String SamplePad::getQuery() const
{
    if (connectedToMaster)
    {
        return queryTextBox.getText().trim();
    }
    else
    {
        return queryTextBox.getText().trim();
    }
}

bool SamplePad::hasQuery() const
{
    return queryTextBox.getText().trim().isNotEmpty();
}

void SamplePad::setConnectedToMaster(bool connected)
{
    if (connectedToMaster != connected)
    {
        connectedToMaster = connected;

        // Only affect normal mode pads
        if (padMode == PadMode::Normal)
        {
            if (connectedToMaster)
            {
                queryTextBox.setReadOnly(true);
                queryTextBox.setColour(TextEditor::backgroundColourId, Colour(0xff1A1A1A).withAlpha(0.0f));
            }
            else
            {
                String queryToRestore;
                if (hasValidSample)
                {
                    queryToRestore = padQuery;
                }
                else
                {
                    queryToRestore = queryTextBox.getText().trim();
                }

                queryTextBox.setText(queryToRestore, dontSendNotification);
                padQuery = queryToRestore;
                queryTextBox.setReadOnly(false);
                queryTextBox.setColour(TextEditor::backgroundColourId, Colour(0xff2A2A2A).withAlpha(0.0f));
            }
        }

        repaint();
    }
}

void SamplePad::syncMasterQuery(const String& masterQuery)
{
    if (connectedToMaster && padMode == PadMode::Normal)
    {
        queryTextBox.setText(masterQuery, dontSendNotification);
    }
}

SamplePad::SampleInfo SamplePad::getSampleInfo() const
{
    SampleInfo info;
    info.audioFile = audioFile;
    info.sampleName = sampleName;
    info.authorName = authorName;
    info.freesoundId = freesoundId;
    info.licenseType = licenseType;
    info.query = getQuery();
    info.tags = tags;
    info.description = description;
    info.hasValidSample = hasValidSample;
    info.padIndex = padIndex;
    info.fileSourceSampleRate = fileSourceSampleRate;
    return info;
}

void SamplePad::clearSample()
{
    audioFile = File();
    sampleName = "";
    authorName = "";
    freesoundId = String();
    licenseType = String();
    padQuery = String();
    tags = "";
    description = "";
    hasValidSample = false;
    isPlaying = false;
    playheadPosition = 0.0f;
    audioThumbnail.clear();

    // Reset preview state if in preview mode
    if (padMode == PadMode::Preview)
    {
        isPreviewPlaying = false;
        previewPlayheadPosition = 0.0f;
        previewRequested = false;
        mouseDownInWaveform = false;
        padColour = defaultColour.withAlpha(0.2f);
    }

    // Only clear text box if not connected to master
    if (!connectedToMaster)
    {
        queryTextBox.setText("", dontSendNotification);
    }

    resized();
    repaint();
}

// Preview mode methods
void SamplePad::setPreviewPlaying(bool playing)
{
    if (padMode != PadMode::Preview)
        return;

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
            previewPlayheadPosition = 0.0f;
        }

        repaint();
    });
}

void SamplePad::setPreviewPlayheadPosition(float position)
{
    if (padMode != PadMode::Preview)
        return;

    // Adjust position using fileSourceSampleRate and processorSampleRate
    position = (position * fileSourceSampleRate) / processorSampleRate;

    MessageManager::callAsync([this, position]()
    {
        previewPlayheadPosition = jlimit(0.0f, 1.0f, position);
        repaint();
    });
}

void SamplePad::startPreviewPlayback()
{
    if (padMode != PadMode::Preview || !hasValidSample || !audioFile.existsAsFile() || !processor)
        return;

    if (previewRequested)
        return;

    previewRequested = true;

    // Load the sample into the preview sampler
    processor->loadPreviewSample(audioFile, freesoundId);

    // Small delay to ensure sample is loaded before playing
    Timer::callAfterDelay(50, [this]() {
        if (previewRequested && processor)
        {
            processor->playPreviewSample();
            setMouseCursor(MouseCursor::PointingHandCursor);
        }
    });
}

void SamplePad::stopPreviewPlayback()
{
    if (padMode != PadMode::Preview)
        return;

    if (!previewRequested && !isPreviewPlaying)
        return;

    if (processor)
    {
        processor->stopPreviewSample();
    }

    previewRequested = false;
    setMouseCursor(MouseCursor::NormalCursor);
}

// Progress bar methods
void SamplePad::startDownloadProgress()
{
    stopTimer();

    isDownloading = true;
    pendingCleanup = false;
    currentDownloadProgress = 0.0;

    if (!progressBar)
    {
        progressBar = std::make_unique<ProgressBar>(currentDownloadProgress);
    }

    if (!progressLabel)
    {
        progressLabel = std::make_unique<Label>();
        progressLabel->setFont(Font(8.0f, Font::bold));
        progressLabel->setColour(Label::textColourId, Colours::white);
        progressLabel->setJustificationType(Justification::centred);
    }

    if (!progressBar->getParentComponent())
        addAndMakeVisible(*progressBar);
    if (!progressLabel->getParentComponent())
        addAndMakeVisible(*progressLabel);

    queryTextBox.setVisible(true);
    progressBar->setVisible(true);
    progressLabel->setVisible(true);

    progressLabel->setText("", dontSendNotification);
    resized();
    repaint();
}

void SamplePad::updateDownloadProgress(double progress, const String& status)
{
    if (!isDownloading || !progressBar || !progressLabel)
        return;

    Component::SafePointer<SamplePad> safeThis(this);

    MessageManager::callAsync([safeThis, progress, status]()
    {
        if (safeThis == nullptr || !safeThis->isDownloading)
            return;

        safeThis->currentDownloadProgress = jlimit(0.0, 1.0, progress);

        // String progressText = status.isEmpty() ?
        //     "Downloading... " + String((int)(progress * 100)) + "%" :
        //     status;
        String progressText = "";

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
                // safeThis->progressLabel->setText("Complete!", dontSendNotification);
                safeThis->progressLabel->setText("", dontSendNotification);
            safeThis->currentDownloadProgress = 1.0;
            if (safeThis->progressBar)
                safeThis->progressBar->repaint();

            safeThis->pendingCleanup = true;
            safeThis->startTimer(1000);
        }
        else
        {
            if (safeThis->progressLabel)
            {
                safeThis->progressLabel->setText(message.isEmpty() ? "Failed!" : message, dontSendNotification);
                safeThis->progressLabel->setColour(Label::textColourId, Colours::red);
            }

            safeThis->pendingCleanup = true;
            safeThis->startTimer(2000);
        }
    });
}

void SamplePad::cleanupProgressComponents()
{
    isDownloading = false;
    pendingCleanup = false;

    if (progressBar)
    {
        progressBar->setVisible(false);
        removeChildComponent(progressBar.get());
    }

    if (progressLabel)
    {
        progressLabel->setVisible(false);
        progressLabel->setColour(Label::textColourId, Colours::white);
        removeChildComponent(progressLabel.get());
    }

    queryTextBox.setVisible(true);

    progressBar.reset();
    progressLabel.reset();

    resized();
    repaint();
}

void SamplePad::drawSvgBadge(Graphics& g, const Badge& badge, SvgFitMode fitMode, int reducedBounds)
{
    if (!badge.usesSvg() || !badge.svgDrawable) return;

    auto bounds = badge.bounds.toFloat();

    switch (fitMode) {
        case SvgFitMode::FillEntireBadge:
            // No padding - SVG fills the entire badge
                badge.svgDrawable->drawWithin(g, bounds.reduced(reducedBounds),
                    RectanglePlacement::centred | RectanglePlacement::fillDestination, 0.6f);
        break;

        case SvgFitMode::FillWithPadding:
            // Small padding to avoid touching edges
                bounds = bounds.reduced(1.0f);
        badge.svgDrawable->drawWithin(g, bounds.reduced(reducedBounds),
            RectanglePlacement::centred | RectanglePlacement::fillDestination, 0.6f);
        break;

        case SvgFitMode::FitProportionally:
            // Maintains aspect ratio, may not fill entire badge
                bounds = bounds.reduced(2.0f);
        badge.svgDrawable->drawWithin(g, bounds.reduced(reducedBounds),
            RectanglePlacement::centred | RectanglePlacement::onlyReduceInSize, 0.6f);
        break;

        case SvgFitMode::Stretch:
            // Stretches to exact size (may distort the SVG)
                badge.svgDrawable->drawWithin(g, bounds.reduced(reducedBounds),
                    RectanglePlacement::stretchToFit, 0.6f);
        break;
    }
}

void SamplePad::initializeBadges()
{
    // Clear existing badges
    topLeftBadges.clear();
    topRightBadges.clear();
    bottomLeftBadges.clear();

    // Top-left badges (only for pads with valid samples)
    if (hasValidSample)
    {
        // Copy/Drag badge - use emplace_back with move
        {
            Badge copyBadge("copy", duplicateSVG2, Colour(0x80404040).withAlpha(0.0f), true);
            copyBadge.width = 16;
            copyBadge.fontSize = 13.0f;
            copyBadge.onClick = [this](const MouseEvent&) { handleCopyClick(); };
            copyBadge.onDrag = [this](const MouseEvent& e) {
                if (e.getDistanceFromDragStart() > 10) performEnhancedDragDrop();
            };
            topLeftBadges.emplace_back(std::move(copyBadge)); // Use emplace_back with move
        }

        // Bookmark badge - use emplace_back with move
        {
            bool isCurrentlyBookmarked = processor && processor->getCollectionManager()
                                        ? processor->getCollectionManager()->isBookmarked(freesoundId)
                                        : false;

            Badge bookmarkBadge("bookmark", String(CharPointer_UTF8("\xE2\x98\x85")),
                               isCurrentlyBookmarked ? Colours::goldenrod : Colour(0x80808080).withAlpha(0.0f));
            bookmarkBadge.width = 16;
            bookmarkBadge.onClick = [this](const MouseEvent&) { handleBookmarkClick(); };
            topLeftBadges.emplace_back(std::move(bookmarkBadge));
        }
    }

    // Top-right badges
    if (hasValidSample)
    {
        // Delete badge - only for Normal mode
        if (padMode == PadMode::Normal)
        {

            // Badge delBadge("delete", String(CharPointer_UTF8("\xE2\x9C\x95")), Colour(0x80C62828).withAlpha(0.0f));
            Badge delBadge("delete", deleteSVG, Colour(0x80404040).withAlpha(0.0f), true);
            delBadge.textColour = Colours::mediumvioletred.withAlpha(0.8f);
            delBadge.onClick = [this](const MouseEvent&) { handleDeleteClick(); };
            delBadge.fontSize = 14.0f;
            delBadge.width = 16;
            delBadge.isBold = true;
            topRightBadges.emplace_back(std::move(delBadge)); // Use emplace_back with move
        }

        // Web badge - available in all modes if freesoundId exists
        if (freesoundId.isNotEmpty())
        {
            Badge webBadge("web", String(CharPointer_UTF8("\xF0\x9F\x94\x97")), Colour(0x800277BD).withAlpha(0.0f));
            webBadge.width = 20;
            webBadge.fontSize = 12.0f;
            webBadge.onDoubleClick = [this]() {
                URL("https://freesound.org/s/" + freesoundId + "/").launchInDefaultBrowser();
            };
            webBadge.onClick = [this](const MouseEvent& e) {
                if (e.mods.isShiftDown()) {
                    URL("https://freesound.org/s/" + freesoundId + "/").launchInDefaultBrowser();
                } else {
                    showDetailedSampleInfo();
                }
            };
            topRightBadges.emplace_back(std::move(webBadge)); // Use emplace_back with move
        }

        // License badge - available in all modes if license exists
        if (licenseType.isNotEmpty())
        {
            Badge licenseBadge("license", getLicenseShortName(licenseType), Colour(0x80EF6C00).withAlpha(0.0f));
            int numChars = getLicenseShortName(licenseType).length();
            licenseBadge.width = jmax(16, int(numChars * 5.0f));
            licenseBadge.fontSize = 9.0f;
            licenseBadge.isBold = false;
            licenseBadge.onDoubleClick = [this]() { URL(licenseType).launchInDefaultBrowser(); };
            topRightBadges.emplace_back(std::move(licenseBadge)); // Use emplace_back with move
        }
    }

    // Bottom-left badges
    if (hasValidSample)
    {
        // WAV export badge using SVG - use emplace_back with move
        {
            Badge wavBadge("wav", wavCircleSVG, Colour(0x80404040).withAlpha(0.0f), true);
            wavBadge.width = 16;
            wavBadge.fontSize = 16.0f;
            wavBadge.onClick = [this](const MouseEvent&) { handleWavCopyClick(); };
            wavBadge.onDrag = [this](const MouseEvent& e) {
                if (e.getDistanceFromDragStart() > 10) {
                    File wavFile = getWavFile();
                    if (!wavFile.existsAsFile()) {
                        if (!convertOggToWav(audioFile, wavFile)) return;
                    }
                    if (wavFile.existsAsFile()) {
                        StringArray filePaths;
                        filePaths.add(wavFile.getFullPathName());
                        performExternalDragDropOfFiles(filePaths, false);
                    }
                }
            };
            bottomLeftBadges.emplace_back(std::move(wavBadge)); // Use emplace_back with move
        }

        // Search badge - only for Normal mode
        if (padMode == PadMode::Normal)
        {
            Badge searchBadge("search", String(CharPointer_UTF8("\xF0\x9F\x94\x8D")), Colours::grey.withAlpha(0.0f));
            searchBadge.width = 16;
            searchBadge.fontSize = 16.0f;
            searchBadge.onClick = [this](const MouseEvent&) {
                String searchQuery = queryTextBox.getText().trim();
                if (searchQuery.isEmpty() && processor) {
                    searchQuery = processor->getQuery();
                }
                if (searchQuery.isNotEmpty()) {
                    queryTextBox.setText(searchQuery);
                    if (auto* gridComponent = findParentComponentOfClass<SampleGridComponent>()) {
                        gridComponent->searchForSinglePadWithQuery(padIndex, searchQuery);
                    }
                } else {
                    AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
                        "Empty Query", "Please enter a search term in the pad's text box or the main search box.");
                }
            };
            bottomLeftBadges.emplace_back(std::move(searchBadge)); // Use emplace_back with move
        }
    }

    // Empty pad search badge - only for Normal mode when no sample
    if (!hasValidSample && padMode == PadMode::Normal)
    {
        Badge searchBadge("search", String(CharPointer_UTF8("\xF0\x9F\x94\x8D")), Colours::grey.withAlpha(0.0f));
        searchBadge.width = 16;
        searchBadge.fontSize = 16.0f;
        searchBadge.onClick = [this](const MouseEvent&) {
            String searchQuery = queryTextBox.getText().trim();
            if (searchQuery.isEmpty() && processor) {
                searchQuery = processor->getQuery();
            }
            if (searchQuery.isNotEmpty()) {
                queryTextBox.setText(searchQuery);
                if (auto* gridComponent = findParentComponentOfClass<SampleGridComponent>()) {
                    gridComponent->searchForSinglePadWithQuery(padIndex, searchQuery);
                }
            } else {
                AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
                    "Empty Query", "Please enter a search term in the pad's text box or the main search box.");
            }
        };
        bottomLeftBadges.emplace_back(std::move(searchBadge)); // Use emplace_back with move
    }
}

void SamplePad::showDetailedSampleInfo()
{
    if (!hasValidSample) return;

    String infoText;
    infoText += "Name: " + sampleName + "\n\n";
    infoText += "Author: " + authorName + "\n\n";

    if (freesoundId.isNotEmpty()) {
        infoText += "Freesound ID: " + freesoundId + "\n";
        infoText += "URL: https://freesound.org/s/" + freesoundId + "/\n\n";
    }

    if (licenseType.isNotEmpty()) {
        infoText += "License: " + licenseType + "\n\n";
    }

    if (tags.isNotEmpty()) {
        infoText += "Tags: " + tags + "\n\n";
    }

    if (description.isNotEmpty()) {
        infoText += "Description: " + description + "\n\n";
    }

    if (audioFile.existsAsFile()) {
        infoText += "File: " + audioFile.getFileName() + "\n";
        infoText += "Size: " + File::descriptionOfSizeInBytes(audioFile.getSize()) + "\n";
    }

    AlertWindow::showMessageBoxAsync(
        AlertWindow::InfoIcon,
        "Sample Information",
        infoText,
        "OK",
        nullptr,
        ModalCallbackFunction::create([this](int) {
            // Optional callback after window closes
        })
    );
}

void SamplePad::layoutBadges()
{
    auto bounds = getLocalBounds().reduced(4);
    int spacing = 3;
    int badgeHeight = 14;

    // Layout top-left badges
    auto topLeftArea = bounds.removeFromTop(badgeHeight);
    int x = topLeftArea.getX();
    for (auto& badge : topLeftBadges) {
        if (badge.visible) {
            badge.bounds = Rectangle<int>(x, topLeftArea.getY(), badge.width, badgeHeight);
            x += badge.width + spacing;
        }
    }

    // Layout top-right badges (right to left)
    auto topRightArea = bounds.removeFromTop(0);
    topRightArea = getLocalBounds().reduced(4).removeFromTop(badgeHeight);
    x = topRightArea.getRight();
    for (auto& badge : topRightBadges) {
        if (badge.visible) {
            x -= badge.width;
            badge.bounds = Rectangle<int>(x, topRightArea.getY(), badge.width, badgeHeight);
            x -= spacing;
        }
    }

    // Layout bottom-left badges
    auto bottomArea = getLocalBounds().reduced(4);
    auto bottomLeftArea = bottomArea.removeFromBottom(badgeHeight);
    x = bottomLeftArea.getX();
    for (auto& badge : bottomLeftBadges) {
        if (badge.visible) {
            badge.bounds = Rectangle<int>(x, bottomLeftArea.getY(), badge.width, badgeHeight);
            x += badge.width + spacing;
        }
    }
}

void SamplePad::paintBadges(Graphics& g)
{
    auto paintBadgeGroup = [this, &g](const std::vector<Badge>& badges) {
        for (const auto& badge : badges) {
            if (!badge.visible) continue;

            // Draw background
            g.setGradientFill(ColourGradient(
                badge.backgroundColour, badge.bounds.getTopLeft().toFloat(),
                badge.backgroundColour.darker(0.2f), badge.bounds.getBottomRight().toFloat(), false));
            g.fillRoundedRectangle(badge.bounds.toFloat(), 3.0f);

            // Draw content - SVG or text/icon
            if (badge.usesSvg() && badge.svgDrawable) {
                // Use the custom SVG drawing method with recommended settings
                drawSvgBadge(g, badge, SvgFitMode::FillWithPadding, (badge.id == "delete") ? 1 : 0);
            } else {
                // Draw text/icon (existing behavior)
                g.setColour(badge.textColour);
                g.setFont(badge.isBold ? Font(badge.fontSize, Font::bold) : Font(badge.fontSize));
                g.drawText(badge.icon.isNotEmpty() ? badge.icon : badge.text,
                          badge.bounds, Justification::centred);
            }
        }
    };

    paintBadgeGroup(topLeftBadges);
    paintBadgeGroup(topRightBadges);
    paintBadgeGroup(bottomLeftBadges);
}

Badge* SamplePad::findBadgeAtPosition(Point<int> position)
{
    auto checkBadgeGroup = [&position](std::vector<Badge>& badges) -> Badge* {
        for (auto& badge : badges) {
            if (badge.visible && badge.bounds.contains(position)) {
                return &badge;
            }
        }
        return nullptr;
    };

    if (auto* badge = checkBadgeGroup(topLeftBadges)) return badge;
    if (auto* badge = checkBadgeGroup(topRightBadges)) return badge;
    if (auto* badge = checkBadgeGroup(bottomLeftBadges)) return badge;

    return nullptr;
}

// Waveform methods
void SamplePad::loadWaveform()
{
    if (audioFile.existsAsFile())
    {
        audioThumbnail.clear();

        auto* fileSource = new FileInputSource(audioFile);
        audioThumbnail.setSource(fileSource);

        // Get sample rate of file source
        {
            juce::AudioFormatManager formatManager;
            formatManager.registerBasicFormats();

            std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(audioFile));

            if (reader != nullptr)
            {
                fileSourceSampleRate = reader->sampleRate;
            }
        }

        fileSourceSampleRate = (fileSourceSampleRate <= 1.0) ? 44100.0 : fileSourceSampleRate;

        Timer::callAfterDelay(100, [this]() {
            repaint();
        });
    }
}

void SamplePad::drawWaveform(Graphics& g, Rectangle<int> bounds)
{
    if (!hasValidSample || audioThumbnail.getTotalLength() == 0.0)
        return;

    g.setColour(Colours::black.withAlpha(0.0f));
    g.fillRect(bounds);

    if (isPreviewMode()) {
        g.setColour(Colours::white.withAlpha(0.5f));
    } else {
        g.setColour(!isPlaying? padColour.brighter(0.1f) :padColour.brighter(0.3f));
    }
    audioThumbnail.drawChannels(g, bounds, 0.0, audioThumbnail.getTotalLength(), 0.5f);
}

void SamplePad::drawPlayhead(Graphics& g, Rectangle<int> bounds)
{
    if (!isPlaying || !hasValidSample)
        return;

    float x = bounds.getX() + (playheadPosition * bounds.getWidth());

    g.setColour(padColour.withAlpha(0.8f)); // Use pad colour with alpha for playhead
    g.drawLine(x, bounds.getY(), x, bounds.getBottom(), 2.0f);
}

void SamplePad::drawPreviewPlayhead(Graphics& g, Rectangle<int> bounds)
{
    if (!isPreviewPlaying || !hasValidSample)
        return;

    float x = bounds.getX() + (previewPlayheadPosition * bounds.getWidth());

    g.setColour(Colours::orange); // Orange for preview playhead vs red for main
    g.drawLine(x, bounds.getY(), x, bounds.getBottom(), 2.0f);
}

String SamplePad::getKeyboardKeyForPad(int padIndex) const
{
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

String SamplePad::getLicenseShortName(const String& license) const
{
    String lowerLicense = license.toLowerCase();
    String licenseVer = "";

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

// Badge callback methods
void SamplePad::handleCopyClick()
{
    if (!hasValidSample)
        return;

    setMouseCursor(MouseCursor::CopyingCursor);

    Timer::callAfterDelay(3000, [this]() {
        setMouseCursor(MouseCursor::NormalCursor);
    });
}

void SamplePad::handleDeleteClick()
{
    if (!hasValidSample || padMode != PadMode::Normal)
        return;

    AlertWindow::showOkCancelBox(
        AlertWindow::QuestionIcon,
        "Delete Sample",
        "Are you sure you want to delete this sample?",
        "Yes", "No",
        nullptr,
        ModalCallbackFunction::create([this](int result) {
            if (result == 1)
            {
                clearSample();

                if (auto* gridComponent = findParentComponentOfClass<SampleGridComponent>())
                {
                    if (processor)
                    {
                        auto& sounds = processor->getCurrentSoundsArrayReference();
                        auto& data = processor->getDataReference();

                        if (padIndex < sounds.size())
                            sounds.set(padIndex, FSSound());

                        if (padIndex < data.size())
                            data[padIndex] = StringArray();

                        processor->setSources();
                    }

                    gridComponent->updateJsonMetadata();
                }
            }
        })
    );
}

void SamplePad::handleWavCopyClick()
{
    if (!hasValidSample)
        return;

    File wavFile = getWavFile();

    if (!wavFile.existsAsFile())
    {
        if (!convertOggToWav(audioFile, wavFile))
        {
            AlertWindow::showMessageBoxAsync(
                AlertWindow::WarningIcon,
                "Conversion Failed",
                "Failed to convert OGG file to WAV format.");
            return;
        }
    }

    setMouseCursor(MouseCursor::CopyingCursor);

    Timer::callAfterDelay(3000, [this]() {
        setMouseCursor(MouseCursor::NormalCursor);
    });
}

void SamplePad::handleBookmarkClick()
{
    if (!hasValidSample || !processor)
        return;

    SampleCollectionManager* collectionManager = processor->getCollectionManager();
    if (!collectionManager)
        return;

    if (collectionManager->isBookmarked(freesoundId))
    {
        if (collectionManager->removeBookmark(freesoundId))
        {
            repaint();
        }
    }
    else
    {
        if (collectionManager->addBookmark(freesoundId))
        {
            repaint();
        }
    }
}

// Drag and drop methods
void SamplePad::performEnhancedDragDrop()
{
    if (!hasValidSample || !audioFile.existsAsFile())
        return;

    bool isShiftPressed = ModifierKeys::getCurrentModifiers().isShiftDown();

    if (isShiftPressed)
    {
        performCrossAppDragDrop();
    }
    else
    {
        performInternalDragDrop();
    }
}

void SamplePad::performInternalDragDrop()
{
    // Create a complete data package including all metadata
    var dataPackage(new DynamicObject());
    dataPackage.getDynamicObject()->setProperty("type", "freesound_sampler_data");
    dataPackage.getDynamicObject()->setProperty("version", "1.0");
    dataPackage.getDynamicObject()->setProperty("freesound_id", freesoundId);
    dataPackage.getDynamicObject()->setProperty("sample_name", sampleName);
    dataPackage.getDynamicObject()->setProperty("author_name", authorName);
    dataPackage.getDynamicObject()->setProperty("license_type", licenseType);
    dataPackage.getDynamicObject()->setProperty("search_query", getQuery());
    dataPackage.getDynamicObject()->setProperty("tags", tags);
    dataPackage.getDynamicObject()->setProperty("description", description);
    dataPackage.getDynamicObject()->setProperty("file_name", audioFile.getFileName());
    dataPackage.getDynamicObject()->setProperty("original_pad_index", padIndex);
    dataPackage.getDynamicObject()->setProperty("exported_at", Time::getCurrentTime().toString(true, true));

    String jsonData = JSON::toString(dataPackage, false);

    var dragDescription(new DynamicObject());
    dragDescription.getDynamicObject()->setProperty("files", var(StringArray(audioFile.getFullPathName())));
    dragDescription.getDynamicObject()->setProperty("metadata", jsonData);
    dragDescription.getDynamicObject()->setProperty("mime_type", FREESOUND_SAMPLER_MIME_TYPE);

    startDragging(dragDescription, this, ScaledImage(), true);
}

void SamplePad::performCrossAppDragDrop()
{
    File tempDir = File::getSpecialLocation(File::tempDirectory)
                      .getChildFile("FreesoundSampler_CrossApp")
                      .getChildFile(String::toHexString(Time::getCurrentTime().toMilliseconds()));

    if (!tempDir.createDirectory()) return;

    var metadata(new DynamicObject());
    metadata.getDynamicObject()->setProperty("type", "freesound_sampler_data");
    metadata.getDynamicObject()->setProperty("freesound_id", freesoundId);
    metadata.getDynamicObject()->setProperty("sample_name", sampleName);
    metadata.getDynamicObject()->setProperty("author_name", authorName);
    metadata.getDynamicObject()->setProperty("license_type", licenseType);
    metadata.getDynamicObject()->setProperty("search_query", getQuery());
    metadata.getDynamicObject()->setProperty("tags", tags);
    metadata.getDynamicObject()->setProperty("description", description);
    metadata.getDynamicObject()->setProperty("audio_file_path", audioFile.getFullPathName());

    File metadataFile = tempDir.getChildFile("metadata.json");
    metadataFile.replaceWithText(JSON::toString(metadata, false));

    StringArray filePaths;
    filePaths.add(metadataFile.getFullPathName());
    filePaths.add(audioFile.getFullPathName());

    performExternalDragDropOfFiles(filePaths, false);

    Timer::callAfterDelay(30000, [tempDir]() {
        tempDir.deleteRecursively();
    });
}

// File conversion methods
File SamplePad::getWavFile() const
{
    if (!hasValidSample || freesoundId.isEmpty())
        return File();

    File oggFile = audioFile;
    File parentDir = oggFile.getParentDirectory();
    String wavFileName = "FS_ID_" + freesoundId + ".wav";
    return parentDir.getChildFile(wavFileName);
}

bool SamplePad::convertOggToWav(const File& oggFile, const File& wavFile)
{
    if (!oggFile.existsAsFile())
        return false;

    AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<AudioFormatReader> reader(formatManager.createReaderFor(oggFile));
    if (!reader)
        return false;

    WavAudioFormat wavFormat;
    std::unique_ptr<FileOutputStream> outputStream(wavFile.createOutputStream());
    if (!outputStream)
        return false;

    std::unique_ptr<AudioFormatWriter> writer(wavFormat.createWriterFor(
        outputStream.get(),
        reader->sampleRate,
        reader->numChannels,
        16,
        {},
        0
    ));

    if (!writer)
        return false;

    outputStream.release();

    const int bufferSize = 8192;
    AudioBuffer<float> buffer(reader->numChannels, bufferSize);

    int64 samplesRemaining = reader->lengthInSamples;
    int64 currentPosition = 0;

    while (samplesRemaining > 0)
    {
        int samplesToRead = jmin((int64)bufferSize, samplesRemaining);

        if (!reader->read(&buffer, 0, samplesToRead, currentPosition, true, true))
            break;

        if (!writer->writeFromAudioSampleBuffer(buffer, 0, samplesToRead))
            break;

        currentPosition += samplesToRead;
        samplesRemaining -= samplesToRead;
    }

    return wavFile.existsAsFile();
}

//==============================================================================
// SampleGridComponent Implementation
//==============================================================================

SampleGridComponent::SampleGridComponent()
    : processor(nullptr)
{
    // Initialize master search connections (all false initially)
    masterSearchConnections.fill(false);

    // Create and add sample pads
    for (int i = 0; i < TOTAL_PADS; ++i)
    {
        samplePads[i] = std::make_unique<SamplePad>(i);
        addAndMakeVisible(*samplePads[i]);
    }

    // Set up master search panel
    masterSearchPanel.setSampleGridComponent(this);
    masterSearchPanel.onSearchSelectedClicked = [this]() {
        handleMasterSearchSelected();
    };
    addAndMakeVisible(masterSearchPanel);

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
    g.setColour(Colour(0x800F0F0F).withAlpha(0.0f));
    g.fillAll();

    auto bounds = getLocalBounds();
    int padding = 4;
    int bottomControlsHeight = 90;
    bounds.removeFromBottom(bottomControlsHeight + padding);

    // Add visual feedback for enhanced drag hover
    if (isEnhancedDragHover && dragHoverPadIndex >= 0 && dragHoverPadIndex < TOTAL_PADS)
    {
        // Get pad bounds and draw special highlight
        auto bounds = getLocalBounds();
        int padding = 4;
        int bottomControlsHeight = 90;
        bounds.removeFromBottom(bottomControlsHeight + padding);

        int totalPadding = padding * (GRID_SIZE + 1);
        int padWidth = (bounds.getWidth() - totalPadding) / GRID_SIZE;
        int padHeight = (bounds.getHeight() - totalPadding) / GRID_SIZE;

        int row = (GRID_SIZE - 1) - (dragHoverPadIndex / GRID_SIZE);
        int col = dragHoverPadIndex % GRID_SIZE;

        int x = padding + col * (padWidth + padding);
        int y = padding + row * (padHeight + padding);

        Rectangle<int> padBounds(x, y, padWidth, padHeight);
    }

}

void SampleGridComponent::resized()
{
    auto bounds = getLocalBounds();
    int padding = 4;
    int bottomControlsHeight = 90; // Increased for master search panel
    int buttonHeight = 20;

    int buttonWidth = 60;
    int spacing = 5;

    // Reserve space for bottom controls
    auto bottomArea = bounds.removeFromBottom(bottomControlsHeight + padding);

    // Position controls in bottom area
    auto controlsBounds = bottomArea.reduced(padding);

    // Right side: stacked buttons (shuffle above clear all)
    auto rightButtonArea = controlsBounds.removeFromRight(buttonWidth);
    controlsBounds.removeFromRight(spacing);
    rightButtonArea.removeFromTop(8);
    shuffleButton.setBounds(rightButtonArea.removeFromTop(buttonHeight));
    rightButtonArea.removeFromBottom(8); // small spacing
    clearAllButton.setBounds(rightButtonArea.removeFromBottom(buttonHeight));

    controlsBounds.removeFromRight(spacing);

    // Left side: master search panel
    masterSearchPanel.setBounds(controlsBounds);

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
        String search_query = sample.getProperty("search_query", "");
        String fsTags = sample.getProperty("tags", "");
        String fsDescription = sample.getProperty("description", "");

        DBG("TAGS ARE : " + fsTags);
        if (fileName.isNotEmpty())
        {
            File audioFile = downloadDir.getChildFile(fileName);
            if (audioFile.existsAsFile())
            {
                samplePads[i]->setSample(audioFile, sampleName, authorName, freesoundId, license, search_query, fsTags, fsDescription);
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
        String fsTags = (i < soundInfo.size() && soundInfo[i].size() > 4) ? soundInfo[i][4] : ""; // Get tags from 5th element
        String fsDescription = (i < soundInfo.size() && soundInfo[i].size() > 5) ? soundInfo[i][5] : ""; // Get description from 6th element

        String expectedFilename = "FS_ID_" + freesoundId + ".ogg";
        File audioFile = downloadDir.getChildFile(expectedFilename);

        // DBG("Pad " + String(i) + ": " + sampleName + " (ID: " + freesoundId + ") with query: '" + sampleQuery + "'");
        // DBG("  Looking for file: " + audioFile.getFullPathName());
        // DBG("  File exists: " + String(audioFile.existsAsFile() ? "YES" : "NO"));

        if (audioFile.existsAsFile())
        {
            // IMPORTANT: Pass the query to setSample so it appears in the text box
            samplePads[i]->setSample(audioFile, sampleName, authorName, freesoundId, license, sampleQuery, fsTags, fsDescription);
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

void SampleGridComponent::shuffleSamples()
{
    if (!processor)
        return;

    // Collect samples WITH their current pad indices
    Array<std::pair<int, SamplePad::SampleInfo>> samplesWithIndices;
    Array<int> occupiedPadIndices;

    for (int i = 0; i < TOTAL_PADS; ++i)
    {
        auto sampleInfo = samplePads[i]->getSampleInfo();
        if (sampleInfo.hasValidSample)
        {
            samplesWithIndices.add({i, sampleInfo});
            occupiedPadIndices.add(i);
        }
    }

    // If we have no samples or only one sample, nothing to shuffle
    if (samplesWithIndices.size() <= 1)
        return;

    // Extract just the sample info for shuffling
    Array<SamplePad::SampleInfo> samplesToShuffle;
    for (const auto& pair : samplesWithIndices)
    {
        samplesToShuffle.add(pair.second);
    }

    // Shuffle the samples array using JUCE's Random class
    Random random;
    for (int i = samplesToShuffle.size() - 1; i > 0; --i)
    {
        int j = random.nextInt(i + 1);
        samplesToShuffle.swap(i, j);
    }

    // Clear only the occupied pads
    for (int padIndex : occupiedPadIndices)
    {
        samplePads[padIndex]->clearSample();
    }

    // Redistribute shuffled samples back to the SAME occupied positions
    for (int i = 0; i < samplesToShuffle.size() && i < occupiedPadIndices.size(); ++i)
    {
        const auto& sample = samplesToShuffle[i];
        int targetPadIndex = occupiedPadIndices[i];

        samplePads[targetPadIndex]->setSample(
            sample.audioFile,
            sample.sampleName,
            sample.authorName,
            sample.freesoundId,
            sample.licenseType,
            sample.query  // Preserve the query
        );
    }

    // Update processor arrays to match new order
    updateProcessorArraysFromGrid();

    // Reload the sampler with new pad order
    if (processor)
    {
        processor->setSources();
    }
}

void SampleGridComponent::updateProcessorArraysFromGrid()
{
    if (!processor)
        return;

    auto& currentSounds = processor->getCurrentSoundsArrayReference();
    auto& soundsData = processor->getDataReference();

    // Ensure arrays are properly sized (16 elements)
    currentSounds.clear();
    soundsData.clear();

    currentSounds.resize(TOTAL_PADS);
    soundsData.resize(TOTAL_PADS);

    // Initialize all positions with empty data
    for (int i = 0; i < TOTAL_PADS; ++i)
    {
        currentSounds.set(i, FSSound()); // Empty sound

        StringArray emptyData;
        emptyData.add(""); // name
        emptyData.add(""); // author
        emptyData.add(""); // license
        emptyData.add(""); // query
        soundsData[i] = emptyData;
    }

    // Fill in actual pad data at correct positions
    for (int padIndex = 0; padIndex < TOTAL_PADS; ++padIndex)
    {
        auto padInfo = samplePads[padIndex]->getSampleInfo();

        if (padInfo.hasValidSample)
        {
            // Create FSSound object for this specific pad position
            FSSound sound;
            sound.id = padInfo.freesoundId;
            sound.name = padInfo.sampleName;
            sound.user = padInfo.authorName;
            sound.license = padInfo.licenseType;
            sound.duration = 0.5; // Default values
            sound.filesize = 50000;

            // Try to get duration and file size from existing data
            if (padInfo.audioFile.existsAsFile())
            {
                sound.filesize = (int)padInfo.audioFile.getSize();
            }

            // Set sound at the correct pad position (this is crucial for MIDI mapping)
            currentSounds.set(padIndex, sound);

            // Create sound info including query as 4th element
            StringArray soundData;
            soundData.add(padInfo.sampleName);
            soundData.add(padInfo.authorName);
            soundData.add(padInfo.licenseType);
            soundData.add(padInfo.query);
            soundsData[padIndex] = soundData;

            // DBG("Updated processor arrays - pad " + String(padIndex) + ": " + padInfo.sampleName + " (ID: " + padInfo.freesoundId + ")");
        }
    }

    // DBG("Processor arrays updated - total pads with samples: " + String(currentSounds.size()));
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
                            sample->setProperty("tags", originalSample.getProperty("tags", ""));
                            sample->setProperty("description", originalSample.getProperty("description", ""));
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
    int padIndex = noteNumber - 36;
    if (padIndex >= 0 && padIndex < TOTAL_PADS)
    {
        samplePads[padIndex]->setIsPlaying(true);
        samplePads[padIndex]->setPlayheadPosition(0.0f);

        // Track play count in collection manager
        if (processor)
        {
            processor->incrementSamplePlayCount(padIndex);
        }
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
    auto [finalSounds, soundInfo] = makeQuerySearchUsingFreesoundAPI(query, 1, true);
    return finalSounds;
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

    // Store the master search data
    pendingMasterSearchSounds = sounds;
    pendingMasterSearchSoundInfo = soundInfo;
    pendingMasterSearchQuery = masterQuery;

    // FIXED: Get target pads that are connected or empty (allowing duplicates)
    pendingMasterSearchPads.clear();
    for (int i = 0; i < TOTAL_PADS; ++i)
    {
        if (samplePads[i]->isConnectedToMaster() || !samplePads[i]->hasQuery())
        {
            pendingMasterSearchPads.add(i);
        }
    }

    // No duplication check - allow same sample on multiple pads
    // DBG("Master search will populate " + String(pendingMasterSearchPads.size()) + " pads with " + String(sounds.size()) + " sounds");
}

void SampleGridComponent::populatePadsFromMasterSearch()
{
    if (!processor || pendingMasterSearchSounds.isEmpty() || pendingMasterSearchPads.isEmpty())
        return;

    File downloadDir = processor->getCurrentDownloadLocation();
    SampleCollectionManager* collectionManager = processor->getCollectionManager();

    if (!collectionManager)
        return;

    // FIXED: Distribute sounds to pads (allowing duplicates if more pads than sounds)
    int soundIndex = 0;
    for (int padIndex : pendingMasterSearchPads)
    {
        if (padIndex >= TOTAL_PADS)
            continue;

        // FIXED: Cycle through sounds if more pads than sounds (allows duplicates)
        const FSSound& sound = pendingMasterSearchSounds[soundIndex % pendingMasterSearchSounds.size()];

        // Add/update sample in collection
        collectionManager->addOrUpdateSample(sound, pendingMasterSearchQuery);

        // Create expected filename
        String expectedFilename = "FS_ID_" + sound.id + ".ogg";
        File audioFile = downloadDir.getChildFile(expectedFilename);

        if (audioFile.existsAsFile())
        {
            // Get sample info with individual query for this pad
            String sampleName = (soundIndex % pendingMasterSearchSoundInfo.size() < pendingMasterSearchSoundInfo.size() &&
                                pendingMasterSearchSoundInfo[soundIndex % pendingMasterSearchSoundInfo.size()].size() > 0) ?
                pendingMasterSearchSoundInfo[soundIndex % pendingMasterSearchSoundInfo.size()][0] : sound.name;
            String authorName = (soundIndex % pendingMasterSearchSoundInfo.size() < pendingMasterSearchSoundInfo.size() &&
                               pendingMasterSearchSoundInfo[soundIndex % pendingMasterSearchSoundInfo.size()].size() > 1) ?
                pendingMasterSearchSoundInfo[soundIndex % pendingMasterSearchSoundInfo.size()][1] : sound.user;
            String license = (soundIndex % pendingMasterSearchSoundInfo.size() < pendingMasterSearchSoundInfo.size() &&
                            pendingMasterSearchSoundInfo[soundIndex % pendingMasterSearchSoundInfo.size()].size() > 2) ?
                pendingMasterSearchSoundInfo[soundIndex % pendingMasterSearchSoundInfo.size()][2] : sound.license;

            // FIXED: Update pad individually with master query
            samplePads[padIndex]->setSample(audioFile, sampleName, authorName, sound.id, license,
                                          pendingMasterSearchQuery, sound.tags.joinIntoString(","), sound.description);

            // CRITICAL: Update processor state for this specific pad
            processor->setPadSample(padIndex, sound.id);
        }

        soundIndex++;
    }

    // CRITICAL: Rebuild sampler to map all pads correctly (including duplicates)
    processor->setSources();

    // Clear pending state
    clearPendingMasterSearchState();
}

void SampleGridComponent::clearPendingMasterSearchState()
{
    pendingMasterSearchPads.clear();
    pendingMasterSearchQuery = "";
    pendingMasterSearchSounds.clear();
    pendingMasterSearchSoundInfo.clear();
}

bool SampleGridComponent::hasPendingMasterSearch() const
{
    return !pendingMasterSearchSounds.isEmpty() && !pendingMasterSearchPads.isEmpty();
}

void SampleGridComponent::updateProcessorArraysForMasterSearch(const Array<FSSound>& sounds,
    const std::vector<StringArray>& soundInfo, const Array<int>& targetPads, const String& masterQuery)
{
    if (!processor)
        return;

    auto& currentSounds = processor->getCurrentSoundsArrayReference();
    auto& soundsData = processor->getDataReference();

    // Ensure arrays are large enough
    while (currentSounds.size() < TOTAL_PADS)
        currentSounds.add(FSSound());
    while (soundsData.size() < TOTAL_PADS)
    {
        StringArray emptyData;
        emptyData.add(""); emptyData.add(""); emptyData.add(""); emptyData.add("");
        soundsData.push_back(emptyData);
    }

    // Update only the target pads with new sounds
    for (int i = 0; i < targetPads.size() && i < sounds.size(); ++i)
    {
        int padIndex = targetPads[i];
        const FSSound& sound = sounds[i];

        currentSounds.set(padIndex, sound);

        StringArray soundData;
        soundData.add((i < soundInfo.size() && soundInfo[i].size() > 0) ? soundInfo[i][0] : sound.name);
        soundData.add((i < soundInfo.size() && soundInfo[i].size() > 1) ? soundInfo[i][1] : sound.user);
        soundData.add((i < soundInfo.size() && soundInfo[i].size() > 2) ? soundInfo[i][2] : sound.license);
        soundData.add(masterQuery); // Set master query for all target pads

        soundsData[padIndex] = soundData;
    }
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

    // Check if pad already has a sample and ask for confirmation
    auto currentSample = samplePads[padIndex]->getSampleInfo();
    if (currentSample.hasValidSample)
    {
        String message = "Pad " + String(padIndex + 1) + " already contains:\n\"" +
                        currentSample.sampleName + "\" by " + currentSample.authorName + "\n\n" +
                        "Replace with a new search for: \"" + query + "\"?";

        AlertWindow::showOkCancelBox(
            AlertWindow::QuestionIcon,
            "Replace Sample",
            message,
            "Yes, Replace", "No, Cancel",
            nullptr,
            ModalCallbackFunction::create([this, padIndex, query](int result) {
                if (result == 1) { // User clicked "Yes, Replace"
                    performSinglePadSearch(padIndex, query);
                }
                // If result == 0, user clicked "No, Cancel" - do nothing
            })
        );
        return;
    }

    // Pad is empty - proceed without confirmation
    performSinglePadSearch(padIndex, query);
}

void SampleGridComponent::performSinglePadSearch(int padIndex, const String& query)
{
    // Search for a single sound with the specific query
    Array<FSSound> searchResults = searchSingleSound(query);

    if (searchResults.isEmpty())
    {
        AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
            "No Results",
            "No sounds found for the query: " + query);
        std::cout << samplePads[padIndex]->getSampleInfo().query << std::endl; // Debugging output
        samplePads[padIndex]->setQuery(samplePads[padIndex]->getSampleInfo().query, true); // Reset query visually)
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

bool SampleGridComponent::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
    // Accept enhanced sampler data
    if (dragSourceDetails.description.hasProperty("mime_type") &&
        dragSourceDetails.description.getProperty("mime_type", "") == FREESOUND_SAMPLER_MIME_TYPE)
    {
        return true;
    }

    // Accept regular file drops
    if (dragSourceDetails.description.hasProperty("files"))
    {
        return true;
    }

    // Accept internal pad swaps (existing functionality)
    if (dragSourceDetails.description.hasProperty("type") &&
        dragSourceDetails.description.getProperty("type", "") == "samplePad")
    {
        return true;
    }

    return false;
}

void SampleGridComponent::itemDragEnter(const SourceDetails& dragSourceDetails)
{
    isEnhancedDragHover = true;

    // Determine which pad we're hovering over
    dragHoverPadIndex = getPadIndexFromPosition(dragSourceDetails.localPosition);

    // Visual feedback for enhanced drag
    if (dragSourceDetails.description.hasProperty("mime_type") &&
        dragSourceDetails.description.getProperty("mime_type", "") == FREESOUND_SAMPLER_MIME_TYPE)
    {
        // Special visual feedback for enhanced drops
        if (dragHoverPadIndex >= 0 && dragHoverPadIndex < TOTAL_PADS)
        {
            // You could add special hover state here
        }
    }

    repaint();
}

void SampleGridComponent::itemDragExit(const SourceDetails& dragSourceDetails)
{
    isEnhancedDragHover = false;
    dragHoverPadIndex = -1;
    repaint();
}

void SampleGridComponent::itemDropped(const SourceDetails& dragSourceDetails)
{
    isEnhancedDragHover = false;
    int targetPadIndex = getPadIndexFromPosition(dragSourceDetails.localPosition);
    dragHoverPadIndex = -1;

    if (targetPadIndex < 0 || targetPadIndex >= TOTAL_PADS)
    {
        repaint();
        return;
    }

    // Handle enhanced sampler data drop
    if (dragSourceDetails.description.hasProperty("mime_type") &&
        dragSourceDetails.description.getProperty("mime_type", "") == FREESOUND_SAMPLER_MIME_TYPE)
    {
        String jsonMetadata = dragSourceDetails.description.getProperty("metadata", "");
        var filesVar = dragSourceDetails.description.getProperty("files", var());

        StringArray filePaths;
        if (filesVar.isArray())
        {
            Array<var>* filesArray = filesVar.getArray();
            for (const auto& fileVar : *filesArray)
            {
                filePaths.add(fileVar.toString());
            }
        }
        else if (filesVar.isString())
        {
            filePaths.add(filesVar.toString());
        }

        handleEnhancedDrop(jsonMetadata, filePaths, targetPadIndex);
        repaint();
        return;
    }

    // Handle regular file drops
    if (dragSourceDetails.description.hasProperty("files"))
    {
        var filesVar = dragSourceDetails.description.getProperty("files", var());
        StringArray filePaths;

        if (filesVar.isArray())
        {
            Array<var>* filesArray = filesVar.getArray();
            for (const auto& fileVar : *filesArray)
            {
                filePaths.add(fileVar.toString());
            }
        }

        handleFileDrop(filePaths, targetPadIndex);
        repaint();
        return;
    }

    // Handle internal pad swaps (existing functionality)
    if (dragSourceDetails.description.hasProperty("type") &&
        dragSourceDetails.description.getProperty("type", "") == "samplePad")
    {
        int sourcePadIndex = dragSourceDetails.description.getProperty("sourcePadIndex", -1);
        swapSamples(sourcePadIndex, targetPadIndex);
        repaint();
        return;
    }

    repaint();
}

// Helper method to determine which pad is at a given position
int SampleGridComponent::getPadIndexFromPosition(Point<int> position)
{
    auto bounds = getLocalBounds();
    int padding = 4;
    int bottomControlsHeight = 90;
    bounds.removeFromBottom(bottomControlsHeight + padding);

    int totalPadding = padding * (GRID_SIZE + 1);
    int padWidth = (bounds.getWidth() - totalPadding) / GRID_SIZE;
    int padHeight = (bounds.getHeight() - totalPadding) / GRID_SIZE;

    // Calculate which pad this position corresponds to
    int col = (position.x - padding) / (padWidth + padding);
    int row = (position.y - padding) / (padHeight + padding);

    if (col >= 0 && col < GRID_SIZE && row >= 0 && row < GRID_SIZE)
    {
        // Convert grid position to pad index (bottom-left is pad 0)
        int padIndex = (GRID_SIZE - 1 - row) * GRID_SIZE + col;
        DBG("padIndex: " +  String(jmin(padIndex, 15)));
        return jmin(padIndex, 15);
    }

    DBG("padIndex: -1");

    return -1;
}

// Handle enhanced drop with full metadata
void SampleGridComponent::handleEnhancedDrop(const String& jsonMetadata, const StringArray& filePaths, int targetPadIndex)
{
    if (jsonMetadata.isEmpty() || filePaths.isEmpty() || targetPadIndex < 0 || targetPadIndex >= TOTAL_PADS)
        return;

    // Parse the metadata
    var metadata = JSON::parse(jsonMetadata);
    if (!metadata.isObject())
    {
        return;
    }

    // Extract sample information
    String freesoundId = metadata.getProperty("freesound_id", "");
    String sampleName = metadata.getProperty("sample_name", "Unknown");
    String authorName = metadata.getProperty("author_name", "Unknown");
    String licenseType = metadata.getProperty("license_type", "");
    String searchQuery = metadata.getProperty("search_query", "");
    String tags = metadata.getProperty("tags", "");
    String description = metadata.getProperty("description", "");

    // Check if target pad already has a sample
    auto targetPadInfo = samplePads[targetPadIndex]->getSampleInfo();
    if (targetPadInfo.hasValidSample)
    {
        // Check if it's the same sample (by Freesound ID)
        if (targetPadInfo.freesoundId == freesoundId)
        {
            // Same sample - proceed without confirmation
            performEnhancedDrop(jsonMetadata, filePaths, targetPadIndex);
            return;
        }
        else
        {
            // Different sample exists - ask for confirmation
            String message = "Pad " + String(targetPadIndex + 1) + " already contains:\n\"" +
                           targetPadInfo.sampleName + "\" by " + targetPadInfo.authorName + "\n\n" +
                           "Replace with:\n\"" + sampleName + "\" by " + authorName + "\"?";

            AlertWindow::showOkCancelBox(
                AlertWindow::QuestionIcon,
                "Replace Sample",
                message,
                "Yes, Replace", "No, Cancel",
                nullptr,
                ModalCallbackFunction::create([this, jsonMetadata, filePaths, targetPadIndex](int result) {
                    if (result == 1) { // User clicked "Yes, Replace"
                        performEnhancedDrop(jsonMetadata, filePaths, targetPadIndex);
                    }
                    // If result == 0, user clicked "No, Cancel" - do nothing
                })
            );
            return;
        }
    }

    // Target pad is empty - proceed without confirmation
    performEnhancedDrop(jsonMetadata, filePaths, targetPadIndex);
}

void SampleGridComponent::performEnhancedDrop(const String& jsonMetadata, const StringArray& filePaths, int targetPadIndex)
{
    // Parse the metadata
    var metadata = JSON::parse(jsonMetadata);
    if (!metadata.isObject())
        return;

    // Extract sample information
    String freesoundId = metadata.getProperty("freesound_id", "");
    String sampleName = metadata.getProperty("sample_name", "Unknown");
    String authorName = metadata.getProperty("author_name", "Unknown");
    String licenseType = metadata.getProperty("license_type", "");
    String searchQuery = metadata.getProperty("search_query", "");
    String originalFileName = metadata.getProperty("file_name", "");
    String tags = metadata.getProperty("tags", "");
    String description = metadata.getProperty("description", "");

    // Copy the file to our samples directory if it's not already there
    File sourceFile(filePaths[0]);
    if (!sourceFile.existsAsFile())
        return;

    // Determine target file location
    File samplesDir = processor->getCurrentDownloadLocation();
    String targetFileName = "FS_ID_" + freesoundId + ".ogg";
    File targetFile = samplesDir.getChildFile(targetFileName);

    // Copy file if it doesn't exist
    if (!targetFile.existsAsFile())
    {
        if (!sourceFile.copyFileTo(targetFile))
            return;
    }

    // Create FSSound object for processor
    FSSound sound;
    sound.id = freesoundId;
    sound.name = sampleName;
    sound.user = authorName;
    sound.license = licenseType;
    sound.duration = 0.5; // Default values
    sound.filesize = (int)targetFile.getSize();
    sound.tags = tags;
    sound.description = description;

    // Update the target pad with full metadata
    samplePads[targetPadIndex]->setSample(targetFile, sampleName, authorName, freesoundId, licenseType, searchQuery, tags, description);

    // Update processor arrays
    updateSinglePadInProcessor(targetPadIndex, sound);

    // Update processor's soundsArray with query
    auto& soundsData = processor->getDataReference();
    while (soundsData.size() <= targetPadIndex)
    {
        StringArray emptyData;
        emptyData.add(""); emptyData.add(""); emptyData.add(""); emptyData.add("");
        soundsData.push_back(emptyData);
    }

    if (soundsData[targetPadIndex].size() < 4)
    {
        while (soundsData[targetPadIndex].size() < 4)
            soundsData[targetPadIndex].add("");
    }

    soundsData[targetPadIndex].set(0, sampleName);
    soundsData[targetPadIndex].set(1, authorName);
    soundsData[targetPadIndex].set(2, licenseType);
    soundsData[targetPadIndex].set(3, searchQuery);

    // Reload sampler
    if (processor)
    {
        processor->setSources();
    }
}

// Handle regular file drops (existing functionality)
void SampleGridComponent::handleFileDrop(const StringArray& filePaths, int targetPadIndex)
{
    // This would handle regular audio file drops without metadata
    // You can implement this if you want to support dropping regular audio files
    AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon,
        "Regular File Drop",
        "Regular file drops not yet implemented. Use Command+Drag from another Freesound Sampler instance.");
}

void SampleGridComponent::setPositionConnectedToMaster(int row, int col, bool connected)
{
    int visualPosition = getVisualPositionFromRowCol(row, col);
    if (visualPosition >= 0 && visualPosition < TOTAL_PADS)
    {
        masterSearchConnections[visualPosition] = connected;
        updatePadConnectionStates();
    }
}

void SampleGridComponent::syncMasterQueryToPosition(int row, int col, const String& masterQuery)
{
    int visualPosition = getVisualPositionFromRowCol(row, col);
    if (visualPosition >= 0 && visualPosition < TOTAL_PADS)
    {
        // Find which pad is currently at this visual position
        for (int i = 0; i < TOTAL_PADS; ++i)
        {
            int padRow, padCol;
            getRowColFromPadIndex(i, padRow, padCol);
            int padVisualPosition = getVisualPositionFromRowCol(padRow, padCol);

            if (padVisualPosition == visualPosition)
            {
                // Only sync if the pad is actually connected to master
                if (samplePads[i]->isConnectedToMaster())
                {
                    samplePads[i]->syncMasterQuery(masterQuery);
                }
                break;
            }
        }
    }
}

void SampleGridComponent::handleMasterSearchSelected()
{
    String masterQuery = masterSearchPanel.getMasterQuery();
    searchSelectedPositions(masterQuery);
}

void SampleGridComponent::performMasterSearch(const String& masterQuery)
{
    if (!processor || masterQuery.isEmpty())
        return;

    // Get all connected pad indices
    Array<int> connectedPadIndices;
    for (int i = 0; i < TOTAL_PADS; ++i)
    {
        if (samplePads[i]->isConnectedToMaster())
        {
            connectedPadIndices.add(i);
        }
    }

    if (connectedPadIndices.isEmpty())
    {
        AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon,
            "No Connected Pads",
            "No pads are connected to master search. Use the mini grid to select pads for master search.");
        return;
    }

    // Check if any connected pads have samples and warn user
    bool hasExistingSamples = false;
    for (int padIndex : connectedPadIndices)
    {
        if (samplePads[padIndex]->getSampleInfo().hasValidSample)
        {
            hasExistingSamples = true;
            break;
        }
    }

    if (hasExistingSamples)
    {
        AlertWindow::showOkCancelBox(
            AlertWindow::QuestionIcon,
            "Replace Existing Samples",
            "Some connected pads already have samples loaded. This will replace them with new search results. Continue?",
            "Yes", "No",
            nullptr,
            ModalCallbackFunction::create([this, masterQuery, connectedPadIndices](int result) {
                if (result == 1) { // User clicked "Yes"
                    executeMasterSearch(masterQuery, connectedPadIndices);
                }
            })
        );
    }
    else
    {
        executeMasterSearch(masterQuery, connectedPadIndices);
    }
}

void SampleGridComponent::executeMasterSearch(const String& masterQuery, const Array<int>& targetPadIndices)
{
    if (!processor)
        return;

    // Store the target pad indices for when the search completes
    pendingMasterSearchPads = targetPadIndices;
    pendingMasterSearchQuery = masterQuery;

    int numSoundsNeeded = targetPadIndices.size();

    auto [finalSounds, soundInfo] = makeQuerySearchUsingFreesoundAPI(masterQuery, numSoundsNeeded, true);

    if (finalSounds.isEmpty())
    {
        AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
            "No Results",
            "No sounds found for the query: " + masterQuery);
        return;
    }

    // Clear the target pads first
    for (int padIndex : targetPadIndices)
    {
        samplePads[padIndex]->clearSample();
    }

    // Calculate how many sounds we need
    processor->newSoundsReady(finalSounds, masterQuery, soundInfo);
    std::cout << "Sound 0 tags : " << finalSounds[0].tags.joinIntoString(",")  << std::endl;
}

int SampleGridComponent::getVisualPositionFromRowCol(int row, int col) const
{
    if (row >= 0 && row < GRID_SIZE && col >= 0 && col < GRID_SIZE)
    {
        return (GRID_SIZE - 1 - row) * GRID_SIZE + col; // Bottom-left = 0
    }
    return -1;
}

void SampleGridComponent::getRowColFromVisualPosition(int position, int& row, int& col) const
{
    if (position >= 0 && position < TOTAL_PADS)
    {
        row = GRID_SIZE - 1 - (position / GRID_SIZE);
        col = position % GRID_SIZE;
    }
    else
    {
        row = col = -1;
    }
}

void SampleGridComponent::getRowColFromPadIndex(int padIndex, int& row, int& col) const
{
    // Since pads maintain their visual position regardless of swapping,
    // we need to find where this pad currently sits in the grid
    if (padIndex >= 0 && padIndex < TOTAL_PADS)
    {
        auto bounds = samplePads[padIndex]->getBounds();

        // Calculate row/col based on bounds position
        auto gridBounds = getLocalBounds();
        int padding = 4;
        int bottomControlsHeight = 90;
        gridBounds.removeFromBottom(bottomControlsHeight + padding);

        int totalPadding = padding * (GRID_SIZE + 1);
        int padWidth = (gridBounds.getWidth() - totalPadding) / GRID_SIZE;
        int padHeight = (gridBounds.getHeight() - totalPadding) / GRID_SIZE;

        col = (bounds.getX() - padding) / (padWidth + padding);
        row = (bounds.getY() - padding) / (padHeight + padding);

        // Clamp values
        row = jlimit(0, GRID_SIZE - 1, row);
        col = jlimit(0, GRID_SIZE - 1, col);
    }
    else
    {
        row = col = -1;
    }
}

void SampleGridComponent::updatePadConnectionStates()
{
    // Update each pad's connection state based on its current visual position
    for (int i = 0; i < TOTAL_PADS; ++i)
    {
        int row, col;
        getRowColFromPadIndex(i, row, col);
        int visualPosition = getVisualPositionFromRowCol(row, col);

        if (visualPosition >= 0 && visualPosition < TOTAL_PADS)
        {
            bool connected = masterSearchConnections[visualPosition];
            samplePads[i]->setConnectedToMaster(connected);

            // Sync current master query if connected
            if (connected)
            {
                String masterQuery = masterSearchPanel.getMasterQuery();
                if (masterQuery.isNotEmpty())
                {
                    samplePads[i]->syncMasterQuery(masterQuery);
                }
            }
        }
    }
}

bool SampleGridComponent::hasAnySamplesInConnectedPositions() const
{
    for (int i = 0; i < TOTAL_PADS; ++i)
    {
        if (samplePads[i]->isConnectedToMaster() && samplePads[i]->getSampleInfo().hasValidSample)
        {
            return true;
        }
    }
    return false;
}

void SampleGridComponent::searchSelectedPositions(const String& masterQuery)
{
    if (masterQuery.isEmpty())
    {
        AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
            "Empty Query", "Please enter a search term in the master search box.");
        return;
    }

    // Check if any connected positions have samples and warn user
    if (hasAnySamplesInConnectedPositions())
    {
        AlertWindow::showOkCancelBox(
            AlertWindow::QuestionIcon,
            "Replace Existing Samples",
            "Some connected pads already have samples loaded. This will replace them with new search results. Continue?",
            "Yes", "No",
            nullptr,
            ModalCallbackFunction::create([this, masterQuery](int result) {
                if (result == 1) { // User clicked "Yes"
                    performMasterSearch(masterQuery);
                }
            })
        );
    }
    else
    {
        performMasterSearch(masterQuery);
    }
}

bool SampleGridComponent::isInterestedInFileDrag(const StringArray& files)
{
    return true; // Accept all file drops, filter in filesDropped
}

void SampleGridComponent::filesDropped(const StringArray& files, int x, int y)
{
    int targetPadIndex = getPadIndexFromPosition(Point<int>(x, y));
    if (targetPadIndex < 0 || targetPadIndex >= TOTAL_PADS) return;

    // Check for cross-app drop (metadata.json + audio file)
    bool hasMetadata = false;
    bool hasAudio = false;

    for (const String& path : files)
    {
        File file(path);
        if (file.getFileName() == "metadata.json") hasMetadata = true;
        if (file.hasFileExtension("ogg;wav;mp3;aiff")) hasAudio = true;
    }

    if (hasMetadata && hasAudio)
    {
        handleCrossAppDrop(files, targetPadIndex);
    }
    else
    {
        // Handle regular file drops or show message
    }
}

void SampleGridComponent::handleCrossAppDrop(const StringArray& files, int targetPadIndex)
{
    File metadataFile, audioFile;

    // Find metadata and audio files
    for (const String& path : files)
    {
        File file(path);
        if (file.getFileName() == "metadata.json") metadataFile = file;
        else if (file.hasFileExtension("ogg") || file.hasFileExtension("wav") ||
                 file.hasFileExtension("mp3") || file.hasFileExtension("aiff")) audioFile = file;
    }

    if (!metadataFile.existsAsFile() || !audioFile.existsAsFile()) return;

    // Parse metadata
    var metadata = JSON::parse(metadataFile.loadFileAsString());
    if (!metadata.isObject()) return;

    // Extract info
    String freesoundId = metadata.getProperty("freesound_id", "");
    String sampleName = metadata.getProperty("sample_name", "");
    String authorName = metadata.getProperty("author_name", "");
    String licenseType = metadata.getProperty("license_type", "");
    String searchQuery = metadata.getProperty("search_query", "");
    String fsTags = metadata.getProperty("tags", "");
    String fsDescription = metadata.getProperty("description", "");

    // Load sample directly from original location
    samplePads[targetPadIndex]->setSample(audioFile, sampleName, authorName, freesoundId, licenseType, searchQuery, fsTags, fsDescription);

    // Create FSSound from metadata
    FSSound sound;
    sound.id = freesoundId;
    sound.name = sampleName;
    sound.user = authorName;
    sound.license = licenseType;
    sound.duration = 0.5; // Default
    sound.filesize = (int)audioFile.getSize();
    sound.tags = fsTags;
    sound.description = fsDescription;

    // Update processor arrays
    updateSinglePadInProcessor(targetPadIndex, sound);  // Complete expression
    processor->setSources();
}

void SampleGridComponent::fileDragEnter(const StringArray& files, int x, int y)
{
    isEnhancedDragHover = true;
    dragHoverPadIndex = getPadIndexFromPosition(Point<int>(x, y));
    repaint();
}

void SampleGridComponent::fileDragExit(const StringArray& files)
{
    isEnhancedDragHover = false;
    dragHoverPadIndex = -1;
    repaint();
}

void SampleGridComponent::updatePadFromCollection(int padIndex, const String& freesoundId)
{
    if (padIndex < 0 || padIndex >= TOTAL_PADS || !processor)
        return;

    SampleCollectionManager* collectionManager = processor->getCollectionManager();
    if (!collectionManager)
        return;

    if (freesoundId.isEmpty())
    {
        // Clear the pad
        samplePads[padIndex]->clearSample();
        processor->clearPad(padIndex);
    }
    else
    {
        // FIXED: Load sample data from collection - works even if duplicate
        SampleMetadata sample = collectionManager->getSample(freesoundId);
        File audioFile = collectionManager->getSampleFile(freesoundId);

        if (!sample.freesoundId.isEmpty() && audioFile.existsAsFile())
        {
            // Get individual query for this specific pad if available
            String individualQuery = sample.searchQuery; // Default to sample's query

            // Try to get pad-specific query from UI state if available
            if (samplePads[padIndex]->hasQuery())
            {
                individualQuery = samplePads[padIndex]->getQuery();
            }

            // Update visual pad with individual data
            samplePads[padIndex]->setSample(
                audioFile,
                sample.originalName,
                sample.authorName,
                sample.freesoundId,
                sample.licenseType,
                individualQuery,  // Use individual query
                sample.tags,
                sample.description
            );

            // CRITICAL: Update processor state for this specific pad
            processor->setPadSample(padIndex, freesoundId);
        }
    }
}

// Load all pads from current processor state
void SampleGridComponent::refreshFromProcessor()
{
    if (!processor)
        return;

    // FIXED: Update each pad individually, allowing duplicates
    for (int padIndex = 0; padIndex < TOTAL_PADS; ++padIndex)
    {
        String freesoundId = processor->getPadFreesoundId(padIndex);
        updatePadFromCollection(padIndex, freesoundId);
    }

    // CRITICAL: Rebuild sampler to ensure all pads are mapped correctly
    processor->setSources();
}

// Updated sample loading for master search
void SampleGridComponent::loadSingleSampleWithQuery(int padIndex, const FSSound& sound, const File& audioFile, const String& query)
{
    if (!processor)
        return;

    SampleCollectionManager* collectionManager = processor->getCollectionManager();
    if (!collectionManager)
        return;

    // Add/update sample in collection with the search query
    collectionManager->addOrUpdateSample(sound, query);

    // Update the pad
    updatePadFromCollection(padIndex, sound.id);

    // Rebuild sampler
    processor->setSources();
}

// Updated clear sample method
void SampleGridComponent::clearSampleFromPad(int padIndex)
{
    if (padIndex < 0 || padIndex >= TOTAL_PADS)
        return;

    updatePadFromCollection(padIndex, "");

    if (processor)
    {
        processor->setSources();
    }
}

// Updated swap samples method
void SampleGridComponent::swapSamples(int sourcePadIndex, int targetPadIndex)
{
    if (sourcePadIndex < 0 || sourcePadIndex >= TOTAL_PADS ||
        targetPadIndex < 0 || targetPadIndex >= TOTAL_PADS ||
        sourcePadIndex == targetPadIndex || !processor)
        return;

    // Get current freesound IDs
    String sourceId = processor->getPadFreesoundId(sourcePadIndex);
    String targetId = processor->getPadFreesoundId(targetPadIndex);

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

    // Update processor pad assignments
    processor->setPadSample(sourcePadIndex, targetId);
    processor->setPadSample(targetPadIndex, sourceId);

    // Rebuild the sampler to apply updated MIDI mappings
    processor->setSources();

    // Repaint both pads
    samplePads[sourcePadIndex]->repaint();
    samplePads[targetPadIndex]->repaint();
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
            pluginChoiceColour.withAlpha(0.5f), bounds.getTopLeft().toFloat(),
            Colour(0x800099CC).withAlpha(0.5f), bounds.getBottomRight().toFloat(), false));
    }
    else
    {
        g.setColour(Colour(0x802A2A2A).withAlpha(0.8f));
    }
    g.fillRoundedRectangle(bounds.toFloat(), 6.0f);

    // Modern border
    g.setColour(isDragging ? pluginChoiceColour : Colour(0x80404040));
    g.drawRoundedRectangle(bounds.toFloat().reduced(1), 6.0f, 1.5f);

    // Modern icon and text styling
    g.setColour(Colours::white);
    g.setFont(Font(10.0f));

    // Draw a modern drag icon (three horizontal lines)
    /*auto iconBounds = bounds.removeFromLeft(20);
    int lineY = iconBounds.getCentreY() - 6;
    for (int i = 0; i < 3; ++i)
    {
        g.fillRoundedRectangle(iconBounds.getX() + 4, lineY + i * 4, 12, 2, 1.0f);
    }*/

    // Draw text
    g.drawText(juce::String(CharPointer_UTF8("\xF0\x9F\x93\x91")) + "\t All Samples", bounds, Justification::centred);
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
        //    pluginChoiceColour.withAlpha(0.5f), bounds.getTopLeft().toFloat(),
        //    Colour(0x800099CC).withAlpha(0.5f), bounds.getBottomRight().toFloat(), false));
        g.setColour(pluginChoiceColour.withAlpha(0.3f));
        g.fillRoundedRectangle(bounds.toFloat(), 6.0f);
    }
    else
    {
        g.setColour(Colour(0x80404040));
        g.fillRoundedRectangle(bounds.toFloat(), 6.0f);
    }

    g.setColour(Colours::white);
    g.setFont(Font(10.0f));
    g.drawText("View Resources", bounds, Justification::centred);


}

void DirectoryOpenButton::setProcessor(FreesoundAdvancedSamplerAudioProcessor* p)
{
    processor = p;
}


