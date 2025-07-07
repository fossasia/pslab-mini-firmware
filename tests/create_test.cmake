function(_unity_set_variables)
  set(_UNITY_CWD ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
  set(_UNITY_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/config.yml PARENT_SCOPE)
  set(_UNITY_RUNNER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/runners PARENT_SCOPE)
  set(
    _UNITY_RUNNER_GENERATOR
    ${CMAKE_CURRENT_SOURCE_DIR}/unity/auto/generate_test_runner.rb
    PARENT_SCOPE
  )
endfunction()

# Creates a unity test
function(unity_add_test TEST_TARGET TEST_SRC)
  message(CHECK_START "[TEST] Registering unity test ${TEST_TARGET}")
  _unity_set_variables()

  if(NOT EXISTS "${_UNITY_CONFIG}")
    message(FATAL_ERROR "${_UNITY_CONFIG} does not exist!")
  endif()

  file(MAKE_DIRECTORY ${_UNITY_RUNNER_DIR})

  get_filename_component(UNITY_TEST_C ${TEST_SRC} REALPATH)

  add_custom_command(
    OUTPUT ${_UNITY_RUNNER_DIR}/${TEST_TARGET}_runner.c
    COMMAND
      ruby ${_UNITY_RUNNER_GENERATOR} ${_UNITY_CONFIG} ${UNITY_TEST_C}
      ${_UNITY_RUNNER_DIR}/${TEST_TARGET}_runner.c
    DEPENDS ${TEST_SRC}
    WORKING_DIRECTORY ${_UNITY_RUNNER_DIR}
  )

  add_executable(
    ${TEST_TARGET}
    ${TEST_SRC}
    ${_UNITY_RUNNER_DIR}/${TEST_TARGET}_runner.c
  )
  target_link_libraries(${TEST_TARGET} unity ${ARGN})
  add_test(${TEST_TARGET} ${TEST_TARGET})

  message(CHECK_PASS "registered target ${TEST_TARGET}")
endfunction()

# INTERNAL
# Defines useful variables for cmock functions
# requires: work on "tests" directory
function(_cmock_set_variables)
  set(_CMOCK_CWD ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
  set(_CMOCK_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/config.yml PARENT_SCOPE)
  set(_CMOCK_MOCK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mocks PARENT_SCOPE)
  set(
    _CMOCK_RUBY_SCRIPT
    ${CMAKE_CURRENT_SOURCE_DIR}/cmock/lib/cmock.rb
    PARENT_SCOPE
  )
endfunction()

# Generates a mock library based on a module's header file
# arguments:
#   MOCK_NAME   target name
#   HEADER      used to generate the mock
# requires: a file called config.yml on the tests directory
function(cmock_generate_mock MOCK_NAME HEADER)
  message(CHECK_START "[TEST] Generating mock library of ${HEADER}")
  _cmock_set_variables()

  if(NOT EXISTS "${_CMOCK_CONFIG}")
    message(FATAL_ERROR "${_CMOCK_CONFIG} does not exist!")
  endif()

  file(MAKE_DIRECTORY ${_CMOCK_MOCK_DIR})

  add_custom_command(
    OUTPUT ${_CMOCK_MOCK_DIR}/${MOCK_NAME}.c ${_CMOCK_MOCK_DIR}/${MOCK_NAME}.h
    COMMAND ruby ${_CMOCK_RUBY_SCRIPT} -o${_CMOCK_CONFIG} ${HEADER}
    WORKING_DIRECTORY ${_CMOCK_CWD}
    DEPENDS ${HEADER}
  )

  get_filename_component(CMOCK_HEADER_DIR ${HEADER} DIRECTORY)

  add_library(
    ${MOCK_NAME}
    ${_CMOCK_MOCK_DIR}/${MOCK_NAME}.c
    ${_CMOCK_MOCK_DIR}/${MOCK_NAME}.h
  )
  target_link_libraries(${MOCK_NAME} unity cmock)
  target_include_directories(
    ${MOCK_NAME}
    PUBLIC ${CMOCK_HEADER_DIR} ${_CMOCK_MOCK_DIR}
  )
  message(CHECK_PASS "generated target " ${MOCK_NAME})
endfunction()

# Creates a unity test linked with cmock generated mocks
function(cmock_add_test TEST_TARGET TEST_SRC)
  message(CHECK_START "[TEST] Registering cmock test ${TEST_TARGET}")

  unity_add_test(${TEST_TARGET} ${TEST_SRC} cmock ${ARGN})

  message(CHECK_PASS "Target ${TEST_TARGET} linked with CMock")
endfunction()
