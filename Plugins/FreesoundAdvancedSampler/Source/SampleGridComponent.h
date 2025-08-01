/*
  ==============================================================================

    SampleGridComponent.h
    Created: New grid component for displaying samples in 4x4 grid
    Author: Generated
    Modified: Added drag-and-drop swap functionality and shuffle button

  ==============================================================================
*/

#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "PluginProcessor.h"
#include "FreesoundKeys.h"
#include <random>

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

    void setSample(const File& audioFile, const String& sampleName, const String& author, String freesoundId = String(), String license = String(), String query = String());
    void setPlayheadPosition(float position); // 0.0 to 1.0
    void setIsPlaying(bool playing);
    void setProcessor(FreesoundAdvancedSamplerAudioProcessor* p);

    // Query management
    void setQuery(const String& query);
    String getQuery() const;
    bool hasQuery() const;

    // Method to get sample info for drag operations
    struct SampleInfo {
        File audioFile;
        String sampleName;
        String authorName;
        String freesoundId;
        String licenseType;
        String query; // Add query to sample info
        bool hasValidSample;
        int padIndex; // Index of the pad (1-based)
    };
    SampleInfo getSampleInfo() const;

    // Get pad index
    int getPadIndex() const { return padIndex; }
    int setPadIndex(int index) { padIndex = index; return padIndex; }

    // Clear sample data
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
    String padQuery; // Individual query for this pad

    float playheadPosition;
    bool isPlaying;
    bool hasValidSample;
    bool isDragHover; // Track drag hover state

    Colour padColour;

    // NEW: Individual query text box
    TextEditor queryTextBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplePad)
};

class SampleGridComponent : public Component,
                           public FreesoundAdvancedSamplerAudioProcessor::PlaybackListener,
                           public DragAndDropContainer,
                           public Timer  // Add Timer inheritance for download checking
{
public:
    SampleGridComponent();
    ~SampleGridComponent() override;

    void paint(Graphics& g) override;
    void resized() override;

    void setProcessor(FreesoundAdvancedSamplerAudioProcessor* p);
    void updateSamples(const Array<FSSound>& sounds, const std::vector<StringArray>& soundInfo);
    void clearSamples();
    SamplePad::SampleInfo getPadInfo(int padIndex) const;

    // Method to get all sample info for drag operations
    Array<SamplePad::SampleInfo> getAllSampleInfo() const;

    // Swap samples between two pads
    void swapSamples(int sourcePadIndex, int targetPadIndex);

    // Shuffle samples randomly
    void shuffleSamples();

    // NEW: Master search that only affects pads with empty queries
    void handleMasterSearch(const Array<FSSound>& sounds, const std::vector<StringArray>& soundInfo, const String& masterQuery);

    // NEW: Single pad search with specific query
    void searchForSinglePadWithQuery(int padIndex, const String& query);

    // PlaybackListener implementation
    void noteStarted(int noteNumber, float velocity) override;
    void noteStopped(int noteNumber) override;
    void playheadPositionChanged(int noteNumber, float position) override;

    // Timer callback for download checking
    void timerCallback() override;

    // used for keyboard focus (playing samples with keyboard)
    void mouseDown(const MouseEvent& event) override;

private:
    static constexpr int GRID_SIZE = 4;
    static constexpr int TOTAL_PADS = GRID_SIZE * GRID_SIZE;

    std::array<std::unique_ptr<SamplePad>, TOTAL_PADS> samplePads;
    FreesoundAdvancedSamplerAudioProcessor* processor;

    // Shuffle button
    TextButton shuffleButton;

    // NEW: Single pad download management
    std::unique_ptr<AudioDownloadManager> singlePadDownloadManager;
    int currentDownloadPadIndex = -1;
    FSSound currentDownloadSound;
    String currentDownloadQuery;

    // Helper methods for loading samples
    void loadSamplesFromJson(const File& metadataFile);
    void loadSamplesFromArrays(const Array<FSSound>& sounds, const std::vector<StringArray>& soundInfo, const File& downloadDir);
    void loadSingleSampleWithQuery(int padIndex, const FSSound& sound, const File& audioFile, const String& query);
    void downloadSingleSampleWithQuery(int padIndex, const FSSound& sound, const String& query);

    // Update JSON metadata after swap
    void updateJsonMetadata();

    // Update processor arrays from current grid state
    void updateProcessorArraysFromGrid();

    // NEW: Single pad search helper methods
    Array<FSSound> searchSingleSound(const String& query);
    void loadSingleSample(int padIndex, const FSSound& sound, const File& audioFile);
    void downloadSingleSample(int padIndex, const FSSound& sound);
    void updateSinglePadInProcessor(int padIndex, const FSSound& sound);

    // clear all pads button and methods
    TextButton clearAllButton;  // Add this line
    void clearAllPads();

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