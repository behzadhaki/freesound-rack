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
    if (name.isEmpty() || slotIndex < 0 || slotIndex >= 8)
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
    if (!presetFile.existsAsFile() || slotIndex < 0 || slotIndex >= 8)
        return false;

    // Load existing preset data
    String existingJson = presetFile.loadFileAsString();
    var parsedJson = juce::JSON::parse(existingJson);

    if (!parsedJson.isObject())
        return false;

    auto* rootObject = parsedJson.getDynamicObject();
    if (!rootObject)
        return false;

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

    // Update slot data
    rootObject->setProperty(getSlotKey(slotIndex), juce::var(slotData.get()));

    // Write back to file
    String jsonString = juce::JSON::toString(parsedJson, true);
    bool success = presetFile.replaceWithText(jsonString);

    if (success)
    {
        setActivePreset(presetFile, slotIndex);
    }

    return success;
}

bool PresetManager::loadPreset(const File& presetFile, int slotIndex, Array<PadInfo>& outPadInfos)
{
    outPadInfos.clear();

    if (!presetFile.existsAsFile() || slotIndex < 0 || slotIndex >= 8)
        return false;

    String jsonText = presetFile.loadFileAsString();
    var parsedJson = juce::JSON::parse(jsonText);

    if (!parsedJson.isObject())
        return false;

    var slotData = parsedJson.getProperty(getSlotKey(slotIndex), var());
    if (!slotData.isObject())
        return false;

    var padMapping = slotData.getProperty("pad_mapping", var());
    if (!padMapping.isArray())
        return false;

    for (int i = 0; i < padMapping.size(); ++i)
    {
        var pad = padMapping[i];
        if (!pad.isObject())
            continue;

        PadInfo padInfo;
        padInfo.padIndex = pad.getProperty("pad_index", -1);
        padInfo.freesoundId = pad.getProperty("freesound_id", "");
        padInfo.fileName = pad.getProperty("file_name", "");
        padInfo.originalName = pad.getProperty("original_name", "");
        padInfo.author = pad.getProperty("author", "");
        padInfo.license = pad.getProperty("license", "");
        padInfo.duration = pad.getProperty("duration", 0.0);
        padInfo.fileSize = pad.getProperty("file_size", 0);
        padInfo.downloadedAt = pad.getProperty("downloaded_at", "");

        outPadInfos.add(padInfo);
    }

    setActivePreset(presetFile, slotIndex);
    return true;
}

bool PresetManager::deletePreset(const File& presetFile)
{
    DBG("Trying to delete preset: " + presetFile.getFullPathName());
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
    if (!presetFile.existsAsFile() || slotIndex < 0 || slotIndex >= 8)
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

    // Check if any slots remain
    bool hasAnySlots = false;
    for (int i = 0; i < 8; ++i)
    {
        if (rootObject->hasProperty(getSlotKey(i)))
        {
            hasAnySlots = true;
            break;
        }
    }

    // If no slots remain, delete the entire preset file
    if (!hasAnySlots)
    {
        return deletePreset(presetFile);
    }

    // Write back to file
    String jsonString = juce::JSON::toString(parsedJson, true);
    bool success = presetFile.replaceWithText(jsonString);

    if (success && presetFile == activePresetFile && slotIndex == activeSlotIndex)
    {
        activePresetFile = File();
        activeSlotIndex = -1;
    }

    return success;
}

bool PresetManager::hasSlotData(const File& presetFile, int slotIndex)
{
    if (!presetFile.existsAsFile() || slotIndex < 0 || slotIndex >= 8)
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
    for (int i = 0; i < 8; ++i)
    {
        info.slots[i] = getSlotInfo(presetFile, i);
    }

    return info;
}

PresetSlotInfo PresetManager::getSlotInfo(const File& presetFile, int slotIndex)
{
    PresetSlotInfo slotInfo;

    if (!presetFile.existsAsFile() || slotIndex < 0 || slotIndex >= 8)
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
    String fileName = "FS_ID_" + freesoundId + ".ogg";
    return samplesFolder.getChildFile(fileName);
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
        for (int slotIndex = 0; slotIndex < 8; ++slotIndex)
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