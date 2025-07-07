# Platform abstraction layer - only built for target

# Create platform library
add_library(pslab-platform STATIC)

# Include the selected platform subdirectory
if(PLATFORM STREQUAL "h563xx")
    add_subdirectory(h563xx)
else()
    message(FATAL_ERROR "Unsupported platform: ${PLATFORM}")
endif()

target_include_directories(pslab-platform
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
