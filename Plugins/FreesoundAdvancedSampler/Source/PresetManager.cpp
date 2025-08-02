/*
  ==============================================================================

    PresetManager.cpp
    Created: New preset management system with multi-slot support
    Author: Generated

  ==============================================================================
*/

#include "PresetManager.h"

PresetManager::PresetManager(const File& baseDirectory)
    : baseDirectory(baseDirectory)
    , samplesFolder(baseDirectory.getChildFile("samples"))
    , presetsFolder(baseDirectory.getChildFile("presets"))
    , activeSlotIndex(-1)
{
    ensureDirectoriesExist();
}

PresetManager::~PresetManager()
{
}

void PresetManager::ensureDirectoriesExist()
{
    samplesFolder.createDirectory();
    presetsFolder.createDirectory();
}

bool PresetManager::saveCurrentPreset(const String& name, const String& description,
                                     const Array<PadInfo>& padInfos, const String& searchQuery,
                                     int slotIndex)
{
    if (name.isEmpty() || slotIndex < 0 || slotIndex >= MAX_SLOTS)
        return false;

    // Generate filename
    String fileName = sanitizeFileName(name) + ".json";
    File presetFile = presetsFolder.getChildFile(fileName);

    // If file exists, load existing data, otherwise create new
    juce::DynamicObject::Ptr root;

    if (presetFile.existsAsFile())
    {
        String existingJson = presetFile.loadFileAsString();
        var parsedJson = juce::JSON::parse(existingJson);
        if (parsedJson.isObject())
        {
            root = parsedJson.getDynamicObject();
        }
    }

    if (!root)
    {
        root = new juce::DynamicObject();

        // Create preset info (only for new presets)
        juce::DynamicObject::Ptr presetInfo = new juce::DynamicObject();
        presetInfo->setProperty("name", name);
        presetInfo->setProperty("created_date", juce::Time::getCurrentTime().toString(true, true));
        presetInfo->setProperty("description", description);
        root->setProperty("preset_info", juce::var(presetInfo.get()));
    }

    // Create empty slot data if no padInfos provided
    Array<PadInfo> emptyPadInfos;
    if (padInfos.isEmpty())
    {
        for (int i = 0; i < MAX_SLOTS; i++)
        {
            PadInfo emptyInfo;
            emptyInfo.padIndex = i;
            emptyPadInfos.add(emptyInfo);
        }
    }

    // Create slot data
    juce::DynamicObject::Ptr slotData = new juce::DynamicObject();

    // Slot info
    juce::DynamicObject::Ptr slotInfo = new juce::DynamicObject();
    slotInfo->setProperty("name", "Slot " + String(slotIndex + 1));
    slotInfo->setProperty("created_date", juce::Time::getCurrentTime().toString(true, true));
    slotInfo->setProperty("search_query", searchQuery);
    slotInfo->setProperty("description", description);
    slotInfo->setProperty("sample_count", padInfos.size());

    slotData->setProperty("slot_info", juce::var(slotInfo.get()));

    // Pad mapping
    juce::Array<juce::var> padMapping;

    for (const auto& padInfo : padInfos)
    {
        juce::DynamicObject::Ptr pad = new juce::DynamicObject();
        pad->setProperty("pad_index", padInfo.padIndex);
        pad->setProperty("freesound_id", padInfo.freesoundId);
        pad->setProperty("file_name", padInfo.fileName);
        pad->setProperty("original_name", padInfo.originalName);
        pad->setProperty("author", padInfo.author);
        pad->setProperty("license", padInfo.license);
        pad->setProperty("duration", padInfo.duration);
        pad->setProperty("file_size", padInfo.fileSize);
        pad->setProperty("downloaded_at", padInfo.downloadedAt);
        pad->setProperty("freesound_url", "https://freesound.org/s/" + padInfo.freesoundId + "/");

        padMapping.add(juce::var(pad.get()));
    }

    slotData->setProperty("pad_mapping", padMapping);

    // Save slot data under slot key
    root->setProperty(getSlotKey(slotIndex), juce::var(slotData.get()));

    // Write to file
    String jsonString = juce::JSON::toString(juce::var(root.get()), true);
    bool success = presetFile.replaceWithText(jsonString);

    if (success)
    {
        setActivePreset(presetFile, slotIndex);
    }

    return success;
}

bool PresetManager::saveToSlot(const File& presetFile, int slotIndex, const String& description,
                              const Array<PadInfo>& padInfos, const String& searchQuery)
{
    // Validate slot index
    if (slotIndex < 0 || slotIndex >= MAX_SLOTS)
    {
        // DBG("Invalid slot index: " + String(slotIndex));
        return false;
    }

    // Create preset file if it doesn't exist
    if (!presetFile.exists())
    {
        // Create empty JSON structure using simple object creation
        DynamicObject::Ptr rootObject = new DynamicObject();
        var rootVar(rootObject.get());
        String jsonString = JSON::toString(rootVar, true);
        if (!presetFile.replaceWithText(jsonString))
        {
            // DBG("Failed to create preset file: " + presetFile.getFullPathName());
            return false;
        }
    }

    // Read existing JSON data
    String jsonText;
    {
        FileInputStream inputStream(presetFile);
        if (!inputStream.openedOk())
        {
            // DBG("Failed to open preset file for reading: " + presetFile.getFullPathName());
            return false;
        }
        jsonText = inputStream.readEntireStreamAsString();
    } // inputStream destructor called here automatically

    // Parse existing JSON
    var parsedJson = JSON::parse(jsonText);
    if (!parsedJson.isObject())
    {
        // Create new root object if parsing failed
        DynamicObject::Ptr newRoot = new DynamicObject();
        parsedJson = var(newRoot.get());
    }

    // Get the root object
    DynamicObject::Ptr rootObj = parsedJson.getDynamicObject();
    if (rootObj == nullptr)
    {
        // DBG("Failed to get root object from JSON");
        return false;
    }

    // Create slot key
    String slotKey = "slot_" + String(slotIndex);

    // Create new slot data using proper JUCE var construction
    DynamicObject::Ptr newSlotObj = new DynamicObject();

    // Create slot info
    DynamicObject::Ptr slotInfoObj = new DynamicObject();
    slotInfoObj->setProperty("name", "Slot " + String(slotIndex + 1));
    slotInfoObj->setProperty("description", description);
    slotInfoObj->setProperty("search_query", searchQuery);
    slotInfoObj->setProperty("total_samples", padInfos.size());
    slotInfoObj->setProperty("created_at", Time::getCurrentTime().toString(true, true));
    slotInfoObj->setProperty("modified_at", Time::getCurrentTime().toString(true, true));

    newSlotObj->setProperty("slot_info", var(slotInfoObj.get()));

    // Create samples array
    Array<var> samplesArray;
    for (const auto& padInfo : padInfos)
    {
        DynamicObject::Ptr sampleObj = new DynamicObject();
        sampleObj->setProperty("pad_index", padInfo.padIndex);
        sampleObj->setProperty("freesound_id", padInfo.freesoundId);
        sampleObj->setProperty("file_name", padInfo.fileName);
        sampleObj->setProperty("original_name", padInfo.originalName);
        sampleObj->setProperty("author", padInfo.author);
        sampleObj->setProperty("license", padInfo.license);
        sampleObj->setProperty("search_query", padInfo.searchQuery);
        sampleObj->setProperty("duration", padInfo.duration);
        sampleObj->setProperty("file_size", padInfo.fileSize);
        sampleObj->setProperty("downloaded_at", padInfo.downloadedAt);
        sampleObj->setProperty("freesound_url", "https://freesound.org/s/" + padInfo.freesoundId + "/");

        samplesArray.add(var(sampleObj.get()));
    }

    newSlotObj->setProperty("samples", samplesArray);

    // Add metadata
    DynamicObject::Ptr metadataObj = new DynamicObject();
    metadataObj->setProperty("version", "1.0");
    metadataObj->setProperty("plugin_name", "Freesound Advanced Sampler");
    metadataObj->setProperty("saved_with_juce_version", SystemStats::getJUCEVersion());
    newSlotObj->setProperty("metadata", var(metadataObj.get()));

    // Set the slot data in the root object
    rootObj->setProperty(slotKey, var(newSlotObj.get()));

    // Update file metadata if it exists
    var fileMetadata = parsedJson.getProperty("file_metadata", var());
    DynamicObject::Ptr fileMetadataObj;

    if (!fileMetadata.isObject())
    {
        fileMetadataObj = new DynamicObject();
        fileMetadata = var(fileMetadataObj.get());
    }
    else
    {
        fileMetadataObj = fileMetadata.getDynamicObject();
    }

    if (fileMetadataObj != nullptr)
    {
        fileMetadataObj->setProperty("last_modified", Time::getCurrentTime().toString(true, true));
        fileMetadataObj->setProperty("total_slots_used", getUsedSlotsCount(parsedJson) + (rootObj->hasProperty(slotKey) ? 0 : 1));
        rootObj->setProperty("file_metadata", fileMetadata);
    }

    // Convert back to JSON string
    String newJsonString = JSON::toString(parsedJson, true);

    // Write to file
    if (!presetFile.replaceWithText(newJsonString))
    {
        // DBG("Failed to write preset file: " + presetFile.getFullPathName());
        return false;
    }

    // DBG("Successfully saved preset to slot " + String(slotIndex) + " in file: " + presetFile.getFullPathName());
    // DBG("Saved " + String(padInfos.size()) + " samples with search query: " + searchQuery);

    return true;
}

// Helper method to count used slots
int PresetManager::getUsedSlotsCount(const var& rootJson) const
{
    if (!rootJson.isObject())
        return 0;

    int count = 0;
    for (int i = 0; i < MAX_SLOTS; ++i)
    {
        String slotKey = "slot_" + String(i);
        if (rootJson.hasProperty(slotKey))
        {
            var slotData = rootJson.getProperty(slotKey, var());
            if (slotData.isObject())
            {
                count++;
            }
        }
    }
    return count;
}

bool PresetManager::loadPreset(const File& presetFile, int slotIndex, Array<PadInfo>& padInfos)
{
    // Clear the output array
    padInfos.clear();

    // Validate slot index
    if (slotIndex < 0 || slotIndex >= MAX_SLOTS)
    {
        // DBG("Invalid slot index: " + String(slotIndex));
        return false;
    }

    // Check if preset file exists
    if (!presetFile.exists())
    {
        // DBG("Preset file does not exist: " + presetFile.getFullPathName());
        return false;
    }

    // Read the JSON file
    String jsonText = presetFile.loadFileAsString();
    if (jsonText.isEmpty())
    {
        // DBG("Preset file is empty: " + presetFile.getFullPathName());
        return false;
    }

    // Parse JSON
    var parsedJson = JSON::parse(jsonText);
    if (!parsedJson.isObject())
    {
        // DBG("Invalid JSON in preset file: " + presetFile.getFullPathName());
        return false;
    }

    // Create slot key
    String slotKey = "slot_" + String(slotIndex);

    // Check if slot exists
    if (!parsedJson.hasProperty(slotKey))
    {
        // DBG("Slot " + String(slotIndex) + " does not exist in preset file");
        return false;
    }

    // Get slot data
    var slotData = parsedJson.getProperty(slotKey, var());
    if (!slotData.isObject())
    {
        // DBG("Invalid slot data for slot " + String(slotIndex));
        return false;
    }

    // Get samples array
    var samplesVar = slotData.getProperty("samples", var());
    if (!samplesVar.isArray())
    {
        // DBG("No samples array found in slot " + String(slotIndex));
        return false;
    }

    Array<var>* samplesArray = samplesVar.getArray();
    if (samplesArray == nullptr)
    {
        // DBG("Failed to get samples array from slot " + String(slotIndex));
        return false;
    }

    // Load each sample
    for (const auto& sampleVar : *samplesArray)
    {
        if (!sampleVar.isObject())
        {
            // DBG("Invalid sample object found, skipping");
            continue;
        }

        // Extract sample properties
        PadInfo padInfo;

        // Get required properties with defaults
        padInfo.padIndex = sampleVar.getProperty("pad_index", -1);
        padInfo.freesoundId = sampleVar.getProperty("freesound_id", "");
        padInfo.fileName = sampleVar.getProperty("file_name", "");
        padInfo.originalName = sampleVar.getProperty("original_name", "");
        padInfo.author = sampleVar.getProperty("author", "");
        padInfo.license = sampleVar.getProperty("license", "");
        padInfo.searchQuery = sampleVar.getProperty("search_query", "");  // Handle query
        padInfo.duration = sampleVar.getProperty("duration", 0.0);
        padInfo.fileSize = sampleVar.getProperty("file_size", 0);
        padInfo.downloadedAt = sampleVar.getProperty("downloaded_at", "");

        // Validate essential properties
        if (padInfo.freesoundId.isEmpty() || padInfo.padIndex < 0 || padInfo.padIndex >= 16)
        {
            // DBG("Invalid sample data: freesoundId=" + padInfo.freesoundId +
                // ", padIndex=" + String(padInfo.padIndex) + ", skipping");
            continue;
        }

        // Check if the sample file exists
        File sampleFile = getSampleFile(padInfo.freesoundId);
        if (!sampleFile.existsAsFile())
        {
            // DBG("Sample file does not exist: " + sampleFile.getFullPathName() +
                // " for freesoundId: " + padInfo.freesoundId);
            // Continue anyway - the processor will handle missing files
        }

        // Add to output array
        padInfos.add(padInfo);

        // DBG("Loaded sample: " + padInfo.originalName +
            // " (ID: " + padInfo.freesoundId +
            // ", Pad: " + String(padInfo.padIndex) +
            // ", Query: " + padInfo.searchQuery + ")");
    }

    // Check if we loaded any samples
    if (padInfos.isEmpty())
    {
        // DBG("No valid samples found in slot " + String(slotIndex));
        return false;
    }

    // DBG("Successfully loaded " + String(padInfos.size()) + " samples from slot " + String(slotIndex));
    return true;
}

bool PresetManager::deletePreset(const File& presetFile)
{
    // DBG("Trying to delete preset: " + presetFile.getFullPathName());
    bool success = presetFile.deleteFile();

    if (success && presetFile == activePresetFile)
    {
        activePresetFile = File();
        activeSlotIndex = -1;
    }

    return success;
}

bool PresetManager::deleteSlot(const File& presetFile, int slotIndex)
{
    if (!presetFile.existsAsFile() || slotIndex < 0 || slotIndex >= MAX_SLOTS)
        return false;

    String jsonText = presetFile.loadFileAsString();
    var parsedJson = juce::JSON::parse(jsonText);

    if (!parsedJson.isObject())
        return false;

    auto* rootObject = parsedJson.getDynamicObject();
    if (!rootObject)
        return false;

    // Remove the slot
    rootObject->removeProperty(getSlotKey(slotIndex));

    // Always keep the preset file even if no slots remain
    String jsonString = juce::JSON::toString(parsedJson, true);
    bool success = presetFile.replaceWithText(jsonString);

    if (success && presetFile == activePresetFile && slotIndex == activeSlotIndex) {
        activePresetFile = File();
        activeSlotIndex = -1;
    }

    return success;
}

bool PresetManager::hasSlotData(const File& presetFile, int slotIndex)
{
    if (!presetFile.existsAsFile() || slotIndex < 0 || slotIndex >= MAX_SLOTS)
        return false;

    String jsonText = presetFile.loadFileAsString();
    var parsedJson = juce::JSON::parse(jsonText);

    if (!parsedJson.isObject())
        return false;

    return parsedJson.hasProperty(getSlotKey(slotIndex));
}

Array<PresetInfo> PresetManager::getAvailablePresets()
{
    Array<PresetInfo> presets;

    DirectoryIterator iter(presetsFolder, false, "*.json");

    while (iter.next())
    {
        File presetFile = iter.getFile();
        PresetInfo info = getPresetInfo(presetFile);

        if (info.name.isNotEmpty()) // Valid preset
        {
            presets.add(info);
        }
    }

    // Sort by creation date (newest first)
    std::sort(presets.begin(), presets.end(),
              [](const PresetInfo& a, const PresetInfo& b) {
                  return a.createdDate > b.createdDate;
              });

    return presets;
}

PresetInfo PresetManager::getPresetInfo(const File& presetFile)
{
    PresetInfo info;
    info.presetFile = presetFile;

    if (!presetFile.existsAsFile())
        return info;

    String jsonText = presetFile.loadFileAsString();
    var parsedJson = juce::JSON::parse(jsonText);

    if (!parsedJson.isObject())
        return info;

    var presetInfoVar = parsedJson.getProperty("preset_info", var());
    if (presetInfoVar.isObject())
    {
        info.name = presetInfoVar.getProperty("name", "");
        info.createdDate = presetInfoVar.getProperty("created_date", "");
        info.description = presetInfoVar.getProperty("description", "");
    }

    // Check if this is the active preset
    if (presetFile == activePresetFile)
    {
        info.activeSlot = activeSlotIndex;
    }

    // Load slot information
    for (int i = 0; i < MAX_SLOTS; ++i)
    {
        info.slots[i] = getSlotInfo(presetFile, i);
    }

    return info;
}

PresetSlotInfo PresetManager::getSlotInfo(const File& presetFile, int slotIndex)
{
    PresetSlotInfo slotInfo;

    if (!presetFile.existsAsFile() || slotIndex < 0 || slotIndex >= MAX_SLOTS)
        return slotInfo;

    String jsonText = presetFile.loadFileAsString();
    var parsedJson = juce::JSON::parse(jsonText);

    if (!parsedJson.isObject())
        return slotInfo;

    var slotData = parsedJson.getProperty(getSlotKey(slotIndex), var());
    if (!slotData.isObject())
        return slotInfo;

    slotInfo.hasData = true;

    var slotInfoVar = slotData.getProperty("slot_info", var());
    if (slotInfoVar.isObject())
    {
        slotInfo.name = slotInfoVar.getProperty("name", "Slot " + String(slotIndex + 1));
        slotInfo.createdDate = slotInfoVar.getProperty("created_date", "");
        slotInfo.searchQuery = slotInfoVar.getProperty("search_query", "");
        slotInfo.description = slotInfoVar.getProperty("description", "");
        slotInfo.sampleCount = slotInfoVar.getProperty("sample_count", 0);
    }

    return slotInfo;
}

bool PresetManager::sampleExists(const String& freesoundId) const
{
    String fileName = "FS_ID_" + freesoundId + ".ogg";
    return samplesFolder.getChildFile(fileName).existsAsFile();
}

File PresetManager::getSampleFile(const String& freesoundId) const
{
    if (freesoundId.isEmpty())
        return File();

    // Use the standard naming convention: FS_ID_XXXX.ogg
    String fileName = "FS_ID_" + freesoundId + ".ogg";
    File sampleFile = samplesFolder.getChildFile(fileName);

    // DBG("Looking for sample file: " + sampleFile.getFullPathName());
    return sampleFile;
}

String PresetManager::generatePresetName(const String& searchQuery) const
{
    Time currentTime = Time::getCurrentTime();
    String dateStr = currentTime.formatted("%Y%m%d_%H%M%S");

    String baseName = searchQuery.isEmpty() ? "Preset" : searchQuery;
    baseName = sanitizeFileName(baseName);

    return baseName + "_" + dateStr;
}

void PresetManager::cleanupUnusedSamples()
{
    // Get all preset files and collect referenced sample IDs
    StringArray referencedIds;

    Array<PresetInfo> presets = getAvailablePresets();
    for (const auto& presetInfo : presets)
    {
        // Check all slots
        for (int slotIndex = 0; slotIndex < MAX_SLOTS; ++slotIndex)
        {
            Array<PadInfo> padInfos;
            if (loadPreset(presetInfo.presetFile, slotIndex, padInfos))
            {
                for (const auto& padInfo : padInfos)
                {
                    if (padInfo.freesoundId.isNotEmpty())
                    {
                        referencedIds.addIfNotAlreadyThere(padInfo.freesoundId);
                    }
                }
            }
        }
    }

    // Scan samples folder and delete unreferenced files
    DirectoryIterator iter(samplesFolder, false, "FS_ID_*.ogg");

    while (iter.next())
    {
        File sampleFile = iter.getFile();
        String fileName = sampleFile.getFileNameWithoutExtension();

        // Extract ID from filename (FS_ID_12345 -> 12345)
        if (fileName.startsWith("FS_ID_"))
        {
            String freesoundId = fileName.substring(6);

            if (!referencedIds.contains(freesoundId))
            {
                sampleFile.deleteFile();
            }
        }
    }
}

void PresetManager::setActivePreset(const File& presetFile, int slotIndex)
{
    activePresetFile = presetFile;
    activeSlotIndex = slotIndex;
}

String PresetManager::sanitizeFileName(const String& input) const
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

    // Replace spaces with underscores
    cleaned = cleaned.replace(" ", "_");

    // Limit length
    if (cleaned.length() > 50)
        cleaned = cleaned.substring(0, 50);

    return cleaned;
}

String PresetManager::getSlotKey(int slotIndex) const
{
    return "slot_" + String(slotIndex);
}