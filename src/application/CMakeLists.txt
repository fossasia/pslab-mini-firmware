# Application layer build configuration
if(BUILD_TESTS)
    include(CMakeLists.tests.txt)
else()
    include(CMakeLists.target.txt)
endif()
