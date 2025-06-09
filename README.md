# pslab-mini

## Build Instructions

This will build the bootloader and the main firmware application. The
bootloader binary, `pslab-mini-bootloader`, will be located in build/boot/,
while the main firmware application, `pslab-mini-firmware` will be located
in build/src.

### Prerequisites

- CMake
- A make tool (e.g., GNU Make or Ninja)
- `gcc-arm-none-eabi`
- `libnewlib-arm-none-eabi`

### Steps

```sh
mkdir build
cd build
cmake ..
make
```

## Flashing the Bootloader

Flashing the bootloader requires a hardware programmer, such as ST-Link. The
Nucleo development board has an onboard ST-Link.

To flash the bootloader using STM32_Programmer_CLI, run:

```sh
STM32_Programmer_CLI -c port=SWD -d build/boot/pslab-mini-bootloader.srec -v -rst
```

## Flashing the Firmware

Once the bootloader is present on the device, the main firmware can be flashed over a serial connection using OpenBLT Host tools, specifically [MicroBoot (GUI)](https://www.feaser.com/openblt/doku.php?id=manual:microboot) or [BootCommander (CLI)](https://www.feaser.com/openblt/doku.php?id=manual:bootcommander).

To flash the firmware using BootCommander, run:

```sh
BootCommander -d=/dev/ttyACM0 build/src/pslab-mini-firmware.srec
```
