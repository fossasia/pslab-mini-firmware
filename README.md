# PSLab Pico

Raspberry Pi Pico firmware for instrument-style data acquisition over USB CDC.
This branch contains the RP2350 firmware skeleton, TinyUSB CDC transport, the
PIO/DMA logic analyser driver, and a SCPI parser based command interface.

The firmware is structured so future instruments can be added without mixing
application protocol code with Pico hardware details.

## Project Layout

```text
.
├── CMakeLists.txt
├── lib
│   └── scpi-parser-2.3
├── pico_sdk_import.cmake
└── src
    ├── application
    │   ├── logic_analyser_commands.c
    │   ├── logic_analyser_commands.h
    │   ├── main.c
    │   ├── protocol
    │   │   ├── common.c
    │   │   └── la.c
    │   └── protocol.h
    ├── platform
    │   ├── status_led.c
    │   ├── status_led.h
    │   ├── test_signal.c
    │   ├── test_signal.h
    │   ├── tusb_config.h
    │   ├── usb_cdc.c
    │   ├── usb_cdc.h
    │   └── usb_descriptors.c
    └── system
        ├── logic_analyser.c
        └── logic_analyser.h
```

## Architecture

### Platform Layer

`src/platform` owns the lowest-level Pico-specific hardware and transport
services.

- Initializes TinyUSB in device mode.
- Exposes USB CDC as a byte stream.
- Holds the USB descriptors and TinyUSB configuration.
- Owns board/peripheral services such as the status LED and PWM test signal.
- Does not know anything about SCPI parsing or instrument state machines.

### System Layer

`src/system` owns instrument drivers built on top of the platform layer.

- Configures the PIO state machine with a one-instruction capture loop.
- Uses DMA to move PIO RX FIFO words into a capture buffer.
- Handles GPIO input setup, PIO program load/unload, state-machine reset, and
  DMA channel ownership.
- Exposes capture metadata such as pin range, sample count, word count, and
  packed bits per word.

### Application Layer

`src/application` owns the host-visible firmware behavior.

- Runs the main loop.
- Reads command bytes from USB CDC.
- Uses `lib/scpi-parser-2.3` to parse IEEE 488.2 and instrument SCPI commands.
- Maintains logic analyser configuration state.
- Returns text responses or SCPI arbitrary binary blocks.

The SCPI parser library is kept in-tree to match the STM32 PSLab Mini firmware
style more closely. Setter commands are silent on success, which is normal SCPI
behavior. Use query commands or `SYSTem:ERRor?` to confirm command state and
errors.

## Build

Configure from the project root:

```bash
cmake -S . -B build-pico2 \
  -DPICO_BOARD=pico2 \
  -DPICO_SDK_PATH=/home/santosh/.pico-sdk/sdk/2.1.0 \
  -Dpicotool_DIR=/home/santosh/.pico-sdk/picotool/2.1.0/picotool
```

Build:

```bash
cmake --build build-pico2 --target pslab_pico -j4
```

Expected firmware outputs:

```text
build-pico2/pslab_pico.elf
build-pico2/pslab_pico.bin
build-pico2/pslab_pico.hex
build-pico2/pslab_pico.uf2
```

## Flash

Put the Pico into BOOTSEL mode, then run:

```bash
sudo /home/santosh/.pico-sdk/picotool/2.1.0/picotool/picotool load --ignore-partitions -f build-pico2/pslab_pico.uf2
sudo /home/santosh/.pico-sdk/picotool/2.1.0/picotool/picotool reboot
```

`--ignore-partitions` is needed when loading this normal RP2040/RP2350 image
onto a device with no partition table.

## Pico VS Code Extension

Open this folder as the project root:

```text
<path-to-this-repository>
```

Use these CMake configure values if the extension does not already know them:

```text
PICO_BOARD=pico2
PICO_SDK_PATH=/home/santosh/.pico-sdk/sdk/2.1.0
picotool_DIR=/home/santosh/.pico-sdk/picotool/2.1.0/picotool
```

Build target:

```text
pslab_pico
```

## Serial Connection

After flashing, the board enumerates as a USB CDC serial device named:

```text
PSLab Pico
```

Commands are ASCII SCPI lines terminated by `\n` or `\r\n`.

Example terminal session:

```text
*IDN?
LA:CONFigure:PINBase 16
LA:CONFigure:PINCount 2
LA:CONFigure:SAMPles 96
LA:CONFigure:DIVider 1
LA:CONFigure:TRIGger:PIN 16
LA:CONFigure:TRIGger:LEVel 1
LA:READ?
```

Long SCPI forms and valid abbreviated forms are accepted. For example,
`LA:CONFigure:DIVider 150` can be written as `LA:CONF:DIV 150`.

Setter commands do not return `OK` on success. If a setter fails, query the
SCPI error queue:

```text
SYSTem:ERRor?
```

## Onboard LED Status

The firmware uses the board's default onboard LED as a simple activity
indicator:

| Event | LED behavior |
| --- | --- |
| Any USB CDC command bytes received | Blink once. |
| Capture command starts | Blink twice, then stay on. |
| Capture completes | Blink twice, then turn off. |

`LA:READ?` shows both the command-received blink and the capture start/end
sequence. If the LED stays on after the two start blinks, the firmware is still
waiting for the trigger condition or the capture is in progress.

## Built-In Test Signal

For quick testing without a second MCU, the firmware can generate a 50% duty
cycle square wave on a Pico GPIO using PWM.

Default test signal:

```text
GPIO15, 1000 Hz
```

Connect one jumper wire:

```text
GPIO15 -> GPIO16
```

Example SCPI session:

```text
TEST:SQUare:CONFigure 15 1000
LA:CONFigure:PINBase 16
LA:CONFigure:PINCount 1
LA:CONFigure:SAMPles 2000
LA:CONFigure:DIVider 150
LA:CONFigure:TRIGger:PIN 16
LA:CONFigure:TRIGger:LEVel 1
LA:CONFigure:TRIGger:MODE EDGE
LA:READ?
```

Stop the signal with:

```text
TEST:SQUare OFF
```

## Command Reference

### Common Commands

| Command | Response | Description |
| --- | --- | --- |
| `*IDN?` | `FOSSASIA, PSLab Pico, 1.0, v0.1.0` | Firmware identity string. |
| `*RST` | none | Reset logic analyser configuration to defaults. |
| `*CLS` | none | Clear the SCPI error queue and status registers. |
| `*TST?` | `0` | Self-test placeholder. `0` means pass. |
| `*OPC?` | `1` | Operation-complete query. Captures are currently blocking. |
| `SYSTem:ERRor?` | SCPI error string | Return and pop the oldest queued SCPI error. |
| `SYSTem:VERSion?` | SCPI version string | Return parser SCPI version. |

### Logic Analyser Configuration

| Long Form | Short Form | Range | Default | Description |
| --- | --- | --- | --- | --- |
| `LA:CONFigure:PINBase <n>` | `LA:CONF:PINB <n>` | `0..29` | `16` | First GPIO sampled by the analyser. |
| `LA:CONFigure:PINBase?` | `LA:CONF:PINB?` | - | - | Query first sampled GPIO. |
| `LA:CONFigure:PINCount <n>` | `LA:CONF:PINC <n>` | `1..8` | `2` | Number of consecutive GPIO pins sampled. |
| `LA:CONFigure:PINCount?` | `LA:CONF:PINC?` | - | - | Query number of sampled pins. |
| `LA:CONFigure:SAMPles <n>` | `LA:CONF:SAMP <n>` | `1..4096` | `96` | Number of samples to capture per pin. |
| `LA:CONFigure:SAMPles?` | `LA:CONF:SAMP?` | - | - | Query sample count. |
| `LA:CONFigure:DIVider <n>` | `LA:CONF:DIV <n>` | `1..16777215` | `1` | PIO clock divider. Sample rate is approximately `clk_sys / divider`. |
| `LA:CONFigure:DIVider?` | `LA:CONF:DIV?` | - | - | Query PIO clock divider. |
| `LA:CONFigure:TRIGger:PIN <n>` | `LA:CONF:TRIG:PIN <n>` | `0..29` | `16` | GPIO used for the trigger wait condition. |
| `LA:CONFigure:TRIGger:PIN?` | `LA:CONF:TRIG:PIN?` | - | - | Query trigger GPIO. |
| `LA:CONFigure:TRIGger:LEVel <v>` | `LA:CONF:TRIG:LEV <v>` | `0`, `1`, `OFF`, `ON`, `FALSE`, `TRUE` | `1` | Trigger level. |
| `LA:CONFigure:TRIGger:LEVel?` | `LA:CONF:TRIG:LEV?` | - | - | Query trigger level as `0` or `1`. |
| `LA:CONFigure:TRIGger:MODE <v>` | `LA:CONF:TRIG:MODE <v>` | `EDGE`, `LEVEL` | `EDGE` | Trigger mode. |
| `LA:CONFigure:TRIGger:MODE?` | `LA:CONF:TRIG:MODE?` | - | - | Query trigger mode. |

`PINBASE + PINCOUNT` must be less than or equal to `30`, because Pico GPIOs
`0..29` are valid user GPIOs.

In `EDGE` trigger mode, the firmware re-arms by waiting for the trigger pin to
leave the selected level, then the PIO waits for the selected level. For
example, `TRIGger:MODE EDGE` with `TRIGger:LEVel 1` waits for low first, then
starts when the pin goes high.

In `LEVEL` trigger mode, capture starts as soon as the selected level is
present. This is useful for immediate captures, but repeated captures will also
start immediately if the trigger pin is still at that level.

### Logic Analyser Acquisition

| Command | Response | Description |
| --- | --- | --- |
| `LA:INITiate` / `LA:INIT` | none | Run one capture using the current configuration. |
| `LA:FETCh?` / `LA:FETC?` | Binary block | Return the most recent capture. Fails if no capture is ready. |
| `LA:FETCh:DATA?` / `LA:FETC:DATA?` | Binary block | Same as `LA:FETCh?`. |
| `LA:READ?` | Binary block | Run one capture, then return it. |
| `LA:STATus?` / `LA:STAT?` | `0`, `1`, or `2` | `0` means idle/no data, `1` means capture data ready, `2` means busy. |
| `LA:STREAM:START` | none | Start repeated capture frames using the current configuration. |
| `LA:STREAM:STOP` | none | Stop repeated capture frames. |
| `LA:STREAM:STATus?` / `LA:STREAM:STAT?` | `enabled,sequence,overruns` | Query stream state. |

Captures are blocking in this first implementation. `LA:INIT` returns only
after DMA has completed.

Each logic analyser stream frame is sent as:

```text
LA:STREAM:FRAME <sequence> <byte_count>
#<digits><byte_count><payload>
```

The payload is the same little-endian `uint32_t` word array returned by
`LA:READ?`.

### Test Signal Commands

| Command | Response | Description |
| --- | --- | --- |
| `TEST:SQUare ON` | none | Start the default test square wave on GPIO15 at 1000 Hz. |
| `TEST:SQUare OFF` | none | Stop the test square wave and drive the output low. |
| `TEST:SQUare?` | `0` or `1` | Query whether the test square wave is enabled. |
| `TEST:SQUare:CONFigure <pin> <frequency_hz>` | none | Start a 50% duty cycle square wave on the selected GPIO. |
| `TEST:SQUare:PIN?` | GPIO number | Query the active test output pin. |
| `TEST:SQUare:FREQuency?` | Frequency in Hz | Query the active test output frequency. |

## Logic Analyser Data Format

Logic analyser capture data is returned as a SCPI arbitrary block:

```text
#<digits><byte_count><payload>
```

For example, a 48-byte capture starts with:

```text
#248
```

The payload is an array of little-endian `uint32_t` words copied directly from
the PIO RX FIFO by DMA.

Samples are packed exactly like the Pico SDK PIO logic analyser example:

- Each sample contributes `PINCOUNT` bits.
- The first configured pin is bit position `0` within each sample group.
- `bits_per_word = 32 - (32 % PINCOUNT)`.
- If `PINCOUNT` does not divide 32, each FIFO word is left-justified and the
  least-significant unused bits are zero.
- `word_count = ceil(SAMPLES * PINCOUNT / bits_per_word)`.

For a host decoder, use the same indexing rule as the Pico example:

```c
bit_index = pin_offset + sample_index * pin_count;
word_index = bit_index / bits_per_word;
word_mask = 1u << (bit_index % bits_per_word + 32 - bits_per_word);
pin_is_high = capture_words[word_index] & word_mask;
```
