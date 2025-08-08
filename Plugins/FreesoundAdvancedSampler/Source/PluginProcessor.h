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
#include "TrackingSamplerVoice.h"

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

   // ==== NEW: Per-pad playback config (backward-compatible defaults) ====
   struct PlaybackConfig
   {
       using Direction = TrackingSamplerVoice::Direction;
       using PlayMode  = TrackingSamplerVoice::PlayMode;

       // If <= 1.0 treated as normalized [0..1]; if > 1.0 treated as absolute samples.
       double startSample     = 0.0;   // default 0
       double endSample       = -1.0;  // default = full length
       double tempStartSample = 0.0;   // one-shot temp start
       bool   tempStartArmed  = false;

       // Signal & envelope
       float  pitchShiftSemitones = 0.0f;
       float  stretchRatio        = 1.0f;
       float  gain                = 1.0f;
       ADSR::Parameters adsr { 0.0f, 0.0f, 1.0f, 0.0f };

       // Transport
       Direction direction = Direction::Forward;
       PlayMode  playMode  = PlayMode::Normal;
   };

   // Read-only access for voices
   const PlaybackConfig& getPadPlaybackConfig (int padIndex) const;

   // Setters (samples)
   void setPadStartEndSamples       (int padIndex, double start, double end);
   void setPadTemporaryStartSamples (int padIndex, double tempStart);

   // Setters (normalized 0..1, interpreted at note-on)
   void setPadStartEndNormalized        (int padIndex, float startNorm, float endNorm);
   void setPadTemporaryStartNormalized  (int padIndex, float startNorm);

   // Other playback parameters
   void setPadPitchShift   (int padIndex, float semitones);
   void setPadStretchRatio (int padIndex, float ratio);
   void setPadGain         (int padIndex, float linearGain);
   void setPadADSR         (int padIndex, const ADSR::Parameters& p);
   void setPadDirection    (int padIndex, TrackingSamplerVoice::Direction d);
   void setPadPlayMode     (int padIndex, TrackingSamplerVoice::PlayMode m);

   // Playback state tracking - made public for voice access
   std::array<PlaybackState, 16> padPlaybackStates;

   // Notification methods for voices to call
   void notifyNoteStarted(int noteNumber, float velocity);
   void notifyNoteStopped(int noteNumber);
   void notifyPlayheadPositionChanged(int noteNumber, float position);
   void notifyPreviewStarted(const String& freesoundId);
   void notifyPreviewStopped(const String& freesoundId);
   void notifyPreviewPlayheadPositionChanged(const String& freesoundId, float position);

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

   //==============================================================================
   // NEW: Smart Sampler Management System
   //==============================================================================

   // State tracking for optimization
   std::array<String, 16> loadedSampleIds;     // What's actually in the sampler
   bool voicesInitialized = false;
   int currentPolyphony = 16;

   // Sound caching for performance
   std::map<int, SamplerSound*> padToSoundMapping; // padIndex -> current sound pointer
   std::map<String, int> soundRefCount;            // Track how many pads use each freesoundId

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

   ListenerList<PreviewPlaybackListener> previewPlaybackListeners;
   String currentPreviewFreesoundId; // Track which sample is currently previewing

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

   // NEW: Per-pad playback configuration backing store
   std::array<PlaybackConfig, 16> padConfigs;

   void savePluginState(XmlElement& xml);
   void loadPluginState(const XmlElement& xml);

   //==============================================================================
   JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreesoundAdvancedSamplerAudioProcessor)
};
