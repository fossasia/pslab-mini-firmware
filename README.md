# PSLab Pico Firmware

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

## Oscilloscope System Driver

- `src/platform/adc_capture.c`: Pico ADC capture driver.
- `src/platform/adc_capture.h`
- `src/system/instrument/dso.c`: rudimentary oscilloscope instrument built on the Pico ADC.
- `src/system/instrument/dso.h`
- `src/application/protocol/dso.c`: oscilloscope SCPI command handlers.
- `src/application/dso_commands.c`
- `src/application/dso_commands.h`

The current oscilloscope uses the RP2350 internal ADC. It supports one ADC
channel at a time, channels `0..3`, mapped to GPIOs `26..29`. Samples are
returned as little-endian 12-bit ADC values stored in `uint16_t` words.

Default oscilloscope configuration:

- Channel: `0` (`GPIO26`)
- Sample rate: `100000` samples per second
- Samples: `1024`
- Trigger mode: `OFF`
- Trigger level: `2048`
- Trigger slope: `RISE`

## Utility, Error, And Logging Support

- `src/util/error.h`: STM32 firmware error API wrapper around CException.
- `src/util/error.c`: Pico default uncaught exception halt handler.
- `src/util/logging.c`
- `src/util/logging.h`
- `src/util/circular_buffer.c`
- `src/util/fixed_point.c`
- `src/util/fixed_point.h`
- `lib/CException-1.3.4`: CException library used by the retained error API.

This branch wires the retained utility layer into the Pico build. `LOG_task`
still writes through the C library output path; the later UART logging
transport/system-init branch will route that output to hardware UART so USB CDC
can stay dedicated to SCPI commands and binary instrument data.

## UART Logging Transport

- `src/system/bus/uart.c`
- `src/system/bus/uart.h`
- `src/platform/uart_ll.c`
- `src/platform/uart_ll.h`
- `src/platform/platform.c`
- `src/platform/platform.h`

The retained `src/system/bus/uart.*` layer stays hardware-independent and uses
the same circular-buffer and callback style as the STM32 firmware. The Pico
specific UART backend lives in `src/platform/uart_ll.*`; it exposes the same
low-level API expected by the system UART layer while using RP2040/RP2350 UART
interrupts internally.

## System Init And Newlib Syscalls

- `src/system/system.c`
- `src/system/system.h`
- `src/system/syscalls.c`

`SYSTEM_init()` is called before protocol initialization. It initializes the
platform layer, utility logging, UART-backed stdout/stderr syscalls, status LED,
test signal generation, and USB CDC. The protocol layer then owns only SCPI
context setup and command processing.

Newlib writes to `stdout` and `stderr` are routed to the hardware UART transport.
USB CDC remains dedicated to SCPI commands and binary instrument data.

## Application Logging

The application layer uses the retained logging API for internal firmware
events. Main startup and SCPI protocol init/deinit are logged through
`util/logging.h`, and the main loop drains pending log entries with `LOG_task`.

SCPI parser errors remain separate from internal firmware errors. `SYST:ERR?`
continues to report command/protocol errors from the SCPI error queue only.
Internal logs are written through the UART-backed stdout/stderr path, not USB
CDC.

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
- `LA:CONF:RATE?`
- `LA:METADATA?`
- `LA:TRIG:MODE <AUTO|LEVEL|EDGE>`
- `LA:TRIG:PIN <gpio>`
- `LA:TRIG:LEVEL <0|1>`
- `LA:CAPT`
- `LA:DATA?`
- `LA:STREAM:START`
- `LA:STREAM:STOP`
- `DSO:CONF:CHAN <channel>`
- `DSO:CONF:GPIO?`
- `DSO:CONF:RATE <sample_rate_hz>`
- `DSO:CONF:SAMP <sample_count>`
- `DSO:CONF:TRIG:MODE <OFF|LEVEL|EDGE>`
- `DSO:CONF:TRIG:LEV <0..4095>`
- `DSO:CONF:TRIG:SLOP <RISE|FALL>`
- `DSO:READ?`
- `DSO:STREAM:START`
- `DSO:STREAM:STOP`
- `TEST:SQUARE:CONF <gpio> <frequency_hz>`
- `TEST:SQUARE:START`
- `TEST:SQUARE:STOP`

## PulseView And sigrok

The repo includes an out-of-tree `libsigrok` hardware driver package in
`tools/sigrok/pslab-pico`. It uses the existing USB CDC SCPI interface and
unpacks `LA:READ?` binary blocks into sigrok logic samples, so PulseView can use
its normal protocol decoders for UART, I2C, SPI, and other digital buses.

## SCPI Bus Gateway

The bus gateway exposes UART1 through SCPI so a host can talk
to external serial sensors or peripherals through the PSLab Pico.

Initial UART gateway uses the platform defaults:

- UART1 TX: GPIO4
- UART1 RX: GPIO5
- Format: 8N1

UART0 is reserved by the firmware logging so the gateway currently allows UART1 only.

Available commands:

- `BUS:UART:CONFigure:BUS <1>`
- `BUS:UART:CONFigure:BUS?`
- `BUS:UART:CONFigure:BAUD <baud>`
- `BUS:UART:CONFigure:BAUD?`
- `BUS:UART:CONFigure:TIMEout <ms>`
- `BUS:UART:CONFigure:TIMEout?`
- `BUS:UART:OPEN`
- `BUS:UART:OPEN?`
- `BUS:UART:CLOSe`
- `BUS:UART:WRITe <arbitrary_block>`
- `BUS:UART:READ? [max_bytes]`
- `BUS:UART:AVAILable?`
- `BUS:UART:CLEar`
- `BUS:UART:FLUSh`
- `BUS:UART:TRANsact? <arbitrary_block>`

`WRITe`, `READ?`, and `TRANsact?` use SCPI arbitrary blocks so binary payloads
are safe. The current implementation caps each transfer at 512 bytes.

The I2C gateway exposes the Pico I2C controller as a SCPI master bus for
external sensors and peripherals. It defaults to I2C0 on GPIO0/GPIO1 at
100 kHz.

I2C defaults:

- I2C0 SDA: GPIO0
- I2C0 SCL: GPIO1
- I2C1 SDA: GPIO6
- I2C1 SCL: GPIO7
- Addressing: 7 bit master mode

Available commands:

- `BUS:I2C:CONFigure:BUS <0|1>`
- `BUS:I2C:CONFigure:BUS?`
- `BUS:I2C:CONFigure:RATE <hz>`
- `BUS:I2C:CONFigure:RATE?`
- `BUS:I2C:CONFigure:ADDRess <address>`
- `BUS:I2C:CONFigure:ADDRess?`
- `BUS:I2C:CONFigure:TIMEout <ms>`
- `BUS:I2C:CONFigure:TIMEout?`
- `BUS:I2C:OPEN`
- `BUS:I2C:OPEN?`
- `BUS:I2C:CLOSe`
- `BUS:I2C:SCAN?`
- `BUS:I2C:WRITe <arbitrary_block>`
- `BUS:I2C:READ? <byte_count>`
- `BUS:I2C:TRANsact? <arbitrary_block>,<read_byte_count>`

`SCAN?` returns comma separated hexadecimal 7 bit addresses. `WRITe`,
`READ?`, and `TRANsact?` use SCPI arbitrary blocks and are capped at 512
bytes per transfer. `TRANsact?` sends the write block followed by a repeated
start read, which is the common register read pattern for I2C sensors.

The SPI gateway exposes SPI1 as a SCPI master bus. SPI0 is reserved by the
ESP/WiFi bridge and is not exposed through the generic gateway.

SPI defaults:

- SPI1 SCK: GPIO10
- SPI1 MOSI/TX: GPIO11
- SPI1 MISO/RX: GPIO12
- SPI1 CS: GPIO13
- Rate: 1 MHz
- Mode: 0
- Bit order: MSB first
- CS polarity: active low
- Dummy byte for reads: `0xff`

Available commands:

- `BUS:SPI:CONFigure:BUS <1>`
- `BUS:SPI:CONFigure:BUS?`
- `BUS:SPI:CONFigure:RATE <hz>`
- `BUS:SPI:CONFigure:RATE?`
- `BUS:SPI:CONFigure:MODE <0|1|2|3>`
- `BUS:SPI:CONFigure:MODE?`
- `BUS:SPI:CONFigure:DUMMY <byte>`
- `BUS:SPI:CONFigure:DUMMY?`
- `BUS:SPI:OPEN`
- `BUS:SPI:OPEN?`
- `BUS:SPI:CLOSe`
- `BUS:SPI:WRITe <arbitrary_block>`
- `BUS:SPI:READ? <byte_count>`
- `BUS:SPI:EXCHange? <arbitrary_block>`
- `BUS:SPI:TRANsact? <arbitrary_block>,<read_byte_count>`

`WRITe`, `READ?`, `EXCHange?`, and `TRANsact?` use SCPI arbitrary blocks and
are capped at 512 bytes per transfer. `EXCHange?` performs a full duplex SPI
transfer and returns the bytes received while the command block is clocked out.
`TRANsact?` writes a command block and then reads a response while keeping CS
asserted, which is the common register read pattern for SPI sensors.

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
