/*
  ==============================================================================

    SampleGridComponent.h
    Created: New grid component for displaying samples in 4x4 grid
    Author: Generated
    Modified: Added drag-and-drop swap functionality

  ==============================================================================
*/

#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "PluginProcessor.h"

class SamplePad : public Component,
                  public DragAndDropContainer,
                  public DragAndDropTarget
{
public:
    SamplePad(int padIndex);
    ~SamplePad() override;

    void paint(Graphics& g) override;
    void resized() override;
    void mouseDown(const MouseEvent& event) override;
    void mouseDrag(const MouseEvent& event) override;

    // DragAndDropTarget implementation
    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
    void itemDragEnter(const SourceDetails& dragSourceDetails) override;
    void itemDragExit(const SourceDetails& dragSourceDetails) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override;

    void setSample(const File& audioFile, const String& sampleName, const String& author, String freesoundId = String(), String license = String());
    void setPlayheadPosition(float position); // 0.0 to 1.0
    void setIsPlaying(bool playing);
    void setProcessor(FreesoundAdvancedSamplerAudioProcessor* p);

    // NEW: Method to get sample info for drag operations
    struct SampleInfo {
        File audioFile;
        String sampleName;
        String authorName;
        String freesoundId;
        String licenseType;
        bool hasValidSample;
        int padIndex; // Index of the pad (1-based)
    };
    SampleInfo getSampleInfo() const;

    // NEW: Get pad index
    int getPadIndex() const { return padIndex; }

    // NEW: Clear sample data
    void clearSample();

private:
    void loadWaveform();
    void drawWaveform(Graphics& g, Rectangle<int> bounds);
    void drawPlayhead(Graphics& g, Rectangle<int> bounds);
    String getLicenseShortName(const String& license) const;

    int padIndex;
    FreesoundAdvancedSamplerAudioProcessor* processor;

    AudioFormatManager formatManager;
    AudioThumbnailCache audioThumbnailCache;
    std::unique_ptr<AudioFormatReader> audioReader;
    AudioThumbnail audioThumbnail;

    String sampleName;
    String authorName;
    File audioFile;
    String freesoundId;
    String licenseType;

    float playheadPosition;
    bool isPlaying;
    bool hasValidSample;
    bool isDragHover; // NEW: Track drag hover state

    Colour padColour;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplePad)
};

class SampleGridComponent : public Component,
                           public FreesoundAdvancedSamplerAudioProcessor::PlaybackListener,
                           public DragAndDropContainer
{
public:
    SampleGridComponent();
    ~SampleGridComponent() override;

    void paint(Graphics& g) override;
    void resized() override;

    void setProcessor(FreesoundAdvancedSamplerAudioProcessor* p);
    void updateSamples(const Array<FSSound>& sounds, const std::vector<StringArray>& soundInfo);
    void clearSamples();

    // NEW: Method to get all sample info for drag operations
    Array<SamplePad::SampleInfo> getAllSampleInfo() const;

    // NEW: Swap samples between two pads
    void swapSamples(int sourcePadIndex, int targetPadIndex);

    // PlaybackListener implementation
    void noteStarted(int noteNumber, float velocity) override;
    void noteStopped(int noteNumber) override;
    void playheadPositionChanged(int noteNumber, float position) override;

private:
    static constexpr int GRID_SIZE = 4;
    static constexpr int TOTAL_PADS = GRID_SIZE * GRID_SIZE;

    std::array<std::unique_ptr<SamplePad>, TOTAL_PADS> samplePads;
    FreesoundAdvancedSamplerAudioProcessor* processor;

    // Helper methods for loading samples
    void loadSamplesFromJson(const File& metadataFile);
    void loadSamplesFromArrays(const Array<FSSound>& sounds, const std::vector<StringArray>& soundInfo, const File& downloadDir);

    // NEW: Update JSON metadata after swap
    void updateJsonMetadata();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleGridComponent)
};

//==============================================================================
// Drag Export Component
//==============================================================================

class SampleDragArea : public Component,
                      public DragAndDropContainer
{
public:
    SampleDragArea();
    ~SampleDragArea() override;

    void paint(Graphics& g) override;
    void resized() override;
    void mouseDown(const MouseEvent& event) override;
    void mouseDrag(const MouseEvent& event) override;

    void setSampleGridComponent(SampleGridComponent* gridComponent);

private:
    SampleGridComponent* sampleGrid;
    bool isDragging;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleDragArea)
};

//==============================================================================
// Directory Open Button
//==============================================================================

class DirectoryOpenButton : public TextButton
{
public:
    DirectoryOpenButton();
    ~DirectoryOpenButton() override;

    void setProcessor(FreesoundAdvancedSamplerAudioProcessor* processor);

private:
    FreesoundAdvancedSamplerAudioProcessor* processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DirectoryOpenButton)
};