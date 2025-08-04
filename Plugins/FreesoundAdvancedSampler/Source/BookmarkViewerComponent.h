/*
  ==============================================================================

    BookmarkViewerComponent.h
    Created: Bookmark viewer component with scrollable sample pads
    Author: Generated

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
// Custom Sample Pad for Bookmarks (with mouse-hold preview)
//==============================================================================
class BookmarkSamplePad : public SamplePad
{
public:
    BookmarkSamplePad(int index, const BookmarkInfo& bookmarkInfo);
    ~BookmarkSamplePad() override;

    // ADD THESE MOUSE METHODS:
    void mouseDown(const MouseEvent& event) override;
    void mouseDrag(const MouseEvent& event) override;
    void mouseUp(const MouseEvent& event) override;

private:
    BookmarkInfo bookmark;

    // ADD THESE PREVIEW CONTROL METHODS:
    void startPreviewPlayback();
    void stopPreviewPlayback();

    // ADD THIS STATE VARIABLE:
    bool isPreviewPlaying = false;
    Colour defaultColour = Colours::darkgrey;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BookmarkSamplePad)
};

//==============================================================================
// Bookmark Viewer Component
//==============================================================================
class BookmarkViewerComponent : public Component,
                               public ScrollBar::Listener
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

private:
    FreesoundAdvancedSamplerAudioProcessor* processor;

    // UI Components
    Label titleLabel;
    StyledButton refreshButton { String(CharPointer_UTF8("\xE2\x9F\xB3")) , 20.0f, false };
    Viewport bookmarkViewport;
    Component bookmarkContainer;

    // Sample pads for bookmarks (non-searchable)
    OwnedArray<BookmarkSamplePad> bookmarkPads;

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BookmarkViewerComponent)
};