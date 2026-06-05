# PSLab Mini Firmware

This repository is being prepared for the Raspberry Pi RP2350 based PSLab
firmware port.

This cleanup branch removes files that are specific to the previous STM32H563
target and bootloader flow, while keeping the reusable firmware architecture in
place for follow-up porting work.

## Kept For Porting

- `src/application`: application and protocol structure.
- `src/system`: system services and instrument-level structure.
- `src/util`: reusable utility, logging, error, buffer, and fixed-point code.
- `doc`: design and architecture notes that are still useful during the port.
- `tests`: existing host-side tests, retained until the Pico build/test layout
  is added.

## Removed In This Branch

- STM32/OpenBLT bootloader sources.
- STM32H5 CMSIS/HAL vendor package.
- STM32-specific CMake toolchain and target files.
- STM32-specific platform implementation files.

The Pico firmware skeleton and Pico SDK based build system are added in the
next branch/PR.
