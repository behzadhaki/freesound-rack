/*
  ==============================================================================

    ExpandablePanelComponent.cpp
    Created: Expandable panel with Search and Bookmarks tabs
    Author: Behzad Haki

  ==============================================================================
*/

#include "ExpandablePanelComponent.h"

//==============================================================================
// TabButton Implementation
//==============================================================================

TabButton::TabButton(const String& tabName)
    : Button(tabName)
{
    setButtonText(tabName);
}

TabButton::~TabButton() {}

void TabButton::paintButton(Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = getLocalBounds().toFloat();

    // Background color based on state
    Colour bgColour;
    if (active)
    {
        bgColour = Colour(0xff00D9FF).withAlpha(0.8f); // Active tab - cyan
    }
    else
    {
        bgColour = Colour(0xff2A2A2A); // Inactive tab - dark grey
    }

    if (shouldDrawButtonAsHighlighted && !active)
        bgColour = bgColour.brighter(0.2f);
    if (shouldDrawButtonAsDown)
        bgColour = bgColour.darker(0.1f);

    g.setColour(bgColour);
    g.fillRoundedRectangle(bounds.withTrimmedBottom(active ? 0 : 2), 4.0f);

    // Border for inactive tabs only
    if (!active)
    {
        g.setColour(Colour(0xff404040));
        g.drawRoundedRectangle(bounds.withTrimmedBottom(2).reduced(0.5f), 4.0f, 1.0f);
    }

    // Text
    g.setColour(active ? Colours::white : Colours::lightgrey);
    g.setFont(Font(11.0f, active ? Font::bold : Font::plain));
    g.drawText(getButtonText(), bounds.toNearestInt(), Justification::centred);
}

void TabButton::setActive(bool isActive)
{
    if (active != isActive)
    {
        active = isActive;
        repaint();
    }
}

//==============================================================================
// SearchTabComponent Implementation
//==============================================================================

SearchTabComponent::SearchTabComponent()
    : processor(nullptr)
{
}

SearchTabComponent::~SearchTabComponent() {}

void SearchTabComponent::paint(Graphics& g)
{
    // Dark background matching preset browser
    g.setColour(Colour(0xff1A1A1A));
    g.fillAll();

    // Placeholder content
    g.setColour(Colours::grey);
    g.setFont(Font(14.0f));
    g.drawText("Search Tab", getLocalBounds(), Justification::centred);

    g.setFont(Font(10.0f));
    g.drawText("Content coming soon...", getLocalBounds().withTrimmedTop(30), Justification::centred);
}

void SearchTabComponent::resized()
{
    // Content layout will be implemented here
}

void SearchTabComponent::setProcessor(FreesoundAdvancedSamplerAudioProcessor* p)
{
    processor = p;
}

//==============================================================================
// BookmarksTabComponent Implementation
//==============================================================================

BookmarksTabComponent::BookmarksTabComponent()
    : processor(nullptr)
{
}

BookmarksTabComponent::~BookmarksTabComponent() {}

void BookmarksTabComponent::paint(Graphics& g)
{
    // Dark background matching preset browser
    g.setColour(Colour(0xff1A1A1A));
    g.fillAll();

    // Placeholder content
    g.setColour(Colours::grey);
    g.setFont(Font(14.0f));
    g.drawText("Bookmarks Tab", getLocalBounds(), Justification::centred);

    g.setFont(Font(10.0f));
    g.drawText("Content coming soon...", getLocalBounds().withTrimmedTop(30), Justification::centred);
}

void BookmarksTabComponent::resized()
{
    // Content layout will be implemented here
}

void BookmarksTabComponent::setProcessor(FreesoundAdvancedSamplerAudioProcessor* p)
{
    processor = p;
}

//==============================================================================
// ExpandablePanelComponent Implementation
//==============================================================================

ExpandablePanelComponent::ExpandablePanelComponent()
    : processor(nullptr)
{
    // Set up expand button
    expandButton.onClick = [this]() {
        handleExpandButtonClicked();
    };
    addAndMakeVisible(expandButton);

    // Set up tab buttons
    searchTabButton.onClick = [this]() {
        handleTabButtonClicked(0);
    };
    bookmarksTabButton.onClick = [this]() {
        handleTabButtonClicked(1);
    };

    // Initially hide tab buttons and content
    searchTabButton.setVisible(false);
    bookmarksTabButton.setVisible(false);
    searchTab.setVisible(false);
    bookmarksTab.setVisible(false);

    addAndMakeVisible(searchTabButton);
    addAndMakeVisible(bookmarksTabButton);
    addAndMakeVisible(searchTab);
    addAndMakeVisible(bookmarksTab);

    // Set initial active tab
    searchTabButton.setActive(true);
    currentActiveTab = 0;
}

ExpandablePanelComponent::~ExpandablePanelComponent() {}

void ExpandablePanelComponent::paint(Graphics& g)
{
    auto bounds = getLocalBounds();

    if (expanded)
    {
        // Dark background matching preset browser
        g.setColour(Colour(0xff1A1A1A));
        g.fillAll();

        // Modern border
        g.setColour(Colour(0xff404040));
        g.drawRoundedRectangle(bounds.toFloat().reduced(2), 6.0f, 1.5f);
    }
    else
    {
        // Collapsed state - minimal background
        g.setColour(Colour(0xff2A2A2A).withAlpha(0.8f));
        g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

        g.setColour(Colour(0xff404040));
        g.drawRoundedRectangle(bounds.toFloat().reduced(1), 4.0f, 1.0f);
    }
}

void ExpandablePanelComponent::resized()
{
    auto bounds = getLocalBounds();

    if (expanded)
    {
        bounds = bounds.reduced(10); // Margin for border

        // Expand button at top-right
        auto expandButtonBounds = bounds.removeFromTop(24);
        expandButton.setBounds(expandButtonBounds.removeFromRight(24));

        // Tab buttons below expand button
        bounds.removeFromTop(5); // Small gap
        auto tabsBounds = bounds.removeFromTop(30);

        int tabWidth = tabsBounds.getWidth() / 2;
        searchTabButton.setBounds(tabsBounds.removeFromLeft(tabWidth));
        bookmarksTabButton.setBounds(tabsBounds);

        // Content area for tabs
        bounds.removeFromTop(5); // Small gap
        searchTab.setBounds(bounds);
        bookmarksTab.setBounds(bounds);
    }
    else
    {
        // Collapsed state - just the expand button
        expandButton.setBounds(bounds.reduced(2));
    }
}

void ExpandablePanelComponent::setProcessor(FreesoundAdvancedSamplerAudioProcessor* p)
{
    processor = p;
    searchTab.setProcessor(p);
    bookmarksTab.setProcessor(p);
}

void ExpandablePanelComponent::setExpanded(bool shouldExpand)
{
    if (expanded != shouldExpand)
    {
        expanded = shouldExpand;

        // Update expand button text
        expandButton.setButtonText(expanded ? "<" : ">");

        // Update component visibility
        updateTabVisibility();

        // Trigger layout update
        resized();
        repaint();

        // Notify parent about state change
        if (onExpandedStateChanged)
            onExpandedStateChanged(expanded);
    }
}

void ExpandablePanelComponent::handleExpandButtonClicked()
{
    setExpanded(!expanded);
}

void ExpandablePanelComponent::handleTabButtonClicked(int tabIndex)
{
    if (currentActiveTab != tabIndex)
    {
        currentActiveTab = tabIndex;
        updateTabVisibility();
    }
}

void ExpandablePanelComponent::updateTabVisibility()
{
    if (expanded)
    {
        // Show tab buttons
        searchTabButton.setVisible(true);
        bookmarksTabButton.setVisible(true);

        // Update tab button states
        searchTabButton.setActive(currentActiveTab == 0);
        bookmarksTabButton.setActive(currentActiveTab == 1);

        // Show active tab content
        searchTab.setVisible(currentActiveTab == 0);
        bookmarksTab.setVisible(currentActiveTab == 1);
    }
    else
    {
        // Hide everything when collapsed
        searchTabButton.setVisible(false);
        bookmarksTabButton.setVisible(false);
        searchTab.setVisible(false);
        bookmarksTab.setVisible(false);
    }
}