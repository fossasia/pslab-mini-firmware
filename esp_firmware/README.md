# ESP SPI UDP Bridge

This ESP32-C3 firmware bridges data from the Pico SPI master to a host computer
over UDP.

```text
Pico 2 -> SPI -> ESP32-C3 -> UDP -> host computer
```

## Behavior

```text
ESP32-C3 joins Wi-Fi as a station.
ESP32-C3 is an SPI slave.
Pico is the SPI master.
ESP receives 512-byte SPI frames into DMA buffers.
ESP batches up to two SPI frames into each UDP packet.
Python receiver validates sequence and payload end-to-end.
```

The ESP starts with broadcast UDP. The Python receiver replies to the first
packet, then the ESP switches to unicast.

## Pins

```text
ESP GPIO4   SCK
ESP GPIO5   MOSI input from Pico GP3
ESP GPIO6   MISO output to Pico GP4
ESP GPIO7   CS
ESP GPIO10  READY output to Pico GP6
GND         common ground
```

## Build

```bash
cd esp_firmware
get_idf
idf.py set-target esp32c3
idf.py menuconfig
idf.py build
```

Set the Wi-Fi SSID and password from:

```text
PSLab ESP SPI UDP bridge -> Station SSID
PSLab ESP SPI UDP bridge -> Station password
```

## Flash

```bash
idf.py -p /dev/ttyACM1 flash monitor
```

## Host Receiver

Connect the host computer to the same Wi-Fi network as the ESP, then run:

```bash
cd esp_firmware
python3 bridge_receiver.py --port 5005
```

## Expected First Test

Use the current known-good physical wiring from the SPI experiments.

Start the ESP first, then the Python receiver, then the Pico. The ESP serial log
should eventually report:

```text
mode=unicast
spi_errors=0
queue_drops=0
```

The Python receiver should show throughput plus `lost=0 bad=0` if the full path
is stable.
