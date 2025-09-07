#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "FreesoundAPI/FreesoundAPI.h"

using namespace juce;

//==============================================================================
// Core Sample Metadata Structure
//==============================================================================
struct SampleMetadata
{
    // Core sample identification
    String freesoundId;
    String fileName;
    String originalName;
    String authorName;
    String licenseType;
    String tags;
    String description;
    String freesoundUrl;
    
    // File information
    double duration;
    int64 fileSize;
    String downloadedAt;
    String searchQuery; // The query that found this sample
    
    // Technical metadata
    double sampleRate;
    int bitDepth;
    int channelCount;

    // Usage tracking
    bool isBookmarked;
    String bookmarkedAt;

    // Preset associations - map of presetId -> Array<slot_pad_positions>
    std::map<String, Array<String>> presetAssociations; // e.g. "preset123" -> ["slot_0_pad_5", "slot_1_pad_12"]

    // Statistics
    String lastModifiedAt;

    SampleMetadata()
        : duration(0.0)
        , fileSize(0)
        , sampleRate(0.0)
        , bitDepth(0)
        , channelCount(0)
        , isBookmarked(false)
    {}

    // Create from FSSound
    static SampleMetadata fromFSSound(const FSSound& sound, const String& searchQuery = "");

    // Convert to/from JSON
    var toJson() const;
    static SampleMetadata fromJson(const var& json);

    // Utility methods
    bool isInPreset(const String& presetId) const;
    Array<String> getPositionsInPreset(const String& presetId) const;
    void addToPreset(const String& presetId, const String& position); // e.g. "slot_0_pad_5"
    void removeFromPreset(const String& presetId, const String& position = "");
    void removeFromAllPresets();
};

//==============================================================================
// Preset Information (simplified)
//==============================================================================
struct PresetInfo
{
    String presetId;
    String name;
    String description;
    String createdAt;
    String modifiedAt;
    String fileName; // JSON filename
    int totalSlots;

    PresetInfo() : totalSlots(8) {}

    var toJson() const;
    static PresetInfo fromJson(const var& json);
};

//==============================================================================
// Master Sample Collection Manager
//==============================================================================
class SampleCollectionManager
{
public:
    SampleCollectionManager(const File& baseDirectory);
    ~SampleCollectionManager();

    //==============================================================================
    // Sample Management
    //==============================================================================

    // Add/update sample in collection
    bool addOrUpdateSample(const FSSound& sound, const String& searchQuery = "");
    bool addOrUpdateSample(const SampleMetadata& metadata);

    // Audio file analysis
    bool analyzeSampleFile(const String& freesoundId);
    bool analyzeAllSampleFiles();

    // Get sample information
    SampleMetadata getSample(const String& freesoundId) const;
    bool hasSample(const String& freesoundId) const;
    Array<SampleMetadata> getAllSamples() const;
    Array<SampleMetadata> getSamplesForPreset(const String& presetId) const;
    Array<SampleMetadata> getBookmarkedSamples() const;
    Array<SampleMetadata> searchSamples(const String& searchTerm) const;

    // Sample file operations
    File getSampleFile(const String& freesoundId) const;
    bool sampleFileExists(const String& freesoundId) const;

    //==============================================================================
    // Bookmark Management
    //==============================================================================

    bool addBookmark(const String& freesoundId, const String& customNote = "");
    bool removeBookmark(const String& freesoundId);
    bool isBookmarked(const String& freesoundId) const;

    //==============================================================================
    // Preset Management
    //==============================================================================

    // Preset CRUD operations
    String createPreset(const String& name, const String& description = "");
    bool deletePreset(const String& presetId);
    bool renamePreset(const String& presetId, const String& newName);
    Array<PresetInfo> getAllPresets() const;
    PresetInfo getPreset(const String& presetId) const;

    // Preset-sample associations
    struct PadSlotData
    {
        int padIndex;
        String freesoundId;
        String individualQuery; // Each pad can have its own query even if same sample

        PadSlotData() : padIndex(-1) {}
        PadSlotData(int pad, const String& id, const String& query = "")
            : padIndex(pad), freesoundId(id), individualQuery(query) {}
    };

    bool setPresetSlot(const String& presetId, int slotIndex, const Array<PadSlotData>& padData);
    Array<PadSlotData> getPresetSlot(const String& presetId, int slotIndex) const;

    bool clearPresetSlot(const String& presetId, int slotIndex);

    // Preset operations
    bool saveCurrentStateToPreset(const String& presetId, int slotIndex, const Array<SampleMetadata>& currentSamples, const Array<int>& padPositions);
    Array<SampleMetadata> loadPresetSlot(const String& presetId, int slotIndex) const;

    //==============================================================================
    // Statistics and Maintenance
    //==============================================================================

    void cleanupOrphanedSamples(); // Remove samples not in any preset and not bookmarked
    void validateSampleFiles(); // Check if sample files still exist

    //==============================================================================
    // File Management
    //==============================================================================

    File getBaseDirectory() const { return baseDirectory; }
    File getSamplesFolder() const { return samplesFolder; }
    File getCollectionFile() const { return collectionFile; }

    // Force save/load operations
    bool saveCollection();
    bool loadCollection();

private:
    File baseDirectory;
    File samplesFolder;
    File collectionFile; // collection_meta.json
    
    // In-memory data
    std::map<String, SampleMetadata> samples; // freesoundId -> metadata
    std::map<String, PresetInfo> presets;     // presetId -> preset info
    
    // Internal helpers
    void ensureDirectoriesExist();
    String generatePresetId() const;
    String makePositionKey(int slotIndex, int padIndex) const; // "slot_0_pad_5"
    std::pair<int, int> parsePositionKey(const String& positionKey) const; // returns {slotIndex, padIndex}
    
    // JSON serialization
    var createCollectionJson() const;
    bool parseCollectionJson(const var& json);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleCollectionManager)
};