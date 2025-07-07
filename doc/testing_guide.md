# Writing Tests for PSLab Mini Firmware

This document explains how to write unit tests for the PSLab Mini firmware using the Unity testing framework and CMock for mocking dependencies.

## Overview

The test infrastructure uses:
- **Unity**: A lightweight C testing framework
- **CMock**: Automatic mock generation for C functions
- **CTest**: CMake's testing framework for running and managing tests

## Getting Started

### Building and Running Tests

```bash
# Create build directory for tests
mkdir build-tests && cd build-tests

# Configure for test build
cmake -DBUILD_TESTS=ON ..

# Build tests
make

# Run all tests
ctest

# Run tests with verbose output
ctest -VV

# Run a specific test
./tests/test_uart
```

## Test File Structure

### Basic Test File Template

Create a new test file following this pattern:

```c
#include "unity.h"
#include "mock_dependency.h"  // Include mocks for dependencies
#include "module_under_test.h"
#include <stdlib.h>
#include <stdbool.h>

// Define missing CMock global symbols (required for current setup)
int GlobalExpectCount = 0;
int GlobalVerifyOrder = 0;

// Test fixtures - global variables used across tests
static SomeBuffer test_buffer;
static uint8_t test_data[256];

void setUp(void)
{
    // Initialize test fixtures before each test
    buffer_init(&test_buffer, test_data, 256);
    
    // Initialize mocks
    mock_dependency_Init();
}

void tearDown(void)
{
    // Clean up after each test
    // Note: mock_dependency_Destroy() automatically verifies all expectations
    mock_dependency_Destroy();
}

void test_function_success(void)
{
    // Arrange - Set up test data and expectations
    mock_dependency_function_Expect(PARAM1, PARAM2);
    
    // Act - Call the function under test
    int result = function_under_test(PARAM1, PARAM2);
    
    // Assert - Verify the results
    TEST_ASSERT_EQUAL(EXPECTED_VALUE, result);
}

void test_function_error_handling(void)
{
    // Test error conditions
    int result = function_under_test(NULL, 0);
    TEST_ASSERT_EQUAL(-1, result);
}
```

### Adding Tests to CMake

Add your test to `tests/CMakeLists.txt`:

```cmake
# Generate mocks for dependencies
cmock_generate_mock(mock_dependency ${CMAKE_CURRENT_SOURCE_DIR}/../src/path/to/dependency.h)

# Add test
cmock_add_test(test_module test_module.c mock_dependency)
target_sources(test_module PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/path/to/module.c)
target_include_directories(test_module PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/path/to/)
```

## Writing Effective Tests

### Test Naming Conventions

Use descriptive test names that follow this pattern:
- `test_[function_name]_[scenario]`
- Examples:
  - `test_UART_init_success`
  - `test_UART_init_null_buffer`
  - `test_UART_write_buffer_full`

### Test Structure (AAA Pattern)

Follow the Arrange-Act-Assert pattern:

```c
void test_example(void)
{
    // Arrange - Set up test data and mock expectations
    uint8_t input_data[] = {0x01, 0x02, 0x03};
    mock_hardware_function_Expect(PARAM1, PARAM2);
    mock_hardware_function_ExpectAndReturn(PARAM3, EXPECTED_RETURN);
    
    // Act - Call the function under test
    int result = function_under_test(input_data, sizeof(input_data));
    
    // Assert - Verify the results
    TEST_ASSERT_EQUAL(EXPECTED_RESULT, result);
    TEST_ASSERT_NOT_NULL(some_pointer);
}
```

### Common Unity Assertions

```c
// Basic equality
TEST_ASSERT_EQUAL(expected, actual);
TEST_ASSERT_NOT_EQUAL(expected, actual);

// Pointers
TEST_ASSERT_NULL(pointer);
TEST_ASSERT_NOT_NULL(pointer);

// Boolean
TEST_ASSERT_TRUE(condition);
TEST_ASSERT_FALSE(condition);

// Numeric comparisons
TEST_ASSERT_GREATER_THAN(threshold, actual);
TEST_ASSERT_LESS_THAN(threshold, actual);
TEST_ASSERT_GREATER_OR_EQUAL(threshold, actual);

// Arrays and memory
TEST_ASSERT_EQUAL_MEMORY(expected, actual, length);
TEST_ASSERT_EQUAL_STRING(expected, actual);

// Floating point (if enabled)
TEST_ASSERT_FLOAT_WITHIN(delta, expected, actual);
```

## Working with CMock

### Mock Expectations

CMock generates various expectation functions for each mocked function:

```c
// Exact parameter matching
mock_function_Expect(param1, param2);

// Return value expectations
mock_function_ExpectAndReturn(param1, param2, return_value);

// Ignore all parameters
mock_function_Ignore();

// Ignore specific parameters
mock_function_ExpectAnyArgs();

// Multiple calls
mock_function_Expect(param1, param2);
mock_function_Expect(param1, param2);  // Expected to be called twice
```

### Mock Verification

Mock verification happens automatically, but you can also verify explicitly:

```c
void test_function_calls_dependency(void)
{
    // Arrange
    mock_dependency_function_Expect(PARAM1);
    
    // Act
    function_under_test();
    
    // Assert - Verification happens automatically in tearDown()
    // Or verify explicitly:
    mock_dependency_Verify();
}
```

### Handling Callbacks and Function Pointers

For functions that take callback parameters, use `Ignore()`:

```c
// If function signature is: void set_callback(callback_t callback)
mock_set_callback_Ignore();  // Don't care about the callback parameter

// Or expect specific callback (if you can reference it):
mock_set_callback_Expect(my_specific_callback);
```

## Best Practices

### 1. Test One Thing at a Time

Each test should verify one specific behavior:

```c
// Good - tests specific error condition
void test_UART_init_null_rx_buffer(void)
{
    UART_Handle *handle = UART_init(0, NULL, &tx_buffer);
    TEST_ASSERT_NULL(handle);
}

// Bad - tests multiple things
void test_UART_init_all_error_conditions(void)
{
    // Tests multiple error conditions in one test
}
```

### 2. Use Descriptive Test Data

```c
// Good - clear test data
uint8_t test_message[] = {0xAA, 0xBB, 0xCC, 0xDD};
uint32_t expected_length = 4;

// Less clear
uint8_t data[] = {1, 2, 3};
```

### 3. Test Error Conditions

Always test error handling:

```c
void test_function_handles_null_pointer(void)
{
    int result = function_under_test(NULL);
    TEST_ASSERT_EQUAL(ERROR_INVALID_PARAM, result);
}

void test_function_handles_invalid_size(void)
{
    int result = function_under_test(buffer, 0);
    TEST_ASSERT_EQUAL(ERROR_INVALID_SIZE, result);
}
```

### 4. Keep Tests Independent

Each test should be able to run independently:

```c
void setUp(void)
{
    // Reset state before each test
    buffer_reset(&test_buffer);
    mock_dependency_Init();
}
```

### 5. Use Fixtures for Common Setup

```c
// Common test data used across multiple tests
static struct {
    uint8_t data[256];
    size_t length;
    UART_Handle *handle;
} test_fixture;

void setUp(void)
{
    // Initialize common test data
    test_fixture.length = 10;
    memset(test_fixture.data, 0xAA, test_fixture.length);
    // ... setup handle
}
```

## Testing Patterns

### Testing State Machines

```c
void test_state_machine_transitions(void)
{
    // Test initial state
    TEST_ASSERT_EQUAL(STATE_IDLE, get_current_state());
    
    // Test transition
    trigger_event(EVENT_START);
    TEST_ASSERT_EQUAL(STATE_RUNNING, get_current_state());
    
    // Test invalid transition
    trigger_event(EVENT_INVALID);
    TEST_ASSERT_EQUAL(STATE_RUNNING, get_current_state()); // Should stay in same state
}
```

### Testing Circular Buffers

```c
void test_circular_buffer_wrap_around(void)
{
    // Fill buffer to near capacity
    for (int i = 0; i < buffer_size - 1; i++) {
        buffer_put(&test_buffer, i);
    }
    
    // Test wrap-around behavior
    buffer_put(&test_buffer, 0xFF);
    uint8_t value;
    buffer_get(&test_buffer, &value);
    TEST_ASSERT_EQUAL(0, value);  // First value put should be first out
}
```

### Testing Interrupt Handlers

```c
void test_interrupt_handler_sets_flag(void)
{
    // Arrange
    interrupt_flag = false;
    
    // Act - simulate interrupt
    uart_interrupt_handler();
    
    // Assert
    TEST_ASSERT_TRUE(interrupt_flag);
}
```

## Example: Complete Test File

Here's the UART driver test as a complete example:

```c
#include "unity.h"
#include "mock_uart_ll.h"
#include "uart.h"
#include <stdlib.h>
#include <stdbool.h>

// Required CMock global symbols
int GlobalExpectCount = 0;
int GlobalVerifyOrder = 0;

// Test fixtures
static BUS_CircBuffer rx_buffer;
static BUS_CircBuffer tx_buffer;
static uint8_t rx_data[256];
static uint8_t tx_data[256];

void setUp(void)
{
    circular_buffer_init(&rx_buffer, rx_data, 256);
    circular_buffer_init(&tx_buffer, tx_data, 256);
    mock_uart_ll_Init();
}

void tearDown(void)
{
    mock_uart_ll_Destroy();
}

void test_UART_init_success(void)
{
    // Arrange
    UART_LL_init_Expect(UART_BUS_0, rx_data, 256);
    UART_LL_set_idle_callback_Ignore();
    UART_LL_set_rx_complete_callback_Ignore();
    UART_LL_set_tx_complete_callback_Ignore();
    
    // Act
    UART_Handle *handle = UART_init(0, &rx_buffer, &tx_buffer);
    
    // Assert
    TEST_ASSERT_NOT_NULL(handle);
}

void test_UART_init_null_rx_buffer(void)
{
    // Act
    UART_Handle *handle = UART_init(0, NULL, &tx_buffer);
    
    // Assert
    TEST_ASSERT_NULL(handle);
}

// ... more tests
```

## Troubleshooting

### Common Issues

1. **Linker errors with CMock globals**: Add the global symbol definitions to your test file
2. **Mock verification failures**: Check that all `Expect()` calls are satisfied
3. **Tests pass individually but fail together**: Check for shared state between tests
4. **Segmentation faults**: Check for uninitialized pointers or buffer overflows

### Debugging Tests

```c
// Add debug output
void test_debug_example(void)
{
    printf("Debug: testing with value %d\n", test_value);
    // ... test code
}

// Use Unity's built-in message feature
TEST_ASSERT_EQUAL_MESSAGE(expected, actual, "Custom failure message");
```

## Adding New Test Modules

To add tests for a new module:

1. Create `tests/test_[module].c`
2. Add mock generation to `tests/CMakeLists.txt`
3. Add the test target to `tests/CMakeLists.txt`
4. Follow the patterns shown in existing tests

This infrastructure provides a robust foundation for testing firmware modules while isolating them from hardware dependencies.
