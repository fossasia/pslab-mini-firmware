# PSLab Mini Programming Manual

## Overview

This document describes the SCPI (Standard Commands for Programmable Instruments) interface for the PSLab Mini firmware. The instrument communicates over USB CDC (Communications Device Class) and implements IEEE 488.2 standard commands along with instrument-specific functionality.

## Communication Interface

- **Transport**: USB CDC (Virtual Serial Port)
- **Protocol**: SCPI (Standard Commands for Programmable Instruments)
- **Manufacturer**: FOSSASIA
- **Model**: PSLab
- **Firmware Version**: 1.0
- **SCPI Version**: 1999.0

## Command Structure

SCPI commands follow the standard hierarchical structure with keywords separated by colons (:). Commands can be abbreviated using their short form (typically the first 3-4 characters of each keyword).

### Command Syntax
- Keywords are case-insensitive
- Commands end with a newline character
- Query commands end with a question mark (?)
- Parameters are separated by spaces or commas
- Square brackets [] indicate optional command nodes

## IEEE 488.2 Mandatory Commands

These commands are required by the IEEE 488.2 standard and provide basic instrument control functionality.

### *RST
**Syntax**: `*RST`
**Description**: Reset the instrument to its default state
**Parameters**: None
**Response**: None
**Example**: `*RST`

### *IDN?
**Syntax**: `*IDN?`
**Description**: Query instrument identification
**Parameters**: None
**Response**: Four comma-separated fields: manufacturer, model, serial number, firmware version
**Example**:
```
*IDN?
FOSSASIA,PSLab,1.0,v1.0.0
```

### *TST?
**Syntax**: `*TST?`
**Description**: Perform instrument self-test
**Parameters**: None
**Response**: 0 if test passes, non-zero if test fails
**Example**:
```
*TST?
0
```

### *CLS
**Syntax**: `*CLS`
**Description**: Clear status registers
**Parameters**: None
**Response**: None
**Example**: `*CLS`

### *ESE
**Syntax**: `*ESE <value>`
**Description**: Set Event Status Enable register
**Parameters**: `<value>` - 8-bit mask value (0-255)
**Response**: None
**Example**: `*ESE 32`

### *ESE?
**Syntax**: `*ESE?`
**Description**: Query Event Status Enable register
**Parameters**: None
**Response**: Current ESE register value
**Example**:
```
*ESE?
32
```

### *ESR?
**Syntax**: `*ESR?`
**Description**: Query Event Status Register
**Parameters**: None
**Response**: Current ESR register value
**Example**:
```
*ESR?
0
```

### *OPC
**Syntax**: `*OPC`
**Description**: Set Operation Complete bit when all pending operations finish
**Parameters**: None
**Response**: None
**Example**: `*OPC`

### *OPC?
**Syntax**: `*OPC?`
**Description**: Query Operation Complete status
**Parameters**: None
**Response**: 1 when all operations complete
**Example**:
```
*OPC?
1
```

### *SRE
**Syntax**: `*SRE <value>`
**Description**: Set Service Request Enable register
**Parameters**: `<value>` - 8-bit mask value (0-255)
**Response**: None
**Example**: `*SRE 8`

### *SRE?
**Syntax**: `*SRE?`
**Description**: Query Service Request Enable register
**Parameters**: None
**Response**: Current SRE register value
**Example**:
```
*SRE?
8
```

### *STB?
**Syntax**: `*STB?`
**Description**: Query Status Byte register
**Parameters**: None
**Response**: Current status byte value
**Example**:
```
*STB?
0
```

### *WAI
**Syntax**: `*WAI`
**Description**: Wait for all pending operations to complete
**Parameters**: None
**Response**: None
**Example**: `*WAI`

## Required SCPI Commands

These commands are required by the SCPI standard for basic system functionality.

### SYSTem:ERRor[:NEXT]?
**Syntax**: `SYST:ERR?` or `SYSTEM:ERROR:NEXT?`
**Description**: Query and remove the oldest error from the error queue
**Parameters**: None
**Response**: Error code and description string
**Example**:
```
SYST:ERR?
0,"No error"
```

### SYSTem:ERRor:COUNt?
**Syntax**: `SYST:ERR:COUN?` or `SYSTEM:ERROR:COUNT?`
**Description**: Query the number of errors in the error queue
**Parameters**: None
**Response**: Number of errors (0-16)
**Example**:
```
SYST:ERR:COUN?
0
```

### SYSTem:VERSion?
**Syntax**: `SYST:VERS?` or `SYSTEM:VERSION?`
**Description**: Query SCPI version supported by the instrument
**Parameters**: None
**Response**: SCPI version string
**Example**:
```
SYST:VERS?
1999.0
```

## Instrument-Specific Commands

These commands provide access to the PSLab Mini's measurement capabilities.

### CONFigure[:VOLTage][:DC]
**Syntax**: `CONF` or `CONF:VOLT` or `CONFIGURE:VOLTAGE:DC`
**Description**: Configure DC voltage measurement parameters
**Parameters**: `[<channel>]` - Optional channel number (default: 0)
**Response**: None
**Example**:
```
CONF:VOLT:DC
CONF:VOLT:DC 0
```

**Notes**:
- Validates the configuration without starting measurement
- Channel parameter is currently optional and defaults to channel 0
- Invalid channel numbers will generate an "Illegal parameter value" error

### INITiate[:VOLTage][:DC]
**Syntax**: `INIT` or `INIT:VOLT` or `INITIATE:VOLTAGE:DC`
**Description**: Initialize DMM and prepare for voltage measurement
**Parameters**: None
**Response**: None
**Example**: `INIT:VOLT:DC`

**Notes**:
- Must be called before FETCH command
- Uses configuration set by previous CONFIGURE command (or defaults)
- Clears any previously cached measurement results

### FETCh[:VOLTage][:DC]?
**Syntax**: `FETC?` or `FETC:VOLT:DC?` or `FETCH:VOLTAGE:DC?`
**Description**: Fetch the most recent voltage measurement result
**Parameters**: None
**Response**: Voltage value in millivolts
**Example**:
```
FETC:VOLT:DC?
1500
```

**Notes**:
- Returns cached result if available, or performs new measurement if DMM is initialized
- Generates "Execution error" if called before INITIATE
- Result is in millivolts (mV)

### READ[:VOLTage][:DC]?
**Syntax**: `READ?` or `READ:VOLT:DC?`
**Description**: Initiate and immediately fetch a voltage measurement
**Parameters**: None
**Response**: Voltage value in millivolts
**Example**:
```
READ:VOLT:DC?
1500
```

**Notes**:
- Combines INITIATE and FETCH operations
- Uses current configuration settings
- Result is in millivolts (mV)

### MEASure[:VOLTage][:DC]?
**Syntax**: `MEAS?` or `MEAS:VOLT:DC?` or `MEASURE:VOLTAGE:DC?`
**Description**: Configure, initiate, and fetch a complete DC voltage measurement
**Parameters**: `[<channel>]` - Optional channel number
**Response**: Voltage value in millivolts
**Example**:
```
MEAS:VOLT:DC?
1500
MEAS:VOLT:DC? 0
```

**Notes**:
- Combines CONFIGURE, INITIATE, and FETCH operations
- Most convenient command for single measurements
- Result is in millivolts (mV)

## Measurement Workflow

### Basic Measurement Sequence
```
*RST                    # Reset instrument
CONF:VOLT:DC           # Configure for DC voltage (optional)
INIT:VOLT:DC           # Initialize measurement
FETC:VOLT:DC?          # Fetch result
```

### Quick Measurement
```
MEAS:VOLT:DC?          # Single command measurement
```

### Multiple Measurements
```
CONF:VOLT:DC           # Configure once
INIT:VOLT:DC           # Initialize
FETC:VOLT:DC?          # Fetch first result
FETC:VOLT:DC?          # Fetch cached result again
INIT:VOLT:DC           # Re-initialize for new measurement
FETC:VOLT:DC?          # Fetch new result
```

## Error Handling

The instrument maintains an error queue with up to 16 errors. Errors are reported using standard SCPI error codes:

- **0**: No error
- **-102**: Syntax error
- **-108**: Parameter not allowed
- **-109**: Missing parameter
- **-113**: Undefined header
- **-123**: Numeric overflow
- **-224**: Illegal parameter value
- **-213**: Execution error
- **-310**: System error

### Error Query Example
```
MEAS:VOLT:DC? 999      # Invalid channel
SYST:ERR?              # Query error
-224,"Illegal parameter value"
SYST:ERR?              # Query next error
0,"No error"
```

## Programming Examples

### Python Example using PyVISA
```python
import pyvisa

# Connect to instrument
rm = pyvisa.ResourceManager()
instr = rm.open_resource('COM3')  # Adjust port as needed

# Reset and identify
instr.write('*RST')
print(instr.query('*IDN?'))

# Single measurement
voltage = instr.query('MEAS:VOLT:DC?')
print(f"Voltage: {voltage.strip()} mV")

# Multiple measurements
instr.write('CONF:VOLT:DC')
for i in range(5):
    instr.write('INIT:VOLT:DC')
    voltage = instr.query('FETC:VOLT:DC?')
    print(f"Reading {i+1}: {voltage.strip()} mV")

instr.close()
```

### Simple Serial Communication (Python)
```python
import serial
import time

# Open serial port
ser = serial.Serial('COM3', baudrate=9600, timeout=1)

# Send command and read response
def query(command):
    ser.write((command + '\n').encode())
    return ser.readline().decode().strip()

def write(command):
    ser.write((command + '\n').encode())

# Reset and measure
write('*RST')
time.sleep(0.1)
print("ID:", query('*IDN?'))
print("Voltage:", query('MEAS:VOLT:DC?'), "mV")

ser.close()
```

## Troubleshooting

### Common Issues

1. **"Execution error" on FETCH**
   - Ensure INITIATE was called first
   - Check that instrument is properly initialized

2. **"Illegal parameter value" on CONFIGURE**
   - Verify channel number is valid for your hardware

3. **Timeout during measurement**
   - Check ADC hardware connections
   - Verify power supply stability

4. **No response from instrument**
   - Check USB connection
   - Verify correct COM port/device
   - Ensure proper line endings (\n or \r\n)

### Debug Commands
```
*IDN?                  # Verify communication
SYST:ERR?              # Check for errors
*STB?                  # Check status
SYST:ERR:COUN?         # Count pending errors
```

## Compliance

This implementation follows:
- IEEE 488.2-1992: Standard Digital Interface for Programmable Instrumentation
- SCPI-1999.0: Standard Commands for Programmable Instruments
- USB CDC: Communications Device Class specification