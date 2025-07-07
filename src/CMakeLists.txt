# Source directory build delegation
#
# This file delegates to the appropriate build configuration
# based on whether we're building for target or tests

if(BUILD_TESTS)
    include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.tests.txt)
else()
    include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.target.txt)
endif()
