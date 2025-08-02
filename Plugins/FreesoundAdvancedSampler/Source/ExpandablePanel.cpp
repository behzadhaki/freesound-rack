/*
  ==============================================================================

    ExpandablePanel.cpp
    Created: Generic expandable panel component
    Author: Behzad Haki

  ==============================================================================
*/

#include "ExpandablePanel.h"

//==============================================================================
// ExpandablePanel Implementation
//==============================================================================

ExpandablePanel::ExpandablePanel(Orientation orientation)
    : orientation(orientation)
{
    // Set up expand button
    expandButton.onClick = [this]() {
        handleExpandButtonClicked();
    };
    addAndMakeVisible(expandButton);

    updateExpandButtonText();
}

ExpandablePanel::~ExpandablePanel()
{
    // Content component will be automatically cleaned up by unique_ptr
}

void ExpandablePanel::paint(Graphics& g)
{
    auto bounds = getLocalBounds();

    if (expanded)
    {
        // Expanded state background
        g.setColour(expandedBgColour);
        g.fillAll();

        // Modern border
        g.setColour(borderColour);
        g.drawRoundedRectangle(bounds.toFloat().reduced(2), 6.0f, 1.5f);
    }
    else
    {
        // Collapsed state background
        g.setColour(collapsedBgColour.withAlpha(0.8f));
        g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

        g.setColour(borderColour);
        g.drawRoundedRectangle(bounds.toFloat().reduced(1), 4.0f, 1.0f);
    }
}

void ExpandablePanel::resized()
{
    auto bounds = getLocalBounds();

    if (expanded)
    {
        auto contentBounds = bounds.reduced(contentMargin); // Margin for border

        // Content component fills the entire area with margin
        if (contentComponent)
        {
            contentComponent->setBounds(contentBounds);
        }

        // Overlay expand button at the appropriate corner
        auto buttonBounds = Rectangle<int>(0, 0, 24, 24);

        if (orientation == Orientation::Left)
        {
            // Button at top-right corner for left-oriented panel
            buttonBounds.setPosition(contentBounds.getRight() - 24, contentBounds.getY());
        }
        else
        {
            // Button at top-left corner for right-oriented panel
            buttonBounds.setPosition(contentBounds.getX(), contentBounds.getY());
        }

        expandButton.setBounds(buttonBounds);
        // Ensure button stays on top after layout
        expandButton.toFront(false);
    }
    else
    {
        // Collapsed state - just the expand button
        expandButton.setBounds(bounds.reduced(2));
    }
}

void ExpandablePanel::setContentComponent(std::unique_ptr<Component> component)
{
    if (contentComponent)
    {
        removeChildComponent(contentComponent.get());
    }

    contentComponent = std::move(component);
    ownsContent = true;

    if (contentComponent)
    {
        addAndMakeVisible(*contentComponent);
        // Ensure expand button is always on top
        expandButton.toFront(false);
        updateContentVisibility();
        resized();
    }
}

void ExpandablePanel::setContentComponent(Component* component)
{
    if (contentComponent && ownsContent)
    {
        removeChildComponent(contentComponent.get());
    }

    contentComponent.reset(component);
    ownsContent = false;

    if (contentComponent)
    {
        addAndMakeVisible(*contentComponent);
        // Ensure expand button is always on top
        expandButton.toFront(false);
        updateContentVisibility();
        resized();
    }
}

void ExpandablePanel::setOrientation(Orientation newOrientation)
{
    if (orientation != newOrientation)
    {
        orientation = newOrientation;
        updateExpandButtonText();
        resized();
        repaint();
    }
}

void ExpandablePanel::setExpanded(bool shouldExpand)
{
    if (expanded != shouldExpand)
    {
        expanded = shouldExpand;

        // Update expand button text
        updateExpandButtonText();

        // Update content visibility
        updateContentVisibility();

        // Trigger layout update
        resized();
        repaint();

        // Notify parent about state change
        if (onExpandedStateChanged)
            onExpandedStateChanged(expanded);
    }
}

void ExpandablePanel::handleExpandButtonClicked()
{
    setExpanded(!expanded);
}

void ExpandablePanel::updateContentVisibility()
{
    if (contentComponent)
    {
        contentComponent->setVisible(expanded);
    }
}

void ExpandablePanel::updateExpandButtonText()
{
    String buttonText;

    if (orientation == Orientation::Left)
    {
        // Left-oriented panel: > when collapsed (expand right), < when expanded (collapse left)
        buttonText = expanded ? "<" : ">";
    }
    else
    {
        // Right-oriented panel: < when collapsed (expand left), > when expanded (collapse right)
        buttonText = expanded ? ">" : "<";
    }

    expandButton.setButtonText(buttonText);
}