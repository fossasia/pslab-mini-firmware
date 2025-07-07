# Test build configuration

# Platform selection for tests
set(PLATFORM "mock" CACHE STRING "Target platform")
set_property(CACHE PLATFORM PROPERTY STRINGS "mock")

# Enable testing
include(CTest)
enable_testing()

# Add test and source subdirectories
add_subdirectory(tests)
add_subdirectory(src)

# Set up code quality targets (without ARM-specific linting)
setup_code_quality_targets()
