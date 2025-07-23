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
	tmpDownloadLocation = File::getSpecialLocation(File::userDocumentsDirectory).getChildFile("FreesoundSimpleSampler");
	tmpDownloadLocation.deleteRecursively();
	tmpDownloadLocation.createDirectory();
	midicounter = 1;
	startTime = Time::getMillisecondCounterHiRes() * 0.001;

	// Add download manager listener
	downloadManager.addListener(this);
}

FreesoundAdvancedSamplerAudioProcessor::~FreesoundAdvancedSamplerAudioProcessor()
{
	// Remove download manager listener
	downloadManager.removeListener(this);

	// Deletes the tmp directory so downloaded files do not stay there
	tmpDownloadLocation.deleteRecursively();
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
	tmpDownloadLocation.deleteRecursively();
	tmpDownloadLocation.createDirectory();
	downloadManager.startDownloads(sounds, tmpDownloadLocation);
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

	Array<File> files = tmpDownloadLocation.findChildFiles(2, false);
	for (int i = 0; i < files.size(); i++) {
		std::unique_ptr<AudioFormatReader> reader(audioFormatManager.createReaderFor(files[i]));

		if (reader != nullptr)
		{
			BigInteger notes;
			notes.setRange(i * 8, i * 8 + 7, true);
			sampler.addSound(new SamplerSound(String(i), *reader, notes, i*8, 0, maxLength, maxLength));
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

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FreesoundAdvancedSamplerAudioProcessor();
}