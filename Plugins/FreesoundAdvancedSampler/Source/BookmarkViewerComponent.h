/*
 ==============================================================================

   BookmarkViewerComponent.h - Updated with Collection Manager Integration
   and Search Functionality

 ==============================================================================
*/

#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "SampleGridComponent.h"
#include "SampleCollectionManager.h"
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

   // NEW: Search Panel Components
   TextEditor searchBox;
   StyledButton clearSearchButton { String(CharPointer_UTF8("\xE2\x9C\x96")), 12.0f, false };
   Label resultCountLabel;

   Viewport bookmarkViewport;
   Component bookmarkContainer;

   // Sample pads for bookmarks (using unified SamplePad in Preview mode)
   OwnedArray<SamplePad> bookmarkPads;
   std::map<String, SamplePad*> bookmarkPadMap; // Map for quick access by Freesound ID

   // Current bookmark data (for UI compatibility)
   Array<SampleMetadata> currentBookmarks;
   Array<SampleMetadata> filteredBookmarks; // NEW: Filtered results based on search

   // NEW: Search state
   String currentSearchTerm;

   // Layout management
   int currentScrollOffset = 0;
   int totalRows = 0;

   void updateBookmarkPads();
   void createBookmarkPads();
   void clearBookmarkPads();
   void updateScrollableArea();

   // NEW: Search functionality
   void performSearch();
   void clearSearch();
   bool matchesSearchTerm(const SampleMetadata& sample, const String& searchTerm) const;
   void updateResultCount();
   void setupSearchComponents();

   // Helper to find pad by freesound ID
   SamplePad* findPadByFreesoundId(const String& freesoundId);

   JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BookmarkViewerComponent)
};