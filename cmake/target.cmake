# Target build configuration for STM32H563

# Platform selection
set(PLATFORM "h563xx" CACHE STRING "Target platform")
set_property(CACHE PLATFORM PROPERTY STRINGS "h563xx")

# Find required packages for target
find_package(CMSIS COMPONENTS STM32H563ZI REQUIRED)
find_package(HAL COMPONENTS STM32H5 REQUIRED)

# Add subdirectories
add_subdirectory(boot)
add_subdirectory(lib)
add_subdirectory(src)

# Set up code quality targets
setup_code_quality_targets()
