/*
  ==============================================================================

    ExpandablePanelComponent.h
    Created: Expandable panel with Search and Bookmarks tabs
    Author: Generated

  ==============================================================================
*/

#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "PluginProcessor.h"
#include "CustomButtonStyle.h"

//==============================================================================
// Tab Button Component
//==============================================================================
class TabButton : public Button
{
public:
    TabButton(const String& tabName);
    ~TabButton() override;

    void paintButton(Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void setActive(bool isActive);
    bool isActive() const { return active; }

private:
    bool active = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TabButton)
};

//==============================================================================
// Search Tab Content Component
//==============================================================================
class SearchTabComponent : public Component
{
public:
    SearchTabComponent();
    ~SearchTabComponent() override;

    void paint(Graphics& g) override;
    void resized() override;

    void setProcessor(FreesoundAdvancedSamplerAudioProcessor* p);

private:
    FreesoundAdvancedSamplerAudioProcessor* processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SearchTabComponent)
};

//==============================================================================
// Bookmarks Tab Content Component
//==============================================================================
class BookmarksTabComponent : public Component
{
public:
    BookmarksTabComponent();
    ~BookmarksTabComponent() override;

    void paint(Graphics& g) override;
    void resized() override;

    void setProcessor(FreesoundAdvancedSamplerAudioProcessor* p);

private:
    FreesoundAdvancedSamplerAudioProcessor* processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BookmarksTabComponent)
};

//==============================================================================
// Main Expandable Panel Component
//==============================================================================
class ExpandablePanelComponent : public Component
{
public:
    ExpandablePanelComponent();
    ~ExpandablePanelComponent() override;

    void paint(Graphics& g) override;
    void resized() override;

    void setProcessor(FreesoundAdvancedSamplerAudioProcessor* p);

    bool isExpanded() const { return expanded; }
    void setExpanded(bool shouldExpand);
    int getExpandedWidth() const { return expandedWidth; }
    int getCollapsedWidth() const { return collapsedWidth; }

    std::function<void(bool)> onExpandedStateChanged;

private:
    FreesoundAdvancedSamplerAudioProcessor* processor;

    // UI Components
    StyledButton expandButton { ">", 12.0f, false };
    TabButton searchTabButton { "Search" };
    TabButton bookmarksTabButton { "Bookmarks" };

    SearchTabComponent searchTab;
    BookmarksTabComponent bookmarksTab;

    // State
    bool expanded = false;
    int expandedWidth = 210;  // Match preset browser width
    int collapsedWidth = 24;  // Just enough for expand button
    int currentActiveTab = 0; // 0 = Search, 1 = Bookmarks

    void handleExpandButtonClicked();
    void handleTabButtonClicked(int tabIndex);
    void updateTabVisibility();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExpandablePanelComponent)
};