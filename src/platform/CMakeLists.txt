# Create platform library
add_library(pslab-platform STATIC)
set_property(TARGET pslab-platform PROPERTY C_STANDARD 23)


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
