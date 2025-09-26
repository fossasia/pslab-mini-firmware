/**
 * @file dmm.h
 * @brief Test header for DMM module with concrete type definitions for CMock
 *
 * This test header provides concrete type definitions for opaque handles
 * to enable CMock to generate proper mocks.
 */

#ifndef TEST_DMM_H
#define TEST_DMM_H

#include <stdint.h>

#include "system/bus/usb.h"
#include "system/instrument/dmm.h"


#ifdef __cplusplus
extern "C" {
#endif

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

#endif // TEST_DMM_H