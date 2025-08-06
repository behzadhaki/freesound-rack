#include "SampleCollectionManager.h"

//==============================================================================
// SampleMetadata Implementation
//==============================================================================

SampleMetadata SampleMetadata::fromFSSound(const FSSound& sound, const String& searchQuery)
{
    SampleMetadata metadata;
    metadata.freesoundId = sound.id;
    metadata.fileName = "FS_ID_" + sound.id + ".ogg";
    metadata.originalName = sound.name;
    metadata.authorName = sound.user;
    metadata.licenseType = sound.license;
    metadata.tags = sound.tags.joinIntoString(",");
    metadata.description = sound.description;
    metadata.freesoundUrl = "https://freesound.org/s/" + sound.id + "/";
    metadata.duration = sound.duration;
    metadata.fileSize = sound.filesize;
    metadata.searchQuery = searchQuery;
    metadata.downloadedAt = Time::getCurrentTime().toString(true, true);
    metadata.lastModifiedAt = metadata.downloadedAt;
    
    return metadata;
}

var SampleMetadata::toJson() const
{
    DynamicObject::Ptr obj = new DynamicObject();
    
    // Core sample data
    obj->setProperty("freesound_id", freesoundId);
    obj->setProperty("file_name", fileName);
    obj->setProperty("original_name", originalName);
    obj->setProperty("author_name", authorName);
    obj->setProperty("license_type", licenseType);
    obj->setProperty("tags", tags);
    obj->setProperty("description", description);
    obj->setProperty("freesound_url", freesoundUrl);
    obj->setProperty("duration", duration);
    obj->setProperty("file_size", (int)fileSize);
    obj->setProperty("downloaded_at", downloadedAt);
    obj->setProperty("search_query", searchQuery);
    
    // Usage tracking
    obj->setProperty("is_bookmarked", isBookmarked);
    obj->setProperty("bookmarked_at", bookmarkedAt);
    obj->setProperty("last_modified_at", lastModifiedAt);
    
    // Preset associations
    DynamicObject::Ptr presetObj = new DynamicObject();
    for (const auto& association : presetAssociations)
    {
        Array<var> positionsArray;
        for (const String& position : association.second)
        {
            positionsArray.add(position);
        }
        presetObj->setProperty(association.first, positionsArray);
    }
    obj->setProperty("preset_associations", var(presetObj.get()));
    
    return var(obj.get());
}

SampleMetadata SampleMetadata::fromJson(const var& json)
{
    SampleMetadata metadata;
    
    if (!json.isObject())
        return metadata;
    
    // Core sample data
    metadata.freesoundId = json.getProperty("freesound_id", "");
    metadata.fileName = json.getProperty("file_name", "");
    metadata.originalName = json.getProperty("original_name", "");
    metadata.authorName = json.getProperty("author_name", "");
    metadata.licenseType = json.getProperty("license_type", "");
    metadata.tags = json.getProperty("tags", "");
    metadata.description = json.getProperty("description", "");
    metadata.freesoundUrl = json.getProperty("freesound_url", "");
    metadata.duration = json.getProperty("duration", 0.0);
    metadata.fileSize = json.getProperty("file_size", 0);
    metadata.downloadedAt = json.getProperty("downloaded_at", "");
    metadata.searchQuery = json.getProperty("search_query", "");
    
    // Usage tracking
    metadata.isBookmarked = json.getProperty("is_bookmarked", false);
    metadata.bookmarkedAt = json.getProperty("bookmarked_at", "");
    metadata.lastModifiedAt = json.getProperty("last_modified_at", "");
    
    // Preset associations
    var presetAssoc = json.getProperty("preset_associations", var());
    if (presetAssoc.isObject())
    {
        DynamicObject::Ptr presetObj = presetAssoc.getDynamicObject();
        if (presetObj)
        {
            for (const auto& property : presetObj->getProperties())
            {
                String presetId = property.name.toString();
                var positionsVar = property.value;
                
                Array<String> positions;
                if (positionsVar.isArray())
                {
                    Array<var>* positionsArray = positionsVar.getArray();
                    for (const auto& posVar : *positionsArray)
                    {
                        positions.add(posVar.toString());
                    }
                }
                
                metadata.presetAssociations[presetId] = positions;
            }
        }
    }
    
    return metadata;
}

bool SampleMetadata::isInPreset(const String& presetId) const
{
    return presetAssociations.find(presetId) != presetAssociations.end();
}

Array<String> SampleMetadata::getPositionsInPreset(const String& presetId) const
{
    auto it = presetAssociations.find(presetId);
    return (it != presetAssociations.end()) ? it->second : Array<String>();
}

void SampleMetadata::addToPreset(const String& presetId, const String& position)
{
    presetAssociations[presetId].addIfNotAlreadyThere(position);
    lastModifiedAt = Time::getCurrentTime().toString(true, true);
}

void SampleMetadata::removeFromPreset(const String& presetId, const String& position)
{
    auto it = presetAssociations.find(presetId);
    if (it != presetAssociations.end())
    {
        if (position.isEmpty())
        {
            // Remove all positions for this preset
            presetAssociations.erase(it);
        }
        else
        {
            // Remove specific position
            it->second.removeAllInstancesOf(position);
            if (it->second.isEmpty())
            {
                presetAssociations.erase(it);
            }
        }
        lastModifiedAt = Time::getCurrentTime().toString(true, true);
    }
}

void SampleMetadata::removeFromAllPresets()
{
    presetAssociations.clear();
    lastModifiedAt = Time::getCurrentTime().toString(true, true);
}

//==============================================================================
// PresetInfo Implementation
//==============================================================================

var PresetInfo::toJson() const
{
    DynamicObject::Ptr obj = new DynamicObject();
    obj->setProperty("preset_id", presetId);
    obj->setProperty("name", name);
    obj->setProperty("description", description);
    obj->setProperty("created_at", createdAt);
    obj->setProperty("modified_at", modifiedAt);
    obj->setProperty("file_name", fileName);
    obj->setProperty("total_slots", totalSlots);
    return var(obj.get());
}

PresetInfo PresetInfo::fromJson(const var& json)
{
    PresetInfo info;
    if (json.isObject())
    {
        info.presetId = json.getProperty("preset_id", "");
        info.name = json.getProperty("name", "");
        info.description = json.getProperty("description", "");
        info.createdAt = json.getProperty("created_at", "");
        info.modifiedAt = json.getProperty("modified_at", "");
        info.fileName = json.getProperty("file_name", "");
        info.totalSlots = json.getProperty("total_slots", 8);
    }
    return info;
}

//==============================================================================
// SampleCollectionManager Implementation
//==============================================================================

SampleCollectionManager::SampleCollectionManager(const File& baseDirectory)
    : baseDirectory(baseDirectory)
    , samplesFolder(baseDirectory.getChildFile("samples"))
    , collectionFile(baseDirectory.getChildFile("collection_meta.json"))
{
    ensureDirectoriesExist();
    loadCollection();
}

SampleCollectionManager::~SampleCollectionManager()
{
    saveCollection();
}

void SampleCollectionManager::ensureDirectoriesExist()
{
    baseDirectory.createDirectory();
    samplesFolder.createDirectory();
}

//==============================================================================
// Sample Management
//==============================================================================

bool SampleCollectionManager::addOrUpdateSample(const FSSound& sound, const String& searchQuery)
{
    SampleMetadata metadata = SampleMetadata::fromFSSound(sound, searchQuery);
    return addOrUpdateSample(metadata);
}

bool SampleCollectionManager::addOrUpdateSample(const SampleMetadata& metadata)
{
    if (metadata.freesoundId.isEmpty())
        return false;

    const String nowStr = Time::getCurrentTime().toString(true, true);

    auto mergeQueries = [](const String& existing, const String& incoming) -> String
    {
        // Split by comma, trim, unique (case-insensitive), preserve order.
        StringArray out;

        auto pushUnique = [&out](const String& s)
        {
            const String t = s.trim();
            if (t.isNotEmpty())
                out.addIfNotAlreadyThere(t, /*ignoreCase*/ true);
        };

        // 1) add existing tokens (normalized + deduped)
        {
            StringArray tokens = StringArray::fromTokens(existing, ",", "");
            for (const auto& tok : tokens)
                pushUnique(tok);
        }

        // 2) add incoming tokens if they’re not already present in the text
        {
            StringArray tokens = StringArray::fromTokens(incoming, ",", "");
            for (const auto& tok : tokens)
            {
                const String t = tok.trim();
                if (t.isEmpty()) continue;

                bool present = out.contains(t, /*ignoreCase*/ true);
                if (!present)
                {
                    const auto tLower = t.toLowerCase();
                    for (int i = 0; i < out.size(); ++i)
                    {
                        if (out[i].toLowerCase().contains(tLower)) { present = true; break; }
                    }
                }

                if (!present)
                    out.add(t);
            }
        }

        return out.joinIntoString(", ");
    };

    auto it = samples.find(metadata.freesoundId);
    if (it != samples.end())
    {
        // Start from fresh metadata but preserve usage + associations
        SampleMetadata updated = metadata;
        updated.isBookmarked       = it->second.isBookmarked;
        updated.bookmarkedAt       = it->second.bookmarkedAt;
        updated.presetAssociations = it->second.presetAssociations;

        // Merge queries (existing + incoming)
        updated.searchQuery    = mergeQueries(it->second.searchQuery, metadata.searchQuery);
        updated.lastModifiedAt = nowStr;

        samples[metadata.freesoundId] = std::move(updated);
    }
    else
    {
        // New entry: normalize query list (trim/dedupe) for cleanliness
        SampleMetadata fresh = metadata;
        fresh.searchQuery    = mergeQueries("", metadata.searchQuery);
        fresh.lastModifiedAt = nowStr;

        samples[metadata.freesoundId] = std::move(fresh);
    }

    return saveCollection();
}

SampleMetadata SampleCollectionManager::getSample(const String& freesoundId) const
{
    auto it = samples.find(freesoundId);
    return (it != samples.end()) ? it->second : SampleMetadata();
}

bool SampleCollectionManager::hasSample(const String& freesoundId) const
{
    return samples.find(freesoundId) != samples.end();
}

Array<SampleMetadata> SampleCollectionManager::getAllSamples() const
{
    Array<SampleMetadata> result;
    for (const auto& pair : samples)
    {
        result.add(pair.second);
    }
    return result;
}

Array<SampleMetadata> SampleCollectionManager::getSamplesForPreset(const String& presetId) const
{
    Array<SampleMetadata> result;
    for (const auto& pair : samples)
    {
        if (pair.second.isInPreset(presetId))
        {
            result.add(pair.second);
        }
    }
    return result;
}

Array<SampleMetadata> SampleCollectionManager::getBookmarkedSamples() const
{
    Array<SampleMetadata> result;
    for (const auto& pair : samples)
    {
        if (pair.second.isBookmarked)
        {
            result.add(pair.second);
        }
    }
    
    // Sort by bookmark date (newest first)
    std::sort(result.begin(), result.end(), 
              [](const SampleMetadata& a, const SampleMetadata& b) {
                  return a.bookmarkedAt > b.bookmarkedAt;
              });
    
    return result;
}

Array<SampleMetadata> SampleCollectionManager::searchSamples(const String& searchTerm) const
{
    Array<SampleMetadata> result;
    String lowerSearchTerm = searchTerm.toLowerCase();
    
    for (const auto& pair : samples)
    {
        const SampleMetadata& sample = pair.second;
        if (sample.originalName.toLowerCase().contains(lowerSearchTerm) ||
            sample.authorName.toLowerCase().contains(lowerSearchTerm) ||
            sample.tags.toLowerCase().contains(lowerSearchTerm) ||
            sample.description.toLowerCase().contains(lowerSearchTerm) ||
            sample.searchQuery.toLowerCase().contains(lowerSearchTerm))
        {
            result.add(sample);
        }
    }
    
    return result;
}

File SampleCollectionManager::getSampleFile(const String& freesoundId) const
{
    return samplesFolder.getChildFile("FS_ID_" + freesoundId + ".ogg");
}

bool SampleCollectionManager::sampleFileExists(const String& freesoundId) const
{
    return getSampleFile(freesoundId).existsAsFile();
}

//==============================================================================
// Bookmark Management
//==============================================================================

bool SampleCollectionManager::addBookmark(const String& freesoundId, const String& customNote)
{
    auto it = samples.find(freesoundId);
    if (it == samples.end())
        return false;
    
    it->second.isBookmarked = true;
    it->second.bookmarkedAt = Time::getCurrentTime().toString(true, true);
    it->second.lastModifiedAt = it->second.bookmarkedAt;
    
    return saveCollection();
}

bool SampleCollectionManager::removeBookmark(const String& freesoundId)
{
    auto it = samples.find(freesoundId);
    if (it == samples.end())
        return false;
    
    it->second.isBookmarked = false;
    it->second.bookmarkedAt = "";
    it->second.lastModifiedAt = Time::getCurrentTime().toString(true, true);
    
    return saveCollection();
}

bool SampleCollectionManager::isBookmarked(const String& freesoundId) const
{
    auto it = samples.find(freesoundId);
    return (it != samples.end()) ? it->second.isBookmarked : false;
}

//==============================================================================
// Preset Management
//==============================================================================

String SampleCollectionManager::createPreset(const String& name, const String& description)
{
    String presetId = generatePresetId();
    
    PresetInfo info;
    info.presetId = presetId;
    info.name = name;
    info.description = description;
    info.createdAt = Time::getCurrentTime().toString(true, true);
    info.modifiedAt = info.createdAt;
    info.fileName = presetId + ".json"; // For future compatibility
    
    presets[presetId] = info;
    saveCollection();
    
    return presetId;
}

bool SampleCollectionManager::deletePreset(const String& presetId)
{
    // Remove preset info
    auto presetIt = presets.find(presetId);
    if (presetIt == presets.end())
        return false;
    
    presets.erase(presetIt);
    
    // Remove all sample associations with this preset
    for (auto& samplePair : samples)
    {
        samplePair.second.removeFromPreset(presetId);
    }
    
    return saveCollection();
}

bool SampleCollectionManager::renamePreset(const String& presetId, const String& newName)
{
    auto it = presets.find(presetId);
    if (it == presets.end())
        return false;
    
    it->second.name = newName;
    it->second.modifiedAt = Time::getCurrentTime().toString(true, true);
    
    return saveCollection();
}

Array<PresetInfo> SampleCollectionManager::getAllPresets() const
{
    Array<PresetInfo> result;
    for (const auto& pair : presets)
    {
        result.add(pair.second);
    }
    
    // Sort by creation date (newest first)
    std::sort(result.begin(), result.end(),
              [](const PresetInfo& a, const PresetInfo& b) {
                  return a.createdAt > b.createdAt;
              });
    
    return result;
}

PresetInfo SampleCollectionManager::getPreset(const String& presetId) const
{
    auto it = presets.find(presetId);
    return (it != presets.end()) ? it->second : PresetInfo();
}

//==============================================================================
// Preset-Sample Associations
//==============================================================================

bool SampleCollectionManager::setPresetSlot(const String& presetId, int slotIndex, const Array<PadSlotData>& padData)
{
    // Clear existing slot associations
    clearPresetSlot(presetId, slotIndex);

    // Add new associations - handles duplicates properly
    for (const auto& data : padData)
    {
        if (data.freesoundId.isEmpty() || data.padIndex < 0 || data.padIndex >= 16)
            continue;

        auto sampleIt = samples.find(data.freesoundId);
        if (sampleIt != samples.end())
        {
            String positionKey = makePositionKey(slotIndex, data.padIndex);
            sampleIt->second.addToPreset(presetId, positionKey);
        }
        else
        {
            // Sample not in collection yet - this could happen during save
            DBG("Sample " + data.freesoundId + " not found in collection when saving preset");
        }
    }

    // Update preset modification time
    auto presetIt = presets.find(presetId);
    if (presetIt != presets.end())
    {
        presetIt->second.modifiedAt = Time::getCurrentTime().toString(true, true);
    }

    return saveCollection();
}

Array<SampleCollectionManager::PadSlotData> SampleCollectionManager::getPresetSlot(const String& presetId, int slotIndex) const
{
    Array<PadSlotData> result;

    // Collect all samples that have positions in this preset slot
    for (const auto& samplePair : samples)
    {
        Array<String> positions = samplePair.second.getPositionsInPreset(presetId);

        for (const String& positionKey : positions)
        {
            auto [slot, pad] = parsePositionKey(positionKey);
            if (slot == slotIndex && pad >= 0 && pad < 16)
            {
                PadSlotData data;
                data.padIndex = pad;
                data.freesoundId = samplePair.second.freesoundId;
                data.individualQuery = samplePair.second.searchQuery; // Use sample's default query for now
                result.add(data);
            }
        }
    }

    // Sort by pad index to maintain consistent order
    std::sort(result.begin(), result.end(),
              [](const PadSlotData& a, const PadSlotData& b) {
                  return a.padIndex < b.padIndex;
              });

    return result;
}


bool SampleCollectionManager::clearPresetSlot(const String& presetId, int slotIndex)
{
    for (auto& samplePair : samples)
    {
        Array<String> positions = samplePair.second.getPositionsInPreset(presetId);
        
        for (const String& positionKey : positions)
        {
            auto [slot, pad] = parsePositionKey(positionKey);
            if (slot == slotIndex)
            {
                samplePair.second.removeFromPreset(presetId, positionKey);
            }
        }
    }
    
    return saveCollection();
}

bool SampleCollectionManager::saveCurrentStateToPreset(const String& presetId, int slotIndex,
                                                      const Array<SampleMetadata>& currentSamples,
                                                      const Array<int>& padPositions)
{
    if (currentSamples.size() != padPositions.size())
        return false;

    Array<String> freesoundIds;
    for (const auto& sample : currentSamples)
    {
        freesoundIds.add(sample.freesoundId);
    }

    Array<PadSlotData> padData;
    for (int i = 0; i < freesoundIds.size() && i < padPositions.size(); ++i)
    {
        padData.add(PadSlotData(padPositions[i], freesoundIds[i]));
    }
    return setPresetSlot(presetId, slotIndex, padData);
}

Array<SampleMetadata> SampleCollectionManager::loadPresetSlot(const String& presetId, int slotIndex) const
{
    Array<SampleMetadata> result;

    // Get pad-specific data
    Array<PadSlotData> padData = getPresetSlot(presetId, slotIndex);

    // Sort by pad index to maintain order
    std::sort(padData.begin(), padData.end(),
              [](const PadSlotData& a, const PadSlotData& b) {
                  return a.padIndex < b.padIndex;
              });

    for (const auto& data : padData)
    {
        if (data.freesoundId.isNotEmpty())
        {
            SampleMetadata sample = getSample(data.freesoundId);
            if (!sample.freesoundId.isEmpty())
            {
                // Store pad index in the metadata for reference
                // (You might need to add padIndex to SampleMetadata struct)
                result.add(sample);
            }
        }
    }
    
    return result;
}

//==============================================================================
// Statistics and Maintenance
//==============================================================================
void SampleCollectionManager::cleanupOrphanedSamples()
{
    Array<String> toRemove;
    
    for (const auto& samplePair : samples)
    {
        const SampleMetadata& sample = samplePair.second;
        
        // Keep if bookmarked or in any preset
        if (!sample.isBookmarked && sample.presetAssociations.empty())
        {
            toRemove.add(sample.freesoundId);
        }
    }
    
    // Remove orphaned samples
    for (const String& freesoundId : toRemove)
    {
        samples.erase(freesoundId);
        
        // Also delete the file
        File sampleFile = getSampleFile(freesoundId);
        if (sampleFile.existsAsFile())
        {
            sampleFile.deleteFile();
        }
    }
    
    if (!toRemove.isEmpty())
    {
        saveCollection();
    }
}

void SampleCollectionManager::validateSampleFiles()
{
    Array<String> missingFiles;
    
    for (const auto& samplePair : samples)
    {
        if (!sampleFileExists(samplePair.first))
        {
            missingFiles.add(samplePair.first);
        }
    }
    
    // Remove samples with missing files
    for (const String& freesoundId : missingFiles)
    {
        samples.erase(freesoundId);
    }
    
    if (!missingFiles.isEmpty())
    {
        saveCollection();
    }
}

//==============================================================================
// JSON Serialization
//==============================================================================

bool SampleCollectionManager::saveCollection()
{
    var collectionJson = createCollectionJson();
    String jsonString = JSON::toString(collectionJson, false);
    
    return collectionFile.replaceWithText(jsonString);
}

bool SampleCollectionManager::loadCollection()
{
    if (!collectionFile.existsAsFile())
    {
        // Create initial structure
        return saveCollection();
    }
    
    String jsonText = collectionFile.loadFileAsString();
    var parsedJson = JSON::parse(jsonText);
    
    return parseCollectionJson(parsedJson);
}

var SampleCollectionManager::createCollectionJson() const
{
    DynamicObject::Ptr root = new DynamicObject();
    
    // File metadata
    DynamicObject::Ptr fileMetadata = new DynamicObject();
    fileMetadata->setProperty("version", "1.0");
    fileMetadata->setProperty("created_at", Time::getCurrentTime().toString(true, true));
    fileMetadata->setProperty("plugin_name", "Freesound Advanced Sampler");
    fileMetadata->setProperty("total_samples", (int)samples.size());
    fileMetadata->setProperty("total_presets", (int)presets.size());
    fileMetadata->setProperty("last_saved", Time::getCurrentTime().toString(true, true));
    root->setProperty("file_metadata", var(fileMetadata.get()));

    // Samples section
    Array<var> samplesArray;
    for (const auto& samplePair : samples)
    {
        samplesArray.add(samplePair.second.toJson());
    }
    root->setProperty("samples", samplesArray);

    // Presets section
    Array<var> presetsArray;
    for (const auto& presetPair : presets)
    {
        presetsArray.add(presetPair.second.toJson());
    }
    root->setProperty("presets", presetsArray);

    return var(root.get());
}

bool SampleCollectionManager::parseCollectionJson(const var& json)
{
    if (!json.isObject())
        return false;

    // Clear existing data
    samples.clear();
    presets.clear();

    // Load samples
    var samplesVar = json.getProperty("samples", var());
    if (samplesVar.isArray())
    {
        Array<var>* samplesArray = samplesVar.getArray();
        for (const auto& sampleVar : *samplesArray)
        {
            SampleMetadata sample = SampleMetadata::fromJson(sampleVar);
            if (!sample.freesoundId.isEmpty())
            {
                samples[sample.freesoundId] = sample;
            }
        }
    }

    // Load presets
    var presetsVar = json.getProperty("presets", var());
    if (presetsVar.isArray())
    {
        Array<var>* presetsArray = presetsVar.getArray();
        for (const auto& presetVar : *presetsArray)
        {
            PresetInfo preset = PresetInfo::fromJson(presetVar);
            if (!preset.presetId.isEmpty())
            {
                presets[preset.presetId] = preset;
            }
        }
    }

    return true;
}

//==============================================================================
// Private Helper Methods
//==============================================================================

String SampleCollectionManager::generatePresetId() const
{
    // Generate unique ID based on timestamp and random component
    Time now = Time::getCurrentTime();
    String timestamp = String(now.toMilliseconds());
    String randomPart = String::toHexString(Random::getSystemRandom().nextInt64());
    return "preset_" + timestamp + "_" + randomPart.substring(0, 8);
}

String SampleCollectionManager::makePositionKey(int slotIndex, int padIndex) const
{
    return "slot_" + String(slotIndex) + "_pad_" + String(padIndex);
}

std::pair<int, int> SampleCollectionManager::parsePositionKey(const String& positionKey) const
{
    // Parse "slot_0_pad_5" -> {0, 5}
    StringArray parts = StringArray::fromTokens(positionKey, "_", "");

    if (parts.size() >= 4 && parts[0] == "slot" && parts[2] == "pad")
    {
        int slotIndex = parts[1].getIntValue();
        int padIndex = parts[3].getIntValue();
        return {slotIndex, padIndex};
    }

    return {-1, -1};
}