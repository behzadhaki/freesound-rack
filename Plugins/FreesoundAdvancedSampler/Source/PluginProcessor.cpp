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
// TrackingPreviewSamplerVoice Implementation (Add after TrackingSamplerVoice)
//==============================================================================

FreesoundAdvancedSamplerAudioProcessor::TrackingPreviewSamplerVoice::TrackingPreviewSamplerVoice(FreesoundAdvancedSamplerAudioProcessor& owner)
    : processor(owner)
{
}

void FreesoundAdvancedSamplerAudioProcessor::TrackingPreviewSamplerVoice::startNote(int midiNoteNumber, float velocity, SynthesiserSound* sound, int currentPitchWheelPosition)
{

    // Call parent first
    SamplerVoice::startNote(midiNoteNumber, velocity, sound, currentPitchWheelPosition);

    // Extract freesound ID from the sound name
    if (auto* samplerSound = dynamic_cast<SamplerSound*>(sound))
    {
        String soundName = samplerSound->getName();

        if (soundName.startsWith("preview_"))
        {
            currentFreesoundId = soundName.substring(8); // Remove "preview_" prefix
            samplePosition = 0.0;
            sampleLength = samplerSound->getAudioData()->getNumSamples();

            // Notify that preview started
            processor.notifyPreviewStarted(currentFreesoundId);
        }
        else
        {
            DBG("  - Sound name doesn't start with 'preview_', ignoring");
        }
    }
    else
    {
        DBG("  - Not a SamplerSound, ignoring");
    }
}

void FreesoundAdvancedSamplerAudioProcessor::TrackingPreviewSamplerVoice::stopNote(float velocity, bool allowTailOff)
{
    if (currentFreesoundId.isNotEmpty())
    {
        processor.notifyPreviewStopped(currentFreesoundId);
        currentFreesoundId = "";
        samplePosition = 0.0;
        sampleLength = 0.0;
    }

    SamplerVoice::stopNote(velocity, allowTailOff);
}

void FreesoundAdvancedSamplerAudioProcessor::TrackingPreviewSamplerVoice::renderNextBlock(AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    // Call parent first to actually render the audio
    SamplerVoice::renderNextBlock(outputBuffer, startSample, numSamples);

    // Update playhead position for preview
    if (currentFreesoundId.isNotEmpty() && sampleLength > 0 && isVoiceActive())
    {
        samplePosition += numSamples;
        float position = (float)samplePosition / (float)sampleLength;

        // Always send position updates for smooth animation
        processor.notifyPreviewPlayheadPositionChanged(currentFreesoundId, jlimit(0.0f, 1.0f, position));

        // Always send the final position when near the end
        if (position >= 0.99f)
        {
            processor.notifyPreviewPlayheadPositionChanged(currentFreesoundId, 1.0f);
        }
    }
}


//==============================================================================
// FreesoundAdvancedSamplerAudioProcessor Implementation
//==============================================================================

FreesoundAdvancedSamplerAudioProcessor::FreesoundAdvancedSamplerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    // Initialize collection manager
    File baseDir = File::getSpecialLocation(File::userDocumentsDirectory)
                      .getChildFile("FreesoundAdvancedSampler");
    collectionManager = std::make_unique<SampleCollectionManager>(baseDir);

    // Initialize pad state
    currentPadFreesoundIds.resize(16);

    tmpDownloadLocation = File::getSpecialLocation(File::userDocumentsDirectory).getChildFile("FreesoundAdvancedSampler");
    tmpDownloadLocation.createDirectory();
    currentSessionDownloadLocation = collectionManager->getSamplesFolder();
    midicounter = 1;
    startTime = Time::getMillisecondCounterHiRes() * 0.001;

    // Initialize preview sampler format manager
    if (previewAudioFormatManager.getNumKnownFormats() == 0) {
        previewAudioFormatManager.registerBasicFormats();
    }

    // FIXED: Add tracking voice for preview sampler (not regular voice)
    previewSampler.addVoice(new TrackingPreviewSamplerVoice(*this));

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
    xml.setAttribute("activePresetId", activePresetId);
    xml.setAttribute("activeSlotIndex", activeSlotIndex);

    // FIXED: Save current pad state with duplicates
    auto* padsXml = xml.createNewChildElement("CurrentPads");
    for (int i = 0; i < 16; ++i)
    {
        String freesoundId = getPadFreesoundId(i);
        if (freesoundId.isNotEmpty())
        {
            auto* padXml = padsXml->createNewChildElement("Pad");
            padXml->setAttribute("index", i);
            padXml->setAttribute("freesoundId", freesoundId);

            // Save individual query if available
            if (auto* editor = dynamic_cast<FreesoundAdvancedSamplerAudioProcessorEditor*>(getActiveEditor()))
            {
                if (i < editor->getSampleGridComponent().samplePads.size())
                {
                    String individualQuery = editor->getSampleGridComponent().samplePads[i]->getQuery();
                    if (individualQuery.isNotEmpty())
                    {
                        padXml->setAttribute("query", individualQuery);
                    }
                }
            }
        }
    }

    // Save basic plugin state
    xml.setAttribute("query", query);
    xml.setAttribute("lastDownloadLocation", currentSessionDownloadLocation.getFullPathName());

    // Save window dimensions
    xml.setAttribute("windowWidth", savedWindowWidth);
    xml.setAttribute("windowHeight", savedWindowHeight);

    // Save panel states
    xml.setAttribute("presetPanelExpanded", presetPanelExpandedState);
    xml.setAttribute("bookmarkPanelExpanded", bookmarkPanelExpandedState);

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
            soundXml->setAttribute("tags", sound.tags.joinIntoString(","));
            soundXml->setAttribute("description", sound.description);

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
    activePresetId = xml.getStringAttribute("activePresetId", "");
    activeSlotIndex = xml.getIntAttribute("activeSlotIndex", -1);

    // Clear current state
    clearAllPads();

    // FIXED: Load pad state with duplicates and individual queries
    if (auto* padsXml = xml.getChildByName("CurrentPads"))
    {
        for (auto* padXml : padsXml->getChildIterator())
        {
            if (padXml->hasTagName("Pad"))
            {
                int padIndex = padXml->getIntAttribute("index", -1);
                String freesoundId = padXml->getStringAttribute("freesoundId", "");
                String individualQuery = padXml->getStringAttribute("query", "");

                if (padIndex >= 0 && padIndex < 16 && freesoundId.isNotEmpty())
                {
                    setPadSample(padIndex, freesoundId);

                    // Restore individual query after UI is ready
                    Timer::callAfterDelay(100, [this, padIndex, individualQuery]() {
                        if (auto* editor = dynamic_cast<FreesoundAdvancedSamplerAudioProcessorEditor*>(getActiveEditor()))
                        {
                            if (padIndex < editor->getSampleGridComponent().samplePads.size() &&
                                individualQuery.isNotEmpty())
                            {
                                editor->getSampleGridComponent().samplePads[padIndex]->setQuery(individualQuery);
                            }
                        }
                    });
                }
            }
        }
    }

    // Load basic plugin state
    query = xml.getStringAttribute("query", "");

    // Load window dimensions
    savedWindowWidth = xml.getIntAttribute("windowWidth", 1000);
    savedWindowHeight = xml.getIntAttribute("windowHeight", 700);

    // Load panel states
    presetPanelExpandedState = xml.getBoolAttribute("presetPanelExpanded", false);
    bookmarkPanelExpandedState = xml.getBoolAttribute("bookmarkPanelExpanded", false);  // ADD THIS

    // NEW: Load active preset state
    String activePresetPath = xml.getStringAttribute("activePresetFile", "");
    int activeSlot = xml.getIntAttribute("activeSlotIndex", -1);

    /*if (activePresetPath.isNotEmpty() && activeSlot >= 0)
    {
        File activePresetFile(activePresetPath);
        if (activePresetFile.existsAsFile())
        {
            collectionManager.setActivePreset(activePresetFile, activeSlot);
        }
    }*/

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
                    sound.tags = soundXml->getStringAttribute("tags");
                    sound.description = soundXml->getStringAttribute("description");

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

}

void FreesoundAdvancedSamplerAudioProcessor::newSoundsReady(Array<FSSound> sounds, String textQuery, std::vector<juce::StringArray> soundInfo)
{
    // Add all sounds to collection
    for (int i = 0; i < sounds.size(); ++i)
    {
        const FSSound& sound = sounds[i];
        String searchQuery = textQuery;

        // Override with individual query if available
        if (i < soundInfo.size() && soundInfo[i].size() > 5)
        {
            searchQuery = soundInfo[i][5]; // Query stored at index 5
        }

        collectionManager->addOrUpdateSample(sound, searchQuery);
    }

    // Handle master search or individual pad population
    if (auto* editor = dynamic_cast<FreesoundAdvancedSamplerAudioProcessorEditor*>(getActiveEditor()))
    {
        editor->getSampleGridComponent().handleMasterSearch(sounds, soundInfo, textQuery);
    }

    startDownloads(sounds);
}

void FreesoundAdvancedSamplerAudioProcessor::startDownloads(const Array<FSSound>& sounds)
{
    // Create main samples folder (no more subfolders)
    File samplesFolder = collectionManager->getSamplesFolder();
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

    // Add tracking voices
    for (int i = 0; i < poliphony; i++) {
        sampler.addVoice(new TrackingSamplerVoice(*this));
    }

    if (audioFormatManager.getNumKnownFormats() == 0) {
        audioFormatManager.registerBasicFormats();
    }

    // FIXED: Load samples by pad index - each pad gets its own sampler sound, even if duplicate
    for (int padIndex = 0; padIndex < 16; ++padIndex)
    {
        String freesoundId = getPadFreesoundId(padIndex);
        if (freesoundId.isNotEmpty() && collectionManager)
        {
            File audioFile = collectionManager->getSampleFile(freesoundId);

            if (audioFile.existsAsFile())
            {
                std::unique_ptr<AudioFormatReader> reader(audioFormatManager.createReaderFor(audioFile));

                if (reader != nullptr)
                {
                    BigInteger notes;
                    int midiNote = 36 + padIndex;
                    notes.setBit(midiNote, true);

                    double attackTime = 0.0;
                    double releaseTime = 0.1;
                    double maxSampleLength = 10.0;

                    // CRITICAL: Use padIndex as name, not freesoundId
                    // This ensures each pad gets its own sampler sound instance
                    sampler.addSound(new SamplerSound("pad_" + String(padIndex), *reader, notes, midiNote,
                                                     attackTime, releaseTime, maxSampleLength));
                }
            }
        }
    }
}

void FreesoundAdvancedSamplerAudioProcessor::addNoteOnToMidiBuffer(int notenumber)
{
	MidiMessage message = MidiMessage::noteOn(10, notenumber, (uint8)100);
	double timestamp = Time::getMillisecondCounterHiRes() * 0.001 - getStartTime();
	message.setTimeStamp(timestamp);

	auto sampleNumber = (int)(timestamp * getSampleRate());
	midiFromEditor.addEvent(message,sampleNumber);
}

void FreesoundAdvancedSamplerAudioProcessor::addNoteOffToMidiBuffer(int noteNumber)
{
    MidiMessage message = MidiMessage::noteOff(10, noteNumber, (uint8)0);
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

String FreesoundAdvancedSamplerAudioProcessor::saveCurrentAsPreset(const String& name, const String& description, int slotIndex)
{
    if (!collectionManager)
        return "";

    // Create new preset
    String presetId = collectionManager->createPreset(name, description);
    if (presetId.isEmpty())
        return "";

    // FIXED: Save current pad-specific state to the new preset
    Array<SampleCollectionManager::PadSlotData> padData;

    for (int padIndex = 0; padIndex < 16; ++padIndex)
    {
        String freesoundId = getPadFreesoundId(padIndex);
        if (freesoundId.isNotEmpty())
        {
            // Get individual query from the pad UI
            String individualQuery = "";
            if (auto* editor = dynamic_cast<FreesoundAdvancedSamplerAudioProcessorEditor*>(getActiveEditor()))
            {
                auto& gridComponent = editor->getSampleGridComponent();
                if (padIndex < gridComponent.samplePads.size() && gridComponent.samplePads[padIndex])
                {
                    individualQuery = gridComponent.samplePads[padIndex]->getQuery();
                }
            }

            SampleCollectionManager::PadSlotData data(padIndex, freesoundId, individualQuery);
            padData.add(data);
        }
    }

    // Save to the specified slot
    if (collectionManager->setPresetSlot(presetId, slotIndex, padData))
    {
        setActivePreset(presetId, slotIndex);
        return presetId;
    }

    // If saving failed, clean up the preset
    collectionManager->deletePreset(presetId);
    return "";
}

bool FreesoundAdvancedSamplerAudioProcessor::loadPreset(const String& presetId, int slotIndex)
{
    if (!collectionManager)
        return false;

    // FIXED: Load pad-specific data (handles duplicates correctly)
    Array<SampleCollectionManager::PadSlotData> padData = collectionManager->getPresetSlot(presetId, slotIndex);

    // Clear current state
    clearAllPads();

    // FIXED: Load each pad individually (allows duplicates)
    for (const auto& data : padData)
    {
        if (data.padIndex >= 0 && data.padIndex < 16 && data.freesoundId.isNotEmpty())
        {
            setPadSample(data.padIndex, data.freesoundId);
        }
    }

    // Update sampler with new pad assignments
    setSources();

    // Update UI and restore individual queries
    if (auto* editor = dynamic_cast<FreesoundAdvancedSamplerAudioProcessorEditor*>(getActiveEditor()))
    {
        editor->getSampleGridComponent().refreshFromProcessor();

        // Restore individual queries after UI refresh
        Timer::callAfterDelay(100, [editor, padData]() {
            auto& gridComponent = editor->getSampleGridComponent();
            for (const auto& data : padData)
            {
                if (data.padIndex >= 0 && data.padIndex < gridComponent.samplePads.size() &&
                    data.individualQuery.isNotEmpty())
                {
                    gridComponent.samplePads[data.padIndex]->setQuery(data.individualQuery);
                }
            }
        });
    }

    setActivePreset(presetId, slotIndex);
    return true;
}

bool FreesoundAdvancedSamplerAudioProcessor::saveToSlot(const String& presetId, int slotIndex, const String& description)
{
    if (!collectionManager)
        return false;

    // FIXED: Collect current pad-specific data (allows duplicates)
    Array<SampleCollectionManager::PadSlotData> padData;

    for (int padIndex = 0; padIndex < 16; ++padIndex)
    {
        String freesoundId = getPadFreesoundId(padIndex);
        if (freesoundId.isNotEmpty())
        {
            // Get individual query from the pad UI
            String individualQuery = "";
            if (auto* editor = dynamic_cast<FreesoundAdvancedSamplerAudioProcessorEditor*>(getActiveEditor()))
            {
                auto& gridComponent = editor->getSampleGridComponent();
                if (padIndex < gridComponent.samplePads.size() && gridComponent.samplePads[padIndex])
                {
                    individualQuery = gridComponent.samplePads[padIndex]->getQuery();
                }
            }

            SampleCollectionManager::PadSlotData data(padIndex, freesoundId, individualQuery);
            padData.add(data);
        }
    }

    return collectionManager->setPresetSlot(presetId, slotIndex, padData);
}

void FreesoundAdvancedSamplerAudioProcessor::setPadSample(int padIndex, const String& freesoundId)
{
    if (padIndex >= 0 && padIndex < 16)
    {
        currentPadFreesoundIds.set(padIndex, freesoundId);
    }
}

String FreesoundAdvancedSamplerAudioProcessor::getPadFreesoundId(int padIndex) const
{
    if (padIndex >= 0 && padIndex < currentPadFreesoundIds.size())
    {
        return currentPadFreesoundIds[padIndex];
    }
    return "";
}

void FreesoundAdvancedSamplerAudioProcessor::clearPad(int padIndex)
{
    if (padIndex >= 0 && padIndex < 16)
    {
        currentPadFreesoundIds.set(padIndex, "");
    }
}

void FreesoundAdvancedSamplerAudioProcessor::clearAllPads()
{
    for (int i = 0; i < 16; ++i)
    {
        currentPadFreesoundIds.set(i, "");
    }
}

void FreesoundAdvancedSamplerAudioProcessor::incrementSamplePlayCount(int padIndex)
{
    String freesoundId = getPadFreesoundId(padIndex);
    if (freesoundId.isNotEmpty() && collectionManager)
    {
        collectionManager->incrementPlayCount(freesoundId);
    }
}

void FreesoundAdvancedSamplerAudioProcessor::setActivePreset(const String& presetId, int slotIndex)
{
    activePresetId = presetId;
    activeSlotIndex = slotIndex;
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
    {
        DBG("  - File doesn't exist, aborting");
        return;
    }

    // CRITICAL: Stop any currently playing preview first
    stopPreviewSample();

    // Clear existing preview sounds but keep the tracking voice
    previewSampler.clearSounds();

    // Store which sample we're about to load
    currentPreviewFreesoundId = freesoundId;

    // Load the preview sample
    std::unique_ptr<AudioFormatReader> reader(previewAudioFormatManager.createReaderFor(audioFile));
    if (reader != nullptr)
    {

        BigInteger notes;
        int previewNote = 127; // Use highest MIDI note for preview
        notes.setBit(previewNote, true);

        // Create the sampler sound with the freesound ID in the name
        // This is crucial for the tracking voice to identify it
        String soundName = "preview_" + freesoundId;

        double maxLength = 10.0;
        double attackTime = 0.0;
        double releaseTime = 0.1;

        auto* samplerSound = new SamplerSound(soundName, *reader, notes, previewNote,
                                             attackTime, releaseTime, maxLength);

        previewSampler.addSound(samplerSound);

    }
    else
    {
        currentPreviewFreesoundId = ""; // Clear on failure
    }
}

void FreesoundAdvancedSamplerAudioProcessor::playPreviewSample()
{

    if (currentPreviewFreesoundId.isEmpty())
    {
        DBG("  - No preview sample loaded, aborting");
        return;
    }

    if (previewSampler.getNumSounds() == 0)
    {
        DBG("  - No sounds in preview sampler, aborting");
        return;
    }

    // Trigger the preview sample
    int previewNote = 127;
    MidiMessage message = MidiMessage::noteOn(2, previewNote, (uint8)100); // Channel 2 for preview
    double timestamp = Time::getMillisecondCounterHiRes() * 0.001 - getStartTime();
    message.setTimeStamp(timestamp);

    auto sampleNumber = (int)(timestamp * getSampleRate());
    midiFromEditor.addEvent(message, sampleNumber);

}

void FreesoundAdvancedSamplerAudioProcessor::stopPreviewSample()
{
    // Send note off for preview - do this even if currentPreviewFreesoundId is empty
    // to ensure any stuck notes are released
    int previewNote = 127;
    MidiMessage message = MidiMessage::noteOff(2, previewNote, (uint8)0);
    double timestamp = Time::getMillisecondCounterHiRes() * 0.001 - getStartTime();
    message.setTimeStamp(timestamp);

    auto sampleNumber = (int)(timestamp * getSampleRate());
    midiFromEditor.addEvent(message, sampleNumber);

    // Don't clear currentPreviewFreesoundId here - let the voice handle it
}

void FreesoundAdvancedSamplerAudioProcessor::notifyPreviewStarted(const String& freesoundId)
{
    currentPreviewFreesoundId = freesoundId;

    previewPlaybackListeners.call([freesoundId](PreviewPlaybackListener& l) {
        l.previewStarted(freesoundId);
    });
}

void FreesoundAdvancedSamplerAudioProcessor::notifyPreviewStopped(const String& freesoundId)
{
    previewPlaybackListeners.call([freesoundId](PreviewPlaybackListener& l) {
        l.previewStopped(freesoundId);
    });

    // Clear the current preview ID after notifying listeners
    if (currentPreviewFreesoundId == freesoundId)
    {
        currentPreviewFreesoundId = "";
    }
}

void FreesoundAdvancedSamplerAudioProcessor::notifyPreviewPlayheadPositionChanged(const String& freesoundId, float position)
{
    // Don't log this one as it's called frequently
    previewPlaybackListeners.call([freesoundId, position](PreviewPlaybackListener& l) {
        l.previewPlayheadPositionChanged(freesoundId, position);
    });
}

// Add these listener management methods:
void FreesoundAdvancedSamplerAudioProcessor::addPreviewPlaybackListener(PreviewPlaybackListener* listener)
{
    previewPlaybackListeners.add(listener);
}

void FreesoundAdvancedSamplerAudioProcessor::removePreviewPlaybackListener(PreviewPlaybackListener* listener)
{
    previewPlaybackListeners.remove(listener);
}

void FreesoundAdvancedSamplerAudioProcessor::migrateFromOldSystem()
{
    // This method can help migrate existing presets/bookmarks to the new system
    // Implementation depends on your current file structure

    File oldBookmarksFile = File::getSpecialLocation(File::userDocumentsDirectory)
                               .getChildFile("FreesoundAdvancedSampler")
                               .getChildFile("bookmarks.json");

    if (oldBookmarksFile.existsAsFile())
    {
        // Load old bookmarks and migrate them
        String jsonText = oldBookmarksFile.loadFileAsString();
        var parsedJson = JSON::parse(jsonText);

        if (parsedJson.isObject())
        {
            var bookmarksArray = parsedJson.getProperty("bookmarks", var());
            if (bookmarksArray.isArray())
            {
                Array<var>* bookmarks = bookmarksArray.getArray();
                for (const auto& bookmarkVar : *bookmarks)
                {
                    if (bookmarkVar.isObject())
                    {
                        String freesoundId = bookmarkVar.getProperty("freesound_id", "");
                        if (freesoundId.isNotEmpty())
                        {
                            // Create sample metadata from old bookmark
                            SampleMetadata sample;
                            sample.freesoundId = freesoundId;
                            sample.originalName = bookmarkVar.getProperty("sample_name", "");
                            sample.authorName = bookmarkVar.getProperty("author_name", "");
                            sample.licenseType = bookmarkVar.getProperty("license_type", "");
                            sample.searchQuery = bookmarkVar.getProperty("search_query", "");
                            sample.tags = bookmarkVar.getProperty("tags", "");
                            sample.description = bookmarkVar.getProperty("description", "");
                            sample.fileName = "FS_ID_" + freesoundId + ".ogg";
                            sample.freesoundUrl = bookmarkVar.getProperty("freesound_url", "");
                            sample.duration = bookmarkVar.getProperty("duration", 0.0);
                            sample.fileSize = bookmarkVar.getProperty("file_size", 0);
                            sample.downloadedAt = bookmarkVar.getProperty("bookmarked_at", "");
                            sample.isBookmarked = true;
                            sample.bookmarkedAt = sample.downloadedAt;

                            collectionManager->addOrUpdateSample(sample);
                        }
                    }
                }
            }
        }

        // Rename old file so we don't migrate again
        oldBookmarksFile.moveFileTo(oldBookmarksFile.getSiblingFile("bookmarks_migrated.json"));
    }

    // Similar migration can be done for old presets...
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