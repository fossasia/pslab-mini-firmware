# PSLab Mini Firmware Build System

## Overview

This document describes the build system for the pslab-mini-firmware project.
The build system is split into parallel paths, `target` and `tests`.

### Target Build (Default)
```bash
mkdir build
cd build
cmake ..
make -j4
```

### Test Build
```bash
mkdir build-tests  
cd build-tests
cmake -DBUILD_TESTS=ON ..
make -j4
ctest
```

## Architecture

The build system uses a delegation pattern throughout the CMake project structure:

```
├── CMakeLists.txt                    # Main entry point with project() call and toolchain setup
├── cmake/
│   ├── pslab-common.cmake            # Shared configuration (C standard, flags, code quality)
│   ├── target.cmake                  # Target-specific build (STM32 packages, subdirectories)
│   ├── tests.cmake                   # Test-specific build (CTest, test subdirectories)
│   ├── stm32_gcc.cmake               # STM32 toolchain
│   ├── FindCMSIS.cmake               # STM32 toolchain
│   ├── FindHAL.cmake                 # STM32 toolchain
│   └── stm32/                        # STM32 toolchain
├── src/
│   ├── CMakeLists.txt                # Delegates to .target.txt or .tests.txt
│   ├── CMakeLists.target.txt         # Creates pslab-mini-firmware executable
│   ├── CMakeLists.tests.txt          # Includes testable subdirectories
│   ├── application/
│   │   └── CMakeLists.txt            # Target-only
│   ├── system/
│   │   ├── CMakeLists.txt            # Delegates to .target.txt or .tests.txt
│   │   ├── CMakeLists.target.txt     # Links sources to executable
│   │   ├── CMakeLists.tests.txt      # Includes testable subdirectories
│   │   ├── bus/
│   │   │   ├── CMakeLists.txt        # Delegates to .target.txt or .tests.txt
│   │   │   ├── CMakeLists.target.txt # Links sources to executable
│   │   │   └── CMakeLists.tests.txt  # Creates pslab-bus library
│   │   └── adc/
│   │       ├── CMakeLists.txt        # Delegates to .target.txt or .tests.txt
│   │       ├── CMakeLists.target.txt # Links sources to executable
│   │       └── CMakeLists.tests.txt  # Creates pslab-adc library
│   └── platform/
│       └── CMakeLists.txt            # Target-only (not included in tests)
└── tests/
    └── CMakeLists.txt                # Links with pslab-bus, pslab-adc libraries
```

## File Naming Convention

- `CMakeLists.txt`: Delegation file (chooses between target/test config)
- `CMakeLists.target.txt`: Target-specific build configuration  
- `CMakeLists.tests.txt`: Test-specific build configuration
- `cmake/pslab-common.cmake`: Shared configuration
