/*
  ==============================================================================

    CustomButtonStyle.h
    Created: Unified button styling system
    Author: Generated

  ==============================================================================
*/

#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "PluginProcessor.h"

using namespace juce;

//==============================================================================
// Custom Button with Styling
//==============================================================================
class StyledButton : public TextButton
{
public:
    StyledButton(const String& buttonText, float fontSize, bool isDangerButton = false):isDangerButton(isDangerButton), fontSize(fontSize) {
        // Modern dark button styling
        setColour(TextButton::buttonColourId, isDangerButton ? dangerButtonColour : buttonColour);
        setColour(TextButton::buttonOnColourId, isDangerButton ? dangerButtonOnColour : buttonOnColour);
        setColour(TextButton::textColourOffId, isDangerButton ? dangerTextColourOff : textColourOff);
        setColour(TextButton::textColourOnId, isDangerButton ? dangerTextColourOn : textColourOn);

        setButtonText(buttonText);
    }

    ~StyledButton() override {}

    void paint(Graphics& g) override {
        // Use same styling as SampleDragArea
        auto bounds = getLocalBounds();

        // Highlight same as SampleDragArea when hovered
        if (isMouseOver())
        {
            g.setColour(isDangerButton ? dangerMouseOverColour : mouseOverColour);
            g.fillRoundedRectangle(bounds.toFloat(), 6.0f);
            g.setColour(isDangerButton ? dangerTextColourOn : textColourOn);
        }
        else
        {
            g.setColour(isDangerButton ? dangerButtonColour : buttonColour);
            g.fillRoundedRectangle(bounds.toFloat(), 6.0f);
            g.setColour(isDangerButton ? dangerTextColourOff : textColourOff);
        }
        g.setFont(Font(fontSize, Font::bold));
        g.drawText(getButtonText(), bounds, Justification::centred);

    }

private:
    float fontSize = 10.0f;
    bool isDangerButton = false;

    juce::Colour borderColour = Colour(0x80404040);

    juce::Colour buttonColour = Colour(0x80404040);
    juce::Colour buttonOnColour = Colour(0x80FFB347);
    juce::Colour textColourOff = Colours::white;
    juce::Colour textColourOn = Colours::white;
    juce::Colour mouseOverColour = Colour(0x8000D9FF).withAlpha(0.3f);

    juce::Colour dangerButtonColour = Colour(0x80FF4D4D).withAlpha(0.3f);
    juce::Colour dangerButtonOnColour = Colour(0x80FF4D4D);
    juce::Colour dangerTextColourOff = Colours::white;
    juce::Colour dangerTextColourOn = Colours::white;
    juce::Colour dangerMouseOverColour = Colour(0x80FF4D4D);


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StyledButton)
};


