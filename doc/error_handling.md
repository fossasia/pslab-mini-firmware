# PSLab Firmware Error Handling Guide

This document explains the error handling system used in the PSLab firmware, designed for robust and structured error management across all layers of the codebase.

## Overview

The PSLab firmware error handling system wraps the [CException](https://github.com/ThrowTheSwitch/CException) library, providing structured exception handling in C. It introduces a domain-specific `Error` enum for PSLab errors and integrates with standard `errno` codes for compatibility with system-level operations and newlib functions.

## Key Features

- **CException Wrapping:**
  - The macros `TRY`, `CATCH`, and `THROW` map directly to CException's `Try`, `Catch`, and `Throw`.
  - Enables exception-like control flow in C, allowing errors to be thrown and caught across function and module boundaries.

- **Layer-Agnostic Usage:**
  - Error handling can be used in all layers (application, platform, system, etc.).
  - Exceptions (`THROW`) can cross layer boundaries, simplifying error propagation and handling.

- **Error Codes:**
  - Use PSLab-specific `Error` codes for domain logic and internal operations.
  - Use standard `errno` codes when interacting with newlib or system-level APIs.
  - Utility functions are provided to convert between `Error` and `errno` codes:
    - `error_to_errno(Error error)`
    - `errno_to_error(int errno_val)`

## Example Usage

```c
#include "error.h"

void example_function() {
    Error ex;
    TRY {
        // ... code that may throw ...
        if (some_error_condition) {
            THROW(ERROR_INVALID_ARGUMENT);
        }
        // ... more code ...
    } CATCH(ex) {
        // Handle error
        printf("Error: %s\n", error_to_string(ex));
        // Optionally convert to errno for system calls
        errno = error_to_errno(ex);
    }
}
```

## Guidelines

- **Interacting with newlib/system APIs:**
  - Use `errno` codes for errors returned to or received from newlib functions.
  - Convert between PSLab `Error` and `errno` using the provided utility functions.

- **Internal PSLab logic:**
  - Use the `Error` enum for all internal error signaling and handling.
  - Don't use CException types or functions directly; use the provided wrappers.

## CException Limitations

When using CException (and thus PSLab's error handling macros), be aware of the following limitations:

- **Return & Goto:**
  - Do not directly `return` from within a `TRY` block, nor `goto` into or out of a `TRY` block.
  - The `TRY` macro allocates local memory and alters a global pointer, which are cleaned up at the top of the `CATCH` macro. Bypassing these steps (via return/goto) can cause memory leaks or unpredictable behavior.

- **Local Variables:**
  - If you change local (stack) variables within a `TRY` block and need to use their updated values after an exception is thrown, mark those variables as `volatile`.

- **Memory Management:**
  - Memory allocated (e.g., via `malloc`) within a `TRY` block is not automatically released if an error is thrown.
  - It is your responsibility to clean up such resources in the `CATCH` block as needed.

## References

[CException Library](https://github.com/ThrowTheSwitch/CException)
See `src/util/error.h` for implementation details and available error codes.
