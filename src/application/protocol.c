#include "application/protocol.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "application/dso_commands.h"
#include "application/logic_analyser_commands.h"
#include "platform/status_led.h"
#include "platform/test_signal.h"
#include "platform/usb_cdc.h"

enum {
    RX_CHUNK_SIZE = 64,
    LINE_BUFFER_SIZE = 160,
    ERROR_BUFFER_SIZE = 64,
};

static bool initialized;
static char line_buffer[LINE_BUFFER_SIZE];
static size_t line_len;
static char last_error[ERROR_BUFFER_SIZE] = "0,\"No error\"";

static void protocol_write_text(char const *text)
{
    usb_cdc_write((uint8_t const *)text, strlen(text));
}

static void set_error(char const *error)
{
    strncpy(last_error, error, sizeof(last_error) - 1);
    last_error[sizeof(last_error) - 1] = '\0';
}

static void write_ok(void) { protocol_write_text("OK\n"); }

static void write_error(char const *error)
{
    set_error(error);
    protocol_write_text(error);
    protocol_write_text("\n");
}

static void write_uint(uint32_t value)
{
    char response[24];
    snprintf(response, sizeof(response), "%lu\n", (unsigned long)value);
    protocol_write_text(response);
}

static void write_bool(bool value) { protocol_write_text(value ? "1\n" : "0\n"); }

static void write_block(uint8_t const *data, size_t len)
{
    char header[16];
    char digits[12];
    snprintf(digits, sizeof(digits), "%lu", (unsigned long)len);
    snprintf(header, sizeof(header), "#%u%s", (unsigned)strlen(digits), digits);
    protocol_write_text(header);
    usb_cdc_write(data, len);
    protocol_write_text("\n");
}

static void write_stream_frame(uint32_t sequence, uint8_t const *data, size_t len)
{
    char line[48];
    snprintf(
        line,
        sizeof(line),
        "LA:STREAM:FRAME %lu %lu\n",
        (unsigned long)sequence,
        (unsigned long)len
    );
    protocol_write_text(line);
    write_block(data, len);
}

static void write_stream_status(void)
{
    char line[80];
    snprintf(
        line,
        sizeof(line),
        "%u,%lu,%lu\n",
        la_stream_is_enabled() ? 1u : 0u,
        (unsigned long)la_stream_get_sequence(),
        (unsigned long)la_stream_get_overruns()
    );
    protocol_write_text(line);
}

static void write_dso_stream_frame(uint32_t sequence, uint8_t const *data, size_t len)
{
    char line[48];
    snprintf(
        line,
        sizeof(line),
        "DSO:STREAM:FRAME %lu %lu\n",
        (unsigned long)sequence,
        (unsigned long)len
    );
    protocol_write_text(line);
    write_block(data, len);
}

static void write_dso_stream_status(void)
{
    char line[80];
    snprintf(
        line,
        sizeof(line),
        "%u,%lu,%lu\n",
        dso_commands_stream_is_enabled() ? 1u : 0u,
        (unsigned long)dso_commands_stream_get_sequence(),
        (unsigned long)dso_commands_stream_get_overruns()
    );
    protocol_write_text(line);
}

static char *trim(char *text)
{
    while (isspace((unsigned char)*text)) {
        ++text;
    }

    char *end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) {
        *--end = '\0';
    }

    return text;
}

static void uppercase(char *text)
{
    for (; *text; ++text) {
        *text = (char)toupper((unsigned char)*text);
    }
}

static bool parse_uint(char const *text, uint32_t *value)
{
    if (!text || !*text || !value) {
        return false;
    }

    char *end = NULL;
    unsigned long parsed = strtoul(text, &end, 0);
    while (end && isspace((unsigned char)*end)) {
        ++end;
    }

    if (!end || *end != '\0' || parsed > UINT32_MAX) {
        return false;
    }

    *value = (uint32_t)parsed;
    return true;
}

static bool parse_bool(char const *text, bool *value)
{
    uint32_t number;
    char normalized[16];

    if (parse_uint(text, &number)) {
        if (number > 1) {
            return false;
        }
        *value = number != 0;
        return true;
    }

    snprintf(normalized, sizeof(normalized), "%s", text ? text : "");
    uppercase(normalized);
    if (strcmp(normalized, "ON") == 0 || strcmp(normalized, "TRUE") == 0) {
        *value = true;
        return true;
    }
    if (strcmp(normalized, "OFF") == 0 || strcmp(normalized, "FALSE") == 0) {
        *value = false;
        return true;
    }

    return false;
}

static bool parse_trigger_mode(char const *text, bool *edge_mode)
{
    char normalized[16];

    if (!text || !*text || !edge_mode) {
        return false;
    }

    snprintf(normalized, sizeof(normalized), "%s", text);
    uppercase(normalized);

    if (strcmp(normalized, "EDGE") == 0) {
        *edge_mode = true;
        return true;
    }

    if (strcmp(normalized, "LEVEL") == 0) {
        *edge_mode = false;
        return true;
    }

    return false;
}

static bool parse_dso_trigger_mode(char const *text)
{
    char normalized[16];

    if (!text || !*text) {
        return false;
    }

    snprintf(normalized, sizeof(normalized), "%s", text);
    uppercase(normalized);

    if (strcmp(normalized, "OFF") == 0) {
        return dso_commands_set_trigger_mode_off();
    }
    if (strcmp(normalized, "LEVEL") == 0) {
        return dso_commands_set_trigger_mode_level();
    }
    if (strcmp(normalized, "EDGE") == 0) {
        return dso_commands_set_trigger_mode_edge();
    }

    return false;
}

static bool parse_dso_trigger_slope(char const *text)
{
    char normalized[16];

    if (!text || !*text) {
        return false;
    }

    snprintf(normalized, sizeof(normalized), "%s", text);
    uppercase(normalized);

    if (strcmp(normalized, "RISE") == 0 || strcmp(normalized, "RISING") == 0) {
        return dso_commands_set_trigger_slope_rising();
    }
    if (strcmp(normalized, "FALL") == 0 || strcmp(normalized, "FALLING") == 0) {
        return dso_commands_set_trigger_slope_falling();
    }

    return false;
}

static bool command_is(char const *command, char const *long_name, char const *short_name)
{
    return strcmp(command, long_name) == 0 || strcmp(command, short_name) == 0;
}

static bool command_param_u32(
    char const *args,
    bool (*setter)(uint32_t)
)
{
    uint32_t value;
    return parse_uint(args, &value) && setter(value);
}

static bool parse_two_uints(char const *args, uint32_t *first, uint32_t *second)
{
    char *end = NULL;
    unsigned long parsed_first = strtoul(args, &end, 0);
    if (end == args) {
        return false;
    }

    while (end && isspace((unsigned char)*end)) {
        ++end;
    }

    if (!end || *end == '\0') {
        return false;
    }

    char *second_end = NULL;
    unsigned long parsed_second = strtoul(end, &second_end, 0);
    if (second_end == end) {
        return false;
    }

    while (second_end && isspace((unsigned char)*second_end)) {
        ++second_end;
    }

    if (!second_end || *second_end != '\0' ||
        parsed_first > UINT32_MAX || parsed_second > UINT32_MAX) {
        return false;
    }

    *first = (uint32_t)parsed_first;
    *second = (uint32_t)parsed_second;
    return true;
}

static void handle_line(char *line)
{
    char *message = trim(line);
    if (*message == '\0') {
        return;
    }
    status_led_command_received();

    char *args = message;
    while (*args && !isspace((unsigned char)*args)) {
        ++args;
    }
    if (*args) {
        *args++ = '\0';
    }
    args = trim(args);
    uppercase(message);

    if (strcmp(message, "*IDN?") == 0) {
        protocol_write_text("FOSSASIA,PSLab Pico,1.0,v0.1.0\n");
    } else if (strcmp(message, "*RST") == 0) {
        la_reset_state();
        dso_commands_reset();
        set_error("0,\"No error\"");
        write_ok();
    } else if (strcmp(message, "*TST?") == 0 || strcmp(message, "*OPC?") == 0) {
        write_uint(0);
    } else if (strcmp(message, "*CLS") == 0) {
        set_error("0,\"No error\"");
        write_ok();
    } else if (strcmp(message, "SYST:ERR?") == 0 ||
               strcmp(message, "SYSTEM:ERROR?") == 0) {
        protocol_write_text(last_error);
        protocol_write_text("\n");
        set_error("0,\"No error\"");
    } else if (command_is(message, "LA:CONFIGURE:PINBASE?", "LA:CONF:PINB?")) {
        write_uint(la_get_pin_base());
    } else if (command_is(message, "LA:CONFIGURE:PINBASE", "LA:CONF:PINB")) {
        command_param_u32(args, la_set_pin_base) ? write_ok()
                                                 : write_error("-224,\"Illegal parameter value\"");
    } else if (command_is(message, "LA:CONFIGURE:PINCOUNT?", "LA:CONF:PINC?")) {
        write_uint(la_get_pin_count());
    } else if (command_is(message, "LA:CONFIGURE:PINCOUNT", "LA:CONF:PINC")) {
        command_param_u32(args, la_set_pin_count) ? write_ok()
                                                  : write_error("-224,\"Illegal parameter value\"");
    } else if (command_is(message, "LA:CONFIGURE:SAMPLES?", "LA:CONF:SAMP?")) {
        write_uint(la_get_samples());
    } else if (command_is(message, "LA:CONFIGURE:SAMPLES", "LA:CONF:SAMP")) {
        command_param_u32(args, la_set_samples) ? write_ok()
                                                : write_error("-224,\"Illegal parameter value\"");
    } else if (command_is(message, "LA:CONFIGURE:DIVIDER?", "LA:CONF:DIV?")) {
        write_uint(la_get_divider());
    } else if (command_is(message, "LA:CONFIGURE:DIVIDER", "LA:CONF:DIV")) {
        command_param_u32(args, la_set_divider) ? write_ok()
                                                : write_error("-224,\"Illegal parameter value\"");
    } else if (command_is(message, "LA:CONFIGURE:TRIGGER:PIN?", "LA:CONF:TRIG:PIN?")) {
        write_uint(la_get_trigger_pin());
    } else if (command_is(message, "LA:CONFIGURE:TRIGGER:PIN", "LA:CONF:TRIG:PIN")) {
        command_param_u32(args, la_set_trigger_pin) ? write_ok()
                                                    : write_error("-224,\"Illegal parameter value\"");
    } else if (command_is(message, "LA:CONFIGURE:TRIGGER:LEVEL?", "LA:CONF:TRIG:LEV?")) {
        write_bool(la_get_trigger_level());
    } else if (command_is(message, "LA:CONFIGURE:TRIGGER:LEVEL", "LA:CONF:TRIG:LEV")) {
        bool value;
        if (parse_bool(args, &value)) {
            la_set_trigger_level(value);
            write_ok();
        } else {
            write_error("-224,\"Illegal parameter value\"");
        }
    } else if (command_is(message, "LA:CONFIGURE:TRIGGER:MODE?", "LA:CONF:TRIG:MODE?")) {
        protocol_write_text(la_get_trigger_mode_edge() ? "EDGE\n" : "LEVEL\n");
    } else if (command_is(message, "LA:CONFIGURE:TRIGGER:MODE", "LA:CONF:TRIG:MODE")) {
        bool edge_mode;
        if (parse_trigger_mode(args, &edge_mode)) {
            la_set_trigger_mode_edge(edge_mode);
            write_ok();
        } else {
            write_error("-224,\"Illegal parameter value\"");
        }
    } else if (command_is(message, "LA:INITIATE", "LA:INIT")) {
        la_initiate() ? write_ok() : write_error("-200,\"Execution error\"");
    } else if (command_is(message, "LA:FETCH?", "LA:FETC?") ||
               command_is(message, "LA:FETCH:DATA?", "LA:FETC:DATA?")) {
        uint8_t const *data;
        size_t len;
        if (la_fetch(&data, &len)) {
            write_block(data, len);
        } else {
            write_error("-200,\"Execution error\"");
        }
    } else if (strcmp(message, "LA:READ?") == 0) {
        uint8_t const *data;
        size_t len;
        if (la_initiate() && la_fetch(&data, &len)) {
            write_block(data, len);
        } else {
            write_error("-200,\"Execution error\"");
        }
    } else if (command_is(message, "LA:STATUS?", "LA:STAT?")) {
        write_uint(la_status());
    } else if (strcmp(message, "LA:STREAM:START") == 0) {
        dso_commands_stream_stop();
        la_stream_start() ? write_ok() : write_error("-200,\"Execution error\"");
    } else if (strcmp(message, "LA:STREAM:STOP") == 0) {
        la_stream_stop();
        write_ok();
    } else if (strcmp(message, "LA:STREAM:STATUS?") == 0 ||
               strcmp(message, "LA:STREAM:STAT?") == 0) {
        write_stream_status();
    } else if (command_is(message, "DSO:CONFIGURE:CHANNEL?", "DSO:CONF:CHAN?")) {
        write_uint(dso_commands_get_channel());
    } else if (command_is(message, "DSO:CONFIGURE:CHANNEL", "DSO:CONF:CHAN")) {
        command_param_u32(args, dso_commands_set_channel) ? write_ok()
                                                          : write_error("-224,\"Illegal parameter value\"");
    } else if (command_is(message, "DSO:CONFIGURE:GPIO?", "DSO:CONF:GPIO?")) {
        write_uint(dso_commands_get_gpio());
    } else if (command_is(message, "DSO:CONFIGURE:SAMPLES?", "DSO:CONF:SAMP?")) {
        write_uint(dso_commands_get_samples());
    } else if (command_is(message, "DSO:CONFIGURE:SAMPLES", "DSO:CONF:SAMP")) {
        command_param_u32(args, dso_commands_set_samples) ? write_ok()
                                                          : write_error("-224,\"Illegal parameter value\"");
    } else if (command_is(message, "DSO:CONFIGURE:RATE?", "DSO:CONF:RATE?")) {
        write_uint(dso_commands_get_sample_rate());
    } else if (command_is(message, "DSO:CONFIGURE:RATE", "DSO:CONF:RATE")) {
        command_param_u32(args, dso_commands_set_sample_rate) ? write_ok()
                                                              : write_error("-224,\"Illegal parameter value\"");
    } else if (command_is(message, "DSO:CONFIGURE:TRIGGER:LEVEL?", "DSO:CONF:TRIG:LEV?")) {
        write_uint(dso_commands_get_trigger_level());
    } else if (command_is(message, "DSO:CONFIGURE:TRIGGER:LEVEL", "DSO:CONF:TRIG:LEV")) {
        command_param_u32(args, dso_commands_set_trigger_level) ? write_ok()
                                                                : write_error("-224,\"Illegal parameter value\"");
    } else if (command_is(message, "DSO:CONFIGURE:TRIGGER:MODE?", "DSO:CONF:TRIG:MODE?")) {
        protocol_write_text(dso_commands_get_trigger_mode());
        protocol_write_text("\n");
    } else if (command_is(message, "DSO:CONFIGURE:TRIGGER:MODE", "DSO:CONF:TRIG:MODE")) {
        parse_dso_trigger_mode(args) ? write_ok()
                                     : write_error("-224,\"Illegal parameter value\"");
    } else if (command_is(message, "DSO:CONFIGURE:TRIGGER:SLOPE?", "DSO:CONF:TRIG:SLOP?")) {
        protocol_write_text(dso_commands_get_trigger_slope());
        protocol_write_text("\n");
    } else if (command_is(message, "DSO:CONFIGURE:TRIGGER:SLOPE", "DSO:CONF:TRIG:SLOP")) {
        parse_dso_trigger_slope(args) ? write_ok()
                                      : write_error("-224,\"Illegal parameter value\"");
    } else if (command_is(message, "DSO:INITIATE", "DSO:INIT")) {
        dso_commands_initiate() ? write_ok() : write_error("-200,\"Execution error\"");
    } else if (command_is(message, "DSO:FETCH?", "DSO:FETC?") ||
               command_is(message, "DSO:FETCH:DATA?", "DSO:FETC:DATA?")) {
        uint8_t const *data;
        size_t len;
        if (dso_commands_fetch(&data, &len)) {
            write_block(data, len);
        } else {
            write_error("-200,\"Execution error\"");
        }
    } else if (strcmp(message, "DSO:READ?") == 0) {
        uint8_t const *data;
        size_t len;
        if (dso_commands_initiate() && dso_commands_fetch(&data, &len)) {
            write_block(data, len);
        } else {
            write_error("-200,\"Execution error\"");
        }
    } else if (command_is(message, "DSO:STATUS?", "DSO:STAT?")) {
        write_uint(dso_commands_status());
    } else if (strcmp(message, "DSO:STREAM:START") == 0) {
        la_stream_stop();
        dso_commands_stream_start() ? write_ok() : write_error("-200,\"Execution error\"");
    } else if (strcmp(message, "DSO:STREAM:STOP") == 0) {
        dso_commands_stream_stop();
        write_ok();
    } else if (strcmp(message, "DSO:STREAM:STATUS?") == 0 ||
               strcmp(message, "DSO:STREAM:STAT?") == 0) {
        write_dso_stream_status();
    } else if (strcmp(message, "TEST:SQUARE?") == 0) {
        protocol_write_text(test_signal_is_enabled() ? "1\n" : "0\n");
    } else if (strcmp(message, "TEST:SQUARE") == 0) {
        bool enable;
        if (!parse_bool(args, &enable)) {
            write_error("-224,\"Illegal parameter value\"");
        } else if (enable && test_signal_start(
                                TEST_SIGNAL_DEFAULT_PIN,
                                TEST_SIGNAL_DEFAULT_FREQUENCY_HZ
                            )) {
            write_ok();
        } else if (!enable) {
            test_signal_stop();
            write_ok();
        } else {
            write_error("-200,\"Execution error\"");
        }
    } else if (strcmp(message, "TEST:SQUARE:CONFIGURE") == 0 ||
               strcmp(message, "TEST:SQUARE:CONF") == 0) {
        uint32_t pin;
        uint32_t frequency;
        if (!parse_two_uints(args, &pin, &frequency) || pin > 29 || frequency == 0) {
            write_error("-224,\"Illegal parameter value\"");
        } else if (test_signal_start(pin, frequency)) {
            write_ok();
        } else {
            write_error("-200,\"Execution error\"");
        }
    } else if (strcmp(message, "TEST:SQUARE:PIN?") == 0) {
        write_uint(test_signal_get_pin());
    } else if (strcmp(message, "TEST:SQUARE:FREQUENCY?") == 0 ||
               strcmp(message, "TEST:SQUARE:FREQ?") == 0) {
        write_uint(test_signal_get_frequency_hz());
    } else {
        write_error("-113,\"Undefined header\"");
    }
}

bool protocol_init(void)
{
    if (initialized) {
        return true;
    }

    usb_cdc_init();
    status_led_init();
    test_signal_init();
    initialized = true;
    return true;
}

void protocol_task(void)
{
    if (!initialized) {
        return;
    }

    uint8_t buffer[RX_CHUNK_SIZE];
    usb_cdc_task();

    uint32_t bytes_read = usb_cdc_read(buffer, sizeof(buffer));
    for (uint32_t i = 0; i < bytes_read; ++i) {
        char ch = (char)buffer[i];
        if (ch == '\r' || ch == '\n') {
            line_buffer[line_len] = '\0';
            handle_line(line_buffer);
            line_len = 0;
        } else if (line_len + 1 < sizeof(line_buffer)) {
            line_buffer[line_len++] = ch;
        } else {
            line_len = 0;
            set_error("-363,\"Input buffer overrun\"");
        }
    }

    if (la_stream_is_enabled()) {
        uint8_t const *data;
        size_t len;
        uint32_t sequence;
        if (la_stream_next_frame(&data, &len, &sequence)) {
            write_stream_frame(sequence, data, len);
        }
    } else if (dso_commands_stream_is_enabled()) {
        uint8_t const *data;
        size_t len;
        uint32_t sequence;
        if (dso_commands_stream_next_frame(&data, &len, &sequence)) {
            write_dso_stream_frame(sequence, data, len);
        }
    }
}

void protocol_deinit(void)
{
    if (!initialized) {
        return;
    }

    la_reset_state();
    dso_commands_reset();
    initialized = false;
}

bool protocol_is_initialized(void) { return initialized; }
