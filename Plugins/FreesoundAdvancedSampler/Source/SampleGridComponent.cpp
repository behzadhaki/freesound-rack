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
    , playheadPosition(0.0f)
    , isPlaying(false)
    , hasValidSample(false)
    , isDragHover(false)
{
    formatManager.registerBasicFormats();

    // Generate a unique color for each pad
    float hue = (float)padIndex / 16.0f;
    padColour = Colour::fromHSV(hue, 0.3f, 0.8f, 1.0f);
}

SamplePad::~SamplePad()
{
}

void SamplePad::paint(Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    if (isDragHover)
    {
        g.setColour(Colours::yellow.withAlpha(0.5f));
    }
    else if (isPlaying)
    {
        g.setColour(padColour.brighter(0.3f));
    }
    else if (hasValidSample)
    {
        g.setColour(padColour);
    }
    else
    {
        g.setColour(Colours::darkgrey);
    }

    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    // Border
    if (isDragHover)
    {
        g.setColour(Colours::yellow);
        g.drawRoundedRectangle(bounds.toFloat().reduced(1), 4.0f, 2.0f);
    }
    else
    {
        g.setColour(Colours::white.withAlpha(0.3f));
        g.drawRoundedRectangle(bounds.toFloat().reduced(1), 4.0f, 1.0f);
    }

    if (hasValidSample)
    {
        auto waveformBounds = bounds.reduced(8, 20);

        // Draw waveform
        drawWaveform(g, waveformBounds);

        // Draw playhead if playing
        if (isPlaying)
        {
            drawPlayhead(g, waveformBounds);
        }

        // Draw filename and author in black text at bottom left of waveform area
        g.setColour(Colours::black);
        g.setFont(9.0f);

        // Get the waveform bounds (same as used for drawing waveform)
        auto textBounds = bounds.reduced(8, 20);

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

        // Position at bottom left of waveform area
        auto filenameBounds = textBounds.removeFromBottom(12);
        g.drawText(displayText, filenameBounds, Justification::bottomLeft, true);

        // Draw "Web" badge in TOP LEFT corner
        if (freesoundId.isNotEmpty())
        {
            g.setFont(9.0f);

            // Create web badge in top-left corner
            auto webBounds = bounds.reduced(3);
            int badgeWidth = 30;
            int badgeHeight = 12;
            webBounds = webBounds.removeFromTop(badgeHeight).removeFromLeft(badgeWidth);

            // Draw blue background
            g.setColour(Colours::blue.withAlpha(0.8f));
            g.fillRoundedRectangle(webBounds.toFloat(), 2.0f);

            // Draw white text
            g.setColour(Colours::white);
            g.drawText("Web", webBounds, Justification::centred);
        }

        // Draw license badge in TOP RIGHT corner
        if (licenseType.isNotEmpty())
        {
            g.setFont(9.0f);
            String shortLicense = getLicenseShortName(licenseType);

            // Create license badge in top-right corner
            auto licenseBounds = bounds.reduced(3);
            int badgeWidth = 35;
            int badgeHeight = 12;
            licenseBounds = licenseBounds.removeFromTop(badgeHeight).removeFromRight(badgeWidth);

            // Draw orange background
            g.setColour(Colours::orange.withAlpha(0.9f));
            g.fillRoundedRectangle(licenseBounds.toFloat(), 2.0f);

            // Draw black text
            g.setColour(Colours::black);
            g.drawText(shortLicense, licenseBounds, Justification::centred);
        }

        // Draw "Drag" badge in BOTTOM LEFT corner
        {
            g.setFont(8.0f);

            // Create drag badge in bottom-left corner
            auto dragBounds = bounds.reduced(3);
            int badgeWidth = 30;
            int badgeHeight = 12;
            dragBounds = dragBounds.removeFromBottom(badgeHeight).removeFromLeft(badgeWidth);

            // Draw green background
            g.setColour(Colours::green.withAlpha(0.8f));
            g.fillRoundedRectangle(dragBounds.toFloat(), 2.0f);

            // Draw white text
            g.setColour(Colours::white);
            g.drawText("Drag", dragBounds, Justification::centred);
        }
    }
    else
    {
        // Empty pad
        g.setColour(Colours::white.withAlpha(0.5f));
        g.setFont(12.0f);
        g.drawText("Empty", bounds, Justification::centred);
    }

    // Pad number in top-left corner (over the web badge if present)
    g.setColour(Colours::white.withAlpha(0.9f));
    g.setFont(8.0f);
    auto numberBounds = bounds.reduced(2);
    auto numberRect = numberBounds.removeFromTop(10).removeFromLeft(15);
    g.drawText(String(padIndex + 1), numberRect, Justification::centred);
}

void SamplePad::resized()
{
}

void SamplePad::mouseDown(const MouseEvent& event)
{
    if (!hasValidSample)
        return;

    // Check if clicked on web badge (top-left)
    if (freesoundId.isNotEmpty())
    {
        auto bounds = getLocalBounds();
        auto webBounds = bounds.reduced(3);
        int badgeWidth = 30;
        int badgeHeight = 12;
        webBounds = webBounds.removeFromTop(badgeHeight).removeFromLeft(badgeWidth);

        if (webBounds.contains(event.getPosition()))
        {
            // Open Freesound page in browser
            String freesoundUrl = "https://freesound.org/s/" + freesoundId + "/";
            URL(freesoundUrl).launchInDefaultBrowser();
            return;
        }
    }

    // Check if clicked on license badge (top-right)
    if (licenseType.isNotEmpty())
    {
        auto bounds = getLocalBounds();
        auto licenseBounds = bounds.reduced(3);
        int badgeWidth = 35;
        int badgeHeight = 12;
        licenseBounds = licenseBounds.removeFromTop(badgeHeight).removeFromRight(badgeWidth);

        if (licenseBounds.contains(event.getPosition()))
        {
            // Open the actual Creative Commons license URL
            URL(licenseType).launchInDefaultBrowser();
            return;
        }
    }

    // Check if clicked on drag badge (bottom-left) - for file export
    {
        auto bounds = getLocalBounds();
        auto dragBounds = bounds.reduced(3);
        int badgeWidth = 30;
        int badgeHeight = 12;
        dragBounds = dragBounds.removeFromBottom(badgeHeight).removeFromLeft(badgeWidth);

        if (dragBounds.contains(event.getPosition()))
        {
            // Store that we clicked on drag badge for mouseDrag to handle file export
            return;
        }
    }

    // Check if clicked in waveform area - for manual triggering
    auto bounds = getLocalBounds();
    auto waveformBounds = bounds.reduced(8, 20);

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

void SamplePad::setSample(const File& file, const String& name, const String& author, String fsId, String license)
{
    audioFile = file;
    sampleName = name;
    authorName = author;
    freesoundId = fsId;
    licenseType = license;

    hasValidSample = audioFile.existsAsFile();

    // Always load waveform when setting a sample
    if (hasValidSample)
    {
        loadWaveform();
    }
    else
    {
        audioThumbnail.clear();
    }

    repaint();
}

void SamplePad::clearSample()
{
    audioFile = File();
    sampleName = "";
    authorName = "";
    freesoundId = String();
    licenseType = String();
    hasValidSample = false;
    isPlaying = false;
    playheadPosition = 0.0f;
    audioThumbnail.clear();
    repaint();
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

SamplePad::SampleInfo SamplePad::getSampleInfo() const
{
    SampleInfo info;
    info.audioFile = audioFile;
    info.sampleName = sampleName;
    info.authorName = authorName;
    info.freesoundId = freesoundId;
    info.licenseType = licenseType;
    info.hasValidSample = hasValidSample;
    info.padIndex = 0; // Will be set by SampleGridComponent
    return info;
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
}

SampleGridComponent::~SampleGridComponent()
{
    if (processor)
    {
        processor->removePlaybackListener(this);
    }
}

void SampleGridComponent::paint(Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
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

    // Position shuffle button in bottom left
    auto shuffleBounds = bottomArea.reduced(padding);
    shuffleButton.setBounds(shuffleBounds.removeFromLeft(buttonWidth));

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

    // Force a repaint of all pads after a brief delay to ensure waveforms are loaded
    Timer::callAfterDelay(200, [this]() {
        for (auto& pad : samplePads)
        {
            pad->repaint();
        }
    });

    DBG("Grid update complete");
}

void SampleGridComponent::clearSamples()
{
    for (auto& pad : samplePads)
    {
        pad->clearSample();
    }
}

void SampleGridComponent::swapSamples(int sourcePadIndex, int targetPadIndex)
{
    if (sourcePadIndex < 0 || sourcePadIndex >= TOTAL_PADS ||
        targetPadIndex < 0 || targetPadIndex >= TOTAL_PADS ||
        sourcePadIndex == targetPadIndex)
    {
        return;
    }

    // Get sample info from both pads
    auto sourcePad = samplePads[sourcePadIndex].get();
    auto targetPad = samplePads[targetPadIndex].get();

    auto sourceInfo = sourcePad->getSampleInfo();
    auto targetInfo = targetPad->getSampleInfo();

    // Swap the samples in the visual grid
    if (targetInfo.hasValidSample)
    {
        sourcePad->setSample(targetInfo.audioFile, targetInfo.sampleName,
                           targetInfo.authorName, targetInfo.freesoundId, targetInfo.licenseType);
    }
    else
    {
        sourcePad->clearSample();
    }

    if (sourceInfo.hasValidSample)
    {
        targetPad->setSample(sourceInfo.audioFile, sourceInfo.sampleName,
                           sourceInfo.authorName, sourceInfo.freesoundId, sourceInfo.licenseType);
    }
    else
    {
        targetPad->clearSample();
    }

    // Update the processor's internal arrays to match the new visual order
    updateProcessorArraysFromGrid();

    // Update JSON metadata
    updateJsonMetadata();

    // Reload the sampler with new pad order
    if (processor)
    {
        processor->setSources();
        // Also update the README file to reflect new order
        processor->updateReadmeFile();
    }
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
            // Add pad index to the info
            info.padIndex = i + 1; // 1-based index for user-friendly numbering
            allSamples.add(info);
        }
    }

    return allSamples;
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

    // Background
    if (isDragging)
    {
        g.setColour(Colours::blue.withAlpha(0.3f));
    }
    else
    {
        g.setColour(Colours::grey.withAlpha(0.2f));
    }
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    // Border
    g.setColour(Colours::white.withAlpha(0.5f));
    g.drawRoundedRectangle(bounds.toFloat().reduced(1), 4.0f, 1.0f);

    // Icon and text
    g.setColour(Colours::white.withAlpha(0.8f));
    g.setFont(10.0f);

    // Draw a simple drag icon (three horizontal lines)
    auto iconBounds = bounds.removeFromLeft(20);
    int lineY = iconBounds.getCentreY() - 6;
    for (int i = 0; i < 3; ++i)
    {
        g.fillRect(iconBounds.getX() + 4, lineY + i * 4, 12, 2);
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
    setButtonText("View\nFiles");
    setSize(60, 40);

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

void DirectoryOpenButton::setProcessor(FreesoundAdvancedSamplerAudioProcessor* p)
{
    processor = p;
}