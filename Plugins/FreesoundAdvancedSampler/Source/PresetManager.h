/*
  ==============================================================================

    PresetManager.h
    Created: New preset management system
    Author: Generated

  ==============================================================================
*/

#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "FreesoundAPI/FreesoundAPI.h"

struct PresetInfo
{
    String name;
    String createdDate;
    String searchQuery;
    String description;
    int sampleCount;
    File presetFile;

    PresetInfo() : sampleCount(0) {}
};

struct PadInfo
{
    int padIndex;
    String freesoundId;
    String fileName;
    String originalName;
    String author;
    String license;
    double duration;
    int fileSize;
    String downloadedAt;

    PadInfo() : padIndex(-1), duration(0.0), fileSize(0) {}
};

class PresetManager
{
public:
    PresetManager(const File& baseDirectory);
    ~PresetManager();

    // Preset operations
    bool saveCurrentPreset(const String& name, const String& description,
                          const Array<PadInfo>& padInfos, const String& searchQuery);
    bool loadPreset(const File& presetFile, Array<PadInfo>& outPadInfos);
    bool deletePreset(const File& presetFile);

    // Preset discovery
    Array<PresetInfo> getAvailablePresets();
    PresetInfo getPresetInfo(const File& presetFile);

    // File management
    File getSamplesFolder() const { return samplesFolder; }
    File getPresetsFolder() const { return presetsFolder; }
    bool sampleExists(const String& freesoundId) const;
    File getSampleFile(const String& freesoundId) const;

    // Utilities
    String generatePresetName(const String& searchQuery) const;
    void cleanupUnusedSamples(); // Remove samples not referenced by any preset

private:
    File baseDirectory;
    File samplesFolder;
    File presetsFolder;

    void ensureDirectoriesExist();
    String sanitizeFileName(const String& input) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)
};