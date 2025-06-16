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
- Connect to the Board using ST-Link and initiate the connection from the application.

  <img src="https://github.com/user-attachments/assets/f126cbd0-4b76-4c50-8680-60b27ee3f38a" height="400">
  
- Browse and select the .srec file for the bootloader inside the build/boot/ folder of your program directory.
 
    <img src="https://github.com/user-attachments/assets/ced379ef-e92b-4e77-84b9-55b23dd30333" height="400">
  
- Initiate Start_Programming, and a successfully uploaded message will appear once programming is complete!

## Flashing the Firmware

Once the bootloader is present on the device, the main firmware can be flashed over a serial connection using OpenBLT Host tools, specifically [MicroBoot (GUI)](https://www.feaser.com/openblt/doku.php?id=manual:microboot) or [BootCommander (CLI)](https://www.feaser.com/openblt/doku.php?id=manual:bootcommander).

To flash the firmware using BootCommander, run:

```sh
BootCommander -d=/dev/ttyACM0 build/src/pslab-mini-firmware.srec
```
To flash the firmware using the MicroBoot GUI tool in Windows, follow these steps:

- Connect the board via USB.
- Open Device Manager, then navigate to the Ports and Comms menu and note down the COMM number the board is connected on.
- Now open the MicroBoot Application, inside Settings, set it to XCP on RS232 Mode, the Device will have the same COMM port as your Device_manager settings.</br>
  <img src="https://github.com/user-attachments/assets/531228d0-93f2-4058-97d3-5c5f92c5f259" height="400">
      
- Now, browse and select the .srec for firmware inside the build/src/ folder.
  <img src="https://github.com/user-attachments/assets/cbcb157c-8954-4dab-be17-e108fb47b926" height="400">

- Click the reset button to put the board in boot mode, and the firmware will be successfully flashed onto the board!
  
