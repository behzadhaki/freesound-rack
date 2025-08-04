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
                        , bookmarkManager(File::getSpecialLocation(File::userDocumentsDirectory).getChildFile("FreesoundAdvancedSampler")) // Add this
#endif
{
    tmpDownloadLocation = File::getSpecialLocation(File::userDocumentsDirectory).getChildFile("FreesoundAdvancedSampler");
    tmpDownloadLocation.createDirectory();
    currentSessionDownloadLocation = presetManager.getSamplesFolder(); // Use samples folder as default
    midicounter = 1;
    startTime = Time::getMillisecondCounterHiRes() * 0.001;

    // Initialize preview sampler (always runs in parallel)
    if (previewAudioFormatManager.getNumKnownFormats() == 0) {
        previewAudioFormatManager.registerBasicFormats();
    }

    // Add a single voice for preview sampler
    previewSampler.addVoice(new SamplerVoice());

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

void FreesoundAdvancedSamplerAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    // Add MIDI events from editor
    midiMessages.addEvents(midiFromEditor, 0, INT_MAX, 0);
    midiFromEditor.clear();

    // Create separate MIDI buffers for main and preview samplers
    MidiBuffer mainMidiBuffer;
    MidiBuffer previewMidiBuffer;

    // Split MIDI messages by channel
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();

        if (message.getChannel() == 2) // Preview channel
        {
            previewMidiBuffer.addEvent(message, metadata.samplePosition);
        }
        else // Main sampler channel(s)
        {
            mainMidiBuffer.addEvent(message, metadata.samplePosition);
        }
    }

    // Render main sampler
    sampler.renderNextBlock(buffer, mainMidiBuffer, 0, buffer.getNumSamples());

    // Render preview sampler on top (mix with main output)
    if (previewSampler.getNumSounds() > 0)
    {
        AudioBuffer<float> previewBuffer(buffer.getNumChannels(), buffer.getNumSamples());
        previewBuffer.clear();

        previewSampler.renderNextBlock(previewBuffer, previewMidiBuffer, 0, buffer.getNumSamples());

        // Mix preview with main output at reduced volume
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            buffer.addFrom(channel, 0, previewBuffer, channel, 0, buffer.getNumSamples(), 0.6f); // 60% volume for preview
        }
    }

    midiMessages.clear();
}

// Modify prepareToPlay to prepare both samplers
void FreesoundAdvancedSamplerAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    sampler.setCurrentPlaybackSampleRate(sampleRate);
    previewSampler.setCurrentPlaybackSampleRate(sampleRate);
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
void FreesoundAdvancedSamplerAudioProcessor::getStateInformation(MemoryBlock& destData)
{
    // Create an outer XML element
    auto xml = std::make_unique<XmlElement>("FreesoundAdvancedSamplerState");

    // Save current state
    savePluginState(*xml);

    // Store as binary
    copyXmlToBinary(*xml, destData);
}

void FreesoundAdvancedSamplerAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Try to load from binary
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml != nullptr)
    {
        loadPluginState(*xml);
    }
}

void FreesoundAdvancedSamplerAudioProcessor::savePluginState(XmlElement& xml)
{
    // Save basic plugin state
    xml.setAttribute("query", query);
    xml.setAttribute("lastDownloadLocation", currentSessionDownloadLocation.getFullPathName());

    // Save window dimensions
    xml.setAttribute("windowWidth", savedWindowWidth);
    xml.setAttribute("windowHeight", savedWindowHeight);

    // NEW: Save active preset state
    xml.setAttribute("activePresetFile", presetManager.getActivePresetFile().getFullPathName());
    xml.setAttribute("activeSlotIndex", presetManager.getActiveSlotIndex());

    // Save current sounds and their positions
    auto* soundsXml = xml.createNewChildElement("Sounds");
    for (int i = 0; i < currentSoundsArray.size(); ++i)
    {
        const auto& sound = currentSoundsArray[i];
        if (sound.id.isNotEmpty())
        {
            auto* soundXml = soundsXml->createNewChildElement("Sound");
            soundXml->setAttribute("padIndex", i);
            soundXml->setAttribute("id", sound.id);
            soundXml->setAttribute("name", sound.name);
            soundXml->setAttribute("user", sound.user);
            soundXml->setAttribute("license", sound.license);
            soundXml->setAttribute("duration", sound.duration);
            soundXml->setAttribute("filesize", sound.filesize);

            // Save associated metadata including query from soundsArray
            if (i < soundsArray.size() && soundsArray[i].size() >= 4)
            {
                soundXml->setAttribute("displayName", soundsArray[i][0]);
                soundXml->setAttribute("displayAuthor", soundsArray[i][1]);
                soundXml->setAttribute("displayLicense", soundsArray[i][2]);
                soundXml->setAttribute("searchQuery", soundsArray[i][3]);
            }
        }
    }

    xml.setAttribute("presetPanelExpanded", presetPanelExpandedState);
}

void FreesoundAdvancedSamplerAudioProcessor::loadPluginState(const XmlElement& xml)
{
    // Load basic plugin state
    query = xml.getStringAttribute("query", "");
    currentSessionDownloadLocation = File(xml.getStringAttribute("lastDownloadLocation",
        presetManager.getSamplesFolder().getFullPathName()));

    // Load window dimensions
    savedWindowWidth = xml.getIntAttribute("windowWidth", 1000);
    savedWindowHeight = xml.getIntAttribute("windowHeight", 700);

    // NEW: Load active preset state
    String activePresetPath = xml.getStringAttribute("activePresetFile", "");
    int activeSlot = xml.getIntAttribute("activeSlotIndex", -1);

    if (activePresetPath.isNotEmpty() && activeSlot >= 0)
    {
        File activePresetFile(activePresetPath);
        if (activePresetFile.existsAsFile())
        {
            presetManager.setActivePreset(activePresetFile, activeSlot);
            DBG("Restored active preset: " + activePresetFile.getFileName() + ", slot " + String(activeSlot));
        }
    }

    // Clear current state
    currentSoundsArray.clear();
    soundsArray.clear();
    currentSoundsArray.resize(16);
    soundsArray.resize(16);

    // Clear all positions - ensure soundsArray has 4 elements
    for (int i = 0; i < 16; ++i)
    {
        currentSoundsArray.set(i, FSSound()); // Empty sound

        StringArray emptyData;
        emptyData.add(""); // Empty name
        emptyData.add(""); // Empty author
        emptyData.add(""); // Empty license
        emptyData.add(""); // Empty query - ADD THIS as 4th element
        soundsArray[i] = emptyData;
    }

    // Load sounds with queries
    if (auto* soundsXml = xml.getChildByName("Sounds"))
    {
        for (auto* soundXml : soundsXml->getChildIterator())
        {
            if (soundXml->hasTagName("Sound"))
            {
                int padIndex = soundXml->getIntAttribute("padIndex", -1);
                if (padIndex >= 0 && padIndex < 16)
                {
                    FSSound sound;
                    sound.id = soundXml->getStringAttribute("id");
                    sound.name = soundXml->getStringAttribute("name");
                    sound.user = soundXml->getStringAttribute("user");
                    sound.license = soundXml->getStringAttribute("license");
                    sound.duration = soundXml->getDoubleAttribute("duration", 0.5);
                    sound.filesize = soundXml->getIntAttribute("filesize", 50000);

                    currentSoundsArray.set(padIndex, sound);

                    // Create sound info including query in 4th position
                    StringArray soundData;
                    soundData.add(soundXml->getStringAttribute("displayName", sound.name));
                    soundData.add(soundXml->getStringAttribute("displayAuthor", sound.user));
                    soundData.add(soundXml->getStringAttribute("displayLicense", sound.license));
                    soundData.add(soundXml->getStringAttribute("searchQuery", ""));  // Query as 4th element
                    soundsArray[padIndex] = soundData;
                }
            }
        }
    }

    // If we have sounds, set up the sampler
    bool hasSounds = false;
    for (const auto& sound : currentSoundsArray)
    {
        if (sound.id.isNotEmpty())
        {
            hasSounds = true;
            break;
        }
    }

    if (hasSounds)
    {
        setSources();

        // Update the editor if available
        if (auto* editor = dynamic_cast<FreesoundAdvancedSamplerAudioProcessorEditor*>(getActiveEditor()))
        {
            editor->getSampleGridComponent().updateSamples(currentSoundsArray, soundsArray);
        }
    }

    presetPanelExpandedState = xml.getBoolAttribute("presetPanelExpanded", false);
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

    // Add tracking voices
    for (int i = 0; i < poliphony; i++) {
        sampler.addVoice(new TrackingSamplerVoice(*this));
    }

    if (audioFormatManager.getNumKnownFormats() == 0) {
        audioFormatManager.registerBasicFormats();
    }

    // Load samples by their actual pad positions - THIS IS CRITICAL
    for (int padIndex = 0; padIndex < 16; ++padIndex)
    {
        // Check if we have a sound for this specific pad position
        if (padIndex < currentSoundsArray.size() && !currentSoundsArray[padIndex].id.isEmpty())
        {
            const FSSound& sound = currentSoundsArray[padIndex];

            String fileName = "FS_ID_" + sound.id + ".ogg";
            File audioFile = currentSessionDownloadLocation.getChildFile(fileName);

            if (audioFile.existsAsFile())
            {
                std::unique_ptr<AudioFormatReader> reader(audioFormatManager.createReaderFor(audioFile));

                if (reader != nullptr)
                {
                    BigInteger notes;
                    int midiNote = 36 + padIndex; // CRITICAL: Note 36 = pad 0, Note 37 = pad 1, etc.
                    notes.setBit(midiNote, true);
                    sampler.addSound(new SamplerSound(String(padIndex), *reader, notes, midiNote, 0, maxLength, maxLength));

                    DBG("Added sample to sampler - Pad " + String(padIndex) + " -> MIDI note " + String(midiNote) + " (" + sound.name + ")");
                }
            }
        }
    }

    DBG("Sampler rebuild complete with " + String(sampler.getNumSounds()) + " sounds");
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

    // Read query from slot info in the JSON (for master query)
    String masterQuery = "";
    {
        String jsonText = presetFile.loadFileAsString();
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
                    masterQuery = slotInfo.getProperty("search_query", "");
                }
            }
        }
    }

    // Clear ALL current state completely
    soundsArray.clear();
    currentSoundsArray.clear();

    // Also clear the sampler before rebuilding
    sampler.clearSounds();
    sampler.clearVoices();

    // Update query from slot info
    query = masterQuery;

    // Initialize arrays to hold 16 positions (all empty initially)
    currentSoundsArray.resize(16);
    soundsArray.resize(16);

    // Clear all positions - ensure soundsArray has 4 elements including query
    for (int i = 0; i < 16; ++i)
    {
        currentSoundsArray.set(i, FSSound()); // Empty sound

        StringArray emptyData;
        emptyData.add(""); // Empty name
        emptyData.add(""); // Empty author
        emptyData.add(""); // Empty license
        emptyData.add(""); // Empty query - 4th element
        soundsArray[i] = emptyData;
    }

    // Load samples into their specific pad positions WITH QUERIES
    for (const auto& padInfo : padInfos)
    {
        // Validate pad index
        if (padInfo.padIndex < 0 || padInfo.padIndex >= 16)
            continue;

        // Check if sample file exists
        File sampleFile = presetManager.getSampleFile(padInfo.freesoundId);
        if (!sampleFile.existsAsFile())
        {
            // DBG("Missing sample file for ID: " + padInfo.freesoundId + " at pad " + String(padInfo.padIndex));
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

        // Create sound info for legacy compatibility WITH QUERY IN 4TH POSITION
        StringArray soundData;
        soundData.add(padInfo.originalName);
        soundData.add(padInfo.author);
        soundData.add(padInfo.license);
        soundData.add(padInfo.searchQuery);  // IMPORTANT: Store query in 4th position
        soundsArray[padInfo.padIndex] = soundData;

        // DBG("Loaded sample at pad " + String(padInfo.padIndex) + ": " + padInfo.originalName + " with query: " + padInfo.searchQuery);
    }

    // Update current session location to samples folder
    currentSessionDownloadLocation = presetManager.getSamplesFolder();

    // Force reload sampler with new data
    setSources();

    // CRITICAL: Update the visual grid with the loaded data INCLUDING QUERIES
    if (auto* editor = dynamic_cast<FreesoundAdvancedSamplerAudioProcessorEditor*>(getActiveEditor()))
    {
        // This will update the visual grid and restore queries to text boxes
        editor->getSampleGridComponent().updateSamples(currentSoundsArray, soundsArray);
    }

    // DBG("Loaded preset slot " + String(slotIndex) + " with master query: " + masterQuery);

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
                padInfo.padIndex = padIndex;
                padInfo.freesoundId = sampleInfo.freesoundId;
                padInfo.fileName = "FS_ID_" + sampleInfo.freesoundId + ".ogg";
                padInfo.originalName = sampleInfo.sampleName;
                padInfo.author = sampleInfo.authorName;
                padInfo.license = sampleInfo.licenseType;
                padInfo.searchQuery = sampleInfo.query;  // Get from pad's UI

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
        // Fallback method - get queries from soundsArray
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

            // Get query from soundsArray 4th element
            padInfo.searchQuery = (soundsArray[i].size() > 3) ? soundsArray[i][3] : "";

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
    /*if (currentSessionDownloadLocation.exists())
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
    }*/
}

void FreesoundAdvancedSamplerAudioProcessor::updateReadmeFile()
{
    /*if (!currentSessionDownloadLocation.exists())
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
    readmeFile.replaceWithText(readmeContent);*/
}

void FreesoundAdvancedSamplerAudioProcessor::loadPreviewSample(const File& audioFile, const String& freesoundId)
{
    if (!audioFile.existsAsFile())
        return;

    // Clear existing preview sounds but keep the voice
    previewSampler.clearSounds();

    // Load the preview sample
    std::unique_ptr<AudioFormatReader> reader(previewAudioFormatManager.createReaderFor(audioFile));
    if (reader != nullptr)
    {
        BigInteger notes;
        int previewNote = 127; // Use highest MIDI note for preview (won't conflict with main grid)
        notes.setBit(previewNote, true);

        // Use 0.0 for release and maxLength to allow proper preview playback
        // For preview, you might want a short release when mouse is released
        double maxLength = 10.0;
        double attackTime = 0.0;
        double releaseTime = 0.1; // set to sample maxLength to play for the full duration

        previewSampler.addSound(new SamplerSound("preview_" + freesoundId, *reader, notes, previewNote,
                                                 attackTime, releaseTime, maxLength));
    }
}

void FreesoundAdvancedSamplerAudioProcessor::playPreviewSample()
{
    // Stop any currently playing preview first
    stopPreviewSample();

    // Trigger the preview sample
    int previewNote = 127;
    MidiMessage message = MidiMessage::noteOn(2, previewNote, (uint8)100); // Channel 2 for preview
    double timestamp = Time::getMillisecondCounterHiRes() * 0.001 - getStartTime();
    message.setTimeStamp(timestamp);

    auto sampleNumber = (int)(timestamp * getSampleRate());
    midiFromEditor.addEvent(message, sampleNumber);

    DBG("Preview sample triggered");
}

void FreesoundAdvancedSamplerAudioProcessor::stopPreviewSample()
{
    // Send note off for preview
    int previewNote = 127;
    MidiMessage message = MidiMessage::noteOff(2, previewNote, (uint8)0);
    double timestamp = Time::getMillisecondCounterHiRes() * 0.001 - getStartTime();
    message.setTimeStamp(timestamp);

    auto sampleNumber = (int)(timestamp * getSampleRate());
    midiFromEditor.addEvent(message, sampleNumber);
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