/**
 * @file common.c
 * @brief SCPI common infrastructure
 *
 * This module implements the common SCPI protocol infrastructure including
 * USB communication, SCPI context management, and IEEE 488.2 commands.
 */

#include "application/protocol.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "scpi/error.h"
#include "scpi/scpi.h"

#include "application/communication_commands.h"
#include "application/dso_commands.h"
#include "application/logic_analyser_commands.h"
#include "platform/status_led.h"
#include "platform/usb_cdc.h"
#include "system/transport.h"
#include "util/logging.h"

// Buffer sizes for USB communication (internal to this module)
enum {
    USB_RX_CHUNK_SIZE = 64,
    SCPI_INPUT_BUFFER_SIZE = 256,
    SCPI_ERROR_QUEUE_SIZE = 16
};

// Forward declarations of logic analyser functions needed by common
extern scpi_result_t scpi_cmd_configure_logic_analyser_pinbase(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_logic_analyser_pinbase_q(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_logic_analyser_pincount(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_logic_analyser_pincount_q(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_logic_analyser_samples(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_logic_analyser_samples_q(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_logic_analyser_divider(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_logic_analyser_divider_q(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_logic_analyser_trigger_pin(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_logic_analyser_trigger_pin_q(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_logic_analyser_trigger_level(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_logic_analyser_trigger_level_q(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_logic_analyser_trigger_mode(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_logic_analyser_trigger_mode_q(scpi_t *context);
extern scpi_result_t scpi_cmd_initiate_logic_analyser(scpi_t *context);
extern scpi_result_t scpi_cmd_fetch_logic_analyser_data_q(scpi_t *context);
extern scpi_result_t scpi_cmd_read_logic_analyser_q(scpi_t *context);
extern scpi_result_t scpi_cmd_status_logic_analyser_q(scpi_t *context);
extern scpi_result_t scpi_cmd_stream_logic_analyser_start(scpi_t *context);
extern scpi_result_t scpi_cmd_stream_logic_analyser_stop(scpi_t *context);
extern scpi_result_t scpi_cmd_stream_logic_analyser_status_q(scpi_t *context);

// Forward declarations of oscilloscope functions needed by common
extern scpi_result_t scpi_cmd_configure_dso_channel(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_dso_channel_q(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_dso_gpio_q(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_dso_samples(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_dso_samples_q(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_dso_rate(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_dso_rate_q(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_dso_trigger_level(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_dso_trigger_level_q(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_dso_trigger_mode(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_dso_trigger_mode_q(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_dso_trigger_slope(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_dso_trigger_slope_q(scpi_t *context);
extern scpi_result_t scpi_cmd_initiate_dso(scpi_t *context);
extern scpi_result_t scpi_cmd_fetch_dso_data_q(scpi_t *context);
extern scpi_result_t scpi_cmd_read_dso_q(scpi_t *context);
extern scpi_result_t scpi_cmd_status_dso_q(scpi_t *context);
extern scpi_result_t scpi_cmd_stream_dso_start(scpi_t *context);
extern scpi_result_t scpi_cmd_stream_dso_stop(scpi_t *context);
extern scpi_result_t scpi_cmd_stream_dso_status_q(scpi_t *context);

// Forward declarations of test signal functions needed by common
extern scpi_result_t scpi_cmd_test_square(scpi_t *context);
extern scpi_result_t scpi_cmd_test_square_q(scpi_t *context);
extern scpi_result_t scpi_cmd_test_square_configure(scpi_t *context);
extern scpi_result_t scpi_cmd_test_square_pin_q(scpi_t *context);
extern scpi_result_t scpi_cmd_test_square_frequency_q(scpi_t *context);

// SCPI context and buffers (internal to protocol module)
static scpi_t g_scpi_context;
static char g_scpi_input_buffer[SCPI_INPUT_BUFFER_SIZE];
static scpi_error_t g_scpi_error_queue_data[SCPI_ERROR_QUEUE_SIZE];

// Protocol state (internal to common.c)
static bool g_protocol_initialized = false;

/**
 * @brief SCPI write function - sends data via USB
 */
static size_t protocol_write(scpi_t *context, char const *data, size_t len)
{
    (void)context; // Unused parameter
    return usb_cdc_write((uint8_t const *)data, len);
}

/**
 * @brief SCPI reset function
 */
static scpi_result_t protocol_reset(scpi_t *context)
{
    (void)context; // Unused parameter
    la_reset_state();
    dso_commands_reset();
    return SCPI_RES_OK;
}

// SCPI interface implementation
static scpi_interface_t g_scpi_interface = {
    .error = NULL,
    .write = protocol_write,
    .control = NULL,
    .flush = NULL,
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

    // Communication transport commands
    { "COMM:TRANsport", scpi_cmd_comm_transport },
    { "COMM:TRANsport?", scpi_cmd_comm_transport_q },
    { "COMM:WIFI:STATus?", scpi_cmd_comm_wifi_status_q },

    // Logic analyser commands
    { "LA:CONFigure:PINBase", scpi_cmd_configure_logic_analyser_pinbase },
    { "LA:CONFigure:PINBase?", scpi_cmd_configure_logic_analyser_pinbase_q },
    { "LA:CONFigure:PINCount", scpi_cmd_configure_logic_analyser_pincount },
    { "LA:CONFigure:PINCount?", scpi_cmd_configure_logic_analyser_pincount_q },
    { "LA:CONFigure:SAMPles", scpi_cmd_configure_logic_analyser_samples },
    { "LA:CONFigure:SAMPles?", scpi_cmd_configure_logic_analyser_samples_q },
    { "LA:CONFigure:DIVider", scpi_cmd_configure_logic_analyser_divider },
    { "LA:CONFigure:DIVider?", scpi_cmd_configure_logic_analyser_divider_q },
    { "LA:CONFigure:TRIGger:PIN", scpi_cmd_configure_logic_analyser_trigger_pin },
    { "LA:CONFigure:TRIGger:PIN?", scpi_cmd_configure_logic_analyser_trigger_pin_q },
    { "LA:CONFigure:TRIGger:LEVel", scpi_cmd_configure_logic_analyser_trigger_level },
    { "LA:CONFigure:TRIGger:LEVel?", scpi_cmd_configure_logic_analyser_trigger_level_q },
    { "LA:CONFigure:TRIGger:MODE", scpi_cmd_configure_logic_analyser_trigger_mode },
    { "LA:CONFigure:TRIGger:MODE?", scpi_cmd_configure_logic_analyser_trigger_mode_q },
    { "LA:INITiate", scpi_cmd_initiate_logic_analyser },
    { "LA:FETCh[:DATa]?", scpi_cmd_fetch_logic_analyser_data_q },
    { "LA:READ?", scpi_cmd_read_logic_analyser_q },
    { "LA:STATus?", scpi_cmd_status_logic_analyser_q },
    { "LA:STREAM:STARt", scpi_cmd_stream_logic_analyser_start },
    { "LA:STREAM:STOP", scpi_cmd_stream_logic_analyser_stop },
    { "LA:STREAM:STATus?", scpi_cmd_stream_logic_analyser_status_q },

    // Oscilloscope commands
    { "DSO:CONFigure:CHANnel", scpi_cmd_configure_dso_channel },
    { "DSO:CONFigure:CHANnel?", scpi_cmd_configure_dso_channel_q },
    { "DSO:CONFigure:GPIO?", scpi_cmd_configure_dso_gpio_q },
    { "DSO:CONFigure:SAMPles", scpi_cmd_configure_dso_samples },
    { "DSO:CONFigure:SAMPles?", scpi_cmd_configure_dso_samples_q },
    { "DSO:CONFigure:RATE", scpi_cmd_configure_dso_rate },
    { "DSO:CONFigure:RATE?", scpi_cmd_configure_dso_rate_q },
    { "DSO:CONFigure:TRIGger:LEVel", scpi_cmd_configure_dso_trigger_level },
    { "DSO:CONFigure:TRIGger:LEVel?", scpi_cmd_configure_dso_trigger_level_q },
    { "DSO:CONFigure:TRIGger:MODE", scpi_cmd_configure_dso_trigger_mode },
    { "DSO:CONFigure:TRIGger:MODE?", scpi_cmd_configure_dso_trigger_mode_q },
    { "DSO:CONFigure:TRIGger:SLOPe", scpi_cmd_configure_dso_trigger_slope },
    { "DSO:CONFigure:TRIGger:SLOPe?", scpi_cmd_configure_dso_trigger_slope_q },
    { "DSO:INITiate", scpi_cmd_initiate_dso },
    { "DSO:FETCh[:DATa]?", scpi_cmd_fetch_dso_data_q },
    { "DSO:READ?", scpi_cmd_read_dso_q },
    { "DSO:STATus?", scpi_cmd_status_dso_q },
    { "DSO:STREAM:STARt", scpi_cmd_stream_dso_start },
    { "DSO:STREAM:STOP", scpi_cmd_stream_dso_stop },
    { "DSO:STREAM:STATus?", scpi_cmd_stream_dso_status_q },

    // Built-in test signal commands
    { "TEST:SQUare", scpi_cmd_test_square },
    { "TEST:SQUare?", scpi_cmd_test_square_q },
    { "TEST:SQUare:CONFigure", scpi_cmd_test_square_configure },
    { "TEST:SQUare:PIN?", scpi_cmd_test_square_pin_q },
    { "TEST:SQUare:FREQuency?", scpi_cmd_test_square_frequency_q },

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

    LOG_INIT("SCPI protocol");

    // Initialize SCPI context
    SCPI_Init(
        &g_scpi_context,
        g_SCPI_COMMANDS,
        &g_scpi_interface,
        scpi_units_def,
        "FOSSASIA",
        "PSLab Pico",
        "1.0",
        "v0.1.0",
        g_scpi_input_buffer,
        SCPI_INPUT_BUFFER_SIZE,
        g_scpi_error_queue_data,
        SCPI_ERROR_QUEUE_SIZE
    );

    g_protocol_initialized = true;
    LOG_INFO("SCPI protocol initialized");
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

    LOG_DEINIT("SCPI protocol");
    protocol_reset((scpi_t *)0);
    g_protocol_initialized = false;
    LOG_DEBUG("SCPI protocol deinitialized");
}

static void write_usb_stream_frame(
    char const *prefix,
    uint32_t sequence,
    uint8_t const *data,
    size_t len
)
{
    char frame_header[48];
    snprintf(
        frame_header,
        sizeof(frame_header),
        "%s %lu %lu\n",
        prefix,
        (unsigned long)sequence,
        (unsigned long)len
    );
    usb_cdc_write((uint8_t const *)frame_header, strlen(frame_header));
    SCPI_ResultArbitraryBlock(&g_scpi_context, data, len);
    usb_cdc_write((uint8_t const *)"\n", 1);
}

static void write_logic_analyser_stream_frame(
    uint32_t sequence,
    uint8_t const *data,
    size_t len
)
{
    if (transport_wifi_is_effective()) {
        TransportCaptureMeta meta = {
            .sample_rate_hz = 150000000u / la_get_divider(),
            .sample_count = la_get_samples(),
            .channel_count = la_get_pin_count(),
            .pin_base_or_channel = la_get_pin_base(),
            .trigger_mode = la_get_trigger_mode_edge() ? 1u : 2u,
            .data_format = TRANSPORT_DATA_FORMAT_LA_U32_PACKED,
        };
        (void)transport_send_capture(
            TRANSPORT_INSTRUMENT_LA,
            sequence,
            &meta,
            data,
            len
        );
        return;
    }

    write_usb_stream_frame("LA:STREAM:FRAME", sequence, data, len);
}

static void write_dso_stream_frame(
    uint32_t sequence,
    uint8_t const *data,
    size_t len
)
{
    if (transport_wifi_is_effective()) {
        TransportCaptureMeta meta = {
            .sample_rate_hz = dso_commands_get_sample_rate(),
            .sample_count = dso_commands_get_samples(),
            .channel_count = 1,
            .pin_base_or_channel = dso_commands_get_channel(),
            .trigger_mode = 0,
            .data_format = TRANSPORT_DATA_FORMAT_DSO_U16_LE,
        };
        (void)transport_send_capture(
            TRANSPORT_INSTRUMENT_DSO,
            sequence,
            &meta,
            data,
            len
        );
        return;
    }

    write_usb_stream_frame("DSO:STREAM:FRAME", sequence, data, len);
}

/**
 * @brief Main protocol task - processes USB data and SCPI commands
 */
void protocol_task(void)
{
    if (!g_protocol_initialized) {
        return;
    }

    // Step USB task
    usb_cdc_task();

    uint8_t buffer[USB_RX_CHUNK_SIZE];
    uint32_t bytes_read = usb_cdc_read(buffer, sizeof(buffer));
    if (bytes_read > 0) {
        status_led_command_received();
        SCPI_Input(&g_scpi_context, (char *)buffer, (int)bytes_read);
    }

    if (la_stream_is_enabled()) {
        uint8_t const *data;
        size_t len;
        uint32_t sequence;
        if (la_stream_next_frame(&data, &len, &sequence)) {
            write_logic_analyser_stream_frame(sequence, data, len);
        }
    }

    if (dso_commands_stream_is_enabled()) {
        uint8_t const *data;
        size_t len;
        uint32_t sequence;
        if (dso_commands_stream_next_frame(&data, &len, &sequence)) {
            write_dso_stream_frame(sequence, data, len);
        }
    }
}

/**
 * @brief Check if protocol is initialized
 */
bool protocol_is_initialized(void) { return g_protocol_initialized; }
