/*
  ==============================================================================

    HtmlAlertWindow.h
    Created: Standalone HTML alert window with copy/paste support
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
        Colour textSelectionColour = Colour(0xff4A9EFF).withAlpha(0.3f);
        
        Font htmlBoldFont = Font(14.0f, Font::bold);
        Font htmlRegularFont = Font(14.0f);
        Font alertTitleFont = Font(16.0f, Font::bold);
        Font alertMessageFont = Font(12.0f);
        
        float alertCornerRadius = 8.0f;
        float buttonCornerRadius = 4.0f;
        float borderWidth = 1.5f;
    };

    // HTML text component with copy/paste support
    class HtmlTextComponent : public Component
    {
    public:
        HtmlTextComponent(const String& htmlText, const Theme& theme) 
            : htmlContent(htmlText), theme(theme)
        {
            parseHtmlToAttributedString();
            setWantsKeyboardFocus(true);
            setMouseClickGrabsKeyboardFocus(true);
        }

        void paint(Graphics& g) override
        {
            auto bounds = getLocalBounds();
            
            // Draw background
            g.fillAll(theme.alertBackgroundColour);
            
            // Draw text
            auto textBounds = bounds.reduced(15);
            textLayout.draw(g, textBounds.toFloat());
            
            // Draw text selection if any
            drawTextSelection(g, textBounds);
        }

        void mouseDown(const MouseEvent& e) override
        {
            grabKeyboardFocus();

            auto textBounds = getLocalBounds().reduced(15);
            auto position = getTextPositionFromPoint(e.getPosition(), textBounds);
            selectionStart = selectionEnd = position;
            isDragging = true;
            repaint();
        }

        void mouseDrag(const MouseEvent& e) override
        {
            if (!isDragging) return;

            auto textBounds = getLocalBounds().reduced(15);
            auto position = getTextPositionFromPoint(e.getPosition(), textBounds);
            selectionEnd = position;
            repaint();
        }

        void mouseUp(const MouseEvent& e) override
        {
            isDragging = false;
        }

        bool keyPressed(const KeyPress& key) override
        {
            if (key == KeyPress('c', ModifierKeys::commandModifier, 0))
            {
                copySelectedText();
                return true;
            }
            else if (key == KeyPress('a', ModifierKeys::commandModifier, 0))
            {
                selectAll();
                return true;
            }
            else if (key == KeyPress::escapeKey)
            {
                clearSelection();
                return true;
            }
            return false;
        }

        void copySelectedText()
        {
            if (hasSelection())
            {
                auto selectedText = getSelectedText();
                SystemClipboard::copyTextToClipboard(selectedText);
            }
        }

        void selectAll()
        {
            selectionStart = 0;
            selectionEnd = plainTextContent.length();
            repaint();
        }

        void clearSelection()
        {
            selectionStart = selectionEnd = 0;
            repaint();
        }

        void resized() override
        {
            auto textBounds = getLocalBounds().reduced(15);
            textLayout.createLayout(attributedString, static_cast<float>(textBounds.getWidth()));
        }

    private:
        String htmlContent;
        String plainTextContent;
        AttributedString attributedString;
        TextLayout textLayout;
        Theme theme;

        int selectionStart = 0;
        int selectionEnd = 0;
        bool isDragging = false;

        void parseHtmlToAttributedString()
        {
            attributedString.clear();
            plainTextContent.clear();

            int pos = 0;
            bool inBoldTag = false;
            bool inItalicTag = false;

            while (pos < htmlContent.length())
            {
                if (htmlContent.substring(pos).startsWith("<b>"))
                {
                    inBoldTag = true;
                    pos += 3;
                }
                else if (htmlContent.substring(pos).startsWith("</b>"))
                {
                    inBoldTag = false;
                    pos += 4;
                }
                else if (htmlContent.substring(pos).startsWith("<i>"))
                {
                    inItalicTag = true;
                    pos += 3;
                }
                else if (htmlContent.substring(pos).startsWith("</i>"))
                {
                    inItalicTag = false;
                    pos += 4;
                }
                else
                {
                    juce_wchar ch = htmlContent[pos];

                    // Handle newlines properly
                    if (ch == '\n')
                    {
                        attributedString.append("\n", theme.htmlRegularFont, theme.alertTextColour);
                        plainTextContent += "\n";
                    }
                    else if (ch == '\r')
                    {
                        // Skip carriage returns, but handle \r\n
                        if (pos + 1 < htmlContent.length() && htmlContent[pos + 1] == '\n')
                        {
                            // This is \r\n, skip the \r and let the \n be handled next iteration
                        }
                        else
                        {
                            // Standalone \r, treat as newline
                            attributedString.append("\n", theme.htmlRegularFont, theme.alertTextColour);
                            plainTextContent += "\n";
                        }
                    }
                    else
                    {
                        // Safe string creation to avoid encoding issues
                        String charStr;
                        charStr += ch;

                        Font currentFont = theme.htmlRegularFont;
                        if (inBoldTag && inItalicTag)
                            currentFont = Font(14.0f, Font::bold | Font::italic);
                        else if (inBoldTag)
                            currentFont = theme.htmlBoldFont;
                        else if (inItalicTag)
                            currentFont = Font(14.0f, Font::italic);

                        attributedString.append(charStr, currentFont, theme.alertTextColour);
                        plainTextContent += charStr;
                    }
                    pos++;
                }
            }

            // Set proper line spacing and word wrap
            attributedString.setLineSpacing(1.2f);
            attributedString.setWordWrap(AttributedString::byWord);

            // Create initial layout with proper text bounds
            auto bounds = getLocalBounds().reduced(15);
            if (bounds.getWidth() > 0)
                textLayout.createLayout(attributedString, static_cast<float>(bounds.getWidth()));
        }

        int getTextPositionFromPoint(Point<int> point, Rectangle<int> textBounds)
        {
            if (textLayout.getNumLines() == 0) return 0;

            auto relativePoint = point - textBounds.getTopLeft();

            // Use actual line height from text layout
            float lineHeight = textLayout.getHeight();
            int lineNumber = jlimit(0, textLayout.getNumLines() - 1,
                                   static_cast<int>(relativePoint.y / lineHeight));

            // Find the character position within the line
            // This is a simplified approach - for production code you'd want more precise glyph hit testing

            // Count characters up to the target line
            int position = 0;
            int currentLine = 0;

            for (int i = 0; i < plainTextContent.length() && currentLine < lineNumber; ++i)
            {
                if (plainTextContent[i] == '\n')
                {
                    currentLine++;
                }
                position++;
            }

            // Now find position within the current line
            if (lineNumber < textLayout.getNumLines())
            {
                // Simple character width estimation
                float averageCharWidth = theme.htmlRegularFont.getStringWidth("M");
                int lineStartPos = position;

                // Find end of current line
                int lineEndPos = plainTextContent.length();
                for (int i = lineStartPos; i < plainTextContent.length(); ++i)
                {
                    if (plainTextContent[i] == '\n')
                    {
                        lineEndPos = i;
                        break;
                    }
                }

                // Calculate character position within line
                int charsFromLineStart = static_cast<int>(relativePoint.x / averageCharWidth);
                int lineLength = lineEndPos - lineStartPos;
                charsFromLineStart = jlimit(0, lineLength, charsFromLineStart);

                position = lineStartPos + charsFromLineStart;
            }

            return jlimit(0, plainTextContent.length(), position);
        }

        bool hasSelection() const
        {
            return selectionStart != selectionEnd;
        }

        String getSelectedText() const
        {
            if (!hasSelection()) return {};

            int start = jmin(selectionStart, selectionEnd);
            int end = jmax(selectionStart, selectionEnd);

            return plainTextContent.substring(start, end);
        }

        void drawTextSelection(Graphics& g, Rectangle<int> textBounds)
        {
            if (!hasSelection()) return;

            int start = jmin(selectionStart, selectionEnd);
            int end = jmax(selectionStart, selectionEnd);

            g.setColour(theme.textSelectionColour);

            // Use actual line height from text layout
            float lineHeight = textLayout.getHeight();
            float averageCharWidth = theme.htmlRegularFont.getStringWidth("M");

            // Track current line and position
            int currentLine = 0;
            int lineStartPos = 0;
            int currentPos = 0;

            // Find which lines contain the selection
            while (currentPos < plainTextContent.length())
            {
                // Find end of current line
                int lineEndPos = plainTextContent.length();
                for (int i = currentPos; i < plainTextContent.length(); ++i)
                {
                    if (plainTextContent[i] == '\n')
                    {
                        lineEndPos = i;
                        break;
                    }
                }

                // Check if this line intersects with selection
                if (lineEndPos >= start && currentPos <= end)
                {
                    int selStart = jmax(start, currentPos);
                    int selEnd = jmin(end, lineEndPos);

                    if (selStart < selEnd)
                    {
                        float y = textBounds.getY() + (currentLine * lineHeight);
                        float x1 = textBounds.getX() + ((selStart - currentPos) * averageCharWidth);
                        float x2 = textBounds.getX() + ((selEnd - currentPos) * averageCharWidth);

                        Rectangle<float> selectionRect(x1, y, x2 - x1, lineHeight);
                        g.fillRect(selectionRect);
                    }
                }

                // Move to next line
                if (lineEndPos < plainTextContent.length()) // There's a newline
                {
                    currentPos = lineEndPos + 1; // Skip the newline character
                    currentLine++;
                }
                else
                {
                    break; // End of text
                }
            }
        }
    };

    // Custom look and feel for this specific alert window
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

    // Static method to show HTML alert window
    static void show(const String& title, 
                     const String& htmlMessage, 
                     const String& buttonText = "OK",
                     int width = 600,
                     int height = 500,
                     const Theme* customTheme = nullptr)
    {
        auto* alertWindow = new AlertWindow(title, "", AlertWindow::InfoIcon);
        
        // Use custom theme if provided, otherwise use default
        Theme theme = customTheme ? *customTheme : Theme{};
        
        // Create and apply custom look and feel
        auto* lookAndFeel = new HtmlAlertLookAndFeel(theme);
        alertWindow->setLookAndFeel(lookAndFeel);

        // Create HTML text component
        auto* htmlComp = new HtmlTextComponent(htmlMessage, theme);
        htmlComp->setSize(width - 40, height - 100);
        
        alertWindow->addCustomComponent(htmlComp);
        alertWindow->addButton(buttonText, 1, KeyPress(KeyPress::returnKey));

        // Use only ASCII characters in the instruction message to avoid encoding issues
        alertWindow->enterModalState(true, ModalCallbackFunction::create([alertWindow, lookAndFeel](int) {
            // Clear the look and feel from the alert window before deleting it
            alertWindow->setLookAndFeel(nullptr);
            delete lookAndFeel;
            delete alertWindow;
        }));
    }
};