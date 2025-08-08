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

   // NEW: Pad State Management (Updated with smart sampler integration)
   void setPadSample(int padIndex, const String& freesoundId);
   String getPadFreesoundId(int padIndex) const;
   void clearPad(int padIndex);
   void clearAllPads();

   // NEW: Playhead Continuation Methods
   void updatePadSampleWithPlayheadContinuation(int padIndex, const String& newFreesoundId);
   bool shouldContinuePlayback(int padIndex, double newSampleLength);
   void continuePlaybackFromPosition(int padIndex, float position, double newSampleLength);

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

   // NEW: Smart Sampler Public Methods
   void setSourcesForSinglePad(int padIndex);
   void setSourcesForPadSwap(int padA, int padB);
   void setSourcesForMultiplePads(const Array<int>& padIndices);
   void rebuildAllSources();
   void clearSoundCache();

private:
   // NEW: Unified Collection Manager (replaces PresetManager and BookmarkManager)
   std::unique_ptr<SampleCollectionManager> collectionManager;

   // NEW: Active Preset State
   String activePresetId;
   int activeSlotIndex = -1;

   // NEW: Current Pad State (16 pads mapped to freesound IDs)
   Array<String> currentPadFreesoundIds;

   // NEW: Playback State Tracking for Continuation
   struct PlaybackState {
       bool isPlaying = false;
       float currentPosition = 0.0f;  // 0.0 to 1.0
       double samplePosition = 0.0;   // Actual sample position
       int noteNumber = -1;
       float velocity = 0.0f;
       double sampleLength = 0.0;
   };

   // Track playback state for each pad
   std::array<PlaybackState, 16> padPlaybackStates;

   // Window and panel state
   int savedWindowWidth = 800;
   int savedWindowHeight = 600;
   bool presetPanelExpandedState = false;
   bool bookmarkPanelExpandedState = false;

   //==============================================================================
   // NEW: Smart Sampler Management System
   //==============================================================================

   // State tracking for optimization
   std::array<String, 16> loadedSampleIds;     // What's actually in the sampler
   bool voicesInitialized = false;
   int currentPolyphony = 16;

   // Sound caching for performance
	std::map<int, SamplerSound*> padToSoundMapping; // padIndex -> current sound pointer
	std::map<String, int> soundRefCount; // Track how many pads use each freesoundId

   // Change detection system
   struct SamplerState {
       std::array<String, 16> padAssignments;
       int polyphony;

       bool operator==(const SamplerState& other) const {
           return padAssignments == other.padAssignments && polyphony == other.polyphony;
       }
       bool operator!=(const SamplerState& other) const { return !(*this == other); }
   };
   SamplerState lastAppliedState;

   // Smart sampler internal methods
   void initializeVoicesIfNeeded();
   void updateChangedSoundsOnly();
   bool hasStateChanged();
   SamplerState getCurrentState();
   void detectAndApplyChanges();

   // Individual sound management
   void updateSinglePadSound(int padIndex);
   void removeSoundForPad(int padIndex);
   void addSoundForPad(int padIndex, const String& freesoundId);
   std::shared_ptr<SamplerSound> getOrCreateCachedSound(const String& freesoundId, int midiNote);
   void updateMultiplePads(const Array<int>& padIndices);
   void swapPadSoundsDirectly(int padA, int padB);

   // Utility methods
   BigInteger createNoteSetForPad(int padIndex);
   SamplerSound* getSamplerSoundForPad(int padIndex);
   void cleanupUnusedCachedSounds();

   #if JUCE_DEBUG
   void validateSamplerState();
   #endif

   //==============================================================================
   // Enhanced sampler voice class for playback tracking
   class TrackingSamplerVoice : public SamplerVoice
   {
   public:
       TrackingSamplerVoice(FreesoundAdvancedSamplerAudioProcessor& owner);

       void startNote(int midiNoteNumber, float velocity, SynthesiserSound*, int currentPitchWheelPosition) override;
       void stopNote(float velocity, bool allowTailOff) override;
       void renderNextBlock(AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

       // NEW: Method to start from a specific position
       void startNoteFromPosition(int midiNoteNumber, float velocity, SynthesiserSound* sound,
                                 int currentPitchWheelPosition, double startPosition);

   private:
       FreesoundAdvancedSamplerAudioProcessor& processor;
       int currentNoteNumber = -1;
       double samplePosition = 0.0;
       double sampleLength = 0.0;
       double startOffset = 0.0; // NEW: Starting offset for continuation
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
   friend class TrackingPreviewSamplerVoice;

   //==============================================================================
   JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreesoundAdvancedSamplerAudioProcessor)
};