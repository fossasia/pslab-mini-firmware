# PLSab Mini Bootloader

This directory contains the bootloader and related files for the PSLab Mini firmware.

## Contents

- Bootloader source code
- Configuration files
- Documentation

## Purpose

The bootloader is responsible for initializing hardware and loading the main firmware.

## Building

- Ensure `gcc-arm-none-eabi` is installed on your system.
- To build the bootloader, run:
    ```
    make clean all
    ```

### Flashing

To flash the bootloader onto your device using STM32_Programmer_CLI, run:

```
STM32_Programmer_CLI -c port=SWD -d bin/openblt_stm32h563.srec -v -rst
```

## Attribution

This bootloader is based on [OpenBLT](https://www.openblt.org/), an open source bootloader.

## License

This bootloader is licensed under the [GNU General Public License v3.0 (GPLv3)](https://www.gnu.org/licenses/gpl-3.0.html), in accordance with the OpenBLT project's licensing.
