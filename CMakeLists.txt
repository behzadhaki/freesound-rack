

cmake_minimum_required(VERSION 3.16)


project(FreeSoundJuce)

#First, we'll add the CMake folder, incase we'll need to find_package later:
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

#Compile commands, useful for some IDEs like VS-Code
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Platform-specific settings
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14" CACHE STRING "Minimum OS X deployment version" FORCE)
    option(UniversalBinary "Build universal binary for mac" OFF)
    if (UniversalBinary)
        message("\n\n\n --- \n (MacOs) Universal Binary \n --- \n\n\n")
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE INTERNAL "")
    endif()

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
        message("\n\n\n --- \n (MacOs)  ARM64 \n --- \n\n\n")
        set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE INTERNAL "")
    else()
        message("\n\n\n --- \n (MacOs) Intel \n --- \n\n\n")
        set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE INTERNAL "")
    endif()

elseif(MSVC)
    message("\n\n\n --- \n Windows \n --- \n\n\n")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
elseif(UNIX AND NOT APPLE) # For Linux
    # make sure the following packages are installed:
    # https://github.com/juce-framework/JUCE/blob/master/docs/Linux%20Dependencies.md
    # udo apt install libasound2-dev libjack-jackd2-dev \
    #    ladspa-sdk \
    #    libcurl4-openssl-dev  \
    #    libfreetype6-dev \
    #    libx11-dev libxcomposite-dev libxcursor-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev \
    #    libwebkit2gtk-4.0-dev \
    #    libglu1-mesa-dev mesa-common-dev
    # Print a message notifying about above packages
    message("\n\n\n --- \n Linux \n --- \n\n\n")
    message(STATUS "Make sure the following packages are installed: see https://github.com/juce-framework/JUCE/blob/master/docs/Linux%20Dependencies.md")
    message(STATUS "libasound2-dev libjack-jackd2-dev ladspa-sdk libcurl4-openssl-dev libfreetype6-dev libx11-dev libxcomposite-dev libxcursor-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev libwebkit2gtk-4.0-dev libglu1-mesa-dev mesa-common-dev")

endif()


#static linking in Windows
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

#We 'require' that we need juce. If JUCE isn't found, it will revert to what's in
#CMake/Findjuce.cmake, where you can see how JUCE is brought in/configured
find_package(juce REQUIRED)
juce_add_modules(shared_plugin_helpers)

# Juce shared plugin helpers
add_subdirectory(FreesoundAPI)

add_subdirectory(Plugins)
