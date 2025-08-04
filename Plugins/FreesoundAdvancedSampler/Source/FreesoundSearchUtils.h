#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "FreesoundAPI/FreesoundAPI.h"
#include "FreesoundKeys.h"
#include <random>

using namespace juce;

inline std::pair<Array<FSSound>, std::vector<juce::StringArray>> getSoundsForQuery (const String& masterQuery, int numSoundsNeeded, bool shuffleResults = true) {

  // Use the existing Freesound search system
    FreesoundClient client(FREESOUND_API_KEY);

    Array<FSSound> finalSounds;
    std::vector<juce::StringArray> soundInfo;

    try
    {
        // Request more results than we need to have variety for shuffling
        // Use page_size parameter (5th parameter) to get multiple results
        int requestedResults = jmax(numSoundsNeeded * 2, 50); // Request at least 50 or 2x what we need

        SoundList list = client.textSearch(
            masterQuery,
            "duration:[0 TO 0.5]",
            "score",
            1,
            1,
            10000,
            "id,name,username,license,previews,tags,description"
        );

        Array<FSSound> sounds = list.toArrayOfSounds();

        // 1. Handle no results case
        if (sounds.isEmpty())
        {
            DBG("No results found for query: " + masterQuery);
            return { finalSounds, soundInfo };
        }

        // 3. Shuffle results if more than one sound found
        if (shuffleResults && sounds.size() > 1)
        {
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(sounds.begin(), sounds.end(), g);
        }

        // 4. downsample or repeat in a cycling manner to fill the required number
        for (int i = 0; i < numSoundsNeeded; ++i)
        {
            int sourceIndex = i % sounds.size(); // Cycle through available sounds
            finalSounds.add(sounds[sourceIndex]);

            // Create sound info for each repeated sound
            StringArray info;
            info.add(sounds[sourceIndex].name);
            info.add(sounds[sourceIndex].user);
            info.add(sounds[sourceIndex].license);
            info.add(masterQuery); // Store the master query
            soundInfo.push_back(info);
        }
    }
    catch (const std::exception& e)
    {
      DBG("Error during Freesound search: " + String(e.what()));
    }

    return { finalSounds, soundInfo };

    // to access the return, use the following code

}


