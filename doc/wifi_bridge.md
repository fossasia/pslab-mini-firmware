# ESP Wi-Fi Bridge

## Overview

This document describes the ESP32-C3 Wi-Fi bridge used with the PSLab Pico
firmware. The bridge connects the RP2350 firmware to a host computer over Wi-Fi
while keeping the Pico-side firmware architecture transport-independent.

The ESP bridge has two responsibilities:

- Forward SCPI command and response traffic between the host and the Pico.
- Forward captured waveform data from the Pico to the host.

SCPI command traffic and waveform data use different network transports because
they have different reliability requirements.

- **SCPI command/control**: TCP, ordered and reliable.
- **Waveform/capture data**: UDP, low overhead and suitable for repeated preview frames.
- **Pico-to-ESP link**: SPI, with a READY GPIO from ESP to Pico.

This bridge is intended as a wireless transport option. USB CDC remains the
primary wired command and data interface.

## Transport Architecture

The complete data path is split into three sections:

```text
Host application <-> Wi-Fi <-> ESP32-C3 <-> SPI <-> RP2350 firmware
```

### SCPI Command Path

SCPI commands use TCP on the ESP bridge port.

```text
Host TCP client
    -> ESP TCP SCPI server
    -> ESP queues SCPI frame for SPI
    -> Pico polls/receives SCPI frame
    -> Pico SCPI parser handles command
    -> Pico sends SCPI response frame over SPI
    -> ESP returns response over TCP
```

TCP is used here because SCPI commands must arrive in order and command queries
must receive the matching response. This avoids the command loss observed with a
UDP-only command path.

### Waveform Data Path

Waveform data uses UDP.

```text
Pico capture buffer
    -> Pico splits capture into SPI frames
    -> ESP receives SPI frames
    -> ESP forwards frames over UDP
    -> Host reconstructs capture frames
```

UDP is used for waveform preview because repeated capture frames can tolerate
occasional packet loss better than SCPI commands can. If a host needs exact,
lossless capture export, a future reliable download mode should be added.

### SPI Link

The SPI link is master-driven by the Pico. The ESP runs as an SPI slave and
keeps one or more transactions queued. The READY pin tells the Pico that the ESP
has queued transactions and can accept an SPI exchange.

Each SPI frame is fixed-size:

```text
512 bytes total
12-byte bridge header
500-byte payload
```

The bridge frame types are:

```text
0x00  POLL
0x02  DATA
0x03  SCPI
```

`DATA` frames carry waveform subframes. `SCPI` frames carry command or response
bytes.

## Network Ports

The default ESP bridge ports are:

```text
5005  UDP waveform/capture data
5006  TCP SCPI command/control
5006  UDP waveform host registration
```

The UDP registration packet is used by the host to tell the ESP where waveform
frames should be sent. The current registration payload is:

```text
PSLAB_UDP_REGISTER
```

After registration, waveform frames are sent to the registering host instead of
only the default broadcast address.

## Firmware Components

### Pico Firmware

The Pico firmware owns the instruments and the SCPI parser. The ESP bridge does
not interpret SCPI commands.

Relevant Pico-side modules:

- `src/platform/esp_spi_bridge.c`
  - Low-level RP2350 SPI master and READY-pin handling.
  - Builds and exchanges bridge frames.
- `src/system/transport.c`
  - Transport selection and capture frame forwarding.
  - Sends SCPI responses back through the ESP bridge when the active command
    source is Wi-Fi.
- `src/application/protocol/common.c`
  - Polls USB CDC and Wi-Fi SCPI command sources.
  - Routes received bytes into the SCPI parser.
  - Provides Wi-Fi capture commands.
- `src/application/communication_commands.c`
  - SCPI commands for selecting the active data transport.

### ESP Firmware

The ESP firmware is intentionally a bridge. It does not know instrument state,
decode SCPI commands, or decode waveform data.

Relevant ESP-side components:

- SPI slave task
  - Exchanges fixed-size bridge frames with the Pico.
  - Places waveform frames into the UDP forwarding queue.
  - Places SCPI responses into the TCP response queue.
- UDP task
  - Handles waveform host registration.
  - Sends captured waveform frames to the registered host.
- TCP SCPI task
  - Accepts SCPI command lines from the host.
  - Queues commands for Pico delivery over SPI.
  - Returns query responses to the TCP client.

## SCPI Commands

Transport selection is controlled by the communication command group:

```text
COMM:TRAN USB
COMM:TRAN WIFI
COMM:TRAN AUTO
COMM:TRAN?
COMM:WIFI:STAT?
```

Behavior:

```text
USB   Capture stream data is sent over USB CDC.
WIFI  Capture stream data is sent over the ESP Wi-Fi bridge.
AUTO  Use Wi-Fi when the ESP bridge is ready, otherwise use USB.
```

Wi-Fi one-shot capture commands:

```text
LA:WIFI:READ?
DSO:WIFI:READ?
```

These commands perform a normal finite capture into Pico memory, send the
completed capture through the Wi-Fi data path, and return the capture sequence
number as the SCPI response.

## Wiring

Default wiring between the Pico and ESP32-C3 bridge:

```text
Pico GP2  SPI0 SCK       -> ESP32-C3 GPIO4 SCK
Pico GP3  SPI0 MOSI/TX   -> ESP32-C3 GPIO5 MOSI input
Pico GP4  SPI0 MISO/RX   <- ESP32-C3 GPIO6 MISO output
Pico GP5  SPI0 CS        -> ESP32-C3 GPIO7 CS
Pico GP6  READY input    <- ESP32-C3 GPIO10 READY output
GND       common ground  <-> GND
```

The READY signal is driven by the ESP. The Pico checks this signal before SPI
exchanges so it does not attempt to transfer when the ESP has no queued SPI
transaction.

## ESP Configuration

The ESP firmware uses ESP-IDF Kconfig options. The default values are in
`esp_firmware/sdkconfig.defaults`.

Important options:

```text
CONFIG_ESP_BRIDGE_STA_SSID
CONFIG_ESP_BRIDGE_STA_PASSWORD
CONFIG_ESP_BRIDGE_DEST_IP
CONFIG_ESP_BRIDGE_DEST_PORT
CONFIG_ESP_BRIDGE_LOCAL_PORT
CONFIG_ESP_BRIDGE_PIN_SCLK
CONFIG_ESP_BRIDGE_PIN_MOSI
CONFIG_ESP_BRIDGE_PIN_MISO
CONFIG_ESP_BRIDGE_PIN_CS
CONFIG_ESP_BRIDGE_PIN_READY
```

The SSID and password defaults are placeholders. They should be set locally with
`idf.py menuconfig` or a local `sdkconfig` file before flashing.

## Build and Flash

From the ESP firmware directory:

```bash
cd esp_firmware
idf.py set-target esp32c3
idf.py menuconfig
idf.py build
idf.py -p <port> flash monitor
```

The Pico firmware is built normally with the Pico SDK. The ESP bridge is only
used when the firmware is configured to use Wi-Fi transport or when Wi-Fi is
selected from the host application.

## Host Behavior

A host application should use:

- TCP to send SCPI commands and receive SCPI query responses.
- UDP to receive waveform frames.
- UDP registration before starting waveform capture.

For waveform display, the host should reconstruct complete captures from the
received UDP frames.