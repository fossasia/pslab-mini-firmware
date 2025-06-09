add_library(tiny-usb)

add_subdirectory(cfg)
add_subdirectory(port)
add_subdirectory(src)

target_include_directories(tiny-usb
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(tiny-usb
    PRIVATE
        USE_HAL_DRIVER
        USE_FULL_LL_DRIVER
        STM32H563xx
)

target_link_libraries(tiny-usb
    PRIVATE
        HAL::STM32::H5::LL_ICACHE
        # Target family to avoid inclusion of startup code and linker scripts.
        CMSIS::STM32::H5
)
