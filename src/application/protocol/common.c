/**
 * @file common.c
 * @brief SCPI protocol common infrastructure
 *
 * This module implements the common SCPI protocol infrastructure including
 * USB communication, SCPI context management, and IEEE 488.2 commands.
 */

#include "protocol.h"

#include <stdio.h>
#include <string.h>

#include "lib/scpi/error.h"
#include "lib/scpi/scpi.h"

#include "system/bus/usb.h"
#include "system/system.h"
#include "util/error.h"
#include "util/util.h"

// Buffer sizes for USB communication (internal to this module)
enum {
    USB_RX_BUFFER_SIZE = 512,
    USB_TX_BUFFER_SIZE = 512,
    SCPI_INPUT_BUFFER_SIZE = 256,
    SCPI_ERROR_QUEUE_SIZE = 16
};

// Forward declarations of DMM functions needed by common
extern scpi_result_t scpi_cmd_configure_voltage_dc(scpi_t *context);
extern scpi_result_t scpi_cmd_initiate_voltage_dc(scpi_t *context);
extern scpi_result_t scpi_cmd_fetch_voltage_dc(scpi_t *context);
extern scpi_result_t scpi_cmd_read_voltage_dc(scpi_t *context);
extern scpi_result_t scpi_cmd_measure_voltage_dc(scpi_t *context);
extern void dmm_reset_state(void);

// Static storage for buffers
static uint8_t g_usb_rx_buffer_data[USB_RX_BUFFER_SIZE];
static CircularBuffer g_usb_rx_buffer;
static USB_Handle *g_usb_handle = nullptr;

// SCPI context and buffers (internal to protocol module)
static scpi_t g_scpi_context;
static char g_scpi_input_buffer[SCPI_INPUT_BUFFER_SIZE];
static scpi_error_t g_scpi_error_queue_data[SCPI_ERROR_QUEUE_SIZE];

// Protocol state (internal to common.c)
static bool g_protocol_initialized = false;

/**
 * @brief USB RX callback - called when data is received
 */
static void usb_rx_callback(USB_Handle *handle, uint32_t bytes_available)
{
    (void)handle; // Unused parameter
    (void)bytes_available; // Unused parameter
    // Data processing will happen in the main protocol task
}

/**
 * @brief SCPI write function - sends data via USB
 */
static size_t protocol_write(scpi_t *context, char const *data, size_t len)
{
    (void)context; // Unused parameter

    if (!g_usb_handle) {
        return 0;
    }

    return USB_write(g_usb_handle, (uint8_t const *)data, (uint32_t)len);
}

/**
 * @brief SCPI reset function
 */
static scpi_result_t protocol_reset(scpi_t *context)
{
    (void)context; // Unused parameter

    // Reset DMM state (implemented in dmm.c)
    dmm_reset_state();

    return SCPI_RES_OK;
}

// SCPI interface implementation
static scpi_interface_t g_scpi_interface = {
    .error = nullptr,
    .write = protocol_write,
    .control = nullptr,
    .flush = nullptr,
    .reset = protocol_reset,
};

// SCPI command tree
static scpi_command_t const g_SCPI_COMMANDS[] = {
    // IEEE 488.2 mandatory commands
    { "*RST", SCPI_CoreRst },
    { "*IDN?", SCPI_CoreIdnQ },
    { "*TST?", SCPI_CoreTstQ },
    { "*CLS", SCPI_CoreCls },
    { "*ESE", SCPI_CoreEse },
    { "*ESE?", SCPI_CoreEseQ },
    { "*ESR?", SCPI_CoreEsrQ },
    { "*OPC", SCPI_CoreOpc },
    { "*OPC?", SCPI_CoreOpcQ },
    { "*SRE", SCPI_CoreSre },
    { "*SRE?", SCPI_CoreSreQ },
    { "*STB?", SCPI_CoreStbQ },
    { "*WAI", SCPI_CoreWai },

    /* Required SCPI commands (SCPI std V1999.0 4.2.1) */
    { "SYSTem:ERRor[:NEXT]?", SCPI_SystemErrorNextQ },
    { "SYSTem:ERRor:COUNt?", SCPI_SystemErrorCountQ },
    { "SYSTem:VERSion?", SCPI_SystemVersionQ },

    // Instrument-specific commands (implemented in dmm.c)
    { "CONFigure[:VOLTage][:DC]", scpi_cmd_configure_voltage_dc },
    { "INITiate[:VOLTage][:DC]", scpi_cmd_initiate_voltage_dc },
    { "FETCh[:VOLTage][:DC]?", scpi_cmd_fetch_voltage_dc },
    { "READ[:VOLTage][:DC]?", scpi_cmd_read_voltage_dc },
    { "MEASure[:VOLTage][:DC]?", scpi_cmd_measure_voltage_dc },

    SCPI_CMD_LIST_END
};

/**
 * @brief Initialize the SCPI protocol
 */
bool protocol_init(void)
{
    if (g_protocol_initialized) {
        return true;
    }

    // Initialize USB RX buffer
    circular_buffer_init(
        &g_usb_rx_buffer, g_usb_rx_buffer_data, USB_RX_BUFFER_SIZE
    );

    // Initialize USB interface
    g_usb_handle = USB_init(0, &g_usb_rx_buffer);
    if (!g_usb_handle) {
        return false;
    }

    // Set USB RX callback
    USB_set_rx_callback(g_usb_handle, usb_rx_callback, 1);

    // Initialize SCPI context
    SCPI_Init(
        &g_scpi_context,
        g_SCPI_COMMANDS,
        &g_scpi_interface,
        scpi_units_def,
        "FOSSASIA",
        "PSLab",
        "1.0",
        "v1.0.0",
        g_scpi_input_buffer,
        SCPI_INPUT_BUFFER_SIZE,
        g_scpi_error_queue_data,
        SCPI_ERROR_QUEUE_SIZE
    );

    g_protocol_initialized = true;
    return true;
}

/**
 * @brief Deinitialize the SCPI protocol
 */
void protocol_deinit(void)
{
    if (!g_protocol_initialized) {
        return;
    }

    // Deinitialize USB
    if (g_usb_handle) {
        USB_deinit(g_usb_handle);
        g_usb_handle = nullptr;
    }

    protocol_reset((scpi_t *)0);
    g_protocol_initialized = false;
}

/**
 * @brief Main protocol task - processes USB data and SCPI commands
 */
void protocol_task(void)
{
    if (!g_protocol_initialized || !g_usb_handle) {
        return;
    }

    // Step USB task
    USB_task(g_usb_handle);

    // Process incoming USB data
    if (USB_rx_ready(g_usb_handle)) {
        uint8_t buffer[64];
        uint32_t bytes_read = USB_read(g_usb_handle, buffer, sizeof(buffer));

        if (bytes_read > 0) {
            // Feed data to SCPI parser
            SCPI_Input(&g_scpi_context, (char *)buffer, (int)bytes_read);
        }
    }
}

/**
 * @brief Check if protocol is initialized
 */
bool protocol_is_initialized(void) { return g_protocol_initialized; }