#include "platform/logic_analyser_ll.h"

#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/structs/bus_ctrl.h"
#include "pico/stdlib.h"

static PIO config_pio(LogicAnalyserLLConfig const *config)
{
    return config && config->pio ? (PIO)config->pio : pio0;
}

void logic_analyser_ll_default_config(
    LogicAnalyserLLConfig *config,
    uint32_t pin_base,
    uint32_t pin_count,
    float clk_div
)
{
    if (!config) {
        return;
    }

    *config = (LogicAnalyserLLConfig){
        .pio = pio0,
        .sm = 0,
        .pin_base = pin_base,
        .pin_count = pin_count,
        .clk_div = clk_div,
    };
}

static bool load_program(LogicAnalyserLL *la)
{
    PIO pio = config_pio(&la->config);
    la->program_instruction = pio_encode_in(pio_pins, la->config.pin_count);
    struct pio_program program = {
        .instructions = &la->program_instruction,
        .length = 1,
        .origin = -1,
    };

    if (!pio_can_add_program(pio, &program)) {
        return false;
    }

    la->program_offset = pio_add_program(pio, &program);
    la->program_loaded = true;
    return true;
}

static void unload_program(LogicAnalyserLL *la)
{
    if (!la->program_loaded) {
        return;
    }

    PIO pio = config_pio(&la->config);
    struct pio_program program = {
        .instructions = &la->program_instruction,
        .length = 1,
        .origin = -1,
    };
    pio_remove_program(pio, &program, la->program_offset);
    la->program_loaded = false;
}

bool logic_analyser_ll_configure(
    LogicAnalyserLL *la,
    LogicAnalyserLLConfig const *config
)
{
    if (!la || !config || config->pin_count == 0 || config->pin_count > 32 ||
        config->clk_div < 1.0f) {
        return false;
    }

    if (la->initialized && logic_analyser_ll_is_busy(la)) {
        return false;
    }

    PIO old_pio = config_pio(&la->config);
    if (la->initialized) {
        pio_sm_set_enabled(old_pio, la->config.sm, false);
        unload_program(la);
    }

    la->config = *config;
    PIO pio = config_pio(&la->config);
    for (uint32_t pin = 0; pin < config->pin_count; ++pin) {
        gpio_init(config->pin_base + pin);
        gpio_set_dir(config->pin_base + pin, GPIO_IN);
    }

    if (!load_program(la)) {
        return false;
    }

    uint32_t bits_per_word = 32u - (32u % config->pin_count);
    pio_sm_config sm_config = pio_get_default_sm_config();
    sm_config_set_in_pins(&sm_config, config->pin_base);
    sm_config_set_wrap(&sm_config, la->program_offset, la->program_offset);
    sm_config_set_clkdiv(&sm_config, config->clk_div);
    sm_config_set_in_shift(&sm_config, true, true, bits_per_word);
    sm_config_set_fifo_join(&sm_config, PIO_FIFO_JOIN_RX);
    pio_sm_init(pio, config->sm, la->program_offset, &sm_config);

    la->initialized = true;
    return true;
}

bool logic_analyser_ll_init(
    LogicAnalyserLL *la,
    LogicAnalyserLLConfig const *config
)
{
    if (!la) {
        return false;
    }

    *la = (LogicAnalyserLL){
        .dma_chan = dma_claim_unused_channel(false),
    };

    if (la->dma_chan < 0) {
        return false;
    }

    bus_ctrl_hw->priority = BUSCTRL_BUS_PRIORITY_DMA_W_BITS |
                            BUSCTRL_BUS_PRIORITY_DMA_R_BITS;

    return logic_analyser_ll_configure(la, config);
}

void logic_analyser_ll_deinit(LogicAnalyserLL *la)
{
    if (!la || !la->initialized) {
        return;
    }

    PIO pio = config_pio(&la->config);
    pio_sm_set_enabled(pio, la->config.sm, false);
    unload_program(la);
    if (la->dma_chan >= 0) {
        dma_channel_unclaim((uint)la->dma_chan);
    }
    la->initialized = false;
}

bool logic_analyser_ll_is_busy(LogicAnalyserLL const *la)
{
    return la && la->initialized && la->dma_chan >= 0 &&
           dma_channel_is_busy((uint)la->dma_chan);
}

static void prepare_trigger_pin(uint32_t trigger_pin, bool trigger_level)
{
    gpio_init(trigger_pin);
    gpio_set_dir(trigger_pin, GPIO_IN);
    gpio_set_pulls(trigger_pin, !trigger_level, trigger_level);
    gpio_set_input_enabled(trigger_pin, true);
}

static bool trigger_wait_timed_out(bool use_timeout, uint64_t deadline_us)
{
    return use_timeout && time_us_64() >= deadline_us;
}

static bool wait_for_trigger_rearm(
    uint32_t trigger_pin,
    bool trigger_level,
    bool use_timeout,
    uint64_t deadline_us
)
{
    while (gpio_get(trigger_pin) == trigger_level) {
        if (trigger_wait_timed_out(use_timeout, deadline_us)) {
            return false;
        }
        tight_loop_contents();
    }

    return true;
}

void logic_analyser_ll_wait_for_trigger(
    uint32_t trigger_pin,
    bool trigger_level,
    bool edge_trigger
)
{
    (void)logic_analyser_ll_wait_for_trigger_timeout(
        trigger_pin,
        trigger_level,
        edge_trigger,
        0
    );
}

bool logic_analyser_ll_wait_for_trigger_timeout(
    uint32_t trigger_pin,
    bool trigger_level,
    bool edge_trigger,
    uint32_t timeout_us
)
{
    prepare_trigger_pin(trigger_pin, trigger_level);
    bool use_timeout = timeout_us > 0;
    uint64_t deadline_us = use_timeout ? time_us_64() + timeout_us : 0;

    if (edge_trigger) {
        if (!wait_for_trigger_rearm(
                trigger_pin,
                trigger_level,
                use_timeout,
                deadline_us
            )) {
            return false;
        }
    }

    while (gpio_get(trigger_pin) != trigger_level) {
        if (trigger_wait_timed_out(use_timeout, deadline_us)) {
            return false;
        }
        tight_loop_contents();
    }

    return true;
}

bool logic_analyser_ll_capture_start(
    LogicAnalyserLL *la,
    uint32_t trigger_pin,
    bool trigger_level,
    bool edge_trigger,
    uint32_t *capture_buf,
    uint32_t sample_count,
    uint32_t word_count,
    uint32_t bits_per_word,
    LogicAnalyserLLCaptureInfo *info,
    bool wait_for_trigger
)
{
    if (!logic_analyser_ll_capture_arm(
            la,
            trigger_pin,
            trigger_level,
            edge_trigger,
            capture_buf,
            sample_count,
            word_count,
            bits_per_word,
            info,
            wait_for_trigger
        )) {
        return false;
    }

    return logic_analyser_ll_capture_start_armed(la);
}

bool logic_analyser_ll_capture_arm(
    LogicAnalyserLL *la,
    uint32_t trigger_pin,
    bool trigger_level,
    bool edge_trigger,
    uint32_t *capture_buf,
    uint32_t sample_count,
    uint32_t word_count,
    uint32_t bits_per_word,
    LogicAnalyserLLCaptureInfo *info,
    bool wait_for_trigger
)
{
    if (!la || !la->initialized || !capture_buf || sample_count == 0 ||
        word_count == 0 || bits_per_word == 0 ||
        logic_analyser_ll_is_busy(la)) {
        return false;
    }

    if (wait_for_trigger) {
        prepare_trigger_pin(trigger_pin, trigger_level);
    }
    if (wait_for_trigger && edge_trigger) {
        (void)wait_for_trigger_rearm(trigger_pin, trigger_level, false, 0);
    }

    PIO pio = config_pio(&la->config);
    pio_sm_set_enabled(pio, la->config.sm, false);
    pio_sm_clear_fifos(pio, la->config.sm);
    pio_sm_restart(pio, la->config.sm);

    dma_channel_config dma_config =
        dma_channel_get_default_config((uint)la->dma_chan);
    channel_config_set_read_increment(&dma_config, false);
    channel_config_set_write_increment(&dma_config, true);
    channel_config_set_dreq(&dma_config, pio_get_dreq(pio, la->config.sm, false));

    dma_channel_configure(
        (uint)la->dma_chan,
        &dma_config,
        capture_buf,
        &pio->rxf[la->config.sm],
        word_count,
        false
    );

    if (wait_for_trigger) {
        pio_sm_exec(
            pio,
            la->config.sm,
            pio_encode_wait_gpio(trigger_level, trigger_pin)
        );
    }
    if (info) {
        *info = (LogicAnalyserLLCaptureInfo){
            .pin_base = la->config.pin_base,
            .pin_count = la->config.pin_count,
            .sample_count = sample_count,
            .word_count = word_count,
            .bits_per_word = bits_per_word,
        };
    }

    return true;
}

bool logic_analyser_ll_capture_start_armed(LogicAnalyserLL *la)
{
    if (!la || !la->initialized || la->dma_chan < 0 ||
        dma_channel_is_busy((uint)la->dma_chan)) {
        return false;
    }

    PIO pio = config_pio(&la->config);
    dma_channel_start((uint)la->dma_chan);
    pio_sm_set_enabled(pio, la->config.sm, true);
    return true;
}

bool logic_analyser_ll_capture_complete(LogicAnalyserLL *la)
{
    if (!la || !la->initialized || la->dma_chan < 0 ||
        dma_channel_is_busy((uint)la->dma_chan)) {
        return false;
    }

    PIO pio = config_pio(&la->config);
    pio_sm_set_enabled(pio, la->config.sm, false);
    return true;
}

void logic_analyser_ll_capture_wait(LogicAnalyserLL *la)
{
    if (!la || la->dma_chan < 0) {
        return;
    }

    dma_channel_wait_for_finish_blocking((uint)la->dma_chan);
}

void logic_analyser_ll_capture_abort(LogicAnalyserLL *la)
{
    if (!la || !la->initialized) {
        return;
    }

    PIO pio = config_pio(&la->config);
    pio_sm_set_enabled(pio, la->config.sm, false);
    if (la->dma_chan >= 0) {
        dma_channel_abort((uint)la->dma_chan);
    }
    pio_sm_clear_fifos(pio, la->config.sm);
    pio_sm_restart(pio, la->config.sm);
}
