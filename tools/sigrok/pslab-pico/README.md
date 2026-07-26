# PSLab Pico libsigrok Driver

This directory is an out-of-tree `libsigrok` hardware driver for the PSLab
Mini/Pico firmware. It talks to the firmware over USB CDC using the existing
SCPI commands and feeds unpacked `SR_DF_LOGIC` samples to PulseView.

## Firmware Requirements

Flash firmware that includes:

- `LA:CONFigure:TRIGger:MODE AUTO`
- `LA:CONFigure:RATE?`
- `LA:METadata?`
- `LA:READ?` returning a SCPI arbitrary block of little-endian packed
  `uint32_t` PIO/DMA words

`AUTO` trigger mode is important because PulseView captures usually start
without a hardware trigger.

## Build Into libsigrok

1. Copy this directory to `libsigrok/src/hardware/pslab-pico`.
2. Add the directory to libsigrok's hardware-driver build files.
3. Add `pslab_pico_driver_info` to the libsigrok hardware driver registry if
   your libsigrok checkout requires explicit registration.
4. Build and install libsigrok, then build PulseView against that libsigrok.

The driver is intentionally small and POSIX-serial based. Linux is the first
target; Windows/macOS can follow by replacing the serial transport in
`protocol.c` with libsigrok's serial helpers.

## Usage

```sh
sigrok-cli --driver pslab-pico:conn=/dev/ttyACM0 --samples 1024
```

PulseView should then show `pslab-pico` as a sigrok hardware driver. Select the
CDC device path as the connection, choose enabled channels, set sample count and
sample rate, capture, and use the normal PulseView protocol decoders.

## Capture Format

Firmware returns packed PIO/DMA words. The host unpacker uses:

```text
bits_per_word = 32 - (32 % pin_count)
```

Each sample consumes `pin_count` bits. The output byte maps bit 0 to `D0`, bit
1 to `D1`, and so on.

## Local Test

The unpacker can be tested without libsigrok:

```sh
make test
```
