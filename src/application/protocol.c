/**
 * @file protocol.c
 * @brief SCPI-based communication protocol implementation
 *
 * This module implements a SCPI (Standard Commands for Programmable
 * Instruments) communication protocol using USB CDC as the backend transport.
 * It provides standard IEEE 488.2 commands and instrument-specific
 * functionality.
 */

#include <stdio.h>
#include <string.h>

#include "lib/scpi/error.h"
#include "lib/scpi/scpi.h"

#include "system/instrument/dmm.h"
#include "system/bus/usb.h"
#include "util/fixed_point.h"
#include "util/si_prefix.h"
#include "util/util.h"

// Buffer sizes for USB communication
enum ProtocolBufferSizes {
    USB_RX_BUFFER_SIZE = 512,
    USB_TX_BUFFER_SIZE = 512,
    SCPI_INPUT_BUFFER_SIZE = 256,
    SCPI_ERROR_QUEUE_SIZE = 16
};

// Static storage for buffers
static uint8_t g_usb_rx_buffer_data[USB_RX_BUFFER_SIZE];
static CircularBuffer g_usb_rx_buffer;
static USB_Handle *g_usb_handle = nullptr;

// Instrument handles
static DMM_Handle *g_dmm_handle = nullptr;

// SCPI context and buffers
static scpi_t g_scpi_context;
static char g_scpi_input_buffer[SCPI_INPUT_BUFFER_SIZE];
static scpi_error_t g_scpi_error_queue_data[SCPI_ERROR_QUEUE_SIZE];

// Protocol state
static bool g_protocol_initialized = false;

// Forward declarations
static size_t protocol_write(scpi_t *context, char const *data, size_t len);
static int protocol_error(scpi_t *context, int_fast16_t err);
static scpi_result_t protocol_control(
    scpi_t *context,
    scpi_ctrl_name_t ctrl,
    scpi_reg_val_t val
);
static scpi_result_t protocol_reset(scpi_t *context);
static scpi_result_t protocol_flush(scpi_t *context);

// SCPI command implementations
static scpi_result_t scpi_cmd_rst(scpi_t *context);
static scpi_result_t scpi_cmd_idn(scpi_t *context);
static scpi_result_t scpi_cmd_tst(scpi_t *context);
static scpi_result_t scpi_cmd_cls(scpi_t *context);
static scpi_result_t scpi_cmd_ese(scpi_t *context);
static scpi_result_t scpi_cmd_esr(scpi_t *context);
static scpi_result_t scpi_cmd_opc(scpi_t *context);
static scpi_result_t scpi_cmd_opc_q(scpi_t *context);
static scpi_result_t scpi_cmd_sre(scpi_t *context);
static scpi_result_t scpi_cmd_stb(scpi_t *context);
static scpi_result_t scpi_cmd_wai(scpi_t *context);

// Instrument-specific commands
static scpi_result_t scpi_cmd_measure_voltage_dc(scpi_t *context);
static scpi_result_t scpi_cmd_configure_voltage_dc(scpi_t *context);

// SCPI interface implementation
static scpi_interface_t g_scpi_interface = {
    .error = protocol_error,
    .write = protocol_write,
    .control = protocol_control,
    .flush = protocol_flush,
    .reset = protocol_reset,
};

// SCPI command tree
static scpi_command_t const g_SCPI_COMMANDS[] = {
    // IEEE 488.2 mandatory commands
    { "*RST", scpi_cmd_rst },
    { "*IDN?", scpi_cmd_idn },
    { "*TST?", scpi_cmd_tst },
    { "*CLS", scpi_cmd_cls },
    { "*ESE", scpi_cmd_ese },
    { "*ESE?", scpi_cmd_ese },
    { "*ESR?", scpi_cmd_esr },
    { "*OPC", scpi_cmd_opc },
    { "*OPC?", scpi_cmd_opc_q },
    { "*SRE", scpi_cmd_sre },
    { "*SRE?", scpi_cmd_sre },
    { "*STB?", scpi_cmd_stb },
    { "*WAI", scpi_cmd_wai },

    // Instrument-specific commands
    { "MEASure:VOLTage:DC?", scpi_cmd_measure_voltage_dc },
    { "CONFigure:VOLTage:DC", scpi_cmd_configure_voltage_dc },

    SCPI_CMD_LIST_END
};

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
 * @brief SCPI error handler
 */
static int protocol_error(scpi_t *context, int_fast16_t err)
{
    (void)context; // Unused parameter
    (void)err; // Unused parameter

    // Error is automatically queued by SCPI library
    return 0;
}

/**
 * @brief SCPI control function
 */
static scpi_result_t protocol_control(
    scpi_t *context,
    scpi_ctrl_name_t ctrl,
    scpi_reg_val_t val
)
{
    (void)context; // Unused parameter
    (void)ctrl; // Unused parameter
    (void)val; // Unused parameter

    return SCPI_RES_OK;
}

/**
 * @brief SCPI reset function
 */
static scpi_result_t protocol_reset(scpi_t *context)
{
    (void)context; // Unused parameter

    // Perform instrument reset
    return SCPI_RES_OK;
}

/**
 * @brief SCPI flush function
 */
static scpi_result_t protocol_flush(scpi_t *context)
{
    (void)context; // Unused parameter

    // USB writes are typically immediate, no buffering to flush
    return SCPI_RES_OK;
}

// SCPI Command Implementations

/**
 * @brief *RST - Reset instrument to default state
 */
static scpi_result_t scpi_cmd_rst(scpi_t *context)
{
    (void)context; // Unused parameter

    // Reset instrument to default state
    // TODO(@bessman): Implement actual reset functionality

    return SCPI_RES_OK;
}

/**
 * @brief *IDN? - Identification query
 */
static scpi_result_t scpi_cmd_idn(scpi_t *context)
{
    SCPI_ResultText(context, "FOSSASIA,PSLab Mini,1.0,v1.0.0");
    return SCPI_RES_OK;
}

/**
 * @brief *TST? - Self-test query
 */
static scpi_result_t scpi_cmd_tst(scpi_t *context)
{
    // Return 0 for pass, non-zero for fail
    SCPI_ResultInt32(context, 0);
    return SCPI_RES_OK;
}

/**
 * @brief *CLS - Clear status
 */
static scpi_result_t scpi_cmd_cls(scpi_t *context)
{
    (void)context; // Unused parameter

    // Clear status registers
    // TODO(@bessman): Implement actual status clearing

    return SCPI_RES_OK;
}

/**
 * @brief *ESE / *ESE? - Event Status Enable
 */
static scpi_result_t scpi_cmd_ese(scpi_t *context)
{
    int32_t param = 0;

    if (SCPI_ParamInt32(context, &param, TRUE)) {
        // Set Event Status Enable register
        // TODO(@bessman): Implement ESE register handling
        return SCPI_RES_OK;
    }
    // Query Event Status Enable register
    SCPI_ResultInt32(context, 0); // TODO(@bessman): Return actual ESE value
    return SCPI_RES_OK;
}

/**
 * @brief *ESR? - Event Status Register query
 */
static scpi_result_t scpi_cmd_esr(scpi_t *context)
{
    // Return and clear Event Status Register
    SCPI_ResultInt32(context, 0); // TODO(@bessman): Return actual ESR value
    return SCPI_RES_OK;
}

/**
 * @brief *OPC - Operation Complete command
 */
static scpi_result_t scpi_cmd_opc(scpi_t *context)
{
    (void)context; // Unused parameter

    // TODO(@bessman): Set OPC bit when operations complete
    return SCPI_RES_OK;
}

/**
 * @brief *OPC? - Operation Complete query
 */
static scpi_result_t scpi_cmd_opc_q(scpi_t *context)
{
    // Always ready for now
    SCPI_ResultInt32(context, 1);
    return SCPI_RES_OK;
}

/**
 * @brief *SRE / *SRE? - Service Request Enable
 */
static scpi_result_t scpi_cmd_sre(scpi_t *context)
{
    int32_t param = 0;

    if (SCPI_ParamInt32(context, &param, TRUE)) {
        // Set Service Request Enable register
        // TODO(@bessman): Implement SRE register handling
        return SCPI_RES_OK;
    }
    // Query Service Request Enable register
    SCPI_ResultInt32(context, 0); // TODO(@bessman): Return actual SRE value
    return SCPI_RES_OK;
}

/**
 * @brief *STB? - Status Byte query
 */
static scpi_result_t scpi_cmd_stb(scpi_t *context)
{
    // Return Status Byte register
    SCPI_ResultInt32(context, 0); // TODO(@bessman): Return actual status byte
    return SCPI_RES_OK;
}

/**
 * @brief *WAI - Wait for operations to complete
 */
static scpi_result_t scpi_cmd_wai(scpi_t *context)
{
    (void)context; // Unused parameter

    // Wait for all pending operations to complete
    // TODO(@bessman): Implement actual wait functionality

    return SCPI_RES_OK;
}

/**
 * @brief MEASure:VOLTage:DC? - Measure DC voltage using ADC
 */
static scpi_result_t scpi_cmd_measure_voltage_dc(scpi_t *context)
{
    FIXED_Q1616 voltage;

    // Read ADC value
    if (DMM_read_voltage(g_dmm_handle, &voltage)) {
        // Convert Q16.16 to millivolts for SCPI output
        int32_t voltage_millivolts = FIXED_TO_INT(voltage * SI_MILLI_DIV);
        SCPI_ResultUInt32(context, voltage_millivolts);
        return SCPI_RES_OK;
    }
    SCPI_ErrorPush(context, SCPI_ERROR_SYSTEM_ERROR);
    return SCPI_RES_ERR;
}

/**
 * @brief CONFigure:VOLTage:DC - Configure DC voltage measurement
 */
static scpi_result_t scpi_cmd_configure_voltage_dc(scpi_t *context)
{
    int32_t range_millivolts = 0;
    int32_t resolution_microvolts = 0;

    // Parse optional range parameter (in millivolts)
    if (SCPI_ParamInt32(context, &range_millivolts, TRUE)) {
        // TODO(@bessman): Configure ADC range based on parameter
        // Range is now in millivolts instead of volts
    }

    // Parse optional resolution parameter (in microvolts)
    if (SCPI_ParamInt32(context, &resolution_microvolts, TRUE)) {
        // TODO(@bessman): Configure ADC resolution based on parameter
        // Resolution is now in microvolts instead of volts
    }

    // Configure ADC for DC voltage measurement
    // TODO(@bessman): Implement actual ADC configuration

    return SCPI_RES_OK;
}

// Public Protocol Functions

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

    // Initialize DMM
    g_dmm_handle = DMM_init(&(DMM_Config)DMM_CONFIG_DEFAULT);

    // Initialize SCPI context
    SCPI_Init(
        &g_scpi_context,
        g_SCPI_COMMANDS,
        &g_scpi_interface,
        scpi_units_def,
        "PSLAB",
        "Mini",
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

    // Stop and deinitialize DMM
    DMM_deinit(g_dmm_handle);
    g_dmm_handle = nullptr;

    // Deinitialize USB
    if (g_usb_handle) {
        USB_deinit(g_usb_handle);
        g_usb_handle = nullptr;
    }

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
