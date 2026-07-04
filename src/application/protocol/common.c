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
#include "application/mixed_signal_commands.h"
#include "platform/platform.h"
#include "platform/status_led.h"
#include "platform/usb_cdc.h"
#include "system/transport.h"
#include "util/logging.h"

// Buffer sizes for USB communication (internal to this module)
enum {
    USB_RX_CHUNK_SIZE = 64,
    WIFI_RX_CHUNK_SIZE = 256,
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
extern scpi_result_t scpi_cmd_configure_logic_analyser_rate_q(scpi_t *context);
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
extern scpi_result_t scpi_cmd_metadata_logic_analyser_q(scpi_t *context);
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

// Forward declarations of mixed-signal functions needed by common
extern scpi_result_t scpi_cmd_configure_mso_digital_pinbase(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_mso_digital_pinbase_q(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_mso_digital_pincount(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_mso_digital_pincount_q(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_mso_analog_channel(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_mso_analog_channel_q(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_mso_samples(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_mso_samples_q(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_mso_rate(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_mso_rate_q(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_mso_trigger_pin(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_mso_trigger_pin_q(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_mso_trigger_level(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_mso_trigger_level_q(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_mso_trigger_mode(scpi_t *context);
extern scpi_result_t scpi_cmd_configure_mso_trigger_mode_q(scpi_t *context);
extern scpi_result_t scpi_cmd_initiate_mso(scpi_t *context);
extern scpi_result_t scpi_cmd_fetch_mso_digital_q(scpi_t *context);
extern scpi_result_t scpi_cmd_fetch_mso_analog_q(scpi_t *context);
extern scpi_result_t scpi_cmd_read_mso_digital_q(scpi_t *context);
extern scpi_result_t scpi_cmd_read_mso_analog_q(scpi_t *context);
extern scpi_result_t scpi_cmd_status_mso_q(scpi_t *context);
extern scpi_result_t scpi_cmd_metadata_mso_q(scpi_t *context);

static scpi_result_t scpi_cmd_la_wifi_read_q(scpi_t *context);
static scpi_result_t scpi_cmd_dso_wifi_read_q(scpi_t *context);
static scpi_result_t scpi_cmd_mso_wifi_read_q(scpi_t *context);

// Forward declarations of test signal functions needed by common
extern scpi_result_t scpi_cmd_test_square(scpi_t *context);
extern scpi_result_t scpi_cmd_test_square_q(scpi_t *context);
extern scpi_result_t scpi_cmd_test_square_configure(scpi_t *context);
extern scpi_result_t scpi_cmd_test_square_pin_q(scpi_t *context);
extern scpi_result_t scpi_cmd_test_square_frequency_q(scpi_t *context);
extern scpi_result_t scpi_cmd_test_analog(scpi_t *context);
extern scpi_result_t scpi_cmd_test_analog_q(scpi_t *context);
extern scpi_result_t scpi_cmd_test_analog_configure(scpi_t *context);
extern scpi_result_t scpi_cmd_test_analog_pin_q(scpi_t *context);
extern scpi_result_t scpi_cmd_test_analog_frequency_q(scpi_t *context);
extern scpi_result_t scpi_cmd_test_analog_duty_q(scpi_t *context);

// SCPI context and buffers (internal to protocol module)
static scpi_t g_scpi_context;
static char g_scpi_input_buffer[SCPI_INPUT_BUFFER_SIZE];
static scpi_error_t g_scpi_error_queue_data[SCPI_ERROR_QUEUE_SIZE];

// Protocol state (internal to common.c)
static bool g_protocol_initialized = false;

typedef enum {
    PROTOCOL_SOURCE_USB,
    PROTOCOL_SOURCE_WIFI,
} ProtocolSource;

static ProtocolSource g_active_source = PROTOCOL_SOURCE_USB;
static uint8_t g_wifi_scpi_response[512];
static size_t g_wifi_scpi_response_len;
static uint32_t g_la_wifi_capture_sequence;
static uint32_t g_dso_wifi_capture_sequence;
static uint32_t g_mso_wifi_capture_sequence;

static uint32_t logic_analyser_sample_rate_hz(void)
{
    uint32_t divider = la_get_divider();
    if (divider == 0) {
        return 0;
    }

    return PLATFORM_get_peripheral_clock_speed(PLATFORM_CLOCK_SYS) / divider;
}

static size_t protocol_flush_wifi_response(void)
{
    if (g_wifi_scpi_response_len == 0) {
        return 0;
    }

    size_t sent = transport_send_scpi_response(
        g_wifi_scpi_response,
        g_wifi_scpi_response_len
    );
    g_wifi_scpi_response_len = 0;
    return sent;
}

/**
 * @brief SCPI write function - sends data via USB
 */
static size_t protocol_write(scpi_t *context, char const *data, size_t len)
{
    (void)context; // Unused parameter
    if (g_active_source == PROTOCOL_SOURCE_WIFI) {
        if (len >= sizeof(g_wifi_scpi_response)) {
            (void)protocol_flush_wifi_response();
            return transport_send_scpi_response((uint8_t const *)data, len);
        }

        if (g_wifi_scpi_response_len + len > sizeof(g_wifi_scpi_response)) {
            (void)protocol_flush_wifi_response();
        }

        memcpy(
            &g_wifi_scpi_response[g_wifi_scpi_response_len],
            data,
            len
        );
        g_wifi_scpi_response_len += len;

        if (memchr(data, '\n', len) || memchr(data, '\r', len)) {
            (void)protocol_flush_wifi_response();
        }
        return len;
    }

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
    mso_commands_reset();
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
    { "LA:CONFigure:RATE?", scpi_cmd_configure_logic_analyser_rate_q },
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
    { "LA:METadata?", scpi_cmd_metadata_logic_analyser_q },
    { "LA:STREAM:STARt", scpi_cmd_stream_logic_analyser_start },
    { "LA:STREAM:STOP", scpi_cmd_stream_logic_analyser_stop },
    { "LA:STREAM:STATus?", scpi_cmd_stream_logic_analyser_status_q },
    { "LA:WIFI:READ?", scpi_cmd_la_wifi_read_q },

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
    { "DSO:WIFI:READ?", scpi_cmd_dso_wifi_read_q },

    // Mixed-signal commands
    { "MSO:CONFigure:DIGital:PINBase", scpi_cmd_configure_mso_digital_pinbase },
    { "MSO:CONFigure:DIGital:PINBase?", scpi_cmd_configure_mso_digital_pinbase_q },
    { "MSO:CONFigure:DIGital:PINCount", scpi_cmd_configure_mso_digital_pincount },
    { "MSO:CONFigure:DIGital:PINCount?", scpi_cmd_configure_mso_digital_pincount_q },
    { "MSO:CONFigure:ANALog:CHANnel", scpi_cmd_configure_mso_analog_channel },
    { "MSO:CONFigure:ANALog:CHANnel?", scpi_cmd_configure_mso_analog_channel_q },
    { "MSO:CONFigure:SAMPles", scpi_cmd_configure_mso_samples },
    { "MSO:CONFigure:SAMPles?", scpi_cmd_configure_mso_samples_q },
    { "MSO:CONFigure:RATE", scpi_cmd_configure_mso_rate },
    { "MSO:CONFigure:RATE?", scpi_cmd_configure_mso_rate_q },
    { "MSO:CONFigure:TRIGger:PIN", scpi_cmd_configure_mso_trigger_pin },
    { "MSO:CONFigure:TRIGger:PIN?", scpi_cmd_configure_mso_trigger_pin_q },
    { "MSO:CONFigure:TRIGger:LEVel", scpi_cmd_configure_mso_trigger_level },
    { "MSO:CONFigure:TRIGger:LEVel?", scpi_cmd_configure_mso_trigger_level_q },
    { "MSO:CONFigure:TRIGger:MODE", scpi_cmd_configure_mso_trigger_mode },
    { "MSO:CONFigure:TRIGger:MODE?", scpi_cmd_configure_mso_trigger_mode_q },
    { "MSO:INITiate", scpi_cmd_initiate_mso },
    { "MSO:FETCh:DIGital?", scpi_cmd_fetch_mso_digital_q },
    { "MSO:FETCh:ANALog?", scpi_cmd_fetch_mso_analog_q },
    { "MSO:READ:DIGital?", scpi_cmd_read_mso_digital_q },
    { "MSO:READ:ANALog?", scpi_cmd_read_mso_analog_q },
    { "MSO:STATus?", scpi_cmd_status_mso_q },
    { "MSO:METadata?", scpi_cmd_metadata_mso_q },
    { "MSO:WIFI:READ?", scpi_cmd_mso_wifi_read_q },

    // Built-in test signal commands
    { "TEST:SQUare", scpi_cmd_test_square },
    { "TEST:SQUare?", scpi_cmd_test_square_q },
    { "TEST:SQUare:CONFigure", scpi_cmd_test_square_configure },
    { "TEST:SQUare:PIN?", scpi_cmd_test_square_pin_q },
    { "TEST:SQUare:FREQuency?", scpi_cmd_test_square_frequency_q },
    { "TEST:ANALog", scpi_cmd_test_analog },
    { "TEST:ANALog?", scpi_cmd_test_analog_q },
    { "TEST:ANALog:CONFigure", scpi_cmd_test_analog_configure },
    { "TEST:ANALog:PIN?", scpi_cmd_test_analog_pin_q },
    { "TEST:ANALog:FREQuency?", scpi_cmd_test_analog_frequency_q },
    { "TEST:ANALog:DUTY?", scpi_cmd_test_analog_duty_q },

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

static void write_stream_frame(
    TransportInstrument instrument,
    char const *prefix,
    uint32_t sequence,
    uint8_t const *data,
    size_t len
)
{
    if (transport_wifi_is_effective()) {
        TransportCaptureMeta meta = {0};
        if (instrument == TRANSPORT_INSTRUMENT_LA) {
            meta = (TransportCaptureMeta){
                .sample_rate_hz = logic_analyser_sample_rate_hz(),
                .sample_count = la_get_samples(),
                .channel_count = la_get_pin_count(),
                .pin_base_or_channel = la_get_pin_base(),
                .trigger_mode = (uint32_t)la_get_trigger_mode(),
                .data_format = 1,
            };
        } else {
            meta = (TransportCaptureMeta){
                .sample_rate_hz = dso_commands_get_sample_rate(),
                .sample_count = dso_commands_get_samples(),
                .channel_count = 1,
                .pin_base_or_channel = dso_commands_get_channel(),
                .trigger_mode = 0,
                .data_format = 2,
            };
        }

        (void)transport_send_capture(instrument, sequence, &meta, data, len);
        return;
    }

    write_usb_stream_frame(prefix, sequence, data, len);
}

static scpi_result_t scpi_cmd_la_wifi_read_q(scpi_t *context)
{
    uint8_t const *data = NULL;
    size_t len = 0;
    uint32_t sequence = g_la_wifi_capture_sequence++;

    transport_set_mode(TRANSPORT_MODE_WIFI);
    if (!la_initiate() || !la_fetch(&data, &len)) {
        SCPI_ErrorPush(context, SCPI_ERROR_EXECUTION_ERROR);
        return SCPI_RES_ERR;
    }

    TransportCaptureMeta meta = {
        .sample_rate_hz = logic_analyser_sample_rate_hz(),
        .sample_count = la_get_samples(),
        .channel_count = la_get_pin_count(),
        .pin_base_or_channel = la_get_pin_base(),
        .trigger_mode = (uint32_t)la_get_trigger_mode(),
        .data_format = 1,
    };
    if (!transport_send_capture(
            TRANSPORT_INSTRUMENT_LA,
            sequence,
            &meta,
            data,
            len
        )) {
        SCPI_ErrorPush(context, SCPI_ERROR_EXECUTION_ERROR);
        return SCPI_RES_ERR;
    }

    SCPI_ResultUInt32(context, sequence);
    return SCPI_RES_OK;
}

static scpi_result_t scpi_cmd_dso_wifi_read_q(scpi_t *context)
{
    uint8_t const *data = NULL;
    size_t len = 0;
    uint32_t sequence = g_dso_wifi_capture_sequence++;

    transport_set_mode(TRANSPORT_MODE_WIFI);
    if (!dso_commands_initiate() || !dso_commands_fetch(&data, &len)) {
        SCPI_ErrorPush(context, SCPI_ERROR_EXECUTION_ERROR);
        return SCPI_RES_ERR;
    }

    TransportCaptureMeta meta = {
        .sample_rate_hz = dso_commands_get_sample_rate(),
        .sample_count = dso_commands_get_samples(),
        .channel_count = 1,
        .pin_base_or_channel = dso_commands_get_channel(),
        .trigger_mode = 0,
        .data_format = 2,
    };
    if (!transport_send_capture(
            TRANSPORT_INSTRUMENT_DSO,
            sequence,
            &meta,
            data,
            len
        )) {
        SCPI_ErrorPush(context, SCPI_ERROR_EXECUTION_ERROR);
        return SCPI_RES_ERR;
    }

    SCPI_ResultUInt32(context, sequence);
    return SCPI_RES_OK;
}

static scpi_result_t scpi_cmd_mso_wifi_read_q(scpi_t *context)
{
    uint8_t const *digital_data = NULL;
    uint8_t const *analog_data = NULL;
    size_t digital_len = 0;
    size_t analog_len = 0;
    uint32_t sequence = g_mso_wifi_capture_sequence++;

    transport_set_mode(TRANSPORT_MODE_WIFI);
    if (!mso_commands_initiate() ||
        !mso_commands_fetch_digital(&digital_data, &digital_len) ||
        !mso_commands_fetch_analog(&analog_data, &analog_len)) {
        SCPI_ErrorPush(context, SCPI_ERROR_EXECUTION_ERROR);
        return SCPI_RES_ERR;
    }

    MixedSignalCaptureInfo const *info = mso_commands_get_last_info();
    if (!info) {
        SCPI_ErrorPush(context, SCPI_ERROR_EXECUTION_ERROR);
        return SCPI_RES_ERR;
    }

    TransportCaptureMeta digital_meta = {
        .sample_rate_hz = info->sample_rate_hz,
        .sample_count = info->sample_count,
        .channel_count = info->digital_pin_count,
        .pin_base_or_channel = info->digital_pin_base,
        .trigger_mode =
            info->trigger_mode == LOGIC_ANALYSER_TRIGGER_EDGE ? 1u : 2u,
        .data_format = 1,
    };
    TransportCaptureMeta analog_meta = {
        .sample_rate_hz = info->sample_rate_hz,
        .sample_count = info->sample_count,
        .channel_count = 1,
        .pin_base_or_channel = info->analog_channel,
        .trigger_mode =
            info->trigger_mode == LOGIC_ANALYSER_TRIGGER_EDGE ? 1u : 2u,
        .data_format = 2,
    };

    if (!transport_send_capture(
            TRANSPORT_INSTRUMENT_MSO_DIGITAL,
            sequence,
            &digital_meta,
            digital_data,
            digital_len
        ) ||
        !transport_send_capture(
            TRANSPORT_INSTRUMENT_MSO_ANALOG,
            sequence,
            &analog_meta,
            analog_data,
            analog_len
        )) {
        SCPI_ErrorPush(context, SCPI_ERROR_EXECUTION_ERROR);
        return SCPI_RES_ERR;
    }

    SCPI_ResultUInt32(context, sequence);
    return SCPI_RES_OK;
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
        g_active_source = PROTOCOL_SOURCE_USB;
        SCPI_Input(&g_scpi_context, (char *)buffer, (int)bytes_read);
    }

    uint8_t wifi_buffer[WIFI_RX_CHUNK_SIZE];
    size_t wifi_len = 0;
    if (transport_poll_scpi_command(wifi_buffer, sizeof(wifi_buffer), &wifi_len)) {
        status_led_command_received();
        g_active_source = PROTOCOL_SOURCE_WIFI;
        SCPI_Input(&g_scpi_context, (char *)wifi_buffer, (int)wifi_len);
        g_active_source = PROTOCOL_SOURCE_USB;
    }

    if (la_stream_is_enabled()) {
        uint8_t const *data;
        size_t len;
        uint32_t sequence;
        if (la_stream_next_frame(&data, &len, &sequence)) {
            write_stream_frame(
                TRANSPORT_INSTRUMENT_LA,
                "LA:STREAM:FRAME",
                sequence,
                data,
                len
            );
        }
    }

    if (dso_commands_stream_is_enabled()) {
        uint8_t const *data;
        size_t len;
        uint32_t sequence;
        if (dso_commands_stream_next_frame(&data, &len, &sequence)) {
            write_stream_frame(
                TRANSPORT_INSTRUMENT_DSO,
                "DSO:STREAM:FRAME",
                sequence,
                data,
                len
            );
        }
    }
}

/**
 * @brief Check if protocol is initialized
 */
bool protocol_is_initialized(void) { return g_protocol_initialized; }
