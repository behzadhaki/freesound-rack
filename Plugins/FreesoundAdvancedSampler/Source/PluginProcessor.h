/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "FreesoundAPI/FreesoundAPI.h"
#include "AudioDownloadManager.h"

using namespace juce;

//==============================================================================
/**
*/
class FreesoundAdvancedSamplerAudioProcessor  : public AudioProcessor,
                                            public AudioDownloadManager::Listener
{
public:
    //==============================================================================
    FreesoundAdvancedSamplerAudioProcessor();
    ~FreesoundAdvancedSamplerAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    File tmpDownloadLocation;
    File currentSessionDownloadLocation; // NEW: Current session's download folder
	void newSoundsReady(Array<FSSound> sounds, String textQuery, std::vector<juce::StringArray> soundInfo);

	void setSources();
	void addToMidiBuffer(int notenumber);

	double getStartTime();
	bool isArrayNotEmpty();
	String getQuery();
	std::vector<juce::StringArray> getData();
    Array<FSSound> getCurrentSounds(); // NEW: Get current sounds array
    File getCurrentDownloadLocation(); // NEW: Get current session's download location

    // Download-related methods
    void startDownloads(const Array<FSSound>& sounds);
    void cancelDownloads();
    AudioDownloadManager& getDownloadManager() { return downloadManager; }

    // AudioDownloadManager::Listener implementation
    void downloadProgressChanged(const AudioDownloadManager::DownloadProgress& progress) override;
    void downloadCompleted(bool success) override;

    // For editor to listen to download events
    class DownloadListener
    {
    public:
        virtual ~DownloadListener() = default;
        virtual void downloadProgressChanged(const AudioDownloadManager::DownloadProgress& progress) = 0;
        virtual void downloadCompleted(bool success) = 0;
    };

    void addDownloadListener(DownloadListener* listener);
    void removeDownloadListener(DownloadListener* listener);

    // NEW: Playback listener for visual feedback
    class PlaybackListener
    {
    public:
        virtual ~PlaybackListener() = default;
        virtual void noteStarted(int noteNumber, float velocity) = 0;
        virtual void noteStopped(int noteNumber) = 0;
        virtual void playheadPositionChanged(int noteNumber, float position) = 0;
    };

    void addPlaybackListener(PlaybackListener* listener);
    void removePlaybackListener(PlaybackListener* listener);

private:
    // Enhanced sampler voice class for playback tracking
    class TrackingSamplerVoice : public SamplerVoice
    {
    public:
        TrackingSamplerVoice(FreesoundAdvancedSamplerAudioProcessor& owner);

        void startNote(int midiNoteNumber, float velocity, SynthesiserSound*, int currentPitchWheelPosition) override;
        void stopNote(float velocity, bool allowTailOff) override;
        void renderNextBlock(AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    private:
        FreesoundAdvancedSamplerAudioProcessor& processor;
        int currentNoteNumber = -1;
        double samplePosition = 0.0;
        double sampleLength = 0.0;
    };

    AudioDownloadManager downloadManager;
    ListenerList<DownloadListener> downloadListeners;
    ListenerList<PlaybackListener> playbackListeners; // NEW

	Synthesiser sampler;
	AudioFormatManager audioFormatManager;
	MidiBuffer midiFromEditor;
	long midicounter;
	double startTime;
	String query;
	std::vector<juce::StringArray> soundsArray;
    Array<FSSound> currentSoundsArray; // NEW: Store current sounds

    // NEW: Methods for playback tracking
    void notifyNoteStarted(int noteNumber, float velocity);
    void notifyNoteStopped(int noteNumber);
    void notifyPlayheadPositionChanged(int noteNumber, float position);

    friend class TrackingSamplerVoice;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreesoundAdvancedSamplerAudioProcessor)
};