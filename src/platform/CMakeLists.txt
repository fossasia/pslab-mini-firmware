# Platform abstraction layer - only built for target

# Create platform library
add_library(pslab-platform STATIC)

target_link_libraries(pslab-platform
    PRIVATE
        pslab-util
)

# Include the selected platform subdirectory
if(PLATFORM STREQUAL "h563xx")
    add_subdirectory(h563xx)
else()
    message(FATAL_ERROR "Unsupported platform: ${PLATFORM}")
endif()

target_include_directories(pslab-platform
    INTERFACE
        # Only expose platform headers, not the entire src directory
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        # Include parent directory for internal use only
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}
)
