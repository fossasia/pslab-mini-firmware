add_library(tinyusb)

target_compile_definitions(tinyusb
    PRIVATE
        STM32H563xx
)

target_link_libraries(tinyusb
    CMSIS::STM32::H5
)

add_subdirectory(src)
