/*
 ==============================================================================

   BookmarkViewerComponent.h - Updated with Collection Manager Integration
   and Search Functionality with Bookmarks/Local Sounds Toggle

 ==============================================================================
*/

#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "SampleGridComponent.h"
#include "SampleCollectionManager.h"
#include "PluginProcessor.h"
#include "CustomButtonStyle.h"
#include "CustomLookAndFeel.h"

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

   // NEW: Toggle Button for Bookmarks vs Local Sounds
   class ToggleButton : public Component
   {
   public:
       ToggleButton();
       void paint(Graphics& g) override;
       void resized() override;
       void mouseUp(const MouseEvent& event) override;

       std::function<void(bool)> onToggleChanged;
       bool isBookmarksMode() const { return bookmarksActive; }
       void setBookmarksMode(bool shouldBeBookmarks);

   private:
       bool bookmarksActive = true; // Default to bookmarks mode
       Rectangle<float> leftBounds, rightBounds;
       String leftText = "Bookmarks";
       String rightText = "Local Sounds";
   };

   ToggleButton modeToggle;

   // Search Panel Components
   TextEditor searchBox;
   StyledButton clearSearchButton { String(CharPointer_UTF8("\xE2\x9C\x96")), 12.0f, false };
   Label resultCountLabel;

   Viewport bookmarkViewport;
   Component bookmarkContainer;

   // Sample pads for bookmarks (using unified SamplePad in Preview mode)
   OwnedArray<SamplePad> bookmarkPads;
   std::map<String, SamplePad*> bookmarkPadMap; // Map for quick access by Freesound ID

   // Current data sets
   Array<SampleMetadata> currentBookmarks;
   Array<SampleMetadata> allLocalSamples; // NEW: All local samples
   Array<SampleMetadata> filteredSamples; // NEW: Filtered results based on search and mode

   // Search state
   String currentSearchTerm;
   bool isBookmarksMode = true; // NEW: Current mode state

   // Layout management
   int currentScrollOffset = 0;
   int totalRows = 0;

   void updateSamplePads();
   void createSamplePads();
   void clearSamplePads();
   void updateScrollableArea();

   // Search and mode functionality
   void performSearch();
   void clearSearch();
   void onModeToggled(bool bookmarksMode);
   void refreshAllSamples(); // NEW: Refresh all local samples
   bool matchesSearchTerm(const SampleMetadata& sample, const String& searchTerm) const;
   void updateResultCount();
   void setupSearchComponents();

   // Helper to find pad by freesound ID
   SamplePad* findPadByFreesoundId(const String& freesoundId);

   JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BookmarkViewerComponent)
};