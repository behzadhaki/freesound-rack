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

class BookmarkViewerComponent : public Component,
                               public ScrollBar::Listener,
                               public FreesoundAdvancedSamplerAudioProcessor::PreviewPlaybackListener
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

    // PreviewPlaybackListener implementation
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

    // Sample pads for bookmarks (using unified SamplePad in Preview mode)
    OwnedArray<SamplePad> bookmarkPads;
    std::map<String, SamplePad*> bookmarkPadMap; // Map for quick access by Freesound ID

    // Current bookmark data
    Array<BookmarkInfo> currentBookmarks;

    // Layout management
    int currentScrollOffset = 0;
    int totalRows = 0;

    void updateBookmarkPads();
    void createBookmarkPads();
    void clearBookmarkPads();
    void updateScrollableArea();

    // Helper to find pad by freesound ID
    SamplePad* findPadByFreesoundId(const String& freesoundId);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BookmarkViewerComponent)
};