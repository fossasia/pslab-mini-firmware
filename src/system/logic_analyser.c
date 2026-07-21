#include "system/logic_analyser.h"

uint32_t logic_analyser_bits_packed_per_word(uint32_t pin_count)
{
    enum { SHIFT_REG_WIDTH = 32 };

    if (pin_count == 0 || pin_count > SHIFT_REG_WIDTH) {
        return 0;
    }

    return SHIFT_REG_WIDTH - (SHIFT_REG_WIDTH % pin_count);
}

uint32_t logic_analyser_capture_word_count(
    uint32_t pin_count,
    uint32_t sample_count
)
{
    uint32_t bits_per_word = logic_analyser_bits_packed_per_word(pin_count);
    if (bits_per_word == 0) {
        return 0;
    }

    uint32_t total_bits = sample_count * pin_count;
    total_bits += bits_per_word - 1;
    return total_bits / bits_per_word;
}

static void copy_capture_info(
    LogicAnalyserCaptureInfo *dst,
    LogicAnalyserLLCaptureInfo const *src
)
{
    if (!dst || !src) {
        return;
    }

    *dst = (LogicAnalyserCaptureInfo){
        .pin_base = src->pin_base,
        .pin_count = src->pin_count,
        .sample_count = src->sample_count,
        .word_count = src->word_count,
        .bits_per_word = src->bits_per_word,
    };
}

bool logic_analyser_configure(LogicAnalyser *la, LogicAnalyserConfig const *config)
{
    if (!la || !config || config->pin_count == 0 || config->pin_count > 32 ||
        config->clk_div < 1.0f) {
        return false;
    }

    if (la->initialized && logic_analyser_is_busy(la)) {
        return false;
    }

    LogicAnalyserLLConfig ll_config;
    logic_analyser_ll_default_config(
        &ll_config,
        config->pin_base,
        config->pin_count,
        config->clk_div
    );

    if (!logic_analyser_ll_configure(&la->platform, &ll_config)) {
        return false;
    }

    la->config = *config;
    la->initialized = true;
    return true;
}

bool logic_analyser_init(LogicAnalyser *la, LogicAnalyserConfig const *config)
{
    if (!la || !config) {
        return false;
    }

    *la = (LogicAnalyser){0};

    LogicAnalyserLLConfig ll_config;
    logic_analyser_ll_default_config(
        &ll_config,
        config->pin_base,
        config->pin_count,
        config->clk_div
    );

    if (!logic_analyser_ll_init(&la->platform, &ll_config)) {
        return false;
    }

    la->config = *config;
    la->initialized = true;
    return true;
}

void logic_analyser_deinit(LogicAnalyser *la)
{
    if (!la || !la->initialized) {
        return;
    }

    logic_analyser_ll_deinit(&la->platform);
    la->initialized = false;
}

bool logic_analyser_is_busy(LogicAnalyser const *la)
{
    return la && la->initialized &&
           logic_analyser_ll_is_busy(&la->platform);
}

bool logic_analyser_capture(
    LogicAnalyser *la,
    uint32_t trigger_pin,
    bool trigger_level,
    LogicAnalyserTriggerMode trigger_mode,
    uint32_t *capture_buf,
    uint32_t sample_count,
    LogicAnalyserCaptureInfo *info
)
{
    if (!logic_analyser_capture_start(
            la,
            trigger_pin,
            trigger_level,
            trigger_mode,
            capture_buf,
            sample_count,
            info,
            true
        )) {
        return false;
    }

    logic_analyser_ll_capture_wait(&la->platform);
    return logic_analyser_capture_complete(la);
}

bool logic_analyser_capture_start(
    LogicAnalyser *la,
    uint32_t trigger_pin,
    bool trigger_level,
    LogicAnalyserTriggerMode trigger_mode,
    uint32_t *capture_buf,
    uint32_t sample_count,
    LogicAnalyserCaptureInfo *info,
    bool wait_for_trigger
)
{
    if (!la || !la->initialized || !capture_buf || sample_count == 0 ||
        logic_analyser_is_busy(la)) {
        return false;
    }

    uint32_t bits_per_word =
        logic_analyser_bits_packed_per_word(la->config.pin_count);
    uint32_t word_count = logic_analyser_capture_word_count(
        la->config.pin_count,
        sample_count
    );
    if (word_count == 0 || bits_per_word == 0) {
        return false;
    }

    LogicAnalyserLLCaptureInfo ll_info;
    bool started = logic_analyser_ll_capture_start(
        &la->platform,
        trigger_pin,
        trigger_level,
        trigger_mode == LOGIC_ANALYSER_TRIGGER_EDGE,
        capture_buf,
        sample_count,
        word_count,
        bits_per_word,
        &ll_info,
        wait_for_trigger
    );
    if (!started) {
        return false;
    }

    copy_capture_info(info, &ll_info);
    return true;
}

bool logic_analyser_capture_complete(LogicAnalyser *la)
{
    if (!la || !la->initialized) {
        return false;
    }

    return logic_analyser_ll_capture_complete(&la->platform);
}

void logic_analyser_capture_abort(LogicAnalyser *la)
{
    if (!la || !la->initialized) {
        return;
    }

    logic_analyser_ll_capture_abort(&la->platform);
}
