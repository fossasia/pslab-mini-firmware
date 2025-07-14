# SCPI Protocol Implementation

This directory contains a SCPI (Standard Commands for Programmable Instruments) protocol implementation for the PSLab Mini firmware. The protocol uses USB CDC as the transport layer and provides both standard IEEE 488.2 commands and instrument-specific functionality.

## Files

- `protocol.c` - Main SCPI protocol implementation
- `protocol.h` - Public API header
- `protocol_example.c` - Example usage
- `README.md` - This documentation file

## Features

### Standard IEEE 488.2 Commands

The implementation provides the following mandatory IEEE 488.2 commands:

- `*RST` - Reset instrument to default state
- `*IDN?` - Identification query (returns "FOSSASIA,PSLab Mini,1.0,v1.0.0")
- `*TST?` - Self-test query (returns 0 for pass)
- `*CLS` - Clear status registers
- `*ESE` / `*ESE?` - Event Status Enable register
- `*ESR?` - Event Status Register query
- `*OPC` / `*OPC?` - Operation Complete command/query
- `*SRE` / `*SRE?` - Service Request Enable register
- `*STB?` - Status Byte query
- `*WAI` - Wait for operations to complete

### Instrument-Specific Commands

- `MEASure:VOLTage:DC?` - Measure DC voltage using the ADC
- `CONFigure:VOLTage:DC` - Configure DC voltage measurement parameters

## Usage

### Initialization

```c
#include "protocol.h"

// Initialize the protocol (USB, ADC, SCPI parser)
if (!protocol_init()) {
    // Handle initialization failure
    return -1;
}
```

### Main Loop

```c
while (1) {
    // Process protocol tasks (USB data, SCPI commands)
    protocol_task();

    // Add other application tasks here
}
```

### Cleanup

```c
// Clean up resources when done
protocol_deinit();
```

## Implementation Details

### Transport Layer

The protocol uses USB CDC (Communication Device Class) as the transport layer:
- USB RX buffer size: 512 bytes
- USB TX buffer size: 512 bytes
- Uses circular buffer for reliable data reception
- Non-blocking read/write operations

### SCPI Parser

- Based on the scpi-parser library
- Input buffer size: 256 bytes
- Error queue size: 16 entries
- Supports standard SCPI syntax and error handling

### ADC Integration

The `MEASure:VOLTage:DC?` command demonstrates integration with the ADC system:
- Reads raw ADC value using `ADC_read()`
- Converts to voltage assuming 12-bit ADC with 3.3V reference
- Returns voltage as floating-point value

## Example SCPI Commands

Connect to the device via USB CDC and send these commands:

```
*IDN?
FOSSASIA,PSLab Mini,1.0,v1.0.0

*TST?
0

MEAS:VOLT:DC?
1.65

*RST

CONF:VOLT:DC 5.0 0.001
```

## Extension Points

### Adding New Commands

1. Add function declaration to the forward declarations section
2. Add command entry to `scpi_commands[]` array
3. Implement the command function following the pattern:

```c
static scpi_result_t scpi_cmd_my_command(scpi_t *context) {
    // Parse parameters using SCPI_Param*() functions
    // Perform the operation
    // Return result using SCPI_Result*() functions
    return SCPI_RES_OK;
}
```

### Adding Hardware Support

The current implementation provides stub functions for status registers and hardware control. To add full functionality:

1. Implement actual status register handling in the ESE/ESR/SRE/STB commands
2. Add real hardware reset functionality to the `*RST` command
3. Implement operation completion tracking for the `*OPC` command
4. Add proper error detection and reporting

### Performance Optimization

For high-throughput applications:
- Increase buffer sizes as needed
- Implement DMA for USB transfers
- Add command queuing for batch operations
- Consider using RTOS for better task scheduling

## Dependencies

- SCPI Parser Library (lib/scpi-parser-2.3/)
- USB CDC API (src/system/bus/usb.h)
- ADC API (src/system/adc/adc.h)
- Circular Buffer Utility (src/util/util.h)

## Compliance

This implementation follows:
- IEEE 488.2 standard for common commands
- SCPI-99 specification for syntax and semantics
- USB CDC specification for transport layer
