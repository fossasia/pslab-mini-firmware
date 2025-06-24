target_sources(tinyusb
    PRIVATE
        usbc.c
)

target_include_directories(tinyusb
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
