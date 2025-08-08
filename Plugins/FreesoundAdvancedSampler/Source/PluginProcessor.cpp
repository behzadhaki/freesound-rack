/*
 ==============================================================================

   This file was auto-generated!

   It contains the basic framework code for a JUCE plugin processor.

 ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "TrackingSamplerVoice.h"

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

    // Add tracking voice for preview sampler
    previewSampler.addVoice(new TrackingPreviewSamplerVoice(*this));

    // Add download manager listener
    downloadManager.addListener(this);
}

FreesoundAdvancedSamplerAudioProcessor::~FreesoundAdvancedSamplerAudioProcessor()
{
   // Remove download manager listener
   downloadManager.removeListener(this);

   // NEW: Clear mappings and let JUCE handle SamplerSound cleanup
   padToSoundMapping.clear();
   soundRefCount.clear();

   // sampler.clearSounds() will be called automatically by Synthesiser destructor
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

   // Split MIDI messages by channel and light up empty pads
   for (const auto metadata : midiMessages)
   {
       const auto message = metadata.getMessage();

       if (message.getChannel() == 2) // Preview channel
       {
           previewMidiBuffer.addEvent(message, metadata.samplePosition);
       }
       else // Main sampler channel(s)
       {
           mainMidiBuffer.addEvent(message, metadata.samplePosition);

           // NEW: Light up empty pads on incoming MIDI
           if (message.isNoteOn() || message.isNoteOff() || (message.isNoteOn() && message.getVelocity() == 0))
           {
               const int noteNumber = message.getNoteNumber();
               const int padIndex   = noteNumber - 36; // pads are mapped 36 + padIndex
               if (padIndex >= 0 && padIndex < 16 && getPadFreesoundId(padIndex).isEmpty())
               {
                   if (message.isNoteOn() && message.getVelocity() > 0.0f)
                       notifyNoteStarted(noteNumber, message.getFloatVelocity());
                   else
                       notifyNoteStopped(noteNumber);
               }
           }
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
           buffer.addFrom(channel, 0, previewBuffer, channel, 0, buffer.getNumSamples(), 0.6f);
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

    // Save current pads + per-pad config
    auto* padsXml = xml.createNewChildElement("CurrentPads");
    for (int i = 0; i < 16; ++i)
    {
        String freesoundId = getPadFreesoundId(i);
        if (freesoundId.isEmpty())
            continue;

        auto* padXml = padsXml->createNewChildElement("Pad");
        padXml->setAttribute("index", i);
        padXml->setAttribute("freesoundId", freesoundId);

        // Try to normalize against actual sample length if available
        double lengthSamples = 0.0;
        if (auto it = padToSoundMapping.find(i); it != padToSoundMapping.end() && it->second != nullptr)
        {
            if (auto* buf = it->second->getAudioData())
                lengthSamples = (double)buf->getNumSamples();
        }

        auto toNorm = [lengthSamples](double v) -> double
        {
            if (v <= 1.0) return juce::jlimit(0.0, 1.0, v);
            if (lengthSamples > 0.0) return juce::jlimit(0.0, 1.0, v / lengthSamples);
            return v; // fallback: store as-is (may be >1)
        };

        const auto& cfg = padConfigs[(size_t)i];

        padXml->setAttribute("startN", toNorm(cfg.startSample));
        padXml->setAttribute("endN",   (cfg.endSample < 0.0 ? 1.0 : toNorm(cfg.endSample)));
        padXml->setAttribute("tempStartN", cfg.tempStartArmed ? toNorm(cfg.tempStartSample) : -1.0);

        padXml->setAttribute("pitch",   cfg.pitchShiftSemitones);
        padXml->setAttribute("stretch", cfg.stretchRatio);
        padXml->setAttribute("gain",    cfg.gain);

        padXml->setAttribute("attack",  cfg.adsr.attack);
        padXml->setAttribute("decay",   cfg.adsr.decay);
        padXml->setAttribute("sustain", cfg.adsr.sustain);
        padXml->setAttribute("release", cfg.adsr.release);

        padXml->setAttribute("direction", (int)cfg.direction);
        padXml->setAttribute("playMode",  (int)cfg.playMode);
    }
}

void FreesoundAdvancedSamplerAudioProcessor::loadPluginState(const XmlElement& xml)
{
    clearAllPads();
    loadedSampleIds.fill({});
    padToSoundMapping.clear();
    soundRefCount.clear();

    activePresetId  = xml.getStringAttribute("activePresetId", "");
    activeSlotIndex = xml.getIntAttribute("activeSlotIndex", -1);

    if (auto* padsXml = xml.getChildByName("CurrentPads"))
    {
        for (auto* padXml : padsXml->getChildIterator())
        {
            if (! padXml->hasTagName("Pad"))
                continue;

            const int    padIndex    = padXml->getIntAttribute("index", -1);
            const String freesoundId = padXml->getStringAttribute("freesoundId", "");
            if (padIndex < 0 || padIndex >= 16 || freesoundId.isEmpty())
                continue;

            setPadSample(padIndex, freesoundId);

            const double startN     = padXml->getDoubleAttribute("startN", 0.0);
            const double endN       = padXml->getDoubleAttribute("endN",   1.0);
            const double tempStartN = padXml->getDoubleAttribute("tempStartN", -1.0);

            setPadStartEndNormalized(padIndex, (float)startN, (float)endN);
            if (tempStartN >= 0.0)
                setPadTemporaryStartNormalized(padIndex, (float)tempStartN);

            ADSR::Parameters p;
            p.attack  = (float) padXml->getDoubleAttribute("attack",  0.0);
            p.decay   = (float) padXml->getDoubleAttribute("decay",   0.0);
            p.sustain = (float) padXml->getDoubleAttribute("sustain", 1.0);
            p.release = (float) padXml->getDoubleAttribute("release", 0.0);

            const float pitch   = (float) padXml->getDoubleAttribute("pitch",   0.0);
            const float stretch = (float) padXml->getDoubleAttribute("stretch", 1.0);
            const float gain    = (float) padXml->getDoubleAttribute("gain",    1.0);

            const int dirVal  = padXml->getIntAttribute("direction", (int)TrackingSamplerVoice::Direction::Forward);
            const int modeVal = padXml->getIntAttribute("playMode",  (int)TrackingSamplerVoice::PlayMode::Normal);

            setPadPitchShift(padIndex, pitch);
            setPadStretchRatio(padIndex, stretch);
            setPadGain(padIndex, gain);
            setPadADSR(padIndex, p);
            setPadDirection(padIndex, static_cast<TrackingSamplerVoice::Direction>(dirVal));
            setPadPlayMode(padIndex,  static_cast<TrackingSamplerVoice::PlayMode>(modeVal));
        }
    }

    // Rebuild sources based on restored pad mapping
    setSources();
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
   // NEW: Smart update - only change what's actually different
   initializeVoicesIfNeeded();

   if (hasStateChanged()) {
       detectAndApplyChanges();
   }

   // Optional: Validate in debug builds
#if JUCE_DEBUG
   validateSamplerState();
#endif
}

void FreesoundAdvancedSamplerAudioProcessor::initializeVoicesIfNeeded()
{
    if (!voicesInitialized || sampler.getNumVoices() != currentPolyphony) {
        sampler.clearVoices();

        // Use the new TrackingSamplerVoice instead of the old nested class
        for (int i = 0; i < currentPolyphony; i++) {
            sampler.addVoice(new TrackingSamplerVoice(*this));
        }

        voicesInitialized = true;
    }

    // Ensure audio format manager is ready (only once)
    if (audioFormatManager.getNumKnownFormats() == 0) {
        audioFormatManager.registerBasicFormats();
    }
}


bool FreesoundAdvancedSamplerAudioProcessor::hasStateChanged()
{
   SamplerState currentState = getCurrentState();
   bool changed = (currentState != lastAppliedState);

   if (changed) {
       lastAppliedState = currentState;
   }

   return changed;
}

FreesoundAdvancedSamplerAudioProcessor::SamplerState FreesoundAdvancedSamplerAudioProcessor::getCurrentState()
{
   SamplerState state;

   for (int i = 0; i < 16; ++i) {
       state.padAssignments[i] = getPadFreesoundId(i);
   }
   state.polyphony = currentPolyphony;

   return state;
}

void FreesoundAdvancedSamplerAudioProcessor::detectAndApplyChanges()
{
   // Find what changed and update only those pads
   Array<int> changedPads;

   for (int i = 0; i < 16; ++i) {
       String currentId = getPadFreesoundId(i);
       String loadedId = loadedSampleIds[i];

       if (currentId != loadedId) {
           changedPads.add(i);
       }
   }

   // Optimize for common swap operations
   if (changedPads.size() == 2) {
       int padA = changedPads[0];
       int padB = changedPads[1];

       // Check if this is actually a swap
       if (getPadFreesoundId(padA) == loadedSampleIds[padB] &&
           getPadFreesoundId(padB) == loadedSampleIds[padA]) {

           swapPadSoundsDirectly(padA, padB);
           return; // Early exit for swap optimization
       }
   }

   // For other changes, update individual pads
   updateMultiplePads(changedPads);
}

void FreesoundAdvancedSamplerAudioProcessor::updateMultiplePads(const Array<int>& padIndices)
{
   for (int padIndex : padIndices) {
       updateSinglePadSound(padIndex);
   }
}

void FreesoundAdvancedSamplerAudioProcessor::updateSinglePadSound(int padIndex)
{
   if (padIndex < 0 || padIndex >= 16) return;

   String newFreesoundId = getPadFreesoundId(padIndex);
   String currentId = loadedSampleIds[padIndex];

   if (newFreesoundId == currentId) return; // No change needed

   // Remove old sound for this pad
   removeSoundForPad(padIndex);

   // Add new sound if we have one
   if (newFreesoundId.isNotEmpty()) {
       addSoundForPad(padIndex, newFreesoundId);
   }

   // Update tracking
   loadedSampleIds[padIndex] = newFreesoundId;
}

void FreesoundAdvancedSamplerAudioProcessor::removeSoundForPad(int padIndex)
{
   // Find and remove the sound currently mapped to this pad
   if (padToSoundMapping.count(padIndex)) {
       SamplerSound* soundToRemove = padToSoundMapping[padIndex];

       // Remove from sampler - this will properly handle reference counting
       for (int i = 0; i < sampler.getNumSounds(); ++i) {
           if (sampler.getSound(i).get() == soundToRemove) {
               sampler.removeSound(i);
               break;
           }
       }

       // Update reference count
       String freesoundId = loadedSampleIds[padIndex];
       if (freesoundId.isNotEmpty() && soundRefCount.count(freesoundId)) {
           soundRefCount[freesoundId]--;
           if (soundRefCount[freesoundId] <= 0) {
               soundRefCount.erase(freesoundId);
           }
       }

       padToSoundMapping.erase(padIndex);
   }
}

void FreesoundAdvancedSamplerAudioProcessor::addSoundForPad(int padIndex, const String& freesoundId)
{
   if (!collectionManager) return;

   File audioFile = collectionManager->getSampleFile(freesoundId);
   if (!audioFile.existsAsFile()) return;

   int midiNote = 36 + padIndex;

   // Create the sound directly and let JUCE manage the reference counting
   std::unique_ptr<AudioFormatReader> reader(audioFormatManager.createReaderFor(audioFile));
   if (!reader) return;

   BigInteger notes = createNoteSetForPad(padIndex);

   // Create SamplerSound and let the Synthesiser manage its lifetime
   auto* newSound = new SamplerSound(
       "pad_" + String(padIndex),       // name
       *reader,                         // AudioFormatReader reference
       notes,                          // MIDI notes this sound responds to
       midiNote,                       // root MIDI note
       0.0,                           // attack time
       0.1,                           // release time
       10.0                           // max sample length
   );

   // Add to sampler - the Synthesiser will manage the reference counting
   sampler.addSound(newSound);

   // Store raw pointer for quick lookup (safe because Synthesiser owns it)
   padToSoundMapping[padIndex] = newSound;

   // Track usage
   soundRefCount[freesoundId]++;
}

std::shared_ptr<SamplerSound> FreesoundAdvancedSamplerAudioProcessor::getOrCreateCachedSound(const String& freesoundId, int midiNote)
{
   // Simplified approach: Create each sound fresh but track usage for optimization

   if (!collectionManager) return nullptr;

   File audioFile = collectionManager->getSampleFile(freesoundId);
   if (!audioFile.existsAsFile()) return nullptr;

   std::unique_ptr<AudioFormatReader> reader(audioFormatManager.createReaderFor(audioFile));
   if (!reader) return nullptr;

   BigInteger notes = createNoteSetForPad(midiNote - 36);

   // Create new SamplerSound instance
   auto newSound = std::make_shared<SamplerSound>(
       "pad_" + String(midiNote - 36),  // name
       *reader,                         // AudioFormatReader reference
       notes,                          // MIDI notes this sound responds to
       midiNote,                       // root MIDI note
       0.0,                           // attack time
       0.1,                           // release time
       10.0                           // max sample length
   );

   // Track usage for potential future optimizations
   soundRefCount[freesoundId]++;

   return newSound;
}

void FreesoundAdvancedSamplerAudioProcessor::swapPadSoundsDirectly(int padA, int padB)
{
   // This is the most efficient way to handle swaps

   // Get current sound pointers
   SamplerSound* soundA = getSamplerSoundForPad(padA);
   SamplerSound* soundB = getSamplerSoundForPad(padB);

   // Swap MIDI note assignments if both sounds exist
   if (soundA && soundB) {
       BigInteger notesA = createNoteSetForPad(padA);
       BigInteger notesB = createNoteSetForPad(padB);

       // Update MIDI mappings directly (if SamplerSound supports this)
       // Otherwise, we need to recreate only these two sounds
       removeSoundForPad(padA);
       removeSoundForPad(padB);
       addSoundForPad(padA, getPadFreesoundId(padA));
       addSoundForPad(padB, getPadFreesoundId(padB));
   }
   else if (soundA) {
       // Only A has sound - move it to B
       removeSoundForPad(padA);
       addSoundForPad(padB, getPadFreesoundId(padB));
   }
   else if (soundB) {
       // Only B has sound - move it to A
       removeSoundForPad(padB);
       addSoundForPad(padA, getPadFreesoundId(padA));
   }

   // Update tracking
   std::swap(loadedSampleIds[padA], loadedSampleIds[padB]);
}

BigInteger FreesoundAdvancedSamplerAudioProcessor::createNoteSetForPad(int padIndex)
{
   BigInteger notes;
   int midiNote = 36 + padIndex;
   notes.setBit(midiNote, true);
   return notes;
}

SamplerSound* FreesoundAdvancedSamplerAudioProcessor::getSamplerSoundForPad(int padIndex)
{
   if (padToSoundMapping.count(padIndex)) {
       return padToSoundMapping[padIndex];
   }
   return nullptr;
}

// Public convenience methods for specific operations

void FreesoundAdvancedSamplerAudioProcessor::setSourcesForSinglePad(int padIndex)
{
   initializeVoicesIfNeeded();
   updateSinglePadSound(padIndex);
}

void FreesoundAdvancedSamplerAudioProcessor::setSourcesForPadSwap(int padA, int padB)
{
   initializeVoicesIfNeeded();
   swapPadSoundsDirectly(padA, padB);
}

void FreesoundAdvancedSamplerAudioProcessor::setSourcesForMultiplePads(const Array<int>& padIndices)
{
   initializeVoicesIfNeeded();
   updateMultiplePads(padIndices);
}

void FreesoundAdvancedSamplerAudioProcessor::rebuildAllSources()
{
   // Fallback method - complete rebuild when needed
   // (Keep your original logic as a backup)

   // Clear everything
   sampler.clearSounds();
   padToSoundMapping.clear();
   std::fill(loadedSampleIds.begin(), loadedSampleIds.end(), String());

   // Force voice reinitialization
   voicesInitialized = false;
   initializeVoicesIfNeeded();

   // Load all pads from scratch
   Array<int> allPads;
   for (int i = 0; i < 16; ++i) allPads.add(i);
   updateMultiplePads(allPads);

   // Update state tracking
    lastAppliedState = getCurrentState();
}

void FreesoundAdvancedSamplerAudioProcessor::clearSoundCache()
{
   soundRefCount.clear();
}

#if JUCE_DEBUG
void FreesoundAdvancedSamplerAudioProcessor::validateSamplerState()
{
   // Debug validation to catch issues early

   DBG("=== Sampler State Validation ===");
   DBG("Voices: " + String(sampler.getNumVoices()));
   DBG("Sounds: " + String(sampler.getNumSounds()));
   DBG("Cached sounds: " + String(soundRefCount.size()));
   DBG("Pad mappings: " + String(padToSoundMapping.size()));

   // Check for consistency
   int activePads = 0;
   for (int i = 0; i < 16; ++i) {
       String padId = getPadFreesoundId(i);
       String loadedId = loadedSampleIds[i];
       bool hasMapping = padToSoundMapping.count(i) > 0;

       if (!padId.isEmpty()) {
           activePads++;
           if (padId != loadedId) {
               DBG("WARNING: Pad " + String(i) + " state mismatch - Expected: " + padId + ", Loaded: " + loadedId);
           }
           if (!hasMapping) {
               DBG("WARNING: Pad " + String(i) + " has freesound ID but no sound mapping");
           }
       } else if (hasMapping) {
           DBG("WARNING: Pad " + String(i) + " has sound mapping but no freesound ID");
       }
   }

   DBG("Active pads: " + String(activePads));
   DBG("=== End Validation ===");
}
#endif

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

void FreesoundAdvancedSamplerAudioProcessor::notifyPlayheadPositionChanged(int noteNumber, float position)
{
   playbackListeners.call([noteNumber, position](PlaybackListener& l) {
       l.playheadPositionChanged(noteNumber, position);
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

   Array<SampleCollectionManager::PadSlotData> padData = collectionManager->getPresetSlot(presetId, slotIndex);

   // Clear current state
   clearAllPads(); // This now clears efficiently

   // Collect all pad indices that will be updated
   Array<int> padsToUpdate;

   // Set pad assignments
   for (const auto& data : padData)
   {
       if (data.padIndex >= 0 && data.padIndex < 16 && data.freesoundId.isNotEmpty())
       {
           currentPadFreesoundIds.set(data.padIndex, data.freesoundId);
           padsToUpdate.add(data.padIndex);
       }
   }

   // NEW: Use batch update for all changed pads
   setSourcesForMultiplePads(padsToUpdate);

   // Update UI
   if (auto* editor = dynamic_cast<FreesoundAdvancedSamplerAudioProcessorEditor*>(getActiveEditor()))
   {
       editor->getSampleGridComponent().refreshFromProcessor();

       // Restore individual queries
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

void FreesoundAdvancedSamplerAudioProcessor::cleanupUnusedCachedSounds()
{
   // Remove cached sounds that aren't currently being used by any pad
   auto it = soundRefCount.begin();
   while (it != soundRefCount.end()) {
       const String& freesoundId = it->first;
       bool isUsed = false;

       // Check if any pad is using this sound
       for (int i = 0; i < 16; ++i) {
           if (getPadFreesoundId(i) == freesoundId) {
               isUsed = true;
               break;
           }
       }

       if (!isUsed) {
           it = soundRefCount.erase(it);
       } else {
           ++it;
       }
   }
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

       // NEW: Use smart single-pad update instead of full rebuild
       setSourcesForSinglePad(padIndex);
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

       // NEW: Use smart single-pad update
       setSourcesForSinglePad(padIndex);
   }
}

void FreesoundAdvancedSamplerAudioProcessor::clearAllPads()
{
   for (int i = 0; i < 16; ++i)
   {
       currentPadFreesoundIds.set(i, "");
   }

   // NEW: Clear everything efficiently and safely
   sampler.clearSounds(); // This properly manages reference counting
   padToSoundMapping.clear();
   soundRefCount.clear();
   std::fill(loadedSampleIds.begin(), loadedSampleIds.end(), String());
   lastAppliedState = getCurrentState();
}

void FreesoundAdvancedSamplerAudioProcessor::setActivePreset(const String& presetId, int slotIndex)
{
   activePresetId = presetId;
   activeSlotIndex = slotIndex;
}

void FreesoundAdvancedSamplerAudioProcessor::updatePadSampleWithPlayheadContinuation(int padIndex, const String& newFreesoundId)
{
    if (padIndex < 0 || padIndex >= 16) return;

    // Store current playback state BEFORE updating
    PlaybackState currentState = padPlaybackStates[padIndex];

    if (currentState.isPlaying && !newFreesoundId.isEmpty())
    {
        // Get the new sample to check if continuation is viable
        File newAudioFile = collectionManager->getSampleFile(newFreesoundId);
        if (newAudioFile.existsAsFile())
        {
            std::unique_ptr<AudioFormatReader> reader(audioFormatManager.createReaderFor(newAudioFile));
            if (reader != nullptr)
            {
                double newSampleLength = reader->lengthInSamples;

                if (shouldContinuePlayback(padIndex, newSampleLength))
                {
                    // FORCED APPROACH: Stop current note, update sample, restart immediately

                    // 1. Send note off to stop current voice
                    addNoteOffToMidiBuffer(currentState.noteNumber);

                    // 2. Force a very brief delay to ensure note off processes
                    Timer::callAfterDelay(1, [this, padIndex, newFreesoundId, currentState]()
                    {
                        // 3. Update the pad sample (loads new sound)
                        setPadSample(padIndex, newFreesoundId);

                        // 4. Store the continuation state
                        padPlaybackStates[padIndex].currentPosition = currentState.currentPosition;
                        padPlaybackStates[padIndex].velocity = currentState.velocity;
                        padPlaybackStates[padIndex].currentFreesoundId = newFreesoundId;

                        // 5. Immediately restart with new sample
                        Timer::callAfterDelay(1, [this, currentState]()
                        {
                            addNoteOnToMidiBuffer(currentState.noteNumber);
                        });
                    });

                    return; // Don't do normal update
                }
                else
                {
                    // Stop playback - position is out of range
                    addNoteOffToMidiBuffer(currentState.noteNumber);
                    padPlaybackStates[padIndex].isPlaying = false;
                    padPlaybackStates[padIndex].currentPosition = 0.0f;
                }
            }
        }
    }

    // Normal update (not playing or no continuation needed)
    setPadSample(padIndex, newFreesoundId);
}

bool FreesoundAdvancedSamplerAudioProcessor::shouldContinuePlayback(int padIndex, double newSampleLength)
{
    if (padIndex < 0 || padIndex >= 16) return false;

    const PlaybackState& state = padPlaybackStates[padIndex];

    // Don't continue if not playing
    if (!state.isPlaying) return false;

    // Don't continue if we're too close to the end (less than 20% remaining)
    if (state.currentPosition > 0.8f) return false;

    // Always allow continuation for simplicity - the new sample will play from start
    // but the visual will continue from current position
    return true;
}

void FreesoundAdvancedSamplerAudioProcessor::generateReadmeFile(const Array<FSSound>& sounds, const std::vector<StringArray>& soundInfo, const String& searchQuery)
{
   // Implementation removed for brevity - the method body was commented out in the original
}

void FreesoundAdvancedSamplerAudioProcessor::updateReadmeFile()
{
   // Implementation removed for brevity - the method body was commented out in the original
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

// ====== NEW: Per-pad playback configuration API ======

const FreesoundAdvancedSamplerAudioProcessor::PlaybackConfig&
FreesoundAdvancedSamplerAudioProcessor::getPadPlaybackConfig (int padIndex) const
{
    static PlaybackConfig fallback; // default-initialized
    if (padIndex < 0 || padIndex >= (int)padConfigs.size())
        return fallback;
    return padConfigs[(size_t)padIndex];
}

static inline int faClampPad(int i) { return juce::jlimit(0, 15, i); }

void FreesoundAdvancedSamplerAudioProcessor::setPadStartEndSamples (int padIndex, double start, double end)
{
    auto& cfg = padConfigs[(size_t)faClampPad(padIndex)];
    cfg.startSample = std::max(0.0, start);
    cfg.endSample   = end; // allow -1.0 => full length
}

void FreesoundAdvancedSamplerAudioProcessor::setPadTemporaryStartSamples (int padIndex, double tempStart)
{
    auto& cfg = padConfigs[(size_t)faClampPad(padIndex)];
    cfg.tempStartSample = std::max(0.0, tempStart);
    cfg.tempStartArmed  = true; // one-shot; consumed by voice at next note-on
}

void FreesoundAdvancedSamplerAudioProcessor::setPadStartEndNormalized (int padIndex, float startNorm, float endNorm)
{
    auto& cfg = padConfigs[(size_t)faClampPad(padIndex)];
    cfg.startSample = juce::jlimit(0.0, 1.0, (double)startNorm);
    cfg.endSample   = juce::jlimit(0.0, 1.0, (double)endNorm);
}

void FreesoundAdvancedSamplerAudioProcessor::setPadTemporaryStartNormalized (int padIndex, float startNorm)
{
    auto& cfg = padConfigs[(size_t)faClampPad(padIndex)];
    cfg.tempStartSample = juce::jlimit(0.0, 1.0, (double)startNorm);
    cfg.tempStartArmed  = true;
}

void FreesoundAdvancedSamplerAudioProcessor::setPadPitchShift (int padIndex, float semitones)
{
    padConfigs[(size_t)faClampPad(padIndex)].pitchShiftSemitones = semitones;
}

void FreesoundAdvancedSamplerAudioProcessor::setPadStretchRatio (int padIndex, float ratio)
{
    padConfigs[(size_t)faClampPad(padIndex)].stretchRatio = juce::jmax(0.001f, ratio);
}

void FreesoundAdvancedSamplerAudioProcessor::setPadGain (int padIndex, float linearGain)
{
    padConfigs[(size_t)faClampPad(padIndex)].gain = juce::jlimit(0.0f, 4.0f, linearGain);
}

void FreesoundAdvancedSamplerAudioProcessor::setPadADSR (int padIndex, const ADSR::Parameters& p)
{
    padConfigs[(size_t)faClampPad(padIndex)].adsr = p;
}

void FreesoundAdvancedSamplerAudioProcessor::setPadDirection (int padIndex, TrackingSamplerVoice::Direction d)
{
    padConfigs[(size_t)faClampPad(padIndex)].direction = d;
}

void FreesoundAdvancedSamplerAudioProcessor::setPadPlayMode (int padIndex, TrackingSamplerVoice::PlayMode m)
{
    padConfigs[(size_t)faClampPad(padIndex)].playMode = m;
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
   return new FreesoundAdvancedSamplerAudioProcessor();
}

Array<FSSound> FreesoundAdvancedSamplerAudioProcessor::getCurrentSounds()
{
   return currentSoundsArray;
}

File FreesoundAdvancedSamplerAudioProcessor::getCurrentDownloadLocation()
{
   return currentSessionDownloadLocation;
}


