/*
  ==============================================================================

    FreesoundSearchComponent.h
    Created: 10 Sep 2019 5:44:51pm
    Author:  Frederic Font Corbera
    Modified: Updated to work with grid layout

  ==============================================================================
*/

#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "PluginProcessor.h"
#include "FreesoundKeys.h"
#include <random>

class FreesoundSearchComponent: public Component,
                                public Button::Listener
{
public:

    FreesoundSearchComponent ()
    {
        searchInput.setText("", dontSendNotification);
        searchInput.setColour (Label::backgroundColourId, getLookAndFeel().findColour (ResizableWindow::backgroundColourId).brighter());
        searchInput.setEditable (true);
        addAndMakeVisible (searchInput);

        searchButton.addListener (this);
        searchButton.setButtonText("Search Sounds");
        addAndMakeVisible (searchButton);

        // Add instruction label
        instructionLabel.setText("Search for sounds and they will appear in the 4x4 grid below. Click on grid pads to play samples.", dontSendNotification);
        instructionLabel.setJustificationType(Justification::centred);
        instructionLabel.setColour(Label::textColourId, Colours::grey);
        addAndMakeVisible(instructionLabel);
    }

    ~FreesoundSearchComponent ()
    {
    }

    void setProcessor (FreesoundAdvancedSamplerAudioProcessor* p)
    {
        processor = p;
		searchInput.setText(p->getQuery(), dontSendNotification);
    }

    void paint (Graphics& g) override
    {
        // Draw a subtle border around the search area
        g.setColour(Colours::grey.withAlpha(0.3f));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2), 4.0f, 1.0f);
    }

    void resized () override
    {
        auto bounds = getLocalBounds().reduced(10);
        int inputHeight = 25;
        int buttonWidth = 120;
        int spacing = 10;

        // Search input and button on top row
        auto topRow = bounds.removeFromTop(inputHeight);
        searchInput.setBounds(topRow.removeFromLeft(topRow.getWidth() - buttonWidth - spacing));
        topRow.removeFromLeft(spacing);
        searchButton.setBounds(topRow);

        bounds.removeFromTop(spacing);

        // Instruction label below
        instructionLabel.setBounds(bounds);
    }

    void buttonClicked (Button* button) override
    {
        if (button == &searchButton)
        {
            String query = searchInput.getText(true);
            if (query.isEmpty())
            {
                AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "Empty Query", "Please enter a search term.");
                return;
            }

            Array<FSSound> sounds = searchSounds();

            if (sounds.size() == 0) {
                AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "No results", "No sounds found for the query: " + query + ". Possibly no network connection.");
                return;
            }

            // Create sound info array for the grid
            std::vector<StringArray> soundInfo;
            for (int i = 0; i < sounds.size(); i++)
            {
                FSSound sound = sounds[i];
                StringArray soundData;
                soundData.add(sound.name);
                soundData.add(sound.user);
                soundData.add(sound.license);
                soundInfo.push_back(soundData);
            }

            processor->newSoundsReady(sounds, query, soundInfo);
        }
    }

    Array<FSSound> searchSounds ()
    {
        // Makes a query to Freesound to retrieve short sounds using the query text from searchInput label
        // Sorts the results randomly and chooses the first 16 to be automatically assigned to the grid

        String query = searchInput.getText(true);
        FreesoundClient client(FREESOUND_API_KEY);
        SoundList list = client.textSearch(query, "duration:[0 TO 0.5]", "score", 1, -1, 150, "id,name,username,license,previews");
        Array<FSSound> sounds = list.toArrayOfSounds();
        auto num_sounds = sounds.size();

        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(sounds.begin(), sounds.end(), g);

        // Limit to 16 sounds for the 4x4 grid
        sounds.resize(std::min(num_sounds, 16));

        return sounds;
    }

private:

    FreesoundAdvancedSamplerAudioProcessor* processor;

    Label searchInput;
    TextButton searchButton;
    Label instructionLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreesoundSearchComponent);
};