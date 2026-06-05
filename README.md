# PSLab Mini Firmware

This repository is being prepared for the Raspberry Pi Pico based PSLab
firmware port.

This branch adds the initial Pico SDK based firmware skeleton after the
STM32-specific cleanup. The skeleton builds a minimal `pslab_pico` firmware
target for RP2350 boards.

## Kept For Porting

- `src/application`: application and protocol structure.
- `src/system`: system services and instrument-level structure.
- `src/util`: reusable utility, logging, error, buffer, and fixed-point code.
- `doc`: design and architecture notes that are still useful during the port.
- `tests`: existing host-side tests, retained until the Pico build/test layout
  is added.

## Pico Skeleton

- Root `CMakeLists.txt` using the Pico SDK.
- `pico_sdk_import.cmake` for Pico SDK discovery.
- Minimal `src/application/main.c` entrypoint.

## Build

Configure from the project root:

```bash
cmake -S . -B build-pico2 \
  -DPICO_BOARD=pico2 \
  -DPICO_SDK_PATH=/path/to/pico/sdk/2.1.0 \
  -Dpicotool_DIR=/path/to/pico/sdk/2.1.0/picotool
```

Build:

```bash
cmake --build build-pico2 --target pslab_pico -j4
```
