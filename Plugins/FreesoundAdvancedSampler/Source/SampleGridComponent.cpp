/*
  ==============================================================================

    SampleGridComponent.cpp
    Created: New grid component for displaying samples in 4x4 grid
    Author: Generated

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
    , playheadPosition(0.0f)
    , isPlaying(false)
    , hasValidSample(false)
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
    if (isPlaying)
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
    g.setColour(Colours::white.withAlpha(0.3f));
    g.drawRoundedRectangle(bounds.toFloat().reduced(1), 4.0f, 1.0f);

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

        // Draw text labels
        g.setColour(Colours::white);
        g.setFont(10.0f);

        auto textBounds = bounds.reduced(4);
        g.drawText(sampleName, textBounds.removeFromTop(12), Justification::centredTop, true);
        g.drawText(authorName, textBounds.removeFromBottom(12), Justification::centredBottom, true);
    }
    else
    {
        // Empty pad
        g.setColour(Colours::white.withAlpha(0.5f));
        g.setFont(12.0f);
        g.drawText("Empty", bounds, Justification::centred);
    }

    // Pad number
    g.setColour(Colours::white.withAlpha(0.7f));
    g.setFont(8.0f);
    g.drawText(String(padIndex + 1), bounds.reduced(2), Justification::topLeft);
}

void SamplePad::resized()
{
}

void SamplePad::mouseDown(const MouseEvent& event)
{
    if (!hasValidSample)
        return;

    if (event.mods.isShiftDown() && freesoundId.isNotEmpty())
    {
        // Open Freesound page in browser
        String freesoundUrl = "https://freesound.org/s/" + freesoundId + "/";
        URL(freesoundUrl).launchInDefaultBrowser();
    }
    else if (processor)
    {
        // Play the sample
        int noteNumber = padIndex * 8;
        processor->addToMidiBuffer(noteNumber);
    }
}

void SamplePad::setSample(const File& file, const String& name, const String& author, String fsId)
{
    audioFile = file;
    sampleName = name;
    authorName = author;
    freesoundId = fsId;

    loadWaveform();
    hasValidSample = audioFile.existsAsFile();
    repaint();
}

void SamplePad::loadWaveform()
{
    if (audioFile.existsAsFile())
    {
        audioThumbnail.setSource(new FileInputSource(audioFile));
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
    clearSamples();

    if (!processor)
        return;

    File downloadDir = processor->tmpDownloadLocation;

    int numSamples = jmin(TOTAL_PADS, sounds.size());

    for (int i = 0; i < numSamples; ++i)
    {
        // Create the expected filename based on the sound ID (matching AudioDownloadManager)
        String expectedFilename = "FS_ID_" + sounds[i].id + ".ogg";
        File audioFile = downloadDir.getChildFile(expectedFilename);

        // Only proceed if the file exists
        if (audioFile.existsAsFile())
        {
            String sampleName = (i < soundInfo.size()) ? soundInfo[i][0] : "Sample " + String(i + 1);
            String authorName = (i < soundInfo.size()) ? soundInfo[i][1] : "Unknown";
            String freesoundId = sounds[i].id;

            samplePads[i]->setSample(audioFile, sampleName, authorName, freesoundId);
        }
    }
}

void SampleGridComponent::clearSamples()
{
    for (auto& pad : samplePads)
    {
        pad->setSample(File(), "", "", String());
    }
}

void SampleGridComponent::noteStarted(int noteNumber, float velocity)
{
    int padIndex = noteNumber / 8; // Each pad gets 8 MIDI notes
    if (padIndex >= 0 && padIndex < TOTAL_PADS)
    {
        // These methods now handle message thread dispatching internally
        samplePads[padIndex]->setIsPlaying(true);
        samplePads[padIndex]->setPlayheadPosition(0.0f);
    }
}

void SampleGridComponent::noteStopped(int noteNumber)
{
    int padIndex = noteNumber / 8;
    if (padIndex >= 0 && padIndex < TOTAL_PADS)
    {
        // This method now handles message thread dispatching internally
        samplePads[padIndex]->setIsPlaying(false);
    }
}

void SampleGridComponent::playheadPositionChanged(int noteNumber, float position)
{
    int padIndex = noteNumber / 8;
    if (padIndex >= 0 && padIndex < TOTAL_PADS)
    {
        // This method now handles message thread dispatching internally
        samplePads[padIndex]->setPlayheadPosition(position);
    }
}