# UART Driver Architecture

This document describes the architecture and usage of the PSLab's UART driver.

## Overview

The UART driver provides a handle-based API for managing multiple UART bus instances. Each instance operates independently with its own buffers, callbacks, and hardware resources, enabling simultaneous communication on multiple UART interfaces.

## Architecture Design

The driver is structured in three layers to provide clean separation between hardware specifics and application logic:

### 1. Hardware Abstraction Layer (`uart_ll.h`/`uart_ll.c`)
- **Purpose**: Direct hardware interface for STM32H563xx UART peripherals
- **Features**: 
  - Bus enumeration with `uart_bus_t` enum (internal to implementation)
  - DMA-based transmission and reception for optimal performance
  - Interrupt handling for TX complete, RX complete, and idle line detection
  - Hardware-specific initialization and configuration

### 2. Hardware-Independent Layer (`uart.c`)
- **Purpose**: Manages UART instances and provides the public API
- **Features**:
  - Opaque handle-based design for instance management
  - Circular buffer management for each instance
  - Callback routing and management
  - Thread-safe buffer operations
  - Handle lifecycle management

### 3. Public API (`uart.h`)
- **Purpose**: Interface for application code
- **Features**:
  - Handle-based operations (no global state)
  - Non-blocking read/write operations
  - Configurable RX callbacks for protocol implementations
  - Buffer status inquiry functions
  - Platform-agnostic design

## Key Design Principles

### Handle-Based API
Each UART instance is managed through an opaque `uart_handle_t` structure:
- **Encapsulation**: Implementation details are hidden from users
- **Instance Isolation**: Each handle maintains independent state
- **Resource Safety**: Handles prevent access to uninitialized or freed resources

### Circular Buffer Management
The driver uses circular buffers for efficient data handling:
- **Non-blocking Operations**: Read/write operations never block the calling thread
- **Automatic Wrap-around**: Buffers handle overflow gracefully
- **DMA Integration**: RX buffers work seamlessly with DMA reception

### Callback-Driven Design
Callbacks enable efficient protocol implementations:
- **Threshold-based**: RX callbacks trigger when specified byte counts are available
- **Context-aware**: Callbacks receive the handle for multi-instance support
- **Low Latency**: Callbacks fire from interrupt context for immediate response

## Hardware Mapping
In the STM32H563xx implementation, the following hardware resources are used:

| Bus Index | STM32 USART | GPIO Pins | GPDMA1 Channels | Baud Rate |
|-----------|-------------|-----------|-----------------|-----------|
| 0         | USART1      | PA9/PA10  | CH0/CH1         | 115200    |
| 1         | USART2      | PA2/PA3   | CH2/CH3         | 115200    |
| 2         | USART3      | PD8/PD9   | CH4/CH5         | 115200    |

Each instance uses dedicated DMA channels to ensure independent, high-performance operation without interference between buses.

## API Reference

### Initialization and Cleanup

```c
// Query available bus count
size_t UART_get_bus_count(void);

// Initialize a UART instance
uart_handle_t *UART_init(size_t bus, 
                         circular_buffer_t *rx_buffer, 
                         circular_buffer_t *tx_buffer);

// Clean up UART instance
void UART_deinit(uart_handle_t *handle);
```

### Data Operations

```c
// Non-blocking write
uint32_t UART_write(uart_handle_t *handle, uint8_t const *txbuf, uint32_t sz);

// Non-blocking read
uint32_t UART_read(uart_handle_t *handle, uint8_t *rxbuf, uint32_t sz);
```

### Status and Monitoring

```c
// Check if data is available
bool UART_rx_ready(uart_handle_t *handle);
uint32_t UART_rx_available(uart_handle_t *handle);

// Check transmission status
bool UART_tx_busy(uart_handle_t *handle);
uint32_t UART_tx_free_space(uart_handle_t *handle);
```

### Callback Configuration

```c
// Callback function signature
typedef void (*uart_rx_callback_t)(uart_handle_t *handle, uint32_t bytes_available);

// Set RX threshold callback
void UART_set_rx_callback(uart_handle_t *handle, 
                          uart_rx_callback_t callback, 
                          uint32_t threshold);
```

## Usage Examples

### Basic Communication

```c
#include "uart.h"
#include "bus.h"

// Set up UART for simple communication
uart_handle_t *setup_uart_communication(void) {
    // Define buffers
    static uint8_t rx_data[256];
    static uint8_t tx_data[256];
    static circular_buffer_t rx_buffer, tx_buffer;
    
    // Initialize circular buffers
    circular_buffer_init(&rx_buffer, rx_data, sizeof(rx_data));
    circular_buffer_init(&tx_buffer, tx_data, sizeof(tx_data));
    
    // Initialize UART bus 0
    uart_handle_t *uart = UART_init(0, &rx_buffer, &tx_buffer);
    if (uart == NULL) {
        // Handle initialization failure
        return NULL;
    }
    
    return uart;
}

// Simple echo server
void echo_server(uart_handle_t *uart) {
    if (UART_rx_ready(uart)) {
        uint8_t buffer[64];
        uint32_t bytes_read = UART_read(uart, buffer, sizeof(buffer));
        UART_write(uart, buffer, bytes_read);
    }
}
```

### Protocol Implementation with Callbacks

```c
#include "uart.h"
#include "bus.h"

#define PACKET_HEADER_SIZE 4
#define MAX_PACKET_SIZE 256

typedef struct {
    uart_handle_t *uart;
    uint8_t packet_buffer[MAX_PACKET_SIZE];
    uint32_t expected_length;
    bool header_received;
} protocol_state_t;

static protocol_state_t protocol;

// Callback for packet header reception
void on_header_received(uart_handle_t *handle, uint32_t bytes_available) {
    if (!protocol.header_received && bytes_available >= PACKET_HEADER_SIZE) {
        uint8_t header[PACKET_HEADER_SIZE];
        UART_read(handle, header, PACKET_HEADER_SIZE);
        
        // Parse header to get expected packet length
        protocol.expected_length = (header[2] << 8) | header[3];
        protocol.header_received = true;
        
        // Set callback for full packet
        UART_set_rx_callback(handle, on_packet_received, protocol.expected_length);
    }
}

// Callback for complete packet reception
void on_packet_received(uart_handle_t *handle, uint32_t bytes_available) {
    if (protocol.header_received && bytes_available >= protocol.expected_length) {
        // Read the complete packet
        uint32_t bytes_read = UART_read(handle, protocol.packet_buffer, protocol.expected_length);
        
        // Process packet
        process_packet(protocol.packet_buffer, bytes_read);
        
        // Reset for next packet
        protocol.header_received = false;
        UART_set_rx_callback(handle, on_header_received, PACKET_HEADER_SIZE);
    }
}

// Initialize protocol handler
void setup_protocol_handler(void) {
    static uint8_t rx_data[512];
    static uint8_t tx_data[256];
    static circular_buffer_t rx_buffer, tx_buffer;
    
    circular_buffer_init(&rx_buffer, rx_data, sizeof(rx_data));
    circular_buffer_init(&tx_buffer, tx_data, sizeof(tx_data));
    
    protocol.uart = UART_init(0, &rx_buffer, &tx_buffer);
    if (protocol.uart != NULL) {
        protocol.header_received = false;
        // Start listening for packet headers
        UART_set_rx_callback(protocol.uart, on_header_received, PACKET_HEADER_SIZE);
    }
}
```

### Multi-Instance Usage

```c
#include "uart.h"
#include "bus.h"

typedef struct {
    uart_handle_t *debug_uart;    // Bus 0: Debug output
    uart_handle_t *sensor_uart;   // Bus 1: Sensor communication
    uart_handle_t *radio_uart;    // Bus 2: Radio module
} system_uarts_t;

static system_uarts_t uarts;

// Initialize all UART instances
bool setup_all_uarts(void) {
    size_t bus_count = UART_get_bus_count();
    
    if (bus_count < 3) {
        return false; // Need at least 3 buses
    }
    
    // Buffer arrays for each instance
    static uint8_t debug_rx[128], debug_tx[128];
    static uint8_t sensor_rx[256], sensor_tx[128];
    static uint8_t radio_rx[512], radio_tx[256];
    
    static circular_buffer_t debug_rx_buf, debug_tx_buf;
    static circular_buffer_t sensor_rx_buf, sensor_tx_buf;
    static circular_buffer_t radio_rx_buf, radio_tx_buf;
    
    // Initialize buffers
    circular_buffer_init(&debug_rx_buf, debug_rx, sizeof(debug_rx));
    circular_buffer_init(&debug_tx_buf, debug_tx, sizeof(debug_tx));
    circular_buffer_init(&sensor_rx_buf, sensor_rx, sizeof(sensor_rx));
    circular_buffer_init(&sensor_tx_buf, sensor_tx, sizeof(sensor_tx));
    circular_buffer_init(&radio_rx_buf, radio_rx, sizeof(radio_rx));
    circular_buffer_init(&radio_tx_buf, radio_tx, sizeof(radio_tx));
    
    // Initialize UART instances
    uarts.debug_uart = UART_init(0, &debug_rx_buf, &debug_tx_buf);
    uarts.sensor_uart = UART_init(1, &sensor_rx_buf, &sensor_tx_buf);
    uarts.radio_uart = UART_init(2, &radio_rx_buf, &radio_tx_buf);
    
    return (uarts.debug_uart != NULL && 
            uarts.sensor_uart != NULL && 
            uarts.radio_uart != NULL);
}

// Send debug message
void debug_printf(const char *format, ...) {
    char buffer[128];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    if (len > 0 && uarts.debug_uart != NULL) {
        UART_write(uarts.debug_uart, (uint8_t*)buffer, len);
    }
}

// Poll sensor data
void poll_sensors(void) {
    if (uarts.sensor_uart != NULL && UART_rx_ready(uarts.sensor_uart)) {
        uint8_t sensor_data[64];
        uint32_t bytes = UART_read(uarts.sensor_uart, sensor_data, sizeof(sensor_data));
        process_sensor_data(sensor_data, bytes);
    }
}

// Cleanup all instances
void cleanup_uarts(void) {
    if (uarts.debug_uart) {
        UART_deinit(uarts.debug_uart);
        uarts.debug_uart = NULL;
    }
    if (uarts.sensor_uart) {
        UART_deinit(uarts.sensor_uart);
        uarts.sensor_uart = NULL;
    }
    if (uarts.radio_uart) {
        UART_deinit(uarts.radio_uart);
        uarts.radio_uart = NULL;
    }
}
```

## Best Practices

### Buffer Sizing
- **RX Buffers**: Size based on expected data bursts and processing speed
- **TX Buffers**: Size based on output data patterns and transmission rate
- **Power of 2**: Use buffer sizes that are powers of 2 for optimal performance

### Error Handling
- Always check return values from `UART_init()`
- Verify bus count before attempting initialization
- Handle partial read/write operations appropriately

### Resource Management
- Call `UART_deinit()` to clean up resources
- Set handle pointers to NULL after deinitialization
- Use static buffers to avoid stack overflow with large buffer sizes

### Performance Optimization
- Use callbacks for time-critical protocol implementations
- Minimize data copying by reading directly into application buffers
- Monitor buffer utilization to prevent overflow conditions

The multi-instance UART driver provides a robust, scalable foundation for complex embedded applications requiring multiple serial communication
