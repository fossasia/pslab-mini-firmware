# System Layer

This directory contains system-level components and utilities for the PSLab Mini firmware.

## Files

- `stubs.c` - System call stubs for newlib (basic stubs for _close_r, _lseek_r)
- `syscalls.c` - UART-based system call implementations for write-only stdio functionality
- `syscalls_config.h.example` - Example configuration for UART I/O
- `system.c` - Main system initialization and management
- `led.c` - LED control utilities

## UART-based stdio Configuration

The `syscalls.c` file provides write-only implementations of system calls that enable `printf()` and other output stdio functions to work over UART. Input functions like `scanf()` are not supported and will return errors.

### Configuration

To enable UART-based stdio:

1. Copy `syscalls_config.h.example` to `syscalls_config.h`
2. Modify the configuration as needed:

   ```c
   #define SYSCALLS_UART_BUS 0  // Use UART bus 0
   #define SYSCALLS_UART_TX_BUFFER_SIZE 512  // Optional: custom buffer size
   ```

3. Include the config header in your main application or build configuration

### Disabling UART stdio

To disable UART stdio (syscalls become no-ops):

```c
#define SYSCALLS_UART_BUS -1
// or simply don't define SYSCALLS_UART_BUS
```

### Usage Example

Once configured, standard output functions work automatically:

```c
#include <stdio.h>

int main(void) {
    printf("Hello, PSLab Mini!\n");
    printf("System initialized successfully\n");

    int value = 42;
    printf("Current value: %d\n", value);

    return 0;
}
```

### Notes

- UART initialization is automatic on first stdio use
- Only output operations are supported (printf, fprintf, etc.)
- Input operations (scanf, fgets, etc.) will return errors
- Output is buffered using circular buffers
- UART configuration (baud rate, etc.) is handled by the platform layer
- Designed for logging and debugging output
