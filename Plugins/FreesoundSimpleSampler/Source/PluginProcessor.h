/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "FreesoundAPI/FreesoundAPI.h"
#include "AudioDownloadManager.h" // Add this include

using namespace juce;
//==============================================================================
/**
*/

class FreesoundSimpleSamplerAudioProcessor  : public AudioProcessor,
                                            public AudioDownloadManager::Listener // Add this inheritance
{
public:
    //==============================================================================
    FreesoundSimpleSamplerAudioProcessor();
    ~FreesoundSimpleSamplerAudioProcessor();

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
	void newSoundsReady(Array<FSSound> sounds, String textQuery, std::vector<juce::StringArray> soundInfo);

	void setSources();
	void addToMidiBuffer(int notenumber);

	double getStartTime();
	bool isArrayNotEmpty();
	String getQuery();
	std::vector<juce::StringArray> getData();

    // Add download-related methods
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

private:
    // Remove old download-related members
	// std::vector<URL::DownloadTask*> downloadTasksToDelete;
	// std::vector<std::unique_ptr<juce::URL::DownloadTask>> downloadTasks;

    // Add new download manager
    AudioDownloadManager downloadManager;
    ListenerList<DownloadListener> downloadListeners;

	Synthesiser sampler;
	AudioFormatManager audioFormatManager;
	MidiBuffer midiFromEditor;
	long midicounter;
	double startTime;
	String query;
	std::vector<juce::StringArray> soundsArray;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreesoundSimpleSamplerAudioProcessor)
};