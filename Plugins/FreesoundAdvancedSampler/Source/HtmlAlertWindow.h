/*
  ==============================================================================

    HtmlAlertWindow.h
    Created: Leak-free HTML alert window using simple text drawing
    Author: Behzad Haki

  ==============================================================================
*/

#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"

using namespace juce;

class HtmlAlertWindow
{
public:
    // Theme colors - matches your CustomLookAndFeel
    struct Theme
    {
        Colour alertBackgroundColour = Colour(0xff1A1A1A);
        Colour alertTextColour = Colours::white;
        Colour alertOutlineColour = Colour(0xff404040);
        Colour buttonColour = Colour(0x804040);
        Colour buttonHoverColour = Colour(0xD8D4D3).darker(0.5f).withAlpha(0.3f);

        Font htmlBoldFont = Font(14.0f, Font::bold);
        Font htmlRegularFont = Font(14.0f);
        Font alertTitleFont = Font(16.0f, Font::bold);
        Font alertMessageFont = Font(12.0f);

        float alertCornerRadius = 8.0f;
        float buttonCornerRadius = 4.0f;
        float borderWidth = 1.5f;
    };

    // Simple text segment for rendering
    struct TextSegment
    {
        String text;
        Font font;
        Colour color;
        bool isBold;

        TextSegment(const String& t, const Font& f, const Colour& c, bool bold = false)
            : text(t), font(f), color(c), isBold(bold) {}
    };

    // Simple HTML text component using basic Graphics calls - no TextLayout
    class HtmlTextComponent : public Component
    {
    public:
        HtmlTextComponent(const String& htmlText, const Theme& theme)
            : theme(theme)
        {
            parseHtmlToSegments(htmlText);
            setSize(400, 300);
        }

        void paint(Graphics& g) override
        {
            auto bounds = getLocalBounds();
            g.fillAll(theme.alertBackgroundColour);

            auto textBounds = bounds.reduced(15);
            drawTextSegments(g, textBounds);
        }

    private:
        Theme theme;
        Array<TextSegment> textSegments;

        void parseHtmlToSegments(const String& htmlText)
        {
            textSegments.clear();

            int pos = 0;
            bool inBoldTag = false;
            String currentText;

            auto flushCurrentText = [&]() {
                if (currentText.isNotEmpty()) {
                    Font font = inBoldTag ? theme.htmlBoldFont : theme.htmlRegularFont;
                    textSegments.add(TextSegment(currentText, font, theme.alertTextColour, inBoldTag));
                    currentText.clear();
                }
            };

            while (pos < htmlText.length())
            {
                if (htmlText.substring(pos).startsWith("<b>"))
                {
                    flushCurrentText();
                    inBoldTag = true;
                    pos += 3;
                }
                else if (htmlText.substring(pos).startsWith("</b>"))
                {
                    flushCurrentText();
                    inBoldTag = false;
                    pos += 4;
                }
                else
                {
                    currentText += htmlText[pos];
                    pos++;
                }
            }

            flushCurrentText();
        }

        void drawTextSegments(Graphics& g, Rectangle<int> textBounds)
        {
            float currentX = textBounds.getX();
            float currentY = textBounds.getY();
            float lineHeight = theme.htmlRegularFont.getHeight() * 1.3f;

            for (const auto& segment : textSegments)
            {
                g.setFont(segment.font);
                g.setColour(segment.color);

                // Split segment by newlines
                StringArray lines = StringArray::fromLines(segment.text);

                for (int lineIndex = 0; lineIndex < lines.size(); ++lineIndex)
                {
                    const String& line = lines[lineIndex];

                    if (lineIndex > 0) // New line
                    {
                        currentY += lineHeight;
                        currentX = textBounds.getX();
                    }

                    if (line.isNotEmpty())
                    {
                        // Check if text would overflow to next line
                        float textWidth = segment.font.getStringWidth(line);
                        if (currentX > textBounds.getX() && currentX + textWidth > textBounds.getRight())
                        {
                            currentY += lineHeight;
                            currentX = textBounds.getX();
                        }

                        // Draw the text
                        g.drawText(line, currentX, currentY, textWidth, lineHeight,
                                  Justification::centredLeft);

                        currentX += textWidth;
                    }

                    // Handle empty lines (just newlines)
                    if (line.isEmpty() && lineIndex < lines.size() - 1)
                    {
                        currentY += lineHeight;
                        currentX = textBounds.getX();
                    }
                }
            }
        }
    };

    // Minimal look and feel
    class HtmlAlertLookAndFeel : public LookAndFeel_V4
    {
    public:
        HtmlAlertLookAndFeel(const Theme& t) : theme(t)
        {
            setColour(AlertWindow::backgroundColourId, theme.alertBackgroundColour);
            setColour(AlertWindow::textColourId, theme.alertTextColour);
            setColour(AlertWindow::outlineColourId, theme.alertOutlineColour);
            setColour(TextButton::buttonColourId, theme.buttonColour);
            setColour(TextButton::textColourOffId, theme.alertTextColour);
        }

        void drawAlertBox(Graphics& g, AlertWindow& alert,
                         const Rectangle<int>& textArea,
                         TextLayout& textLayout) override
        {
            auto bounds = alert.getLocalBounds().toFloat();

            ColourGradient gradient(
                theme.alertBackgroundColour, bounds.getTopLeft(),
                theme.alertBackgroundColour.darker(0.3f), bounds.getBottomRight(), false);
            g.setGradientFill(gradient);
            g.fillRoundedRectangle(bounds, theme.alertCornerRadius);

            g.setColour(theme.alertOutlineColour);
            g.drawRoundedRectangle(bounds.reduced(1), theme.alertCornerRadius, theme.borderWidth);

            g.setColour(alert.findColour(AlertWindow::textColourId));
            textLayout.draw(g, textArea.toFloat());
        }

        void drawButtonBackground(Graphics& g, Button& button,
                                const Colour& backgroundColour,
                                bool shouldDrawButtonAsHighlighted,
                                bool shouldDrawButtonAsDown) override
        {
            auto bounds = button.getLocalBounds().toFloat();

            Colour baseColour = backgroundColour;
            if (shouldDrawButtonAsDown)
                baseColour = baseColour.darker(0.2f);
            else if (shouldDrawButtonAsHighlighted)
                baseColour = theme.buttonHoverColour;

            ColourGradient gradient(
                baseColour.brighter(0.1f), bounds.getTopLeft(),
                baseColour.darker(0.1f), bounds.getBottomRight(), false);
            g.setGradientFill(gradient);
            g.fillRoundedRectangle(bounds, theme.buttonCornerRadius);

            g.setColour(theme.alertOutlineColour);
            g.drawRoundedRectangle(bounds.reduced(0.5f), theme.buttonCornerRadius, 1.0f);
        }

        Font getAlertWindowTitleFont() override { return theme.alertTitleFont; }
        Font getAlertWindowMessageFont() override { return theme.alertMessageFont; }

    private:
        Theme theme;
    };

    // Static show method with automatic cleanup
    static void show(const String& title,
                     const String& htmlMessage,
                     const String& buttonText = "OK",
                     int width = 600,
                     int height = 500,
                     const Theme* customTheme = nullptr)
    {
        Theme theme = customTheme ? *customTheme : Theme{};

        // Use shared_ptr for automatic cleanup
        struct AlertData
        {
            std::unique_ptr<AlertWindow> alert;
            std::unique_ptr<HtmlAlertLookAndFeel> lookAndFeel;
            std::unique_ptr<HtmlTextComponent> component;
        };

        auto data = std::make_shared<AlertData>();
        data->alert = std::make_unique<AlertWindow>(title, "", AlertWindow::InfoIcon);
        data->lookAndFeel = std::make_unique<HtmlAlertLookAndFeel>(theme);
        data->component = std::make_unique<HtmlTextComponent>(htmlMessage, theme);

        // Set up the window
        data->component->setSize(width - 40, height - 100);
        data->alert->setLookAndFeel(data->lookAndFeel.get());
        data->alert->addCustomComponent(data->component.get());
        data->alert->addButton(buttonText, 1, KeyPress(KeyPress::returnKey));

        // Capture the shared_ptr in the callback to keep everything alive
        auto* alertPtr = data->alert.get();
        alertPtr->enterModalState(true, ModalCallbackFunction::create([data](int) {
            // Clear references before destruction
            data->alert->setLookAndFeel(nullptr);
            // shared_ptr will automatically clean up everything in the right order
        }));
    }
};