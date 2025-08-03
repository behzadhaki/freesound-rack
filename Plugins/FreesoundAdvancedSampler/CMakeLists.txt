project(FreesoundAdvancedSampler VERSION 0.0.1)

set (BaseTargetName FreesoundAdvancedSampler)

juce_add_plugin("${BaseTargetName}"
        # VERSION ...                               # Set this if the plugin version is different to the project version
        # ICON_BIG ...                              # ICON_* arguments specify a path to an image file to use as an icon for the Standalone
        # ICON_SMALL ...
        COMPANY_NAME "MusicTechnologyGroup"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD TRUE`
        PLUGIN_MANUFACTURER_CODE Juce
        PLUGIN_CODE FrAS
        FORMATS AU VST3 Standalone
        PRODUCT_NAME "Freesound Advanced Sampler")


target_sources(${BaseTargetName} PRIVATE
        ../../shared_plugin_helpers/shared_plugin_helpers.cpp
        ../../FreesoundAPI/FreesoundAPI.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/AudioDownloadManager.cpp
        Source/SampleGridComponent.cpp
        Source/PresetBrowserComponent.cpp
        Source/PresetManager.cpp
        Source/ExpandablePanel.cpp
        Source/MasterSearchPanel.cpp
        Source/BookmarkManager.cpp
        Source/BookmarkViewerComponent.cpp
)

target_compile_definitions(${BaseTargetName}
        PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(${BaseTargetName} PRIVATE
        shared_plugin_helpers
        juce_recommended_config_flags
        juce_recommended_lto_flags
        juce_recommended_warning_flags)


# Ensure the directory is included so the file can be found by your code
include_directories(Source)

