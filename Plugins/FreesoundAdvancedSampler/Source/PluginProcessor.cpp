/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// TrackingSamplerVoice Implementation
//==============================================================================

FreesoundAdvancedSamplerAudioProcessor::TrackingSamplerVoice::TrackingSamplerVoice(FreesoundAdvancedSamplerAudioProcessor& owner)
    : processor(owner)
{
}

void FreesoundAdvancedSamplerAudioProcessor::TrackingSamplerVoice::startNote(int midiNoteNumber, float velocity, SynthesiserSound* sound, int currentPitchWheelPosition)
{
    SamplerVoice::startNote(midiNoteNumber, velocity, sound, currentPitchWheelPosition);

    currentNoteNumber = midiNoteNumber;
    samplePosition = 0.0;

    // Get sample length from the sound
    if (auto* samplerSound = dynamic_cast<SamplerSound*>(sound))
    {
        sampleLength = samplerSound->getAudioData()->getNumSamples();
    }

    processor.notifyNoteStarted(midiNoteNumber, velocity);
}

void FreesoundAdvancedSamplerAudioProcessor::TrackingSamplerVoice::stopNote(float velocity, bool allowTailOff)
{
    if (currentNoteNumber >= 0)
    {
        processor.notifyNoteStopped(currentNoteNumber);
        currentNoteNumber = -1;
    }

    SamplerVoice::stopNote(velocity, allowTailOff);
}

void FreesoundAdvancedSamplerAudioProcessor::TrackingSamplerVoice::renderNextBlock(AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    SamplerVoice::renderNextBlock(outputBuffer, startSample, numSamples);

    // Update playhead position
    if (currentNoteNumber >= 0 && sampleLength > 0)
    {
        samplePosition += numSamples;
        float position = (float)samplePosition / (float)sampleLength;
        processor.notifyPlayheadPositionChanged(currentNoteNumber, jlimit(0.0f, 1.0f, position));
    }
}

//==============================================================================
// FreesoundAdvancedSamplerAudioProcessor Implementation
//==============================================================================

FreesoundAdvancedSamplerAudioProcessor::FreesoundAdvancedSamplerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     :         AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       ), presetManager(File::getSpecialLocation(File::userDocumentsDirectory).getChildFile("FreesoundAdvancedSampler"))
#endif
{
    tmpDownloadLocation = File::getSpecialLocation(File::userDocumentsDirectory).getChildFile("FreesoundAdvancedSampler");
    tmpDownloadLocation.createDirectory();
    currentSessionDownloadLocation = presetManager.getSamplesFolder(); // Use samples folder as default
    midicounter = 1;
    startTime = Time::getMillisecondCounterHiRes() * 0.001;

    // Add download manager listener
    downloadManager.addListener(this);
}
FreesoundAdvancedSamplerAudioProcessor::~FreesoundAdvancedSamplerAudioProcessor()
{
	// Remove download manager listener
	downloadManager.removeListener(this);

	// Note: We no longer delete the tmp directory to preserve downloaded files

}

//==============================================================================
const String FreesoundAdvancedSamplerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool FreesoundAdvancedSamplerAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool FreesoundAdvancedSamplerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool FreesoundAdvancedSamplerAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double FreesoundAdvancedSamplerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int FreesoundAdvancedSamplerAudioProcessor::getNumPrograms()
{
    return 1;
}

int FreesoundAdvancedSamplerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void FreesoundAdvancedSamplerAudioProcessor::setCurrentProgram (int index)
{
}

const String FreesoundAdvancedSamplerAudioProcessor::getProgramName (int index)
{
    return {};
}

void FreesoundAdvancedSamplerAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void FreesoundAdvancedSamplerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
	sampler.setCurrentPlaybackSampleRate(sampleRate);
}

void FreesoundAdvancedSamplerAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FreesoundAdvancedSamplerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void FreesoundAdvancedSamplerAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	midiMessages.addEvents(midiFromEditor, 0, INT_MAX, 0);
	midiFromEditor.clear();
	sampler.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
	midiMessages.clear();
}

//==============================================================================
bool FreesoundAdvancedSamplerAudioProcessor::hasEditor() const
{
    return true;
}

AudioProcessorEditor* FreesoundAdvancedSamplerAudioProcessor::createEditor()
{
    return new FreesoundAdvancedSamplerAudioProcessorEditor (*this);
}

//==============================================================================
void FreesoundAdvancedSamplerAudioProcessor::getStateInformation (MemoryBlock& destData)
{
}

void FreesoundAdvancedSamplerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
}

void FreesoundAdvancedSamplerAudioProcessor::newSoundsReady(Array<FSSound> sounds, String textQuery, std::vector<juce::StringArray> soundInfo)
{
    query = textQuery;
    soundsArray = soundInfo;
    currentSoundsArray = sounds; // Store the sounds array

    // Check if we have an editor with a grid component to handle master search
    if (auto* editor = dynamic_cast<FreesoundAdvancedSamplerAudioProcessorEditor*>(getActiveEditor()))
    {
        // Use the new master search method that only affects pads with empty queries
        editor->getSampleGridComponent().handleMasterSearch(sounds, soundInfo, textQuery);
    }

    // Always start downloads to ensure all samples are available locally
    startDownloads(sounds);

    // NOTE: We do NOT call updateSamples here anymore since handleMasterSearch handles the grid update
    // This prevents the text boxes from being cleared after master search
}

void FreesoundAdvancedSamplerAudioProcessor::startDownloads(const Array<FSSound>& sounds)
{
    // Create main samples folder (no more subfolders)
    File samplesFolder = presetManager.getSamplesFolder();
    samplesFolder.createDirectory();

    // Filter out sounds that already exist to avoid re-downloading
    Array<FSSound> soundsToDownload;
    for (const auto& sound : sounds)
    {
        String fileName = "FS_ID_" + sound.id + ".ogg";
        File audioFile = samplesFolder.getChildFile(fileName);

        if (!audioFile.existsAsFile())
        {
            soundsToDownload.add(sound);
        }
    }

    // Store the current download location for this session (now points to samples folder)
    currentSessionDownloadLocation = samplesFolder;

    // If no new downloads needed, just load existing samples
    if (soundsToDownload.isEmpty())
    {
        // All samples already exist, just set up the sampler
        setSources();

        // Notify listeners that "download" is complete
        downloadListeners.call([](DownloadListener& l) {
            l.downloadCompleted(true);
        });
        return;
    }

    // Start downloading only the new samples
    downloadManager.startDownloads(soundsToDownload, samplesFolder, query);
}

void FreesoundAdvancedSamplerAudioProcessor::cancelDownloads()
{
	downloadManager.stopThread(2000);
}

void FreesoundAdvancedSamplerAudioProcessor::downloadProgressChanged(const AudioDownloadManager::DownloadProgress& progress)
{
	downloadListeners.call([progress](DownloadListener& l) {
		l.downloadProgressChanged(progress);
	});
}

void FreesoundAdvancedSamplerAudioProcessor::downloadCompleted(bool success)
{
    if (success)
    {
        setSources();
    }

    downloadListeners.call([success](DownloadListener& l) {
        l.downloadCompleted(success);
    });

    // DO NOT call updateSamples here for master search downloads
    // The grid has already been updated by handleMasterSearch
    // Only update if this was a preset load or other non-master-search operation
}

void FreesoundAdvancedSamplerAudioProcessor::addDownloadListener(DownloadListener* listener)
{
	downloadListeners.add(listener);
}

void FreesoundAdvancedSamplerAudioProcessor::removeDownloadListener(DownloadListener* listener)
{
	downloadListeners.remove(listener);
}

void FreesoundAdvancedSamplerAudioProcessor::setSources()
{
    sampler.clearSounds();
    sampler.clearVoices();

    int poliphony = 16;
    int maxLength = 10;

    // Add tracking voices instead of regular sampler voices
    for (int i = 0; i < poliphony; i++) {
        sampler.addVoice(new TrackingSamplerVoice(*this));
    }

    if(audioFormatManager.getNumKnownFormats() == 0){
        audioFormatManager.registerBasicFormats();
    }

    // Load samples by their actual pad positions, not sequentially
    // This ensures that pad N always maps to MIDI note (36 + N)
    for (int padIndex = 0; padIndex < 16; ++padIndex)
    {
        // Check if we have a sound for this specific pad position
        if (padIndex < currentSoundsArray.size())
        {
            const FSSound& sound = currentSoundsArray[padIndex];

            // Skip empty sounds (sounds with empty ID)
            if (sound.id.isEmpty())
            {
                DBG("Pad " + String(padIndex) + " is empty, skipping");
                continue;
            }

            String fileName = "FS_ID_" + sound.id + ".ogg";
            File audioFile = currentSessionDownloadLocation.getChildFile(fileName);

            if (audioFile.existsAsFile())
            {
                std::unique_ptr<AudioFormatReader> reader(audioFormatManager.createReaderFor(audioFile));

                if (reader != nullptr)
                {
                    BigInteger notes;
                    // CRITICAL: Use the actual pad index for MIDI note, not the loop counter
                    int midiNote = 36 + padIndex;
                    notes.setBit(midiNote, true);
                    sampler.addSound(new SamplerSound(String(padIndex), *reader, notes, midiNote, 0, maxLength, maxLength));

                    DBG("Successfully loaded sample for pad " + String(padIndex) + " to MIDI note " + String(midiNote) + ": " + audioFile.getFileName());
                }
                else
                {
                    DBG("Failed to create reader for pad " + String(padIndex) + ": " + audioFile.getFullPathName());
                }
            }
            else
            {
                DBG("Audio file not found for pad " + String(padIndex) + ": " + audioFile.getFullPathName());
            }
        }
        else
        {
            DBG("No sound data for pad " + String(padIndex));
        }
    }

    DBG("Sampler now has " + String(sampler.getNumSounds()) + " sounds loaded");
}

void FreesoundAdvancedSamplerAudioProcessor::addToMidiBuffer(int notenumber)
{
	MidiMessage message = MidiMessage::noteOn(10, notenumber, (uint8)100);
	double timestamp = Time::getMillisecondCounterHiRes() * 0.001 - getStartTime();
	message.setTimeStamp(timestamp);

	auto sampleNumber = (int)(timestamp * getSampleRate());
	midiFromEditor.addEvent(message,sampleNumber);
}

double FreesoundAdvancedSamplerAudioProcessor::getStartTime(){
	return startTime;
}

bool FreesoundAdvancedSamplerAudioProcessor::isArrayNotEmpty()
{
	return soundsArray.size() != 0;
}

String FreesoundAdvancedSamplerAudioProcessor::getQuery()
{
	return query;
}

std::vector<juce::StringArray> FreesoundAdvancedSamplerAudioProcessor::getData()
{
	return soundsArray;
}

// NEW: Playback listener methods
void FreesoundAdvancedSamplerAudioProcessor::addPlaybackListener(PlaybackListener* listener)
{
    playbackListeners.add(listener);
}

void FreesoundAdvancedSamplerAudioProcessor::removePlaybackListener(PlaybackListener* listener)
{
    playbackListeners.remove(listener);
}

void FreesoundAdvancedSamplerAudioProcessor::notifyNoteStarted(int noteNumber, float velocity)
{
    playbackListeners.call([noteNumber, velocity](PlaybackListener& l) {
        l.noteStarted(noteNumber, velocity);
    });
}

void FreesoundAdvancedSamplerAudioProcessor::notifyNoteStopped(int noteNumber)
{
    playbackListeners.call([noteNumber](PlaybackListener& l) {
        l.noteStopped(noteNumber);
    });
}

bool FreesoundAdvancedSamplerAudioProcessor::saveCurrentAsPreset(const String& name, const String& description, int slotIndex)
{
    Array<PadInfo> padInfos;

    // Get pad info from the grid if editor is available
    if (auto* editor = dynamic_cast<FreesoundAdvancedSamplerAudioProcessorEditor*>(getActiveEditor()))
    {
        auto& gridComponent = editor->getSampleGridComponent();
        auto allSampleInfo = gridComponent.getAllSampleInfo();

        for (const auto& sampleInfo : allSampleInfo)
        {
            if (sampleInfo.hasValidSample)
            {
                PadInfo padInfo;
                padInfo.padIndex = sampleInfo.padIndex - 1; // Convert from 1-based to 0-based
                padInfo.freesoundId = sampleInfo.freesoundId;
                padInfo.fileName = "FS_ID_" + sampleInfo.freesoundId + ".ogg";
                padInfo.originalName = sampleInfo.sampleName;
                padInfo.author = sampleInfo.authorName;
                padInfo.license = sampleInfo.licenseType;

                // Get additional info from the existing arrays if available
                for (int i = 0; i < currentSoundsArray.size(); ++i)
                {
                    if (currentSoundsArray[i].id == sampleInfo.freesoundId)
                    {
                        padInfo.duration = currentSoundsArray[i].duration;
                        padInfo.fileSize = currentSoundsArray[i].filesize;
                        break;
                    }
                }

                padInfo.downloadedAt = Time::getCurrentTime().toString(true, true);
                padInfos.add(padInfo);
            }
        }
    }
    else
    {
        // Fallback to the old method
        padInfos = getCurrentPadInfos();
    }

    return presetManager.saveCurrentPreset(name, description, padInfos, query, slotIndex);
}

bool FreesoundAdvancedSamplerAudioProcessor::loadPreset(const File& presetFile, int slotIndex)
{
    Array<PadInfo> padInfos;
    if (!presetManager.loadPreset(presetFile, slotIndex, padInfos))
        return false;

    // Clear ALL current state completely
    soundsArray.clear();
    currentSoundsArray.clear();

    // Also clear the sampler before rebuilding
    sampler.clearSounds();
    sampler.clearVoices();

    // Update query from slot info (read from JSON)
    FileInputStream inputStream(presetFile);
    if (inputStream.openedOk())
    {
        String jsonText = inputStream.readEntireStreamAsString();
        var parsedJson = juce::JSON::parse(jsonText);
        if (parsedJson.isObject())
        {
            String slotKey = "slot_" + String(slotIndex);
            var slotData = parsedJson.getProperty(slotKey, var());
            if (slotData.isObject())
            {
                var slotInfo = slotData.getProperty("slot_info", var());
                if (slotInfo.isObject())
                {
                    query = slotInfo.getProperty("search_query", "");
                }
            }
        }
    }

    // Initialize arrays to hold 16 positions (all empty initially)
    currentSoundsArray.resize(16);
    soundsArray.resize(16);

    // Clear all positions
    for (int i = 0; i < 16; ++i)
    {
        currentSoundsArray.set(i, FSSound()); // Empty sound

        StringArray emptyData;
        emptyData.add(""); // Empty name
        emptyData.add(""); // Empty author
        emptyData.add(""); // Empty license
        soundsArray[i] = emptyData;
    }

    // Load samples into their specific pad positions
    for (const auto& padInfo : padInfos)
    {
        // Validate pad index
        if (padInfo.padIndex < 0 || padInfo.padIndex >= 16)
            continue;

        // Check if sample file exists
        File sampleFile = presetManager.getSampleFile(padInfo.freesoundId);
        if (!sampleFile.existsAsFile())
        {
            DBG("Missing sample file for ID: " + padInfo.freesoundId + " at pad " + String(padInfo.padIndex));
            continue;
        }

        // Create FSSound object
        FSSound sound;
        sound.id = padInfo.freesoundId;
        sound.name = padInfo.originalName;
        sound.user = padInfo.author;
        sound.license = padInfo.license;
        sound.duration = padInfo.duration;
        sound.filesize = padInfo.fileSize;

        // Place sound at its original pad position
        currentSoundsArray.set(padInfo.padIndex, sound);

        // Create sound info for legacy compatibility
        StringArray soundData;
        soundData.add(padInfo.originalName);
        soundData.add(padInfo.author);
        soundData.add(padInfo.license);
        soundsArray[padInfo.padIndex] = soundData;

        DBG("Loaded sample at pad " + String(padInfo.padIndex) + ": " + padInfo.originalName);
    }

    // Update current session location to samples folder
    currentSessionDownloadLocation = presetManager.getSamplesFolder();

    // Force reload sampler with new data
    setSources();

    // Debug output
    int loadedCount = 0;
    for (int i = 0; i < 16; ++i)
    {
        if (!currentSoundsArray[i].id.isEmpty())
        {
            loadedCount++;
            DBG("Pad " + String(i) + ": " + currentSoundsArray[i].name);
        }
        else
        {
            DBG("Pad " + String(i) + ": empty");
        }
    }

    DBG("Loaded preset slot " + String(slotIndex) + " with " + String(loadedCount) + " samples in their original positions");

    return true;
}

bool FreesoundAdvancedSamplerAudioProcessor::saveToSlot(const File& presetFile, int slotIndex, const String& description)
{
    Array<PadInfo> padInfos;

    // Get pad info from the grid if editor is available
    if (auto* editor = dynamic_cast<FreesoundAdvancedSamplerAudioProcessorEditor*>(getActiveEditor()))
    {
        auto& gridComponent = editor->getSampleGridComponent();
        auto allSampleInfo = gridComponent.getAllSampleInfo();

        for (const auto& sampleInfo : allSampleInfo)
        {
            if (sampleInfo.hasValidSample)
            {
                PadInfo padInfo;
                padInfo.padIndex = sampleInfo.padIndex - 1; // Convert from 1-based to 0-based
                padInfo.freesoundId = sampleInfo.freesoundId;
                padInfo.fileName = "FS_ID_" + sampleInfo.freesoundId + ".ogg";
                padInfo.originalName = sampleInfo.sampleName;
                padInfo.author = sampleInfo.authorName;
                padInfo.license = sampleInfo.licenseType;

                // Get additional info from the existing arrays
                for (int i = 0; i < currentSoundsArray.size(); ++i)
                {
                    if (currentSoundsArray[i].id == sampleInfo.freesoundId)
                    {
                        padInfo.duration = currentSoundsArray[i].duration;
                        padInfo.fileSize = currentSoundsArray[i].filesize;
                        break;
                    }
                }

                padInfo.downloadedAt = Time::getCurrentTime().toString(true, true);
                padInfos.add(padInfo);
            }
        }
    }
    else
    {
        // Fallback to the old method
        padInfos = getCurrentPadInfos();
    }

    return presetManager.saveToSlot(presetFile, slotIndex, description, padInfos, query);
}

Array<PadInfo> FreesoundAdvancedSamplerAudioProcessor::getCurrentPadInfos() const
{
    Array<PadInfo> padInfos;

    // Get pad info from the grid to preserve exact positions
    if (auto* editor = dynamic_cast<FreesoundAdvancedSamplerAudioProcessorEditor*>(getActiveEditor()))
    {
        auto& gridComponent = editor->getSampleGridComponent();

        // Iterate through all 16 pad positions
        for (int padIndex = 0; padIndex < 16; ++padIndex)
        {
            auto sampleInfo = gridComponent.getPadInfo(padIndex);

            if (sampleInfo.hasValidSample)
            {
                PadInfo padInfo;
                padInfo.padIndex = padIndex; // Use actual pad position (0-based)
                padInfo.freesoundId = sampleInfo.freesoundId;
                padInfo.fileName = "FS_ID_" + sampleInfo.freesoundId + ".ogg";
                padInfo.originalName = sampleInfo.sampleName;
                padInfo.author = sampleInfo.authorName;
                padInfo.license = sampleInfo.licenseType;

                // Get duration and file size from processor arrays if available
                for (const auto& sound : currentSoundsArray)
                {
                    if (sound.id == sampleInfo.freesoundId)
                    {
                        padInfo.duration = sound.duration;
                        padInfo.fileSize = sound.filesize;
                        break;
                    }
                }

                padInfo.downloadedAt = Time::getCurrentTime().toString(true, true);
                padInfos.add(padInfo);
            }
        }
    }
    else
    {
        // Fallback method when no editor is available
        for (int i = 0; i < jmin(currentSoundsArray.size(), (int)soundsArray.size()); ++i)
        {
            const FSSound& sound = currentSoundsArray[i];

            if (sound.id.isEmpty())
                continue;

            PadInfo padInfo;
            padInfo.padIndex = i;
            padInfo.freesoundId = sound.id;
            padInfo.fileName = "FS_ID_" + sound.id + ".ogg";
            padInfo.originalName = sound.name;
            padInfo.author = sound.user;
            padInfo.license = sound.license;
            padInfo.duration = sound.duration;
            padInfo.fileSize = sound.filesize;
            padInfo.downloadedAt = Time::getCurrentTime().toString(true, true);

            padInfos.add(padInfo);
        }
    }

    return padInfos;
}

Array<PadInfo> FreesoundAdvancedSamplerAudioProcessor::getCurrentPadInfosFromGrid() const
{
    Array<PadInfo> padInfos;

    // Get the grid component from the editor
    if (auto* editor = dynamic_cast<FreesoundAdvancedSamplerAudioProcessorEditor*>(getActiveEditor()))
    {
        auto& gridComponent = editor->getSampleGridComponent();
        auto allSampleInfo = gridComponent.getAllSampleInfo();

        for (const auto& sampleInfo : allSampleInfo)
        {
            if (sampleInfo.hasValidSample)
            {
                PadInfo padInfo;
                padInfo.padIndex = sampleInfo.padIndex - 1; // Convert from 1-based to 0-based
                padInfo.freesoundId = sampleInfo.freesoundId;
                padInfo.fileName = "FS_ID_" + sampleInfo.freesoundId + ".ogg";
                padInfo.originalName = sampleInfo.sampleName;
                padInfo.author = sampleInfo.authorName;
                padInfo.license = sampleInfo.licenseType;

                // Get additional info from the existing arrays if available
                for (int i = 0; i < currentSoundsArray.size(); ++i)
                {
                    if (currentSoundsArray[i].id == sampleInfo.freesoundId)
                    {
                        padInfo.duration = currentSoundsArray[i].duration;
                        padInfo.fileSize = currentSoundsArray[i].filesize;
                        break;
                    }
                }

                padInfo.downloadedAt = Time::getCurrentTime().toString(true, true);
                padInfos.add(padInfo);
            }
        }
    }
    else
    {
        // Fallback to the old method if no editor is available
        return getCurrentPadInfos();
    }

    return padInfos;
}

void FreesoundAdvancedSamplerAudioProcessor::generateReadmeFile(const Array<FSSound>& sounds, const std::vector<StringArray>& soundInfo, const String& searchQuery)
{
    if (currentSessionDownloadLocation.exists())
    {
        File readmeFile = currentSessionDownloadLocation.getChildFile("README.md");

        // Get current date and time
        Time currentTime = Time::getCurrentTime();
        String dateTimeStr = currentTime.formatted("%B %d, %Y at %H:%M:%S");

        String readmeContent;

        // Header section
        readmeContent += "# Freesound Sample Collection\n\n";
        readmeContent += "**Search Query:** `" + searchQuery + "`\n";
        readmeContent += "**Date Downloaded:** " + dateTimeStr + "\n";
        readmeContent += "**Total Samples:** " + String(sounds.size()) + "\n\n";

        // Description
        readmeContent += "## Description\n\n";
        readmeContent += "This collection contains audio samples downloaded from [Freesound.org](https://freesound.org) ";
        readmeContent += "using the search query \"" + searchQuery + "\". ";
        readmeContent += "Files are named using their original Freesound names plus the Freesound ID for uniqueness.\n\n";

        // Table header
        readmeContent += "## Sample Details\n\n";
        readmeContent += "| # | File Name | Original Name | Author | License | Search Query | Duration | File Size | Freesound ID | URL |\n";
        readmeContent += "|---|-----------|---------------|--------|---------|--------------|----------|-----------|--------------|-----|\n";

        // Table rows
        for (int i = 0; i < sounds.size(); ++i)
        {
            const FSSound& sound = sounds[i];

            // File name using new naming scheme
            String originalName = sound.name;
            String fileName = "_FS_" + sound.id + ".ogg";

            // Sample info
            String sampleName = (i < soundInfo.size()) ? soundInfo[i][0] : "Unknown";
            String authorName = (i < soundInfo.size()) ? soundInfo[i][1] : "Unknown";
            String license = (i < soundInfo.size()) ? soundInfo[i][2] : "Unknown";

            // Clean up text for markdown table (escape pipes and newlines)
            sampleName = sampleName.replace("|", "\\|").replace("\n", " ").replace("\r", "");
            authorName = authorName.replace("|", "\\|").replace("\n", " ").replace("\r", "");
            license = license.replace("|", "\\|").replace("\n", " ").replace("\r", "");

            // Duration (convert to human readable)
            String durationStr = String(sound.duration) + "s";

            // File size (convert to human readable)
            String fileSizeStr;
            if (sound.filesize > 1024 * 1024)
                fileSizeStr = String(sound.filesize / (1024 * 1024)) + " MB";
            else if (sound.filesize > 1024)
                fileSizeStr = String(sound.filesize / 1024) + " KB";
            else
                fileSizeStr = String(sound.filesize) + " B";

            // Freesound URL
            String freesoundUrl = "https://freesound.org/s/" + sound.id + "/";

            // Add table row
            readmeContent += "| " + String(i + 1) + " | ";
            readmeContent += "`" + fileName + "` | ";
            readmeContent += sampleName + " | ";
            readmeContent += authorName + " | ";
            readmeContent += license + " | ";
            readmeContent += "`" + searchQuery + "` | ";
            readmeContent += durationStr + " | ";
            readmeContent += fileSizeStr + " | ";
            readmeContent += sound.id + " | ";
            readmeContent += "[Link](" + freesoundUrl + ") |\n";
        }

        // Footer section
        readmeContent += "\n## License Information\n\n";
        readmeContent += "Read more about licenses used in Freesound, refer to [https://freesound.org/help/faq/#licenses](https://freesound.org/help/faq/#licenses).\n\n";

        readmeContent += "## Usage\n\n";
        readmeContent += "- **File Naming:** Files are named using their original Freesound names plus Freesound ID\n";
        readmeContent += "- **MIDI Mapping:** Each sample responds to MIDI notes starting from 36 (C2) in the order they appear in this list\n";
        readmeContent += "- **Reordering:** You can easily reorder samples by moving the files around\n";
        readmeContent += "- **File Format:** All samples are in OGG format (Freesound previews)\n";
        readmeContent += "- **Metadata:** Complete metadata is stored in `metadata.json` for programmatic access\n\n";

        readmeContent += "---\n";
        readmeContent += "*Generated by Freesound Advanced Sampler*\n";

        // Write to file
        readmeFile.replaceWithText(readmeContent);
    }
}

// Add this method implementation to PluginProcessor.cpp

void FreesoundAdvancedSamplerAudioProcessor::updateReadmeFile()
{
    if (!currentSessionDownloadLocation.exists())
        return;

    File metadataFile = currentSessionDownloadLocation.getChildFile("metadata.json");

    if (!metadataFile.existsAsFile())
        return;

    // Read current JSON metadata
    FileInputStream inputStream(metadataFile);
    if (!inputStream.openedOk())
        return;

    String jsonText = inputStream.readEntireStreamAsString();
    var parsedJson = juce::JSON::parse(jsonText);

    if (!parsedJson.isObject())
        return;

    var sessionInfo = parsedJson.getProperty("session_info", var());
    var samplesArray = parsedJson.getProperty("samples", var());

    if (!sessionInfo.isObject() || !samplesArray.isArray())
        return;

    // Extract session info
    String searchQuery = sessionInfo.getProperty("search_query", query);
    String downloadDate = sessionInfo.getProperty("download_date", "Unknown");
    int totalSamples = (int)samplesArray.size();

    // Regenerate README with current order
    File readmeFile = currentSessionDownloadLocation.getChildFile("README.md");

    String readmeContent;

    // Header section
    readmeContent += "# Freesound Sample Collection\n\n";
    readmeContent += "**Search Query:** `" + searchQuery + "`\n";
    readmeContent += "**Date Downloaded:** " + downloadDate + "\n";
    readmeContent += "**Total Samples:** " + String(totalSamples) + "\n\n";

    // Description
    readmeContent += "## Description\n\n";
    readmeContent += "This collection contains audio samples downloaded from [Freesound.org](https://freesound.org) ";
    readmeContent += "using the search query \"" + searchQuery + "\". ";
    readmeContent += "Files are named using their original Freesound names plus the Freesound ID for uniqueness.\n\n";

    // Note about reordering
    readmeContent += "**Note:** This collection has been reordered by dragging and dropping samples in the grid interface.\n\n";

    // Table header
    readmeContent += "## Sample Details\n\n";
    readmeContent += "| # | File Name | Original Name | Author | License | Search Query | Duration | File Size | Freesound ID | URL |\n";
    readmeContent += "|---|-----------|---------------|--------|---------|--------------|----------|-----------|--------------|-----|\n";

    // Table rows based on current JSON order
    for (int i = 0; i < samplesArray.size(); ++i)
    {
        var sample = samplesArray[i];
        if (!sample.isObject())
            continue;

        String fileName = sample.getProperty("file_name", "");
        String originalName = sample.getProperty("original_name", "Unknown");
        String authorName = sample.getProperty("author", "Unknown");
        String license = sample.getProperty("license", "Unknown");
        String freesoundId = sample.getProperty("freesound_id", "");
        String freesoundUrl = sample.getProperty("freesound_url", "");
        double duration = sample.getProperty("duration", 0.0);
        int fileSize = sample.getProperty("file_size", 0);

        // Clean up text for markdown table (escape pipes and newlines)
        originalName = originalName.replace("|", "\\|").replace("\n", " ").replace("\r", "");
        authorName = authorName.replace("|", "\\|").replace("\n", " ").replace("\r", "");
        license = license.replace("|", "\\|").replace("\n", " ").replace("\r", "");

        // Duration (convert to human readable)
        String durationStr = String(duration) + "s";

        // File size (convert to human readable)
        String fileSizeStr;
        if (fileSize > 1024 * 1024)
            fileSizeStr = String(fileSize / (1024 * 1024)) + " MB";
        else if (fileSize > 1024)
            fileSizeStr = String(fileSize / 1024) + " KB";
        else
            fileSizeStr = String(fileSize) + " B";

        // Add table row
        readmeContent += "| " + String(i + 1) + " | ";
        readmeContent += "`" + fileName + "` | ";
        readmeContent += originalName + " | ";
        readmeContent += authorName + " | ";
        readmeContent += license + " | ";
        readmeContent += "`" + searchQuery + "` | ";
        readmeContent += durationStr + " | ";
        readmeContent += fileSizeStr + " | ";
        readmeContent += freesoundId + " | ";
        readmeContent += "[Link](" + freesoundUrl + ") |\n";
    }

    // Footer section
    readmeContent += "\n## License Information\n\n";
    readmeContent += "Read more about licenses used in Freesound, refer to [https://freesound.org/help/faq/#licenses](https://freesound.org/help/faq/#licenses).\n\n";

    readmeContent += "## Usage\n\n";
    readmeContent += "- **File Naming:** Files are named using their original Freesound names plus Freesound ID\n";
    readmeContent += "- **MIDI Mapping:** Each sample responds to MIDI notes starting from 36 (C2) in the order they appear in this list\n";
    readmeContent += "- **Reordering:** You can easily reorder samples by dragging and dropping them in the grid interface\n";
    readmeContent += "- **File Format:** All samples are in OGG format (Freesound previews)\n";
    readmeContent += "- **Metadata:** Complete metadata is stored in `metadata.json` for programmatic access\n\n";

    readmeContent += "---\n";
    readmeContent += "*Generated by Freesound Advanced Sampler*\n";

    // Write to file
    readmeFile.replaceWithText(readmeContent);
}


//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FreesoundAdvancedSamplerAudioProcessor();
}

void FreesoundAdvancedSamplerAudioProcessor::notifyPlayheadPositionChanged(int noteNumber, float position)
{
    playbackListeners.call([noteNumber, position](PlaybackListener& l) {
        l.playheadPositionChanged(noteNumber, position);
    });
}

// Add these method implementations to your PluginProcessor.cpp file:

Array<FSSound> FreesoundAdvancedSamplerAudioProcessor::getCurrentSounds()
{
    return currentSoundsArray;
}

File FreesoundAdvancedSamplerAudioProcessor::getCurrentDownloadLocation()
{
    return currentSessionDownloadLocation;
}