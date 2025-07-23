# -------------------------------------
# Freesound API Configuration Handling
# -------------------------------------

# Find all subdirectories that contain a Source folder
file(GLOB SUBDIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)
set(PROJECT_DIRS "")
set(FREESOUND_HEADERS "")

foreach(SUBDIR ${SUBDIRS})
    set(FULL_SUBDIR_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}")
    if(IS_DIRECTORY ${FULL_SUBDIR_PATH})
        set(SOURCE_PATH "${FULL_SUBDIR_PATH}/Source")
        if(EXISTS ${SOURCE_PATH} AND IS_DIRECTORY ${SOURCE_PATH})
            list(APPEND PROJECT_DIRS ${SUBDIR})
            list(APPEND FREESOUND_HEADERS "${SOURCE_PATH}/FreesoundKeys.h")
        endif()
    endif()
endforeach()

# Display found projects
if(PROJECT_DIRS)
    message(STATUS "Found projects with Source directories:")
    foreach(PROJECT ${PROJECT_DIRS})
        message(STATUS "  - ${PROJECT}")
    endforeach()
else()
    message(WARNING "No subdirectories with Source folders found!")
endif()

# Check if FreesoundKeys.h exists in all subdirectories and contains valid keys
set(NEED_API_SETUP FALSE)
set(MISSING_OR_INVALID_PROJECTS "")

foreach(HEADER_PATH ${FREESOUND_HEADERS})
    # Get the project name from the path
    get_filename_component(SOURCE_DIR ${HEADER_PATH} DIRECTORY)
    get_filename_component(PROJECT_DIR ${SOURCE_DIR} DIRECTORY)
    get_filename_component(PROJECT_NAME ${PROJECT_DIR} NAME)

    if(NOT EXISTS ${HEADER_PATH})
        set(NEED_API_SETUP TRUE)
        list(APPEND MISSING_OR_INVALID_PROJECTS ${PROJECT_NAME})
        message(STATUS "Missing: FreesoundKeys.h in ${PROJECT_NAME}/Source/")
    else()
        file(READ ${HEADER_PATH} KEYS_CONTENT)
        if(KEYS_CONTENT MATCHES "INSERT_API_KEY_HERE" OR KEYS_CONTENT MATCHES "INSERT_CLIENT_ID_HERE")
            set(NEED_API_SETUP TRUE)
            list(APPEND MISSING_OR_INVALID_PROJECTS ${PROJECT_NAME})
            message(STATUS "Invalid placeholders in: ${PROJECT_NAME}/Source/FreesoundKeys.h")
        else()
            message(STATUS "✓ Valid FreesoundKeys.h found in ${PROJECT_NAME}")
        endif()
    endif()
endforeach()

# Only proceed if we have valid API credentials or can get them
if(NEED_API_SETUP)
    message(STATUS "\n=== Freesound API Configuration Required ===")
    if(MISSING_OR_INVALID_PROJECTS)
        message(STATUS "FreesoundKeys.h missing or contains placeholder values in:")
        foreach(PROJECT ${MISSING_OR_INVALID_PROJECTS})
            message(STATUS "  - ${PROJECT}")
        endforeach()
    endif()
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

    # Generate the FreesoundKeys.h file content
    set(KEYS_HEADER_CONTENT
            "#pragma once\n\n"
            "#define FREESOUND_API_KEY \"${FREESOUND_API_KEY}\"\n"
            "#define FREESOUND_CLIENT_ID \"${FREESOUND_CLIENT_ID}\"\n")

    # Write to all project Source directories
    set(UPDATED_PROJECTS "")
    foreach(HEADER_PATH ${FREESOUND_HEADERS})
        # Ensure the Source directory exists
        get_filename_component(SOURCE_DIR ${HEADER_PATH} DIRECTORY)
        get_filename_component(PROJECT_DIR ${SOURCE_DIR} DIRECTORY)
        get_filename_component(PROJECT_NAME ${PROJECT_DIR} NAME)

        file(MAKE_DIRECTORY ${SOURCE_DIR})
        file(WRITE ${HEADER_PATH} ${KEYS_HEADER_CONTENT})
        list(APPEND UPDATED_PROJECTS ${PROJECT_NAME})
    endforeach()

    message(STATUS "✓ FreesoundKeys.h has been generated successfully in all projects!")
    foreach(PROJECT ${UPDATED_PROJECTS})
        message(STATUS "  - ${PROJECT}/Source/FreesoundKeys.h")
    endforeach()
    message(STATUS "✓ API Key: ${FREESOUND_API_KEY}")
    message(STATUS "✓ Client ID: ${FREESOUND_CLIENT_ID}")
    message(STATUS "")
else()
    message(STATUS "✓ FreesoundKeys.h found with valid configuration in all projects")
endif()

# -------------------------------------
# Add Subdirectories
# -------------------------------------

# Add all found project directories
foreach(PROJECT_DIR ${PROJECT_DIRS})
    add_subdirectory(${PROJECT_DIR})
    message(STATUS "Added subdirectory: ${PROJECT_DIR}")
endforeach()