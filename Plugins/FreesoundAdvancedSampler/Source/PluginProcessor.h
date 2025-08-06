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
#include "SampleCollectionManager.h"

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
   File currentSessionDownloadLocation; // Current session's download folder
   void newSoundsReady(Array<FSSound> sounds, String textQuery, std::vector<juce::StringArray> soundInfo);

   // Add these methods to public section
   void loadPreviewSample(const File& audioFile, const String& freesoundId);
   void playPreviewSample();
   void stopPreviewSample();

   // main sampler methods for sample pads in 4x4 grid
   void setSources();
   void addNoteOnToMidiBuffer(int notenumber);
   void addNoteOffToMidiBuffer(int noteNumber);

   double getStartTime();
   bool isArrayNotEmpty();
   String getQuery();
   std::vector<juce::StringArray> getData();
   Array<FSSound> getCurrentSounds();
   File getCurrentDownloadLocation();
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

   // Playback listener for visual feedback on 4x4 grid
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

   // Preview playback listener for preview sample visual feedback
   class PreviewPlaybackListener
   {
   public:
   	virtual ~PreviewPlaybackListener() = default;
   	virtual void previewStarted(const String& freesoundId) = 0;
   	virtual void previewStopped(const String& freesoundId) = 0;
   	virtual void previewPlayheadPositionChanged(const String& freesoundId, float position) = 0;
   };

   void addPreviewPlaybackListener(PreviewPlaybackListener* listener);
   void removePreviewPlaybackListener(PreviewPlaybackListener* listener);

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

   // NEW: Collection Manager Integration
   SampleCollectionManager* getCollectionManager() { return collectionManager.get(); }

   // NEW: Updated Preset Operations (using collection manager)
   String saveCurrentAsPreset(const String& name, const String& description = "", int slotIndex = 0);
   bool loadPreset(const String& presetId, int slotIndex = 0);
   bool saveToSlot(const String& presetId, int slotIndex, const String& description = "");

   // NEW: Active Preset Tracking
   String getActivePresetId() const { return activePresetId; }
   int getActiveSlotIndex() const { return activeSlotIndex; }
   void setActivePreset(const String& presetId, int slotIndex);

   // NEW: Pad State Management
   void setPadSample(int padIndex, const String& freesoundId);
   String getPadFreesoundId(int padIndex) const;
   void clearPad(int padIndex);
   void clearAllPads();

   // Window size methods
   void setWindowSize(int width, int height)
   {
   	savedWindowWidth = width;
   	savedWindowHeight = height;
   }

   int getSavedWindowWidth() const { return savedWindowWidth; }
   int getSavedWindowHeight() const { return savedWindowHeight; }

   // Panel state methods
   bool getPresetPanelExpandedState() const { return presetPanelExpandedState; }
   void setPresetPanelExpandedState(bool state) { presetPanelExpandedState = state; }

   bool getBookmarkPanelExpandedState() const { return bookmarkPanelExpandedState; }
   void setBookmarkPanelExpandedState(bool state) { bookmarkPanelExpandedState = state; }

   // NEW: Migration Helper
   void migrateFromOldSystem();

private:
   // NEW: Unified Collection Manager (replaces PresetManager and BookmarkManager)
   std::unique_ptr<SampleCollectionManager> collectionManager;

   // NEW: Active Preset State
   String activePresetId;
   int activeSlotIndex = -1;

   // NEW: Current Pad State (16 pads mapped to freesound IDs)
   Array<String> currentPadFreesoundIds;

   // Window and panel state
   int savedWindowWidth = 800;
   int savedWindowHeight = 600;
   bool presetPanelExpandedState = false;
   bool bookmarkPanelExpandedState = false;

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

   // Enhanced preview sampler voice class for playback tracking of preview samples
   class TrackingPreviewSamplerVoice : public SamplerVoice
   {
   public:
   	TrackingPreviewSamplerVoice(FreesoundAdvancedSamplerAudioProcessor& owner);

   	void startNote(int midiNoteNumber, float velocity, SynthesiserSound*, int currentPitchWheelPosition) override;
   	void stopNote(float velocity, bool allowTailOff) override;
   	void renderNextBlock(AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

   private:
   	FreesoundAdvancedSamplerAudioProcessor& processor;
   	String currentFreesoundId;
   	double samplePosition = 0.0;
   	double sampleLength = 0.0;
   };

   ListenerList<PreviewPlaybackListener> previewPlaybackListeners;
   String currentPreviewFreesoundId; // Track which sample is currently previewing

   // Preview notification methods
   void notifyPreviewStarted(const String& freesoundId);
   void notifyPreviewStopped(const String& freesoundId);
   void notifyPreviewPlayheadPositionChanged(const String& freesoundId, float position);

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
   Array<FSSound> currentSoundsArray; // Keep for compatibility during transition

   // Add dedicated preview sampler (runs in parallel)
   Synthesiser previewSampler;
   AudioFormatManager previewAudioFormatManager;

   // Methods for playback tracking
   void notifyNoteStarted(int noteNumber, float velocity);
   void notifyNoteStopped(int noteNumber);
   void notifyPlayheadPositionChanged(int noteNumber, float position);

   void savePluginState(XmlElement& xml);
   void loadPluginState(const XmlElement& xml);

   friend class TrackingSamplerVoice;

   //==============================================================================
   JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreesoundAdvancedSamplerAudioProcessor)
};