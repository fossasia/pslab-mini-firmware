# System layer build delegation

if(BUILD_TESTS)
    include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.tests.txt)
else()
    include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.target.txt)
endif()
