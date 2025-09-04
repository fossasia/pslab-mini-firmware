/**
 * @file protocol.h
 * @brief SCPI-based communication protocol interface
 *
 * This header defines the public API for the SCPI communication protocol.
 * The protocol uses USB CDC as the transport layer and provides standard
 * IEEE 488.2 commands plus instrument-specific functionality.
 */

#ifndef PSLAB_PROTOCOL_H
#define PSLAB_PROTOCOL_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the SCPI protocol
 *
 * This function initializes the USB interface, ADC, and SCPI parser.
 * Must be called before using any other protocol functions.
 *
 * @return true on success, false on failure
 */
bool protocol_init(void);

/**
 * @brief Deinitialize the SCPI protocol
 *
 * Cleans up all resources used by the protocol including USB and ADC.
 */
void protocol_deinit(void);

/**
 * @brief Main protocol task
 *
 * This function should be called periodically (typically in the main loop)
 * to process incoming USB data and handle SCPI commands.
 */
void protocol_task(void);

/**
 * @brief Check if protocol is initialized
 *
 * @return true if protocol is initialized and ready, false otherwise
 */
bool protocol_is_initialized(void);

#ifdef __cplusplus
}
#endif

#endif // PSLAB_PROTOCOL_H
