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

### DMM:CONFigure:VOLTage:DC
**Syntax**: `DMM:CONF` or `DMM:CONF:VOLT` or `DMM:CONFIGURE:VOLTAGE:DC`
**Description**: Configure DC voltage measurement parameters
**Parameters**: `[<channel>]` - Optional channel number (default: 0)
**Response**: None
**Example**:
```
DMM:CONF:VOLT:DC
DMM:CONF:VOLT:DC 0
```

**Notes**:
- Validates the configuration without starting measurement
- Channel parameter is currently optional and defaults to channel 0
- Invalid channel numbers will generate an "Illegal parameter value" error

### DMM:INITiate:VOLTage:DC
**Syntax**: `DMM:INIT` or `DMM:INIT:VOLT` or `DMM:INITIATE:VOLTAGE:DC`
**Description**: Initialize DMM and prepare for voltage measurement
**Parameters**: None
**Response**: None
**Example**: `DMM:INIT:VOLT:DC`

**Notes**:
- Must be called before DMM:FETCH command
- Uses configuration set by previous DMM:CONFIGURE command (or defaults)
- Clears any previously cached measurement results

### DMM:FETCh:VOLTage:DC?
**Syntax**: `DMM:FETC?` or `DMM:FETC:VOLT:DC?` or `DMM:FETCH:VOLTAGE:DC?`
**Description**: Fetch the most recent voltage measurement result
**Parameters**: None
**Response**: Voltage value in millivolts
**Example**:
```
DMM:FETC:VOLT:DC?
1500
```

**Notes**:
- Returns cached result if available, or performs new measurement if DMM is initialized
- Generates "Execution error" if called before DMM:INITIATE
- Result is in millivolts (mV)

### DMM:READ:VOLTage:DC?
**Syntax**: `DMM:READ?` or `DMM:READ:VOLT:DC?`
**Description**: Initiate and immediately fetch a voltage measurement
**Parameters**: None
**Response**: Voltage value in millivolts
**Example**:
```
DMM:READ:VOLT:DC?
1500
```

**Notes**:
- Combines DMM:INITIATE and DMM:FETCH operations
- Uses current configuration settings
- Result is in millivolts (mV)

### DMM:MEASure:VOLTage:DC?
**Syntax**: `DMM:MEAS?` or `DMM:MEAS:VOLT:DC?` or `DMM:MEASURE:VOLTAGE:DC?`
**Description**: Configure, initiate, and fetch a complete DC voltage measurement
**Parameters**: `[<channel>]` - Optional channel number
**Response**: Voltage value in millivolts
**Example**:
```
DMM:MEAS:VOLT:DC?
1500
DMM:MEAS:VOLT:DC? 0
```

**Notes**:

- Combines DMM:CONFIGURE, DMM:INITIATE, and DMM:FETCH operations
- Most convenient command for single measurements
- Result is in millivolts (mV)

## OSCilloscope Commands

These commands provide access to the PSLab Mini's digital storage oscilloscope capabilities.

### OSCilloscope:CONFigure:CHANnel
**Syntax**: `OSC:CONF:CHAN` or `OSCilloscope:CONFigure:CHANnel {CH1|CH2|CH1CH2}`
**Description**: Configure oscilloscope channel selection
**Parameters**: `{CH1|CH2|CH1CH2}` - Channel configuration
**Response**: None
**Example**:
```
OSC:CONF:CHAN CH1
OSC:CONF:CHAN CH1CH2
```

**Notes**:

- CH1: Single channel mode, channel 1
- CH2: Single channel mode, channel 2
- CH1CH2: Dual channel mode
- Cannot be changed during active acquisition

### OSCilloscope:CONFigure:CHANnel?
**Syntax**: `OSC:CONF:CHAN?` or `OSCilloscope:CONFigure:CHANnel?`
**Description**: Query current channel configuration
**Parameters**: None
**Response**: Current channel setting (CH1, CH2, or CH1CH2)
**Example**:
```
OSC:CONF:CHAN?
CH1
```

### OSCilloscope:CONFigure:TIMEbase
**Syntax**: `OSC:CONF:TIME` or `OSCilloscope:CONFigure:TIMEbase <timebase_us>`
**Description**: Set oscilloscope timebase in microseconds per division
**Parameters**: `<timebase_us>` - Timebase value in µs/div
**Response**: None
**Example**:
```
OSC:CONF:TIME 100
OSC:CONF:TIME 500
```

**Notes**:

- Sets horizontal time scale
- Sample rate is automatically calculated based on buffer size
- Must be greater than 0

### OSCilloscope:CONFigure:TIMEbase?
**Syntax**: `OSC:CONF:TIME?` or `OSCilloscope:CONFigure:TIMEbase?`
**Description**: Query current timebase setting
**Parameters**: None
**Response**: Current timebase in µs/div
**Example**:
```
OSC:CONF:TIME?
100
```

### OSCilloscope:CONFigure:ACQuire:POINts
**Syntax**: `OSC:CONF:ACQ:POIN` or `OSCilloscope:CONFigure:ACQuire:POINts <n>`
**Description**: Set acquisition buffer size in samples
**Parameters**: `<n>` - Number of samples to acquire
**Response**: None
**Example**:
```
OSC:CONF:ACQ:POIN 512
OSC:CONF:ACQ:POIN 1024
```

**Notes**:

- Determines memory allocation for acquisition
- Sample rate is automatically recalculated
- Must be greater than 0

### OSCilloscope:CONFigure:ACQuire:POINts?
**Syntax**: `OSC:CONF:ACQ:POIN?` or `OSCilloscope:CONFigure:ACQuire:POINts?`
**Description**: Query current buffer size
**Parameters**: None
**Response**: Current buffer size in samples
**Example**:
```
OSC:CONF:ACQ:POIN?
512
```

### OSCilloscope:CONFigure:ACQuire:SRATe?
**Syntax**: `OSC:CONF:ACQ:SRAT?` or `OSCilloscope:CONFigure:ACQuire:SRATe?`
**Description**: Query calculated sample rate
**Parameters**: None
**Response**: Sample rate in samples per second
**Example**:
```
OSC:CONF:ACQ:SRAT?
10000
```

**Notes**:

- Read-only parameter calculated from timebase and buffer size
- Sample rate = buffer_size × 1,000,000 / (timebase_us × 10)

### OSCilloscope:INITiate
**Syntax**: `OSC:INIT` or `OSCilloscope:INITiate`
**Description**: Start oscilloscope data acquisition
**Parameters**: None
**Response**: None
**Example**: `OSC:INIT`

**Notes**:

- Must be called before OSC:FETCH command
- Uses current configuration settings
- Will use default configuration if not previously configured

### OSCilloscope:FETCh:DATa?
**Syntax**: `OSC:FETC?` or `OSC:FETC:DAT?` or `OSCilloscope:FETCh:DATa?`
**Description**: Fetch acquired oscilloscope data
**Parameters**: None
**Response**: Comma-separated list of sample values
**Example**:
```
OSC:FETC:DAT?
1234,1235,1236,1237,...
```

**Notes**:

- Returns raw ADC values
- Must be called after OSC:INIT
- Waits for acquisition completion if still in progress

### OSCilloscope:READ?
**Syntax**: `OSC:READ?` or `OSCilloscope:READ?`
**Description**: Initiate and immediately fetch oscilloscope data
**Parameters**: None
**Response**: Comma-separated list of sample values
**Example**:
```
OSC:READ?
1234,1235,1236,1237,...
```

**Notes**:

- Combines OSC:INIT and OSC:FETCH operations
- Uses current configuration settings
- Aborts any ongoing acquisition first

### OSCilloscope:MEASure?
**Syntax**: `OSC:MEAS?` or `OSCilloscope:MEASure?`
**Description**: Configure, initiate, and fetch complete oscilloscope measurement
**Parameters**: None
**Response**: Comma-separated list of sample values
**Example**:
```
OSC:MEAS?
1234,1235,1236,1237,...
```

**Notes**:

- Most convenient command for single acquisitions
- Uses default or previously set configuration
- Equivalent to OSC:CONF + OSC:READ

### OSCilloscope:ABORt
**Syntax**: `OSC:ABOR` or `OSCilloscope:ABORt`
**Description**: Abort ongoing oscilloscope acquisition
**Parameters**: None
**Response**: None
**Example**: `OSC:ABOR`

**Notes**:

- Stops any acquisition in progress
- Safe to call even if no acquisition is running

### OSCilloscope:STATus:ACQuisition?
**Syntax**: `OSC:STAT:ACQ?` or `OSCilloscope:STATus:ACQuisition?`
**Description**: Query acquisition status
**Parameters**: None
**Response**: Status code (0=not started, 1=in progress, 2=complete)
**Example**:
```
OSC:STAT:ACQ?
2
```

**Notes**:

- 0: No acquisition started
- 1: Acquisition in progress
- 2: Acquisition complete

## Measurement Workflow

### Basic DMM Measurement Sequence
```
*RST                    # Reset instrument
DMM:CONF:VOLT:DC       # Configure for DC voltage (optional)
DMM:INIT:VOLT:DC       # Initialize measurement
DMM:FETC:VOLT:DC?      # Fetch result
```

### Quick DMM Measurement
```
DMM:MEAS:VOLT:DC?      # Single command measurement
```

### Multiple DMM Measurements
```
DMM:CONF:VOLT:DC       # Configure once
DMM:INIT:VOLT:DC       # Initialize
DMM:FETC:VOLT:DC?      # Fetch first result
DMM:FETC:VOLT:DC?      # Fetch cached result again
DMM:INIT:VOLT:DC       # Re-initialize for new measurement
DMM:FETC:VOLT:DC?      # Fetch new result
```

### Basic OSCilloscope Measurement Sequence
```
*RST                    # Reset instrument
OSC:CONF:CHAN CH1      # Configure channel (optional)
OSC:CONF:TIME 100      # Set timebase to 100 µs/div
OSC:CONF:ACQ:POIN 512  # Set buffer size (optional)
OSC:INIT               # Start acquisition
OSC:FETC:DAT?          # Fetch data
```

### Quick OSCilloscope Measurement
```
OSC:MEAS?              # Single command measurement
```

### OSCilloscope Dual Channel Measurement
```
OSC:CONF:CHAN CH1CH2   # Configure for dual channel
OSC:CONF:TIME 50       # Set timebase
OSC:READ?              # Initiate and fetch
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
DMM:MEAS:VOLT:DC? 999  # Invalid channel
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
voltage = instr.query('DMM:MEAS:VOLT:DC?')
print(f"Voltage: {voltage.strip()} mV")

# Multiple measurements
instr.write('DMM:CONF:VOLT:DC')
for i in range(5):
    instr.write('DMM:INIT:VOLT:DC')
    voltage = instr.query('DMM:FETC:VOLT:DC?')
    print(f"Reading {i+1}: {voltage.strip()} mV")

# Oscilloscope measurement
instr.write('OSC:CONF:CHAN CH1')
instr.write('OSC:CONF:TIME 100')  # 100 µs/div
instr.write('OSC:CONF:ACQ:POIN 512')  # 512 samples
data = instr.query('OSC:READ?')
samples = [int(x) for x in data.strip().split(',')]
print(f"Captured {len(samples)} samples")

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
print("Voltage:", query('DMM:MEAS:VOLT:DC?'), "mV")

ser.close()
```

## Troubleshooting

### Common Issues

1. **"Execution error" on DMM:FETCH**
   - Ensure DMM:INITIATE was called first
   - Check that instrument is properly initialized

2. **"Illegal parameter value" on DMM:CONFIGURE**
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