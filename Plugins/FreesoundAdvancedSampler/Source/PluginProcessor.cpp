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
	 : 		 AudioProcessor (BusesProperties()
					 #if ! JucePlugin_IsMidiEffect
					  #if ! JucePlugin_IsSynth
					   .withInput  ("Input",  AudioChannelSet::stereo(), true)
					  #endif
					   .withOutput ("Output", AudioChannelSet::stereo(), true)
					 #endif
					   )
#endif
{
	tmpDownloadLocation = File::getSpecialLocation(File::userDocumentsDirectory).getChildFile("FreesoundAdvancedSampler");
	tmpDownloadLocation.createDirectory();
	currentSessionDownloadLocation = tmpDownloadLocation; // Initialize to main directory
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

//==============================================================================
void FreesoundAdvancedSamplerAudioProcessor::newSoundsReady (Array<FSSound> sounds, String textQuery, std::vector<juce::StringArray> soundInfo)
{
	query = textQuery;
	soundsArray = soundInfo;
    currentSoundsArray = sounds; // NEW: Store the sounds array
	startDownloads(sounds);
}

void FreesoundAdvancedSamplerAudioProcessor::startDownloads(const Array<FSSound>& sounds)
{
    // Create timestamped subfolder without search query: {date}_{time}
    Time currentTime = Time::getCurrentTime();
    String dateStr = currentTime.formatted("%Y%m%d");
    String timeStr = currentTime.formatted("%H%M%S");

    String folderName = dateStr + "_" + timeStr;
    File currentDownloadLocation = tmpDownloadLocation.getChildFile(folderName);
    currentDownloadLocation.createDirectory();

    // Store the current download location for this session
    currentSessionDownloadLocation = currentDownloadLocation;

    // Generate README file with search information
    generateReadmeFile(sounds, soundsArray, query);

    // Pass search query to download manager
    downloadManager.startDownloads(sounds, currentDownloadLocation, query);
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

    // Load JSON metadata to get file mappings
    File metadataFile = currentSessionDownloadLocation.getChildFile("metadata.json");
    Array<File> audioFiles;

    if (metadataFile.existsAsFile())
    {
        // Load files based on JSON metadata order
        FileInputStream inputStream(metadataFile);
        if (inputStream.openedOk())
        {
            String jsonText = inputStream.readEntireStreamAsString();
            var parsedJson = juce::JSON::parse(jsonText);

            if (parsedJson.isObject())
            {
                var samplesArray = parsedJson.getProperty("samples", var());
                if (samplesArray.isArray())
                {
                    for (int i = 0; i < samplesArray.size() && i < 16; ++i)
                    {
                        var sample = samplesArray[i];
                        if (sample.isObject())
                        {
                            String fileName = sample.getProperty("file_name", "");
                            if (fileName.isNotEmpty())
                            {
                                File audioFile = currentSessionDownloadLocation.getChildFile(fileName);
                                if (audioFile.existsAsFile())
                                {
                                    audioFiles.add(audioFile);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        // Fallback: scan directory for .ogg files (legacy support)
        DirectoryIterator iter(currentSessionDownloadLocation, false, "*.ogg");
        while (iter.next())
        {
            audioFiles.add(iter.getFile());
            if (audioFiles.size() >= 16) break; // Limit to 16 samples
        }
    }

    // Load the audio files in order
    for (int i = 0; i < audioFiles.size() && i < 16; ++i)
    {
        File audioFile = audioFiles[i];
        std::unique_ptr<AudioFormatReader> reader(audioFormatManager.createReaderFor(audioFile));

        if (reader != nullptr)
        {
            BigInteger notes;
            // Each pad gets a single MIDI note starting at 36
            int midiNote = 36 + i; // Pad 0 = note 36, Pad 1 = note 37, etc.
            notes.setBit(midiNote, true);
            sampler.addSound(new SamplerSound(String(i), *reader, notes, midiNote, 0, maxLength, maxLength));
        }
    }
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

Array<FSSound> FreesoundAdvancedSamplerAudioProcessor::getCurrentSounds()
{
    return currentSoundsArray;
}

File FreesoundAdvancedSamplerAudioProcessor::getCurrentDownloadLocation()
{
    return currentSessionDownloadLocation;
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

void FreesoundAdvancedSamplerAudioProcessor::notifyPlayheadPositionChanged(int noteNumber, float position)
{
    playbackListeners.call([noteNumber, position](PlaybackListener& l) {
        l.playheadPositionChanged(noteNumber, position);
    });
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

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FreesoundAdvancedSamplerAudioProcessor();
}