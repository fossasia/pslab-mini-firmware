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

#include "system/bus/usb.h"
#include "system/instrument/dmm.h"
#include "system/system.h"
#include "util/error.h"
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

/* CLEARED BY *RST BEGIN */

// Instrument handles
static DMM_Handle *g_dmm_handle = nullptr;

// DMM configuration storage
static DMM_Config g_dmm_config = DMM_CONFIG_DEFAULT;

// DMM cached voltage reading
static FIXED_Q1616 g_cached_voltage = 0;
static bool g_has_cached_voltage = false;

/* CLEARED BY *RST END */

// SCPI context and buffers
static scpi_t g_scpi_context;
static char g_scpi_input_buffer[SCPI_INPUT_BUFFER_SIZE];
static scpi_error_t g_scpi_error_queue_data[SCPI_ERROR_QUEUE_SIZE];

// Protocol state
static bool g_protocol_initialized = false;

// Forward declarations
static size_t protocol_write(scpi_t *context, char const *data, size_t len);
static scpi_result_t protocol_reset(scpi_t *context);

// Instrument-specific commands
static scpi_result_t scpi_cmd_configure_voltage_dc(scpi_t *context);
static scpi_result_t scpi_cmd_initiate_voltage_dc(scpi_t *context);
static scpi_result_t scpi_cmd_fetch_voltage_dc(scpi_t *context);
static scpi_result_t scpi_cmd_read_voltage_dc(scpi_t *context);
static scpi_result_t scpi_cmd_measure_voltage_dc(scpi_t *context);

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

    // Instrument-specific commands
    { "CONFigure[:VOLTage][:DC]", scpi_cmd_configure_voltage_dc },
    { "INITiate[:VOLTage][:DC]", scpi_cmd_initiate_voltage_dc },
    { "FETCh[:VOLTage][:DC]?", scpi_cmd_fetch_voltage_dc },
    { "READ[:VOLTage][:DC]?", scpi_cmd_read_voltage_dc },
    { "MEASure[:VOLTage][:DC]?", scpi_cmd_measure_voltage_dc },

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
 * @brief SCPI reset function
 */
static scpi_result_t protocol_reset(scpi_t *context)
{
    (void)context; // Unused parameter

    // Reset instrument state
    if (g_dmm_handle) {
        DMM_deinit(g_dmm_handle);
        g_dmm_handle = nullptr;
    }

    g_dmm_config = (DMM_Config)DMM_CONFIG_DEFAULT;
    g_cached_voltage = 0;
    g_has_cached_voltage = false;

    return SCPI_RES_OK;
}

// SCPI Instrument Implementations

/**
 * @brief CONFigure:VOLTage:DC - Configure DC voltage measurement parameters
 */
static scpi_result_t scpi_cmd_configure_voltage_dc(scpi_t *context)
{
    Error err = ERROR_NONE;
    DMM_Config config = DMM_CONFIG_DEFAULT;
    DMM_Handle *temp_handle = nullptr;

    // Parse channel parameter if provided
    SCPI_ParamUInt32(context, (uint32_t *)&config.channel, false);

    // Validate configuration by attempting to initialize and deinitialize
    TRY { temp_handle = DMM_init(&config); }
    CATCH(err)
    {
        switch (err) {
        case ERROR_INVALID_ARGUMENT:
            SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
            return SCPI_RES_ERR;
        default:
            LOG_ERROR("DMM configuration error: 0x%08X", err);
            SCPI_ErrorPush(context, SCPI_ERROR_SYSTEM_ERROR);
            return SCPI_RES_ERR;
        }
    }

    // Configuration is valid, clean up and store it
    DMM_deinit(temp_handle);
    g_dmm_config = config;

    return SCPI_RES_OK;
}

/**
 * @brief INITiate:VOLTage:DC - Initialize DMM and start voltage measurement
 */
static scpi_result_t scpi_cmd_initiate_voltage_dc(scpi_t *context)
{
    Error err = ERROR_NONE;

    // Clean up any existing DMM handle
    if (g_dmm_handle) {
        DMM_deinit(g_dmm_handle);
        g_dmm_handle = nullptr;
    }

    // Clear cached voltage since we're starting a new measurement
    g_has_cached_voltage = false;
    g_cached_voltage = 0;

    // Initialize DMM with stored configuration (defaults to DMM_CONFIG_DEFAULT
    // if never configured)
    TRY { g_dmm_handle = DMM_init(&g_dmm_config); }
    CATCH(err)
    {
        LOG_ERROR("DMM initialization error: 0x%08X", err);
        SCPI_ErrorPush(context, SCPI_ERROR_SYSTEM_ERROR);
        return SCPI_RES_ERR;
    }

    return SCPI_RES_OK;
}

static scpi_result_t fetch_new_voltage_dc(scpi_t *context)
{
    Error err = ERROR_NONE;
    FIXED_Q1616 voltage = 0;
    uint32_t timeout = SI_MILLI_DIV; // 1 second timeout
    uint32_t start_time = SYSTEM_get_tick();

    // Read ADC value with timeout
    TRY
    {
        while (!DMM_read_voltage(g_dmm_handle, &voltage)) {
            // Wait for ADC to be ready
            if (SYSTEM_get_tick() - start_time > timeout) {
                LOG_ERROR("DMM read timeout");
                SCPI_ErrorPush(context, SCPI_ERROR_SYSTEM_ERROR);
                return SCPI_RES_ERR;
            }
        }
    }
    CATCH(err)
    {
        LOG_ERROR("DMM read error: 0x%08X", err);
        SCPI_ErrorPush(context, SCPI_ERROR_SYSTEM_ERROR);
        return SCPI_RES_ERR;
    }

    // Cache the new reading
    g_cached_voltage = voltage;
    g_has_cached_voltage = true;

    // Clean up DMM handle
    DMM_deinit(g_dmm_handle);
    g_dmm_handle = nullptr;
    return SCPI_RES_OK;
}

/**
 * @brief FETCh:VOLTage:DC? - Fetch the voltage measurement result
 */
static scpi_result_t scpi_cmd_fetch_voltage_dc(scpi_t *context)
{
    // If DMM handle exists, read a new value and cache it
    if (g_dmm_handle) {
        if (fetch_new_voltage_dc(context) != SCPI_RES_OK) {
            return SCPI_RES_ERR;
        }
    }

    // If no cached value exists by now, INIT has never run
    if (!g_has_cached_voltage) {
        SCPI_ErrorPush(context, SCPI_ERROR_EXECUTION_ERROR);
        return SCPI_RES_ERR;
    }

    // Convert Q16.16 to millivolts for SCPI output
    int32_t voltage_millivolts = FIXED_TO_INT(g_cached_voltage * SI_MILLI_DIV);
    SCPI_ResultUInt32(context, voltage_millivolts);
    return SCPI_RES_OK;
}

/**
 * @brief READ:VOLTage:DC? - Initiate and fetch voltage measurement
 */
static scpi_result_t scpi_cmd_read_voltage_dc(scpi_t *context)
{
    scpi_result_t result = SCPI_RES_OK;

    // Initiate measurement
    result = scpi_cmd_initiate_voltage_dc(context);
    if (result != SCPI_RES_OK) {
        return result;
    }

    // Fetch the result
    return scpi_cmd_fetch_voltage_dc(context);
}

/**
 * @brief MEASure:VOLTage:DC? - Configure, initiate, and fetch DC voltage
 * measurement
 */
static scpi_result_t scpi_cmd_measure_voltage_dc(scpi_t *context)
{
    scpi_result_t result = SCPI_RES_OK;

    // Configure (validate the configuration)
    result = scpi_cmd_configure_voltage_dc(context);
    if (result != SCPI_RES_OK) {
        return result;
    }

    // Read (initiate + fetch)
    return scpi_cmd_read_voltage_dc(context);
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
