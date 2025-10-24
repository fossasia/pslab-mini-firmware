/**
 * @file opaque.h
 * @brief Test header for DMM module with concrete type definitions for CMock
 *
 * This test header provides concrete type definitions for opaque handles
 * to enable CMock to generate proper mocks.
 */

#ifndef TEST_OPAQUE_H
#define TEST_OPAQUE_H

#include <stdint.h>

#include "platform/uart_ll.h"
#include "system/bus/uart.h"
#include "system/bus/usb.h"
#include "system/instrument/dmm.h"
#include "system/instrument/dso.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief UART handle structure (concrete definition for testing)
 */
struct UART_Handle {
    UART_Bus bus_id;
    CircularBuffer *rx_buffer;
    CircularBuffer *tx_buffer;
    CircularBuffer *original_tx_buffer; /* Stored during passthrough */
    uint32_t volatile rx_dma_head;
    UART_RxCallback rx_callback;
    uint32_t rx_threshold;
    bool initialized;
    UART_Handle *passthrough_target;
};

/**
 * @brief DMM handle structure (concrete definition for testing)
 */
struct DMM_Handle {
    DMM_Config config;
    uint16_t adc_value;
    bool volatile conversion_complete;
    bool initialized;
};

/**
 * @brief DSO handle structure (concrete definition for testing)
 */
struct DSO_Handle {
    DSO_Config config;
    bool running;
};

/**
 * @brief USB handle structure (concrete definition for testing)
 */
struct USB_Handle {
    uint8_t interface_id;
    CircularBuffer *rx_buffer;
    USB_RxCallback rx_callback;
    uint32_t rx_threshold;
    uint32_t tx_timeout_counter;
    bool initialized;
};

#ifdef __cplusplus
}
#endif

#endif // TEST_OPAQUE_H