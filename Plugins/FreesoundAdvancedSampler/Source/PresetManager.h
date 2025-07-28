/*
  ==============================================================================

    PresetManager.h
    Created: New preset management system with multi-slot support
    Author: Generated

  ==============================================================================
*/

#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "FreesoundAPI/FreesoundAPI.h"

struct PresetSlotInfo
{
    String name;
    String createdDate;
    String searchQuery;
    String description;
    int sampleCount;
    bool hasData;

    PresetSlotInfo() : sampleCount(0), hasData(false) {}
};

struct PresetInfo
{
    String name;
    String createdDate;
    String description;
    File presetFile;
    std::array<PresetSlotInfo, 8> slots;
    int activeSlot; // Currently loaded slot (0-7, -1 if none)

    PresetInfo() : activeSlot(-1) {}
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

    // Preset operations with slot support
    bool saveCurrentPreset(const String& name, const String& description,
                          const Array<PadInfo>& padInfos, const String& searchQuery,
                          int slotIndex = 0); // Default to slot 0
    bool saveToSlot(const File& presetFile, int slotIndex, const String& description,
                   const Array<PadInfo>& padInfos, const String& searchQuery);
    bool loadPreset(const File& presetFile, int slotIndex, Array<PadInfo>& outPadInfos);
    bool deletePreset(const File& presetFile);
    bool deleteSlot(const File& presetFile, int slotIndex);
    bool hasSlotData(const File& presetFile, int slotIndex);

    // Preset discovery
    Array<PresetInfo> getAvailablePresets();
    PresetInfo getPresetInfo(const File& presetFile);
    PresetSlotInfo getSlotInfo(const File& presetFile, int slotIndex);

    // File management
    File getSamplesFolder() const { return samplesFolder; }
    File getPresetsFolder() const { return presetsFolder; }
    bool sampleExists(const String& freesoundId) const;
    File getSampleFile(const String& freesoundId) const;

    // Utilities
    String generatePresetName(const String& searchQuery) const;
    void cleanupUnusedSamples(); // Remove samples not referenced by any preset

    // Active preset tracking
    void setActivePreset(const File& presetFile, int slotIndex);
    File getActivePresetFile() const { return activePresetFile; }
    int getActiveSlotIndex() const { return activeSlotIndex; }
    String sanitizeFileName(const String& input) const;

private:
    File baseDirectory;
    File samplesFolder;
    File presetsFolder;

    // Active preset tracking
    File activePresetFile;
    int activeSlotIndex;

    void ensureDirectoriesExist();
    String getSlotKey(int slotIndex) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)
};