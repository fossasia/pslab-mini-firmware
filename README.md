# pslab-mini

## Build Instructions

### Prerequisites

- [CMake](https://cmake.org/)
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

### Flashing the Firmware

To flash the firmware using the BootCommander tool from OpenBLT, run:

```sh
BootCommander -d=/dev/ttyACM0 build/pslab-mini-firmware.srec
```
