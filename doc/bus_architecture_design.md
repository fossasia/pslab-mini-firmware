# UART and USB Driver Architecture

This document describes the architecture and usage of the PSLab's UART and USB drivers. Both drivers use a handle-based API and share similar high-level interfaces for consistency and ease of integration, while their low-level implementations are hardware-specific.

## Overview

Both the UART and USB drivers provide handle-based APIs for managing multiple communication interfaces. Each instance operates independently with its own buffers, callbacks, and hardware resources, enabling simultaneous communication on multiple interfaces (UART buses or USB CDC interfaces).

- **UART**: Supports multiple buses, each with independent RX and TX buffers.
- **USB**: Currently supports a single CDC interface, but the API is future-proofed for multiple interfaces. Only an RX buffer is required for initialization.

## Architecture Design

The drivers are structured in three layers to provide clean separation between hardware specifics and application logic. Each layer's purpose and features are described below for both UART and USB:

### 1. Hardware Abstraction Layer
- **UART**
  - **Purpose**: Direct hardware interface for STM32H563xx UART peripherals
  - **Features**:
    - Bus enumeration with `uart_bus_t` enum (internal to implementation)
    - DMA-based transmission and reception for optimal performance
    - Interrupt handling for TX complete, RX complete, and idle line detection
    - Hardware-specific initialization and configuration
- **USB**
  - **Purpose**: Direct hardware interface for STM32H563xx USB peripheral
  - **Features**:
    - Bus enumeration with `usb_bus_t` enum (consistent with UART driver)
    - Hardware-specific initialization and configuration with bus parameter support
    - Device identification and serial number generation per bus
    - Low-level USB interrupt handling
    - Instance management and state tracking

### 2. Hardware-Independent Layer
- **UART**
  - **Purpose**: Manages UART instances and provides the public API
  - **Features**:
    - Handle-based instance management
    - Circular buffer management for each instance
    - Callback routing and management
    - Handle lifecycle management
- **USB**
  - **Purpose**: Manages USB instances and provides the public API
  - **Features**:
    - Handle-based instance management
    - Circular buffer management for RX data
    - Callback handling for protocol implementations
    - TinyUSB task management

### 3. Public API
- **UART**
  - **Purpose**: Interface for application code
  - **Features**:
    - Handle-based operations (no global state)
    - Non-blocking read/write operations
    - Configurable RX callbacks for protocol implementations
    - Buffer status inquiry functions
    - Platform-agnostic design
- **USB**
  - **Purpose**: Interface for application code
  - **Features**:
    - Handle-based operations (consistent with UART API)
    - Platform-agnostic design
    - Non-blocking read/write operations

## High-Level API Reference (UART & USB)

The high-level APIs for UART and USB are designed to be as consistent as possible. Key differences are noted below.

### Initialization and Cleanup

```c
// Query available interface/bus count
size_t UART_get_bus_count(void);
size_t USB_get_interface_count(void);

// Initialize an interface
// UART: requires both RX and TX buffers
uart_handle_t *UART_init(size_t bus, circular_buffer_t *rx_buffer, circular_buffer_t *tx_buffer);
// USB: requires only RX buffer
usb_handle_t *USB_init(size_t interface, circular_buffer_t *rx_buffer);

// Clean up interface
void UART_deinit(uart_handle_t *handle);
void USB_deinit(usb_handle_t *handle);
```

### Task Management (USB only)

```c
// Must be called periodically to handle USB events
void USB_task(usb_handle_t *handle);
```

### Data Operations

```c
// Non-blocking write
uint32_t UART_write(uart_handle_t *handle, uint8_t const *txbuf, uint32_t sz);
uint32_t USB_write(usb_handle_t *handle, uint8_t const *buf, uint32_t sz);

// Non-blocking read
uint32_t UART_read(uart_handle_t *handle, uint8_t *rxbuf, uint32_t sz);
uint32_t USB_read(usb_handle_t *handle, uint8_t *buf, uint32_t sz);
```

### Status and Monitoring

```c
// Check if data is available
bool UART_rx_ready(uart_handle_t *handle);
bool USB_rx_ready(usb_handle_t *handle);
uint32_t UART_rx_available(uart_handle_t *handle);
uint32_t USB_rx_available(usb_handle_t *handle);

// Check transmission status
bool UART_tx_busy(uart_handle_t *handle);
bool USB_tx_busy(usb_handle_t *handle);
uint32_t UART_tx_free_space(uart_handle_t *handle);
uint32_t USB_tx_free_space(usb_handle_t *handle);
```

### Callback Configuration

```c
// Callback function signatures
// UART
typedef void (*uart_rx_callback_t)(uart_handle_t *handle, uint32_t bytes_available);
// USB
typedef void (*usb_rx_callback_t)(usb_handle_t *handle, uint32_t bytes_available);

// Set RX threshold callback
void UART_set_rx_callback(uart_handle_t *handle, uart_rx_callback_t callback, uint32_t threshold);
void USB_set_rx_callback(usb_handle_t *handle, usb_rx_callback_t callback, uint32_t threshold);
```

## Usage Examples

### Basic Communication

```c
#include "uart.h"
#include "usb.h"
#include "bus.h"

// UART setup
uart_handle_t *setup_uart_communication(void) {
    static uint8_t rx_data[256];
    static uint8_t tx_data[256];
    static circular_buffer_t rx_buffer, tx_buffer;
    circular_buffer_init(&rx_buffer, rx_data, sizeof(rx_data));
    circular_buffer_init(&tx_buffer, tx_data, sizeof(tx_data));
    uart_handle_t *uart = UART_init(0, &rx_buffer, &tx_buffer);
    return uart;
}

// USB setup
usb_handle_t *setup_usb_communication(void) {
    static uint8_t rx_data[256];
    static circular_buffer_t rx_buffer;
    circular_buffer_init(&rx_buffer, rx_data, 256);
    usb_handle_t *usb = USB_init(0, &rx_buffer);
    return usb;
}

// UART echo server
void uart_echo_server(uart_handle_t *uart) {
    if (UART_rx_ready(uart)) {
        uint8_t buffer[64];
        uint32_t bytes_read = UART_read(uart, buffer, sizeof(buffer));
        UART_write(uart, buffer, bytes_read);
    }
}

// USB echo server
void usb_echo_server(usb_handle_t *usb) {
    USB_task(usb);  // Must be called periodically
    if (USB_rx_ready(usb)) {
        uint8_t buffer[32];
        uint32_t bytes = USB_read(usb, buffer, sizeof(buffer));
        USB_write(usb, buffer, bytes);
    }
}
```

### Protocol Implementation with Callbacks

```c
#include "uart.h"
#include "usb.h"
#include "bus.h"

#define PACKET_HEADER_SIZE 4
#define MAX_PACKET_SIZE 256

typedef struct {
    uart_handle_t *uart;
    usb_handle_t *usb;
    uint8_t packet_buffer[MAX_PACKET_SIZE];
    uint32_t expected_length;
    bool header_received;
} protocol_state_t;

static protocol_state_t protocol;

// UART header callback
void uart_on_header_received(uart_handle_t *handle, uint32_t bytes_available) {
    if (!protocol.header_received && bytes_available >= PACKET_HEADER_SIZE) {
        uint8_t header[PACKET_HEADER_SIZE];
        UART_read(handle, header, PACKET_HEADER_SIZE);
        protocol.expected_length = (header[2] << 8) | header[3];
        protocol.header_received = true;
        UART_set_rx_callback(handle, uart_on_packet_received, protocol.expected_length);
    }
}

// UART packet callback
void uart_on_packet_received(uart_handle_t *handle, uint32_t bytes_available) {
    if (protocol.header_received && bytes_available >= protocol.expected_length) {
        uint32_t bytes_read = UART_read(handle, protocol.packet_buffer, protocol.expected_length);
        process_packet(protocol.packet_buffer, bytes_read);
        protocol.header_received = false;
        UART_set_rx_callback(handle, uart_on_header_received, PACKET_HEADER_SIZE);
    }
}

// USB header callback
void usb_on_header_received(usb_handle_t *handle, uint32_t bytes_available) {
    if (!protocol.header_received && bytes_available >= PACKET_HEADER_SIZE) {
        uint8_t header[PACKET_HEADER_SIZE];
        USB_read(handle, header, PACKET_HEADER_SIZE);
        protocol.expected_length = header[2] | (header[3] << 8);
        protocol.header_received = true;
        USB_set_rx_callback(handle, usb_on_packet_received, PACKET_HEADER_SIZE + protocol.expected_length);
    }
}

// USB packet callback
void usb_on_packet_received(usb_handle_t *handle, uint32_t bytes_available) {
    if (protocol.header_received && bytes_available >= (PACKET_HEADER_SIZE + protocol.expected_length)) {
        uint8_t packet[MAX_PACKET_SIZE];
        uint32_t total_size = PACKET_HEADER_SIZE + protocol.expected_length;
        USB_read(handle, packet, total_size);
        process_packet(packet, total_size);
        protocol.header_received = false;
        USB_set_rx_callback(handle, usb_on_header_received, PACKET_HEADER_SIZE);
    }
}

// UART protocol handler setup
void setup_uart_protocol_handler(void) {
    static uint8_t rx_data[512];
    static uint8_t tx_data[256];
    static circular_buffer_t rx_buffer, tx_buffer;
    circular_buffer_init(&rx_buffer, rx_data, sizeof(rx_data));
    circular_buffer_init(&tx_buffer, tx_data, sizeof(tx_data));
    protocol.uart = UART_init(0, &rx_buffer, &tx_buffer);
    protocol.header_received = false;
    UART_set_rx_callback(protocol.uart, uart_on_header_received, PACKET_HEADER_SIZE);
}

// USB protocol handler setup
void setup_usb_protocol_handler(void) {
    static uint8_t rx_data[512];
    static circular_buffer_t rx_buffer;
    circular_buffer_init(&rx_buffer, rx_data, 512);
    protocol.usb = USB_init(0, &rx_buffer);
    protocol.header_received = false;
    USB_set_rx_callback(protocol.usb, usb_on_header_received, PACKET_HEADER_SIZE);
}
```

### Multi-Interface Communication Example

```c
#include "uart.h"
#include "usb.h"
#include "bus.h"

typedef struct {
    uart_handle_t *debug_uart;    // UART for debug output
    usb_handle_t *main_usb;       // USB for main communication
} system_comms_t;

static system_comms_t comms;

// USB callback
void usb_data_callback(usb_handle_t *handle, uint32_t bytes_available) {
    uint8_t buffer[64];
    uint32_t bytes = USB_read(handle, buffer, sizeof(buffer));
    UART_write(comms.debug_uart, buffer, bytes);
}

// UART callback  
void uart_data_callback(uart_handle_t *handle, uint32_t bytes_available) {
    uint8_t buffer[64];
    uint32_t bytes = UART_read(handle, buffer, sizeof(buffer));
    USB_write(comms.main_usb, buffer, bytes);
}

// Initialize all communication interfaces
bool setup_all_comms(void) {
    static uint8_t uart_rx_data[256], uart_tx_data[256];
    static circular_buffer_t uart_rx_buffer, uart_tx_buffer;
    circular_buffer_init(&uart_rx_buffer, uart_rx_data, 256);
    circular_buffer_init(&uart_tx_buffer, uart_tx_data, 256);
    comms.debug_uart = UART_init(0, &uart_rx_buffer, &uart_tx_buffer);
    static uint8_t usb_rx_data[256];
    static circular_buffer_t usb_rx_buffer;
    circular_buffer_init(&usb_rx_buffer, usb_rx_data, 256);
    comms.main_usb = USB_init(0, &usb_rx_buffer);
    if (!comms.debug_uart || !comms.main_usb) {
        return false;
    }
    UART_set_rx_callback(comms.debug_uart, uart_data_callback, 1);
    USB_set_rx_callback(comms.main_usb, usb_data_callback, 1);
    return true;
}

// Main communication loop
void communication_loop(void) {
    while (1) {
        USB_task(comms.main_usb);  // Handle USB events
        // UART events are handled by DMA and interrupts
        // Other application logic here
    }
}

// Cleanup all interfaces
void cleanup_comms(void) {
    if (comms.debug_uart) {
        UART_deinit(comms.debug_uart);
        comms.debug_uart = NULL;
    }
    if (comms.main_usb) {
        USB_deinit(comms.main_usb);
        comms.main_usb = NULL;
    }
}
```

## UART-Specific Implementation Details

- **Hardware Mapping**: Each UART bus is mapped to a specific STM32 USART peripheral, GPIO pins, and DMA channels.
- **DMA-based TX/RX**: Uses DMA for efficient, non-blocking data transfer.
- **Interrupts**: Handles TX complete, RX complete, and idle line detection.
- **Multiple Buses**: Supports multiple independent UART buses.

| Bus Index | STM32 USART | GPIO Pins | GPDMA1 Channels | Baud Rate |
|-----------|-------------|-----------|-----------------|-----------|
| 0         | USART1      | PA9/PA10  | CH0/CH1         | 115200    |
| 1         | USART2      | PA2/PA3   | CH2/CH3         | 115200    |
| 2         | USART3      | PD8/PD9   | CH4/CH5         | 115200    |

## USB-Specific Implementation Details

- **Hardware Mapping**: Uses STM32H563xx USB_DRD_FS controller (PA11/PA12), MCU unique ID for serial number, HSI48 clock.
- **CDC Class**: Implements USB CDC (virtual serial port) functionality.
- **TinyUSB Integration**: Uses TinyUSB stack for USB protocol handling.
- **Single Interface**: Currently only one USB interface is supported, but the API is future-proofed for more.

| Bus Index | USB Controller | GPIO Pins | Serial Number Source | Clock Source |
|-----------|----------------|-----------|---------------------|--------------|
| 0         | USB_DRD_FS     | PA11/PA12 | MCU Unique ID       | HSI48        |

## Best Practices

- **Buffer Sizing**:
  - RX buffers: Size based on expected data bursts and processing speed (UART and USB)
  - TX buffers: Only required for UART
  - Use power-of-2 buffer sizes for optimal circular buffer performance
- **Error Handling**:
  - Always check return values from `*_init()`
  - Verify bus/interface count before initialization
  - Handle partial read/write operations appropriately
- **Resource Management**:
  - Call `*_deinit()` to clean up resources
  - Set handle pointers to NULL after deinitialization
  - Use static buffers to avoid stack overflow with large buffer sizes
- **Performance Optimization**:
  - For USB, call `USB_task()` regularly (at least every 1ms)
  - Use callbacks for time-critical protocol implementations
  - Minimize data copying by reading directly into application buffers
  - Monitor buffer utilization to prevent overflow conditions
- **Integration**:
  - Both drivers use identical handle-based patterns for consistent code
  - Can be used simultaneously without interference
  - Share the same circular buffer implementation for code reuse
