# PSLab Mini Firmware

This repository is being prepared for the Raspberry Pi Pico based PSLab
firmware port.

This branch adds the Pico SCPI command interface on top of the USB CDC and
PIO/DMA logic analyser driver branches. The firmware now exposes a USB CDC
serial endpoint that accepts SCPI commands for logic analyser capture and
test signal control.

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

## USB CDC Platform Driver

- `src/platform/usb_cdc.c`
- `src/platform/usb_cdc.h`
- `src/platform/usb_descriptors.c`
- `src/platform/tusb_config.h`

## Logic Analyser System Driver

- `src/system/logic_analyser.c`
- `src/system/logic_analyser.h`

## SCPI Command Interface

- `src/application/protocol/common.c`: SCPI context, transport callbacks, and
  shared protocol state.
- `src/application/protocol/la.c`: logic analyser SCPI command table.
- `src/application/logic_analyser_commands.c`: command handlers for logic
  analyser configuration, capture, data reads, streaming, and test signal
  control.
- `lib/scpi-parser-2.3`: embedded SCPI parser used by the application layer.

Common commands include:

- `*IDN?`
- `SYST:ERR?`
- `LA:CONF:PINS <first_gpio> <pin_count>`
- `LA:CONF:DIV <divider>`
- `LA:CONF:SAMPLES <sample_count>`
- `LA:TRIG:MODE <AUTO|LEVEL|EDGE>`
- `LA:TRIG:PIN <gpio>`
- `LA:TRIG:LEVEL <0|1>`
- `LA:CAPT`
- `LA:DATA?`
- `LA:STREAM:START`
- `LA:STREAM:STOP`
- `TEST:SQUARE:CONF <gpio> <frequency_hz>`
- `TEST:SQUARE:START`
- `TEST:SQUARE:STOP`

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
