/*
  ==============================================================================

    PresetManager.cpp
    Created: New preset management system
    Author: Generated

  ==============================================================================
*/

#include "PresetManager.h"

PresetManager::PresetManager(const File& baseDirectory)
    : baseDirectory(baseDirectory)
    , samplesFolder(baseDirectory.getChildFile("samples"))
    , presetsFolder(baseDirectory.getChildFile("presets"))
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
                                     const Array<PadInfo>& padInfos, const String& searchQuery)
{
    if (name.isEmpty())
        return false;

    // Generate filename
    String fileName = sanitizeFileName(name) + ".json";
    File presetFile = presetsFolder.getChildFile(fileName);

    // Create JSON structure
    juce::DynamicObject::Ptr root = new juce::DynamicObject();

    // Preset info
    juce::DynamicObject::Ptr presetInfo = new juce::DynamicObject();
    presetInfo->setProperty("name", name);
    presetInfo->setProperty("created_date", juce::Time::getCurrentTime().toString(true, true));
    presetInfo->setProperty("search_query", searchQuery);
    presetInfo->setProperty("description", description);
    presetInfo->setProperty("sample_count", padInfos.size());

    root->setProperty("preset_info", juce::var(presetInfo.get()));

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

    root->setProperty("pad_mapping", padMapping);

    // Write to file
    String jsonString = juce::JSON::toString(juce::var(root.get()), true);
    return presetFile.replaceWithText(jsonString);
}

bool PresetManager::loadPreset(const File& presetFile, Array<PadInfo>& outPadInfos)
{
    outPadInfos.clear();

    if (!presetFile.existsAsFile())
        return false;

    String jsonText = presetFile.loadFileAsString();
    var parsedJson = juce::JSON::parse(jsonText);

    if (!parsedJson.isObject())
        return false;

    var padMapping = parsedJson.getProperty("pad_mapping", var());
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

    return true;
}

bool PresetManager::deletePreset(const File& presetFile)
{
    return presetFile.deleteFile();
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
        info.searchQuery = presetInfoVar.getProperty("search_query", "");
        info.description = presetInfoVar.getProperty("description", "");
        info.sampleCount = presetInfoVar.getProperty("sample_count", 0);
    }

    return info;
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
        Array<PadInfo> padInfos;
        if (loadPreset(presetInfo.presetFile, padInfos))
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