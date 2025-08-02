/*
  ==============================================================================

    ExpandablePanel.h
    Created: Generic expandable panel component
    Author: Behzad Haki

  ==============================================================================
*/

#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "CustomButtonStyle.h"

//==============================================================================
// Generic Expandable Panel Component
//==============================================================================
class ExpandablePanel : public Component
{
public:
    enum class Orientation
    {
        Left,   // Expand button on left, expands to the right
        Right   // Expand button on right, expands to the left
    };

    ExpandablePanel(Orientation orientation = Orientation::Left);
    ~ExpandablePanel() override;

    void paint(Graphics& g) override;
    void resized() override;

    // Content management
    void setContentComponent(std::unique_ptr<Component> component);
    void setContentComponent(Component* component); // Non-owning version
    Component* getContentComponent() const { return contentComponent.get(); }

    // State management
    bool isExpanded() const { return expanded; }
    void setExpanded(bool shouldExpand);

    // Orientation
    void setOrientation(Orientation newOrientation);
    Orientation getOrientation() const { return orientation; }

    // Size configuration
    int getExpandedWidth() const { return expandedWidth; }
    int getCollapsedWidth() const { return collapsedWidth; }
    void setExpandedWidth(int width) { expandedWidth = width; }
    void setCollapsedWidth(int width) { collapsedWidth = width; }

    // Styling configuration
    void setExpandedBackgroundColour(Colour colour) { expandedBgColour = colour; }
    void setCollapsedBackgroundColour(Colour colour) { collapsedBgColour = colour; }
    void setBorderColour(Colour colour) { borderColour = colour; }
    void setContentMargin(int margin) { contentMargin = margin; }

    // Callbacks
    std::function<void(bool)> onExpandedStateChanged;

private:
    // UI Components
    StyledButton expandButton { ">", 12.0f, false };
    std::unique_ptr<Component> contentComponent;
    bool ownsContent = false;

    // State
    bool expanded = false;
    int expandedWidth = 210;
    int collapsedWidth = 24;
    int contentMargin = 4;
    Orientation orientation = Orientation::Left;

    // Styling
    Colour expandedBgColour = Colour(0xff1A1A1A);
    Colour collapsedBgColour = Colour(0xff2A2A2A);
    Colour borderColour = Colour(0xff404040);

    void handleExpandButtonClicked();
    void updateContentVisibility();
    void updateExpandButtonText();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExpandablePanel)
};