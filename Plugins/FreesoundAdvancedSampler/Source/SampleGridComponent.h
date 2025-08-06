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
#include "CustomButtonStyle.h"
#include "MasterSearchPanel.h"
#include "FreesoundSearchUtils.h"

static const String FREESOUND_SAMPLER_MIME_TYPE = "application/x-freesound-sampler-data"; // for inter plugin drag and drop

//==============================================================================
enum class SvgFitMode {
    FillEntireBadge,    // No padding, fills entire badge
    FillWithPadding,    // Small padding, recommended
    FitProportionally,  // Maintains aspect ratio
    Stretch             // May distort SVG
};

struct Badge {
    String id;
    String text;
    String icon;
    String svgContent;  // SVG content as string
    std::unique_ptr<juce::Drawable> svgDrawable;  // Parsed SVG drawable
    Colour backgroundColour;
    Colour textColour;
    std::function<void(const MouseEvent&)> onClick;
    std::function<void()> onDoubleClick;
    std::function<void(const MouseEvent&)> onDrag;
    bool visible = true;
    int width = 16;
    int height = 14;
    float fontSize = 11.0f;
    bool isBold = true;
    Rectangle<int> bounds;

    // Default constructor
    Badge(const String& badgeId, const String& badgeText, Colour bgColor = Colour(0x80404040))
        : id(badgeId), text(badgeText), backgroundColour(bgColor), textColour(Colours::white) {}

    // SVG constructor
    Badge(const String& badgeId, const String& svgString, Colour bgColor, bool /*isSvg*/)
        : id(badgeId), svgContent(svgString), backgroundColour(bgColor), textColour(Colours::white)
    {
        if (svgContent.isNotEmpty()) {
            createSvgDrawable();
        }
    }

    // DELETE copy constructor and assignment operator
    Badge(const Badge&) = delete;
    Badge& operator=(const Badge&) = delete;

    // DEFAULT move constructor and assignment operator
    Badge(Badge&&) = default;
    Badge& operator=(Badge&&) = default;

    // Method to create drawable from SVG string
    void createSvgDrawable() {
        if (svgContent.isNotEmpty()) {
            auto xmlDoc = std::unique_ptr<XmlElement>(XmlDocument::parse(svgContent));
            if (xmlDoc) {
                svgDrawable = Drawable::createFromSVG(*xmlDoc);
            }
        }
    }

    // Check if this badge uses SVG
    bool usesSvg() const { return svgDrawable != nullptr; }
};

//==============================================================================
// SamplePad Component (unified implementation with preview mode)
class SamplePad : public Component,
                  public DragAndDropContainer,
                  public DragAndDropTarget,
                  public Timer
{
public:
    enum class PadMode {
        Normal,     // Standard searchable pad (default)
        NonSearchable, // Non-searchable pad (for presets/readonly)
        Preview     // Preview mode (for bookmarks with mouse-hold preview)
    };

    SamplePad(int index, PadMode mode = PadMode::Normal);
    ~SamplePad() override;

    void paint(Graphics& g) override;
    void resized() override;
    void mouseDown(const MouseEvent& event) override;
    void mouseUp(const MouseEvent& event) override;
    void mouseDrag(const MouseEvent& event) override;
    void mouseDoubleClick(const MouseEvent& event) override;
    void mouseEnter(const MouseEvent& event) override;
    void mouseExit(const MouseEvent& event) override;
    void timerCallback() override;

    // DragAndDropTarget implementation
    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
    void itemDragEnter(const SourceDetails& dragSourceDetails) override;
    void itemDragExit(const SourceDetails& dragSourceDetails) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override;

    void setSample(const File& audioFile, const String& sampleName, const String& author,
                   String freesoundId = String(), String license = String(), String query = String(),
                   String fsTags = String(), String fsDescription = String());
    void setPlayheadPosition(float position);
    void setIsPlaying(bool playing);
    void setProcessor(FreesoundAdvancedSamplerAudioProcessor* p);

    // Query management
    void setQuery(const String& query, bool dontUpdatePadInfoQuery = false);
    String getQuery() const;
    bool hasQuery() const;

    // Master search connection
    void setConnectedToMaster(bool connected);
    bool isConnectedToMaster() const { return connectedToMaster; }
    void syncMasterQuery(const String& masterQuery);

    // Progress bar methods (thread-safe)
    void startDownloadProgress();
    void updateDownloadProgress(double progress, const String& status = String());
    void finishDownloadProgress(bool success, const String& message = String());

    // Preview mode methods
    void setPreviewPlaying(bool playing);
    void setPreviewPlayheadPosition(float position);
    String getFreesoundId() const { return freesoundId; }

    // Sample info struct and method
    struct SampleInfo {
        File audioFile;
        String sampleName;
        String authorName;
        String freesoundId;
        String licenseType;
        String query;
        String tags;
        String description;
        float fileSourceSampleRate = 44100.0f;
        bool hasValidSample;
        int padIndex;
    };
    SampleInfo getSampleInfo() const;

    int getPadIndex() const { return padIndex; }
    int setPadIndex(int index) { padIndex = index; return padIndex; }

    // Mode queries
    bool isSearchable() const { return padMode == PadMode::Normal; }
    bool isPreviewMode() const { return padMode == PadMode::Preview; }
    PadMode getPadMode() const { return padMode; }
    void setPadMode(PadMode mode) { padMode = mode; }

    // Callbacks for badges
    void clearSample();
    void handleDeleteClick();
    void performEnhancedDragDrop();
    void handleCopyClick();

    void refreshStyleFromPadIndex();

    FreesoundAdvancedSamplerAudioProcessor* processor;

protected:
    // badges for various actions
    std::vector<Badge> topLeftBadges;
    std::vector<Badge> topRightBadges;
    std::vector<Badge> bottomLeftBadges;

    void initializeBadges();
    void layoutBadges();
    void paintBadges(Graphics& g);
    Badge* findBadgeAtPosition(Point<int> position);
    void updateBadgeVisibility();

    // Waveform loading and drawing
    void loadWaveform();
    void drawWaveform(Graphics& g, Rectangle<int> bounds);
    void drawPlayhead(Graphics& g, Rectangle<int> bounds);
    void drawPreviewPlayhead(Graphics& g, Rectangle<int> bounds); // NEW: Preview playhead
    String getLicenseShortName(const String& license) const;
    void cleanupProgressComponents();

    // Preview mode methods
    void startPreviewPlayback();
    void stopPreviewPlayback();

    PadMode padMode;
    int padIndex;

    AudioFormatManager formatManager;
    AudioThumbnailCache audioThumbnailCache;
    std::unique_ptr<AudioFormatReader> audioReader;
    AudioThumbnail audioThumbnail;

    String sampleName;
    String authorName;
    File audioFile;
    String freesoundId;
    String licenseType;
    String padQuery;
    String tags;
    String description;
    float fileSourceSampleRate = 44100.0f;
    float processorSampleRate = 44100.0f;
    String getKeyboardKeyForPad(int padIndex) const;

    // Playback state
    float playheadPosition;
    bool isPlaying;
    bool hasValidSample;
    bool isDragHover;
    bool isCopyDragHover = false;
    bool connectedToMaster = false;

    // Preview state (only used in preview mode)
    bool isPreviewPlaying = false;
    float previewPlayheadPosition = 0.0f;
    bool mouseDownInWaveform = false;
    bool previewRequested = false;

    Colour padColour;
    Colour defaultColour = Colours::darkgrey; // For preview mode
    TextEditor queryTextBox;

    // Progress bar components
    std::unique_ptr<ProgressBar> progressBar;
    std::unique_ptr<Label> progressLabel;
    bool isDownloading = false;
    bool pendingCleanup = false;
    double currentDownloadProgress = 0.0;

    File getWavFile() const;
    bool convertOggToWav(const File& oggFile, const File& wavFile);
    void handleWavCopyClick();
    void handleBookmarkClick();

    void performCrossAppDragDrop();
    void performInternalDragDrop();

    void showDetailedSampleInfo();

    void drawSvgBadge(Graphics& g, const Badge& badge, SvgFitMode fitMode = SvgFitMode::FillWithPadding, int reducedBounds = 0);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplePad)
};

//==============================================================================
// SampleGridComponent (the grid of sample pads)
//==============================================================================
class SampleGridComponent : public Component,
                            public FreesoundAdvancedSamplerAudioProcessor::PlaybackListener,
                            public DragAndDropContainer,
                            public DragAndDropTarget,      // for drag and drop between pads or different instances of same VST
                            public FileDragAndDropTarget,  // Add for external files between different targets or compatible apps
                            public Timer
{
public:
    SampleGridComponent();
    ~SampleGridComponent() override;

    void paint(Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    void mouseDown(const MouseEvent& event) override;

    void setProcessor(FreesoundAdvancedSamplerAudioProcessor* p);
    void updateSamples(const Array<FSSound>& sounds, const std::vector<StringArray>& soundInfo);
    void clearSamples();
    SamplePad::SampleInfo getPadInfo(int padIndex) const;

    Array<SamplePad::SampleInfo> getAllSampleInfo() const;
    void swapSamples(int sourcePadIndex, int targetPadIndex);
    void shuffleSamples();

    void handleMasterSearch(const Array<FSSound>& sounds,
        const std::vector<StringArray>& soundInfo, const String& masterQuery);
    void searchForSinglePadWithQuery(int padIndex, const String& query);

    // Master search integration
    void setPositionConnectedToMaster(int row, int col, bool connected);
    bool isPositionConnectedToMaster(int row, int col) const;
    void syncMasterQueryToPosition(int row, int col, const String& masterQuery);
    void searchSelectedPositions(const String& masterQuery);
    void performSinglePadSearch(int padIndex, const String& query);

    // PlaybackListener implementation
    void noteStarted(int noteNumber, float velocity) override;
    void noteStopped(int noteNumber) override;
    void playheadPositionChanged(int noteNumber, float position) override;

    void updateJsonMetadata();

    String getQueryForPad(int padIndex) const;
    void setQueryForPad(int padIndex, const String& query);

    // Add DragAndDropTarget methods (for drag and drop between pads or different instances of same VST)
    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
    void itemDragEnter(const SourceDetails& dragSourceDetails) override;
    void itemDragExit(const SourceDetails& dragSourceDetails) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override;
    void handleEnhancedDrop(const String& jsonMetadata, const StringArray& filePaths, int targetPadIndex);

    // Add file drop methods (cross-platform / cross-app drag and drop)
    bool isInterestedInFileDrag(const StringArray& files) override;
    void filesDropped(const StringArray& files, int x, int y) override;
    void fileDragEnter(const StringArray& files, int x, int y) override;
    void fileDragExit(const StringArray& files) override;

    // Master search management
    void populatePadsFromMasterSearch();
    void clearPendingMasterSearchState();
    bool hasPendingMasterSearch() const;

    MasterSearchPanel& getMasterSearchPanel() { return masterSearchPanel; }
    FreesoundAdvancedSamplerAudioProcessor* getProcessor() const { return processor; }

    static constexpr int GRID_SIZE = 4;
    static constexpr int TOTAL_PADS = GRID_SIZE * GRID_SIZE;
    std::array<std::unique_ptr<SamplePad>, TOTAL_PADS> samplePads;

    void refreshFromProcessor();
    void clearSampleFromPad(int padIndex);
    void updatePadFromCollection(int padIndex, const String& freesoundId);

private:

    FreesoundAdvancedSamplerAudioProcessor* processor;

    // Master search system (position-based, not pad-index based)
    std::array<bool, TOTAL_PADS> masterSearchConnections; // tracks visual positions (0-15)
    MasterSearchPanel masterSearchPanel;

    // Single pad download management (simplified)
    std::unique_ptr<AudioDownloadManager> singlePadDownloadManager;
    int currentDownloadPadIndex = -1;
    FSSound currentDownloadSound;
    String currentDownloadQuery;

    // Helper methods
    void loadSamplesFromJson(const File& metadataFile);
    void loadSamplesFromArrays(const Array<FSSound>& sounds,
        const std::vector<StringArray>& soundInfo, const File& downloadDir);
    void loadSingleSampleWithQuery(int padIndex,
        const FSSound& sound, const File& audioFile, const String& query);
    void downloadSingleSampleWithQuery(int padIndex, const FSSound& sound, const String& query);
    void updateProcessorArraysFromGrid();
    Array<FSSound> searchSingleSound(const String& query);
    void loadSingleSample(int padIndex, const FSSound& sound, const File& audioFile);
    void downloadSingleSample(int padIndex, const FSSound& sound);
    void updateSinglePadInProcessor(int padIndex, const FSSound& sound);
    void cleanupSingleDownload();  // Add cleanup method

    // Position conversion helpers
    int getVisualPositionFromRowCol(int row, int col) const;
    void getRowColFromVisualPosition(int position, int& row, int& col) const;
    void getRowColFromPadIndex(int padIndex, int& row, int& col) const;

    // Master search helpers
    void updatePadConnectionStates();
    bool hasAnySamplesInConnectedPositions() const;
    void handleMasterSearchSelected();
    void performMasterSearch(const String& masterQuery);

    StyledButton shuffleButton {"Shuffle", 10.0f, false};
    StyledButton clearAllButton {"Clear All", 10.0f, true};
    void clearAllPads();

    bool isEnhancedDragHover = false;
    int dragHoverPadIndex = -1;

    void performEnhancedDrop(const String& jsonMetadata, const StringArray& filePaths, int targetPadIndex); // NEW helper method
    void handleFileDrop(const StringArray& filePaths, int targetPadIndex);
    int getPadIndexFromPosition(Point<int> position);
    void handleCrossAppDrop(const StringArray& files, int targetPadIndex);

    // Master search
    void updateProcessorArraysForMasterSearch(const Array<FSSound>& sounds,
    const std::vector<StringArray>& soundInfo, const Array<int>& targetPads, const String& masterQuery);
    void executeMasterSearch(const String& masterQuery, const Array<int>& targetPadIndices);

    Array<int> pendingMasterSearchPads;
    String pendingMasterSearchQuery;
    Array<FSSound> pendingMasterSearchSounds;
    std::vector<StringArray> pendingMasterSearchSoundInfo;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleGridComponent)
};

//==============================================================================
// Drag Export Component (button for exporting ALL samples via drag-and-drop)
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
// Directory Open Button (for opening the resources directory containing samples
// and presets)
//==============================================================================

class DirectoryOpenButton : public TextButton
{
public:
    DirectoryOpenButton();
    ~DirectoryOpenButton() override;
    void paint(Graphics &g) override;
    void setProcessor(FreesoundAdvancedSamplerAudioProcessor* processor);

private:
    FreesoundAdvancedSamplerAudioProcessor* processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DirectoryOpenButton)
};