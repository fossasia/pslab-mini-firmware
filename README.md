# PSLab Pico

Standalone Raspberry Pi Pico firmware for instrument-style data acquisition
over USB CDC using SCPI-style text commands.

The current firmware contains a logic analyser and an early oscilloscope path.
The project is laid out so more instruments can be added without mixing
application protocol code with PIO/DMA/ADC hardware details.

## Project Layout

```text
.
├── CMakeLists.txt
├── pico_sdk_import.cmake
└── src
    ├── application
    │   ├── main.c
    │   ├── protocol.c
    │   ├── protocol.h
    │   ├── dso_commands.c
    │   ├── dso_commands.h
    │   ├── logic_analyser_commands.c
    │   └── logic_analyser_commands.h
    ├── platform
    │   ├── adc_capture.c
    │   ├── adc_capture.h
    │   ├── status_led.c
    │   ├── status_led.h
    │   ├── test_signal.c
    │   ├── test_signal.h
    │   ├── tusb_config.h
    │   ├── usb_cdc.c
    │   ├── usb_cdc.h
    │   └── usb_descriptors.c
    └── system
        ├── dso.c
        ├── dso.h
        ├── logic_analyser.c
        └── logic_analyser.h
```

## Architecture

### Platform Layer

`src/platform` owns the lowest-level Pico-specific hardware and transport
services.

- Initializes TinyUSB in device mode.
- Exposes USB CDC as a simple byte stream.
- Holds the USB descriptors and TinyUSB configuration.
- Owns board/peripheral services such as status LED, PWM test signal, and raw
  ADC DMA capture.
- Wraps direct Pico SDK hardware blocks such as ADC, DMA, PWM, GPIO, and USB.
- Does not know anything about SCPI parsing.
- Does not own instrument state machines.

### System Layer

`src/system` owns instrument drivers built on top of the platform layer.

- Configures the PIO state machine with a one-instruction capture loop.
- Uses DMA to move PIO RX FIFO words into a caller-provided buffer.
- Owns the DSO instrument state machine and uses the platform ADC capture
  service.
- Handles GPIO input setup, PIO program load/unload, state-machine reset, and
  DMA channel ownership.
- Exposes capture metadata such as captured pin range, ADC channel, sample
  count, word count, and packed bits per word.

### Application Layer

`src/application` owns the firmware behavior seen from the host.

- Runs the main loop.
- Reads command lines from USB CDC.
- Dispatches SCPI-style commands.
- Maintains logic analyser and oscilloscope configuration state.
- Returns text responses or SCPI arbitrary binary blocks.

The command dispatcher is intentionally small for the current firmware. It
supports the command set below and can later be replaced by a full SCPI parser
if the command tree grows.

## Build

Configure from the project root:

```bash
cmake -S . -B build \
  -DPICO_BOARD=pico2 \
  -DPICO_SDK_PATH=/home/santosh/.pico-sdk/sdk/2.1.0 \
  -Dpicotool_DIR=/home/santosh/.pico-sdk/picotool/2.1.0/picotool
```

Build:

```bash
cmake --build build --target pslab_pico -j4
```

Expected firmware outputs:

```text
build/pslab_pico.elf
build/pslab_pico.bin
build/pslab_pico.hex
build/pslab_pico.uf2
```

## Flash

Put the Pico into BOOTSEL mode, then run:

```bash
sudo /home/santosh/.pico-sdk/picotool/2.1.0/picotool/picotool load --ignore-partitions -f build/pslab_pico.uf2
sudo /home/santosh/.pico-sdk/picotool/2.1.0/picotool/picotool reboot
```

In picotool, `-f` means force a compatible running device into BOOTSEL mode. It
does not mean "file". `--ignore-partitions` is needed when loading this normal
RP2040 image onto a device with no partition table.

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

Commands are ASCII lines terminated by `\n` or `\r\n`.

Example terminal session:

```text
*IDN?
LA:CONF:PINB 16
LA:CONF:PINC 2
LA:CONF:SAMP 96
LA:CONF:DIV 1
LA:CONF:TRIG:PIN 16
LA:CONF:TRIG:LEV 1
LA:READ?
```

Commands that set state return `OK` on success. Query commands return a value.
If a command fails, query the last error with `SYST:ERR?`.

## Onboard LED Status

The firmware uses the board's default onboard LED as a simple activity indicator:

| Event | LED behavior |
| --- | --- |
| Any complete command line received | Blink once. |
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

Then enable the test signal and capture GPIO16 from Python:

```python
from pslab import PicoLogicAnalyzer
import matplotlib.pyplot as plt

la = PicoLogicAnalyzer(sys_clock_hz=150_000_000)
la.start_test_square(pin=15, frequency=1000)

capture = la.capture(
    pin_base=16,
    pin_count=1,
    samples=2000,
    divider=150,
    trigger_pin=16,
    trigger_level=1,
    trigger_mode="edge",
)

la.plot(capture)
plt.show()
```

Stop the signal with:

```python
la.stop_test_square()
```

## Command Reference

### Common Commands

| Command | Response | Description |
| --- | --- | --- |
| `*IDN?` | `FOSSASIA,PSLab Pico,1.0,v0.1.0` | Firmware identity string. |
| `*RST` | `OK` | Reset logic analyser configuration to defaults. |
| `*CLS` | `OK` | Clear the stored protocol error. |
| `*TST?` | `0` | Self-test placeholder. `0` means pass. |
| `*OPC?` | `0` | Operation-complete placeholder. Captures are currently blocking. |
| `SYST:ERR?` | SCPI-style error string | Return and clear the last stored protocol error. |

### Logic Analyser Configuration

Long and short command forms are both accepted.

| Long Form | Short Form | Range | Default | Description |
| --- | --- | --- | --- | --- |
| `LA:CONFIGURE:PINBASE <n>` | `LA:CONF:PINB <n>` | `0..29` | `16` | First GPIO sampled by the analyser. |
| `LA:CONFIGURE:PINBASE?` | `LA:CONF:PINB?` | - | - | Query first sampled GPIO. |
| `LA:CONFIGURE:PINCOUNT <n>` | `LA:CONF:PINC <n>` | `1..8` | `2` | Number of consecutive GPIO pins sampled. |
| `LA:CONFIGURE:PINCOUNT?` | `LA:CONF:PINC?` | - | - | Query number of sampled pins. |
| `LA:CONFIGURE:SAMPLES <n>` | `LA:CONF:SAMP <n>` | `1..4096` | `96` | Number of samples to capture per pin. |
| `LA:CONFIGURE:SAMPLES?` | `LA:CONF:SAMP?` | - | - | Query sample count. |
| `LA:CONFIGURE:DIVIDER <n>` | `LA:CONF:DIV <n>` | `1..16777215` | `1` | PIO clock divider. Sample rate is approximately `clk_sys / divider`. |
| `LA:CONFIGURE:DIVIDER?` | `LA:CONF:DIV?` | - | - | Query PIO clock divider. |
| `LA:CONFIGURE:TRIGGER:PIN <n>` | `LA:CONF:TRIG:PIN <n>` | `0..29` | `16` | GPIO used for the trigger wait condition. |
| `LA:CONFIGURE:TRIGGER:PIN?` | `LA:CONF:TRIG:PIN?` | - | - | Query trigger GPIO. |
| `LA:CONFIGURE:TRIGGER:LEVEL <v>` | `LA:CONF:TRIG:LEV <v>` | `0`, `1`, `OFF`, `ON`, `FALSE`, `TRUE` | `1` | Trigger level. Capture starts when trigger pin matches this level. |
| `LA:CONFIGURE:TRIGGER:LEVEL?` | `LA:CONF:TRIG:LEV?` | - | - | Query trigger level as `0` or `1`. |
| `LA:CONFIGURE:TRIGGER:MODE <v>` | `LA:CONF:TRIG:MODE <v>` | `EDGE`, `LEVEL` | `EDGE` | Trigger mode. `EDGE` re-arms before waiting for the selected level. `LEVEL` starts immediately if the selected level is already present. |
| `LA:CONFIGURE:TRIGGER:MODE?` | `LA:CONF:TRIG:MODE?` | - | - | Query trigger mode. |

`PINBASE + PINCOUNT` must be less than or equal to `30`, because Pico GPIOs
`0..29` are valid user GPIOs.

In `EDGE` trigger mode, the firmware re-arms by waiting for the trigger pin to
leave the selected level, then the PIO waits for the selected level. For example,
`TRIG:MODE EDGE` with `TRIG:LEV 1` waits for low first, then starts when the pin
goes high. `TRIG:MODE EDGE` with `TRIG:LEV 0` waits for high first, then starts
when the pin goes low.

In `LEVEL` trigger mode, capture starts as soon as the selected level is present.
This is useful for immediate captures, but repeated captures will also start
immediately if the trigger pin is still at that level.

### Logic Analyser Acquisition

| Command | Response | Description |
| --- | --- | --- |
| `LA:INITIATE` / `LA:INIT` | `OK` | Run one capture using the current configuration. |
| `LA:FETCH?` / `LA:FETC?` | Binary block | Return the most recent capture. Fails if no capture is ready. |
| `LA:FETCH:DATA?` / `LA:FETC:DATA?` | Binary block | Same as `LA:FETCH?`. |
| `LA:READ?` | Binary block | Run one capture, then return it. |
| `LA:STATUS?` / `LA:STAT?` | `0`, `1`, or `2` | `0` means idle/no data, `1` means capture data ready, `2` means busy. |

Captures are blocking in this first implementation. `LA:INIT` returns only
after DMA has completed.

### Oscilloscope Configuration

The oscilloscope currently supports single-channel raw ADC captures using the
Pico ADC FIFO and DMA. ADC samples are returned as raw 12-bit readings in
little-endian `uint16_t` form.

Analog inputs must stay within the Pico ADC input range. Do not connect signals
below GND or above 3.3 V directly to GPIO26-GPIO29.

| Long Form | Short Form | Range | Default | Description |
| --- | --- | --- | --- | --- |
| `DSO:CONFIGURE:CHANNEL <n>` | `DSO:CONF:CHAN <n>` | `0..3` | `0` | ADC channel to sample. Channel 0 maps to GPIO26, channel 1 to GPIO27, channel 2 to GPIO28, and channel 3 to GPIO29. |
| `DSO:CONFIGURE:CHANNEL?` | `DSO:CONF:CHAN?` | - | - | Query ADC channel. |
| `DSO:CONFIGURE:GPIO?` | `DSO:CONF:GPIO?` | - | - | Query the GPIO used by the selected ADC channel. |
| `DSO:CONFIGURE:RATE <hz>` | `DSO:CONF:RATE <hz>` | `1..500000` | `100000` | Requested ADC sample rate in samples per second. |
| `DSO:CONFIGURE:RATE?` | `DSO:CONF:RATE?` | - | - | Query sample rate. |
| `DSO:CONFIGURE:SAMPLES <n>` | `DSO:CONF:SAMP <n>` | `1..4096` | `1024` | Number of ADC samples to capture. |
| `DSO:CONFIGURE:SAMPLES?` | `DSO:CONF:SAMP?` | - | - | Query sample count. |
| `DSO:CONFIGURE:TRIGGER:LEVEL <n>` | `DSO:CONF:TRIG:LEV <n>` | `0..4095` | `2048` | ADC threshold for analog triggering. |
| `DSO:CONFIGURE:TRIGGER:LEVEL?` | `DSO:CONF:TRIG:LEV?` | - | - | Query trigger threshold. |
| `DSO:CONFIGURE:TRIGGER:MODE <v>` | `DSO:CONF:TRIG:MODE <v>` | `OFF`, `LEVEL`, `EDGE` | `OFF` | Trigger mode. `OFF` captures immediately, `LEVEL` waits for the selected level condition, and `EDGE` rearms before waiting for the selected level condition. |
| `DSO:CONFIGURE:TRIGGER:MODE?` | `DSO:CONF:TRIG:MODE?` | - | - | Query trigger mode. |
| `DSO:CONFIGURE:TRIGGER:SLOPE <v>` | `DSO:CONF:TRIG:SLOP <v>` | `RISE`, `RISING`, `FALL`, `FALLING` | `RISE` | Trigger direction. Rising means sample is greater than or equal to threshold; falling means sample is less than or equal to threshold. |
| `DSO:CONFIGURE:TRIGGER:SLOPE?` | `DSO:CONF:TRIG:SLOP?` | - | - | Query trigger slope. |

### Oscilloscope Acquisition

| Command | Response | Description |
| --- | --- | --- |
| `DSO:INITIATE` / `DSO:INIT` | `OK` | Run one ADC DMA capture using the current DSO configuration. |
| `DSO:FETCH?` / `DSO:FETC?` | Binary block | Return the most recent DSO capture. Fails if no capture is ready. |
| `DSO:FETCH:DATA?` / `DSO:FETC:DATA?` | Binary block | Same as `DSO:FETCH?`. |
| `DSO:READ?` | Binary block | Run one DSO capture, then return it. |
| `DSO:STATUS?` / `DSO:STAT?` | `0`, `1`, or `2` | `0` means idle/no data, `1` means capture data ready, `2` means busy. |
| `DSO:STREAM:START` | `OK` followed by stream frames | Start repeated ADC capture streaming using the current DSO configuration. |
| `DSO:STREAM:STOP` | `OK` | Stop DSO streaming. |
| `DSO:STREAM:STATUS?` / `DSO:STREAM:STAT?` | `enabled,sequence,overruns` | Query DSO stream state. |

Example DSO session:

```text
DSO:CONF:CHAN 0
DSO:CONF:RATE 100000
DSO:CONF:SAMP 1024
DSO:CONF:TRIG:MODE OFF
DSO:READ?
```

Each DSO stream frame is sent as:

```text
DSO:STREAM:FRAME <sequence> <byte_count>
#<digits><byte_count><payload>
```

The payload is the same little-endian `uint16_t` sample array returned by
`DSO:READ?`. Stream metadata such as channel, sample rate, sample count, trigger
mode, and ADC GPIO is currently exposed through SCPI query commands instead of
being embedded into the binary sample payload.

### Test Signal Commands

| Command | Response | Description |
| --- | --- | --- |
| `TEST:SQUARE ON` | `OK` | Start the default test square wave on GPIO15 at 1000 Hz. |
| `TEST:SQUARE OFF` | `OK` | Stop the test square wave and drive the output low. |
| `TEST:SQUARE?` | `0` or `1` | Query whether the test square wave is enabled. |
| `TEST:SQUARE:CONF <pin> <frequency_hz>` | `OK` | Start a 50% duty cycle square wave on the selected GPIO. |
| `TEST:SQUARE:PIN?` | GPIO number | Query the active test output pin. |
| `TEST:SQUARE:FREQ?` | Frequency in Hz | Query the active test output frequency. |

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

## Oscilloscope Data Format

Oscilloscope capture data is also returned as a SCPI arbitrary block:

```text
#<digits><byte_count><payload>
```

The payload is an array of little-endian `uint16_t` ADC samples copied from the
Pico ADC FIFO by DMA. The ADC is configured for 12-bit samples, so the useful
range is approximately:

```text
0      -> 0 V
4095   -> 3.3 V
```

Host-side voltage conversion can start with:

```c
volts = sample * 3.3f / 4095.0f;
```

This assumes the signal is connected directly to a Pico ADC pin and uses the
board's 3.3 V ADC reference. A future analog front-end will need its own scale,
offset, protection, and calibration handling.
