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

To flash the bootloader using the Windows application of STM32_Programmer, follow these steps:

- Open the STM32_Programmer application and switch to the Erasing & Programming tab from the left menu bar.
- Browse and select the .srec file for the bootloader inside the build/boot/ folder of your program directory.
- Connect to the Board using ST-Link and initiate the connection from the application.
- Initiate Start_Programming, and a successfully uploaded message will appear once programming is complete!

## Flashing the Firmware

Once the bootloader is present on the device, the main firmware can be flashed over a serial connection using OpenBLT Host tools, specifically [MicroBoot (GUI)](https://www.feaser.com/openblt/doku.php?id=manual:microboot) or [BootCommander (CLI)](https://www.feaser.com/openblt/doku.php?id=manual:bootcommander).

To flash the firmware using BootCommander, run:

```sh
BootCommander -d=/dev/ttyACM0 build/src/pslab-mini-firmware.srec
```
To flash the firmware using the MicroBoot GUI tool in Windows, follow these steps:

- Connect the board via USB.
- Open Device Manager, then navigate to the Ports and Comms menu and open the COMM settings where the board is connected. 
- Inside the Port settings, set the Bits per Second to 57600.
- Now open the MicroBoot Application, inside Settings, set it to XCP on RS232 Mode, the Device will have the same COMM port as your Device_manager settings, and set the baudrate at 56700.
- Now, browse and select the .srec for firmware inside the build/src/ folder.
- Click the reset button to put the board in boot mode, and the firmware will be successfully flashed onto the board!
  
