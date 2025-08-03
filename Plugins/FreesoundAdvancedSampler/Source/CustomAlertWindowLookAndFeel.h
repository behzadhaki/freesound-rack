/*
  ==============================================================================

    CustomAlertWindowLookAndFeel.h
    Created: Custom styling for AlertWindows to match plugin theme
    Author: Generated

  ==============================================================================
*/

#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"

using namespace juce;

class CustomAlertWindowLookAndFeel : public LookAndFeel_V4
{
public:
    CustomAlertWindowLookAndFeel()
    {
        // Set up dark color scheme to match your plugin
        setColour(AlertWindow::backgroundColourId, Colour(0xff1A1A1A));
        setColour(AlertWindow::textColourId, Colours::white);
        setColour(AlertWindow::outlineColourId, Colour(0xff404040));

        // Button styling to match your StyledButton
        setColour(TextButton::buttonColourId, Colour(0xff404040));
        setColour(TextButton::buttonOnColourId, Colour(0xff606060));
        setColour(TextButton::textColourOffId, Colours::white);
        setColour(TextButton::textColourOnId, Colours::white);
        setColour(ComboBox::outlineColourId, Colour(0xff404040));

        // Text editor styling (for rename dialogs, etc.)
        setColour(TextEditor::backgroundColourId, Colour(0xff2A2A2A));
        setColour(TextEditor::textColourId, Colours::white);
        setColour(TextEditor::outlineColourId, Colour(0xff404040));
        setColour(TextEditor::focusedOutlineColourId, Colour(0xff606060));

        // Label styling
        setColour(Label::textColourId, Colours::white);
    }

    void drawAlertBox(Graphics& g, AlertWindow& alert,
                     const Rectangle<int>& textArea,
                     TextLayout& textLayout) override
    {
        // Modern rounded rectangle background
        auto bounds = alert.getLocalBounds().toFloat();

        // Dark background with subtle gradient
        ColourGradient gradient(
            Colour(0xff1A1A1A), bounds.getTopLeft(),
            Colour(0xff0D0D0D), bounds.getBottomRight(), false);
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(bounds, 8.0f);

        // Modern border
        g.setColour(Colour(0xff404040));
        g.drawRoundedRectangle(bounds.reduced(1), 8.0f, 1.5f);

        // Draw the text
        g.setColour(alert.findColour(AlertWindow::textColourId));
        textLayout.draw(g, textArea.toFloat());
    }

    void drawButtonBackground(Graphics& g, Button& button,
                            const Colour& backgroundColour,
                            bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat();

        // Use your StyledButton coloring logic
        Colour baseColour = backgroundColour;

        if (shouldDrawButtonAsDown)
            baseColour = baseColour.darker(0.2f);
        else if (shouldDrawButtonAsHighlighted)
            baseColour = baseColour.brighter(0.1f);

        // Gradient background like your StyledButton
        ColourGradient gradient(
            baseColour.brighter(0.1f), bounds.getTopLeft(),
            baseColour.darker(0.1f), bounds.getBottomRight(), false);
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(bounds, 4.0f);

        // Border
        g.setColour(baseColour.brighter(0.2f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
    }

    Font getAlertWindowTitleFont() override
    {
        return Font(16.0f, Font::bold);
    }

    Font getAlertWindowMessageFont() override
    {
        return Font(14.0f);
    }

    Font getAlertWindowFont() override
    {
        return Font(12.0f);
    }

    int getAlertWindowButtonHeight() override
    {
        return 32; // Match your button heights
    }

    int getAlertBoxWindowFlags() override
    {
        // Remove system decorations for cleaner look
        return ComponentPeer::windowAppearsOnTaskbar
             | ComponentPeer::windowHasDropShadow;
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomAlertWindowLookAndFeel)
};