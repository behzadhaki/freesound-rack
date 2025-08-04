/*
  ==============================================================================

    BookmarkViewerComponent.h - Updated with Preview Playback Listener

  ==============================================================================
*/

#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "SampleGridComponent.h"
#include "BookmarkManager.h"
#include "PluginProcessor.h"
#include "CustomButtonStyle.h"

// Add forward declaration
class FreesoundAdvancedSamplerAudioProcessorEditor;

//==============================================================================
// Custom Sample Pad for Bookmarks (with mouse-hold preview and playhead)
//==============================================================================
class BookmarkSamplePad : public SamplePad
{
public:
    BookmarkSamplePad(int index, const BookmarkInfo& bookmarkInfo);
    ~BookmarkSamplePad() override;

    void mouseDown(const MouseEvent& event) override;
    void mouseDrag(const MouseEvent& event) override;
    void mouseUp(const MouseEvent& event) override;
    void mouseEnter(const MouseEvent &event) override;
    void mouseExit(const MouseEvent &event) override;

    // NEW: Preview playback state methods
    void setPreviewPlaying(bool playing);
    void setPreviewPlayheadPosition(float position);
    String getFreesoundId() const { return bookmark.freesoundId; }

protected:
    // Override paint to show preview playhead
    void paint(Graphics& g) override;

private:
    BookmarkInfo bookmark;

    void startPreviewPlayback();
    void stopPreviewPlayback();

    bool isPreviewPlaying = false;
    float previewPlayheadPosition = 0.0f; // NEW: Track preview playhead
    Colour defaultColour = Colours::darkgrey;

    bool mouseDownInWaveform = false;  // Track where mouse went down
    bool previewRequested = false;     // Track if we requested preview

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BookmarkSamplePad)
};

//==============================================================================
// Bookmark Viewer Component - Now implements PreviewPlaybackListener
//==============================================================================
class BookmarkViewerComponent : public Component,
                               public ScrollBar::Listener,
                               public FreesoundAdvancedSamplerAudioProcessor::PreviewPlaybackListener // NEW
{
public:
    BookmarkViewerComponent();
    ~BookmarkViewerComponent() override;

    void paint(Graphics& g) override;
    void resized() override;

    void setProcessor(FreesoundAdvancedSamplerAudioProcessor* p);
    void refreshBookmarks();

    // ScrollBar::Listener
    void scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;

    // NEW: PreviewPlaybackListener implementation
    void previewStarted(const String& freesoundId) override;
    void previewStopped(const String& freesoundId) override;
    void previewPlayheadPositionChanged(const String& freesoundId, float position) override;

private:
    FreesoundAdvancedSamplerAudioProcessor* processor;

    // UI Components
    Label titleLabel;
    StyledButton refreshButton { String(CharPointer_UTF8("\xE2\x9F\xB3")) , 20.0f, false };
    Viewport bookmarkViewport;
    Component bookmarkContainer;

    // Sample pads for bookmarks (non-searchable)
    OwnedArray<BookmarkSamplePad> bookmarkPads;
    std::map<String, BookmarkSamplePad*> bookmarkPadMap; // Map for quick access by Freesound ID

    // Current bookmark data
    Array<BookmarkInfo> currentBookmarks;

    // Layout management
    int currentScrollOffset = 0;
    int totalRows = 0;

    void updateBookmarkPads();
    void createBookmarkPads();
    void clearBookmarkPads();
    void updateScrollableArea();
    void loadBookmarkIntoSampleGrid(const BookmarkInfo& bookmark);

    // NEW: Helper to find pad by freesound ID
    BookmarkSamplePad* findPadByFreesoundId(const String& freesoundId);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BookmarkViewerComponent)
};