# -------------------------------------
# Freesound API Configuration Handling
# -------------------------------------

# Define paths for FreesoundKeys.h in both subdirectories
set(FREESOUND_UPLOADER_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/FreesoundUploader/Source/FreesoundKeys.h")
set(FREESOUND_SAMPLER_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/FreesoundSimpleSampler/Source/FreesoundKeys.h")

# Check if FreesoundKeys.h exists in either subdirectory and contains valid keys
set(NEED_API_SETUP FALSE)

# Check FreesoundUploader
if(NOT EXISTS ${FREESOUND_UPLOADER_HEADER})
    set(NEED_API_SETUP TRUE)
else()
    file(READ ${FREESOUND_UPLOADER_HEADER} UPLOADER_KEYS_CONTENT)
    if(UPLOADER_KEYS_CONTENT MATCHES "INSERT_API_KEY_HERE" OR UPLOADER_KEYS_CONTENT MATCHES "INSERT_CLIENT_ID_HERE")
        set(NEED_API_SETUP TRUE)
    endif()
endif()

# Check FreesoundSimpleSampler
if(NOT EXISTS ${FREESOUND_SAMPLER_HEADER})
    set(NEED_API_SETUP TRUE)
else()
    file(READ ${FREESOUND_SAMPLER_HEADER} SAMPLER_KEYS_CONTENT)
    if(SAMPLER_KEYS_CONTENT MATCHES "INSERT_API_KEY_HERE" OR SAMPLER_KEYS_CONTENT MATCHES "INSERT_CLIENT_ID_HERE")
        set(NEED_API_SETUP TRUE)
    endif()
endif()

# Only proceed if we have valid API credentials or can get them
if(NEED_API_SETUP)
    message(STATUS "\n=== Freesound API Configuration Required ===")
    message(STATUS "FreesoundKeys.h not found or contains placeholder values in one or both projects.")
    message(STATUS "")
    message(STATUS "To use these plugins, you need Freesound API credentials.")
    message(STATUS "Get them at: https://freesound.org/apiv2/apply/")
    message(STATUS "")

    # Check if credentials are provided via command line
    if((NOT DEFINED FREESOUND_API_KEY OR FREESOUND_API_KEY STREQUAL "") OR
    (NOT DEFINED FREESOUND_CLIENT_ID OR FREESOUND_CLIENT_ID STREQUAL ""))

        message(FATAL_ERROR
                "\nCredentials required! Please run:\n"
                "cmake -DFREESOUND_API_KEY=your_api_key -DFREESOUND_CLIENT_ID=your_client_id .\n"
                "\nGet your credentials at: https://freesound.org/apiv2/apply/")
    endif()

    # Validate credentials
    if(FREESOUND_API_KEY STREQUAL "" OR FREESOUND_CLIENT_ID STREQUAL "")
        message(FATAL_ERROR "\nEmpty credentials provided. Build aborted.")
    endif()

    # Create the Source directories if they don't exist
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/FreesoundUploader/Source)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/FreesoundSimpleSampler/Source)

    # Generate the FreesoundKeys.h file content
    set(KEYS_HEADER_CONTENT
            "#pragma once\n\n"
            "#define FREESOUND_API_KEY \"${FREESOUND_API_KEY}\"\n"
            "#define FREESOUND_CLIENT_ID \"${FREESOUND_CLIENT_ID}\"\n")

    # Write to both subdirectories
    file(WRITE ${FREESOUND_UPLOADER_HEADER} ${KEYS_HEADER_CONTENT})
    file(WRITE ${FREESOUND_SAMPLER_HEADER} ${KEYS_HEADER_CONTENT})

    message(STATUS "✓ FreesoundKeys.h has been generated successfully in both projects!")
    message(STATUS "  - FreesoundUploader/Source/FreesoundKeys.h")
    message(STATUS "  - FreesoundSimpleSampler/Source/FreesoundKeys.h")
    message(STATUS "✓ API Key: ${FREESOUND_API_KEY}")
    message(STATUS "✓ Client ID: ${FREESOUND_CLIENT_ID}")
    message(STATUS "")
else()
    message(STATUS "✓ FreesoundKeys.h found with valid configuration in both projects")
endif()

# -------------------------------------
# Add Subdirectories
# -------------------------------------

add_subdirectory(FreesoundUploader)
add_subdirectory(FreesoundSimpleSampler)