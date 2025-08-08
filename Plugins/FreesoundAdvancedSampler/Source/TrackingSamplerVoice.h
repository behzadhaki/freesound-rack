#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
using namespace juce;

class FreesoundAdvancedSamplerAudioProcessor;

// Mono-per-note Synthesiser: guarantees only one voice plays for a (channel,note)
struct MonoPerNoteSynth : public juce::Synthesiser
{
    void noteOn (int midiChannel, int midiNoteNumber, float velocity) override
    {
        // Velocity 0 = note-off (per MIDI spec)
        if (velocity <= 0.0f)
        {
            noteOff(midiChannel, midiNoteNumber, 0.0f, false);
            return;
        }

        // Kill any existing voice for this note before retriggering
        for (int i = 0; i < getNumVoices(); ++i)
        {
            auto* v = getVoice(i);
            if (v->isVoiceActive() && v->getCurrentlyPlayingNote() == midiNoteNumber)
                v->stopNote(0.0f, false); // hard stop so no overlap
        }

        Synthesiser::noteOn(midiChannel, midiNoteNumber, velocity);
    }
};


//==============================================================================
// Playback State Structure
//==============================================================================
struct PlaybackState {
    bool isPlaying = false;
    float currentPosition = 0.0f;  // 0.0 to 1.0
    int noteNumber = -1;
    float velocity = 0.0f;
    double sampleLength = 0.0;      // Length of current sample in samples
    String currentFreesoundId = "";  // Track which sample is playing
};

class TrackingSamplerVoice : public SamplerVoice
{
public:
    enum class Direction { Forward, Reverse };
    enum class PlayMode  { Normal, Loop, PingPong };

    TrackingSamplerVoice(FreesoundAdvancedSamplerAudioProcessor& owner);

    // Existing overrides (unchanged signatures)
    void startNote (int midiNoteNumber, float velocity, SynthesiserSound*, int currentPitchWheelPosition) override;
    void stopNote  (float velocity, bool allowTailOff) override;
    void renderNextBlock (AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    // #### New: playback controls (backward compatible defaults) ####
    // Positions are in samples.
    void setStartSample        (double s)               { startSample = std::max(0.0, s); }
    void setEndSample          (double e)               { endSample   = std::max(0.0, e); }
    void setTemporaryStart     (double s)               { tempStartSample = std::max(0.0, s); tempStartArmed = true; }

    // Pitch shift in semitones (positive/negative). Defaults to 0.
    void setPitchShiftSemitones(float st)               { pitchShiftSemitones = st; }

    // Time stretching as a simple playback-rate scalar (1.0 = normal, 0.5 = half speed, 2.0 = double).
    // This is naive resampling (pitch changes unless you counter with pitchShiftSemitones).
    void setStretchRatio       (float r)                { stretchRatio = std::max(0.001f, r); }

    // Linear gain (1.0 = unity)
    void setGain               (float g)                { gain = jlimit(0.0f, 4.0f, g); }

    // ADSR envelope (defaults: A=0, D=0, S=1, R=0)
    void setADSR               (const ADSR::Parameters& p) { adsrParams = p; adsr.setParameters(adsrParams); }

    void setOnsetDirection     (Direction d)            { onsetDirection = d; }
    void setPlayMode           (PlayMode m)             { playMode = m; }

    // Optional helpers if you prefer normalized positions:
    void setStartNormalized(float n) { normalizedToSamples(n, startSample); }
    void setEndNormalized  (float n) { normalizedToSamples(n, endSample);  }
    void setTemporaryStartNormalized(float n) { normalizedToSamples(n, tempStartSample); tempStartArmed = true; }

private:
    // Utilities
    void normalizedToSamples(float n, double& dest) const
    {
        if (sampleLength > 0.0)
            dest = jlimit(0.0, sampleLength, (double) jlimit(0.0f, 1.0f, n) * sampleLength);
    }

    FreesoundAdvancedSamplerAudioProcessor& processor;

    // ---- Existing state (kept for compatibility with your processor UI) ----
    int    currentNoteNumber = -1;
    double samplePosition    = 0.0;   // absolute within full buffer
    double sampleLength      = 0.0;
    bool noteHeld = true;
    bool   firstRender       = true;

    // ---- New playback config/state ----
    double startSample       = 0.0;   // inclusive
    double endSample         = -1.0;  // -1 means "use full length"
    double tempStartSample   = 0.0;   // used once if tempStartArmed
    bool   tempStartArmed    = false;

    float  pitchShiftSemitones = 0.0f;
    float  stretchRatio        = 1.0f;
    float  gain                = 1.0f;
    double sourceSampleRate = 0.0;

    ADSR                 adsr;
    ADSR::Parameters     adsrParams { 0.0f, 0.0f, 1.0f, 0.0f };

    Direction onsetDirection = Direction::Forward;
    PlayMode  playMode       = PlayMode::Normal;
    int       playDirection  = +1;    // +1 forward, -1 reverse at runtime

    // Cached source
    AudioBuffer<float>* sourceBuffer = nullptr;
    int                 sourceChannels = 0;

    // Helpers
    inline double currentIncrement() const
    {
        // naive playback increment: stretching * pitch shift
        const double semitone = std::pow(2.0, (double)pitchShiftSemitones / 12.0);
        return semitone * (double)stretchRatio;
    }

    inline void clampUserLoopPoints()
    {
        const double len = sampleLength > 0.0 ? sampleLength : 0.0;
        startSample = jlimit(0.0, len, startSample);
        if (endSample < 0.0) endSample = len;
        endSample   = jlimit(0.0, len, endSample);
        if (endSample < startSample) std::swap(startSample, endSample);
    }

    void resetRuntimeState();
};

// Preview voice left as-is (no change).
class TrackingPreviewSamplerVoice : public SamplerVoice
{
public:
    TrackingPreviewSamplerVoice(FreesoundAdvancedSamplerAudioProcessor& owner);
    void startNote (int midiNoteNumber, float velocity, SynthesiserSound*, int currentPitchWheelPosition) override;
    void stopNote  (float velocity, bool allowTailOff) override;
    void renderNextBlock(AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

private:
    FreesoundAdvancedSamplerAudioProcessor& processor;
    String currentFreesoundId;
    double samplePosition = 0.0;
    double sampleLength   = 0.0;
};
