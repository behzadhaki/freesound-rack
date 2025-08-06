/*
  ==============================================================================

    CustomLookAndFeel.h
    Created: Custom styling for AlertWindows and ProgressBars to match plugin theme
    Author: Behzad Haki

  ==============================================================================
*/

#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"

using namespace juce;

inline const Colour pluginChoiceColour =  Colour(0xD8D4D3).darker(0.5f); // Colour(0x9D9999); 0xBAB6B6

class CustomLookAndFeel : public LookAndFeel_V4
{
public:
    CustomLookAndFeel()
    {
        // Set up dark color scheme to match your plugin
        setColour(AlertWindow::backgroundColourId, alertBackgroundColour);
        setColour(AlertWindow::textColourId, alertTextColour);
        setColour(AlertWindow::outlineColourId, alertOutlineColour);

        // Button styling to match your StyledButton
        setColour(TextButton::buttonColourId, buttonColour);
        setColour(TextButton::buttonOnColourId, buttonOnColour);
        setColour(TextButton::textColourOffId, buttonTextOffColour);
        setColour(TextButton::textColourOnId, buttonTextOnColour);
        setColour(ComboBox::outlineColourId, comboBoxOutlineColour);

        // Text editor styling (for rename dialogs, etc.)
        setColour(TextEditor::backgroundColourId, textEditorBackgroundColour);
        setColour(TextEditor::textColourId, textEditorTextColour);
        setColour(TextEditor::outlineColourId, textEditorOutlineColour);
        setColour(TextEditor::focusedOutlineColourId, textEditorFocusedOutlineColour);

        // Label styling
        setColour(Label::textColourId, labelTextColour);

        // Progress bar styling to match your theme
        setColour(ProgressBar::backgroundColourId, progressBarBackgroundColour);
        setColour(ProgressBar::foregroundColourId, progressBarForegroundColour);
    }

    // Color configuration variables
    Colour alertBackgroundColour = Colour(0xff1A1A1A);
    Colour alertTextColour = Colours::white;
    Colour alertOutlineColour = Colour(0xff404040);

    Colour buttonColour = Colour(0x804040);
    Colour buttonOnColour = Colour(0xff606060);
    Colour buttonTextOffColour = Colours::white;
    Colour buttonTextOnColour = Colours::white;
    Colour buttonHoverColour = pluginChoiceColour.withAlpha(0.3f);

    Colour comboBoxOutlineColour = Colour(0xff404040);

    Colour textEditorBackgroundColour = Colour(0xff2A2A2A);
    Colour textEditorTextColour = Colours::white;
    Colour textEditorOutlineColour = Colour(0xff404040);
    Colour textEditorFocusedOutlineColour = Colour(0xff606060);

    Colour labelTextColour = Colours::grey;

    Colour progressBarBackgroundColour = Colour(0xff2A2A2A);
    Colour progressBarForegroundColour = Colour(0xff404040);
    Colour progressBarBorderColour = Colour(0xff404040);

    // Font configuration variables
    Font alertTitleFont = Font(16.0f, Font::bold);
    Font alertMessageFont = Font(14.0f);
    Font alertFont = Font(12.0f);
    Font progressBarTextFont = Font(9.0f, Font::bold);
    Font labelFont = Font(14.0f, Font::bold);

    // Justification configuration
    Justification labelJustification = Justification::centred;

    // Size configuration variables
    int alertButtonHeight = 32;
    float cornerRadius = 3.0f;
    float borderWidth = 1.0f;
    float alertCornerRadius = 8.0f;
    float alertBorderWidth = 1.5f;
    float buttonCornerRadius = 4.0f;

    // Custom label drawing method
    void drawLabel(Graphics& g, Label& label) override
    {
        g.fillAll(label.findColour(Label::backgroundColourId));

        if (!label.isBeingEdited())
        {
            auto alpha = label.isEnabled() ? 1.0f : 0.5f;
            auto font = getLabelFont(label);

            g.setColour(label.findColour(Label::textColourId).withMultipliedAlpha(alpha));
            g.setFont(font);

            auto textArea = getLabelBorderSize(label).subtractedFrom(label.getLocalBounds());

            g.drawFittedText(label.getText(), textArea, labelJustification,
                           jmax(1, (int)((float)textArea.getHeight() / font.getHeight())),
                           label.getMinimumHorizontalScale());

            g.setColour(label.findColour(Label::outlineColourId).withMultipliedAlpha(alpha));
        }
        else if (label.isEnabled())
        {
            g.setColour(label.findColour(Label::outlineColourId));
        }

        g.drawRect(label.getLocalBounds());
    }

    Font getLabelFont(Label& label) override
    {
        return labelFont;
    }

    // Custom progress bar drawing method
    void drawProgressBar(Graphics& g, ProgressBar& progressBar,
                        int width, int height, double progress,
                        const String& textToShow) override
    {
        auto background = progressBar.findColour(ProgressBar::backgroundColourId);
        auto foreground = progressBar.findColour(ProgressBar::foregroundColourId);

        auto barBounds = Rectangle<int>(0, 0, width, height).toFloat();

        // Draw background with rounded corners like your other components
        g.setColour(background);
        g.fillRoundedRectangle(barBounds, cornerRadius);

        // Draw border like your text editors
        g.setColour(progressBarBorderColour);
        g.drawRoundedRectangle(barBounds.reduced(0.5f), cornerRadius, borderWidth);

        if (progress > 0.0)
        {
            // Draw progress fill with gradient like your StyledButton hover effect
            auto progressWidth = static_cast<float>(progress * width);
            auto progressBounds = barBounds.withWidth(progressWidth).reduced(1.0f);

            // Create gradient similar to your button hover effect
            ColourGradient gradient(
                foreground.brighter(0.1f), progressBounds.getTopLeft(),
                foreground.darker(0.1f), progressBounds.getBottomRight(), false);

            g.setGradientFill(gradient);
            g.fillRoundedRectangle(progressBounds, cornerRadius - 1.0f);

            // Add subtle inner highlight
            g.setColour(foreground.brighter(0.3f).withAlpha(0.3f));
            g.fillRoundedRectangle(progressBounds.removeFromTop(2.0f), cornerRadius - 1.0f);
        }

        // Draw text if provided
        if (textToShow.isNotEmpty())
        {
            g.setColour(labelTextColour);
            g.setFont(progressBarTextFont);
            g.drawText(textToShow, barBounds.toNearestInt(), Justification::centred);
        }
    }

    // Enhanced button drawing to match your hover colors
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
            baseColour = buttonHoverColour.withAlpha(0.3f);

        // Gradient background like your StyledButton
        ColourGradient gradient(
            baseColour.brighter(0.1f), bounds.getTopLeft(),
            baseColour.darker(0.1f), bounds.getBottomRight(), false);
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(bounds, buttonCornerRadius);

        // Border
        g.setColour(alertOutlineColour);
        g.drawRoundedRectangle(bounds.reduced(0.5f), buttonCornerRadius, borderWidth);
    }

    Font getAlertWindowTitleFont() override
    {
        return alertTitleFont;
    }

    Font getAlertWindowMessageFont() override
    {
        return alertMessageFont;
    }

    Font getAlertWindowFont() override
    {
        return alertFont;
    }

    int getAlertWindowButtonHeight() override
    {
        return alertButtonHeight;
    }

    int getAlertBoxWindowFlags() override
    {
        // Remove system decorations for cleaner look
        return ComponentPeer::windowAppearsOnTaskbar
             | ComponentPeer::windowHasDropShadow;
    }

    void drawAlertBox(Graphics& g, AlertWindow& alert,
                     const Rectangle<int>& textArea,
                     TextLayout& textLayout) override
    {
        // Modern rounded rectangle background
        auto bounds = alert.getLocalBounds().toFloat();

        // Dark background with subtle gradient
        ColourGradient gradient(
            alertBackgroundColour, bounds.getTopLeft(),
            alertBackgroundColour.darker(0.3f), bounds.getBottomRight(), false);
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(bounds, alertCornerRadius);

        // Modern border
        g.setColour(alertOutlineColour);
        g.drawRoundedRectangle(bounds.reduced(1), alertCornerRadius, alertBorderWidth);

        // Draw the text
        g.setColour(alert.findColour(AlertWindow::textColourId));
        textLayout.draw(g, textArea.toFloat());
    }

    void fillTextEditorBackground (juce::Graphics& g, int width, int height, juce::TextEditor& textEditor) override
    {
        g.setColour (textEditor.findColour (juce::TextEditor::backgroundColourId));
        g.fillRoundedRectangle (0, 0, width, height, 2.0f); // 10.0f is the corner radius
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomLookAndFeel)
};