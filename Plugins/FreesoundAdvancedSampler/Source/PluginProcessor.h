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
#include "PresetManager.h"

using namespace juce;

//==============================================================================
// Enhanced SamplerSound class with pitch and volume control
//==============================================================================
class PitchableVolumeSound : public SamplerSound
{
public:
    PitchableVolumeSound(const String& name,
                        AudioFormatReader& source,
                        const BigInteger& notes,
                        int midiNoteForNormalPitch,
                        double attackTimeSecs,
                        double releaseTimeSecs,
                        double maxSampleLengthSeconds)
        : SamplerSound(name, source, notes, midiNoteForNormalPitch,
                      attackTimeSecs, releaseTimeSecs, maxSampleLengthSeconds)
        , pitchCents(0.0f)
        , volume(0.7f)
    {
    }

    void setPitchCents(float cents) { pitchCents = cents; }
    void setVolume(float vol) { volume = jlimit(0.0f, 1.0f, vol); }

    float getPitchCents() const { return pitchCents; }
    float getVolume() const { return volume; }

private:
    float pitchCents;
    float volume;
};

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
    File currentSessionDownloadLocation; // Current session's download folder
	void newSoundsReady(Array<FSSound> sounds, String textQuery, std::vector<juce::StringArray> soundInfo);

	void setSources();
	void addToMidiBuffer(int notenumber);

	double getStartTime();
	bool isArrayNotEmpty();
	String getQuery();
	std::vector<juce::StringArray> getData();
    Array<FSSound> getCurrentSounds(); // Get current sounds array
    File getCurrentDownloadLocation(); // Get current session's download location
	Array<FSSound>& getCurrentSoundsArrayReference() { return currentSoundsArray; }
	std::vector<juce::StringArray>& getDataReference() { return soundsArray; }

    // Download-related methods
    void startDownloads(const Array<FSSound>& sounds);
    void cancelDownloads();
    AudioDownloadManager& getDownloadManager() { return downloadManager; }

    // README generation
    void generateReadmeFile(const Array<FSSound>& sounds, const std::vector<StringArray>& soundInfo, const String& searchQuery);
	void updateReadmeFile();

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

    // Playback listener for visual feedback
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

	static String cleanFilename(const String& input)
	{
		String cleaned = input;

		// Remove invalid filename characters
		cleaned = cleaned.replace("\\", "_");
		cleaned = cleaned.replace("/", "_");
		cleaned = cleaned.replace(":", "_");
		cleaned = cleaned.replace("*", "_");
		cleaned = cleaned.replace("?", "_");
		cleaned = cleaned.replace("\"", "_");
		cleaned = cleaned.replace("<", "_");
		cleaned = cleaned.replace(">", "_");
		cleaned = cleaned.replace("|", "_");
		cleaned = cleaned.replace(" ", "_");

		return cleaned;
	}

	PresetManager& getPresetManager() { return presetManager; }
	bool saveCurrentAsPreset(const String& name, const String& description = "", int slotIndex = 0);
	bool loadPreset(const File& presetFile, int slotIndex = 0);
	bool saveToSlot(const File& presetFile, int slotIndex, const String& description = "");
	Array<PadInfo> getCurrentPadInfos() const;

    // NEW: Pitch and volume parameter methods
    void updatePadParameters(int padIndex, float pitchCents, float volume);

    struct PadParameters {
        float pitchCents = 0.0f;
        float volume = 0.7f;
    };
    PadParameters getPadParameters(int padIndex) const;

private:
    // Enhanced sampler voice class for playback tracking with pitch/volume support
    class TrackingSamplerVoice : public SamplerVoice
    {
    public:
        TrackingSamplerVoice(FreesoundAdvancedSamplerAudioProcessor& owner);

        void startNote(int midiNoteNumber, float velocity, SynthesiserSound*, int currentPitchWheelPosition) override;
        void stopNote(float velocity, bool allowTailOff) override;
        void renderNextBlock(AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;
    	int getCurrentlyPlayingNote() const { return currentNoteNumber; }

    private:
        FreesoundAdvancedSamplerAudioProcessor& processor;
        int currentNoteNumber = -1;
        double samplePosition = 0.0;
        double sampleLength = 0.0;
        float pitchRatio = 1.0f;
        float volumeMultiplier = 1.0f;
    };

    AudioDownloadManager downloadManager;
    ListenerList<DownloadListener> downloadListeners;
    ListenerList<PlaybackListener> playbackListeners;

	Synthesiser sampler;
	AudioFormatManager audioFormatManager;
	MidiBuffer midiFromEditor;
	long midicounter;
	double startTime;
	String query;
	std::vector<juce::StringArray> soundsArray;
    Array<FSSound> currentSoundsArray; // Store current sounds

    // Methods for playback tracking
    void notifyNoteStarted(int noteNumber, float velocity);
    void notifyNoteStopped(int noteNumber);
    void notifyPlayheadPositionChanged(int noteNumber, float position);

	PresetManager presetManager;

    // NEW: Store pad parameters (pitch and volume for each of 16 pads)
    std::array<PadParameters, 16> padParameters;

    friend class TrackingSamplerVoice;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreesoundAdvancedSamplerAudioProcessor)
};