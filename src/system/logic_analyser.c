#include "system/logic_analyser.h"

#include <stdbool.h>

#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/structs/bus_ctrl.h"

uint logic_analyser_bits_packed_per_word(uint pin_count)
{
    enum { SHIFT_REG_WIDTH = 32 };

    if (pin_count == 0 || pin_count > SHIFT_REG_WIDTH) {
        return 0;
    }

    return SHIFT_REG_WIDTH - (SHIFT_REG_WIDTH % pin_count);
}

uint logic_analyser_capture_word_count(uint pin_count, uint sample_count)
{
    uint bits_per_word = logic_analyser_bits_packed_per_word(pin_count);
    if (bits_per_word == 0) {
        return 0;
    }

    uint total_bits = sample_count * pin_count;
    total_bits += bits_per_word - 1;
    return total_bits / bits_per_word;
}

static bool load_program(LogicAnalyser *la)
{
    la->program_instruction = pio_encode_in(pio_pins, la->config.pin_count);
    struct pio_program program = {
        .instructions = &la->program_instruction,
        .length = 1,
        .origin = -1,
    };

    if (!pio_can_add_program(la->config.pio, &program)) {
        return false;
    }

    la->program_offset = pio_add_program(la->config.pio, &program);
    la->program_loaded = true;
    return true;
}

static void unload_program(LogicAnalyser *la)
{
    if (!la->program_loaded) {
        return;
    }

    struct pio_program program = {
        .instructions = &la->program_instruction,
        .length = 1,
        .origin = -1,
    };
    pio_remove_program(la->config.pio, &program, la->program_offset);
    la->program_loaded = false;
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

    if (la->initialized) {
        pio_sm_set_enabled(la->config.pio, la->config.sm, false);
        unload_program(la);
    }

    la->config = *config;
    for (uint pin = 0; pin < config->pin_count; ++pin) {
        gpio_init(config->pin_base + pin);
        gpio_set_dir(config->pin_base + pin, GPIO_IN);
    }

    if (!load_program(la)) {
        return false;
    }

    uint bits_per_word = logic_analyser_bits_packed_per_word(config->pin_count);
    pio_sm_config sm_config = pio_get_default_sm_config();
    sm_config_set_in_pins(&sm_config, config->pin_base);
    sm_config_set_wrap(&sm_config, la->program_offset, la->program_offset);
    sm_config_set_clkdiv(&sm_config, config->clk_div);
    sm_config_set_in_shift(&sm_config, true, true, bits_per_word);
    sm_config_set_fifo_join(&sm_config, PIO_FIFO_JOIN_RX);
    pio_sm_init(config->pio, config->sm, la->program_offset, &sm_config);

    la->initialized = true;
    return true;
}

bool logic_analyser_init(LogicAnalyser *la, LogicAnalyserConfig const *config)
{
    if (!la) {
        return false;
    }

    *la = (LogicAnalyser){
        .dma_chan = dma_claim_unused_channel(false),
    };

    if (la->dma_chan < 0) {
        return false;
    }

    bus_ctrl_hw->priority = BUSCTRL_BUS_PRIORITY_DMA_W_BITS |
                            BUSCTRL_BUS_PRIORITY_DMA_R_BITS;

    return logic_analyser_configure(la, config);
}

void logic_analyser_deinit(LogicAnalyser *la)
{
    if (!la || !la->initialized) {
        return;
    }

    pio_sm_set_enabled(la->config.pio, la->config.sm, false);
    unload_program(la);
    if (la->dma_chan >= 0) {
        dma_channel_unclaim((uint)la->dma_chan);
    }
    la->initialized = false;
}

bool logic_analyser_is_busy(LogicAnalyser const *la)
{
    return la && la->initialized && la->dma_chan >= 0 &&
           dma_channel_is_busy((uint)la->dma_chan);
}

static void prepare_trigger_pin(uint trigger_pin, bool trigger_level)
{
    gpio_init(trigger_pin);
    gpio_set_dir(trigger_pin, GPIO_IN);
    gpio_set_pulls(trigger_pin, !trigger_level, trigger_level);
    gpio_set_input_enabled(trigger_pin, true);
}

static void wait_for_trigger_rearm(uint trigger_pin, bool trigger_level)
{
    while (gpio_get(trigger_pin) == trigger_level) {
        tight_loop_contents();
    }
}

bool logic_analyser_capture(
    LogicAnalyser *la,
    uint trigger_pin,
    bool trigger_level,
    LogicAnalyserTriggerMode trigger_mode,
    uint32_t *capture_buf,
    uint sample_count,
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

    dma_channel_wait_for_finish_blocking((uint)la->dma_chan);
    return logic_analyser_capture_complete(la);
}

bool logic_analyser_capture_start(
    LogicAnalyser *la,
    uint trigger_pin,
    bool trigger_level,
    LogicAnalyserTriggerMode trigger_mode,
    uint32_t *capture_buf,
    uint sample_count,
    LogicAnalyserCaptureInfo *info,
    bool wait_for_trigger
)
{
    if (!la || !la->initialized || !capture_buf || sample_count == 0 ||
        logic_analyser_is_busy(la)) {
        return false;
    }

    uint word_count = logic_analyser_capture_word_count(
        la->config.pin_count, sample_count
    );
    if (word_count == 0) {
        return false;
    }

    if (wait_for_trigger) {
        prepare_trigger_pin(trigger_pin, trigger_level);
    }
    if (wait_for_trigger && trigger_mode == LOGIC_ANALYSER_TRIGGER_EDGE) {
        wait_for_trigger_rearm(trigger_pin, trigger_level);
    }

    pio_sm_set_enabled(la->config.pio, la->config.sm, false);
    pio_sm_clear_fifos(la->config.pio, la->config.sm);
    pio_sm_restart(la->config.pio, la->config.sm);

    dma_channel_config dma_config =
        dma_channel_get_default_config((uint)la->dma_chan);
    channel_config_set_read_increment(&dma_config, false);
    channel_config_set_write_increment(&dma_config, true);
    channel_config_set_dreq(
        &dma_config, pio_get_dreq(la->config.pio, la->config.sm, false)
    );

    dma_channel_configure(
        (uint)la->dma_chan,
        &dma_config,
        capture_buf,
        &la->config.pio->rxf[la->config.sm],
        word_count,
        true
    );

    if (wait_for_trigger) {
        pio_sm_exec(
            la->config.pio,
            la->config.sm,
            pio_encode_wait_gpio(trigger_level, trigger_pin)
        );
    }
    pio_sm_set_enabled(la->config.pio, la->config.sm, true);

    if (info) {
        *info = (LogicAnalyserCaptureInfo){
            .pin_base = la->config.pin_base,
            .pin_count = la->config.pin_count,
            .sample_count = sample_count,
            .word_count = word_count,
            .bits_per_word = logic_analyser_bits_packed_per_word(
                la->config.pin_count
            ),
        };
    }

    return true;
}

bool logic_analyser_capture_complete(LogicAnalyser *la)
{
    if (!la || !la->initialized || la->dma_chan < 0 ||
        dma_channel_is_busy((uint)la->dma_chan)) {
        return false;
    }

    pio_sm_set_enabled(la->config.pio, la->config.sm, false);
    return true;
}

void logic_analyser_capture_abort(LogicAnalyser *la)
{
    if (!la || !la->initialized) {
        return;
    }

    pio_sm_set_enabled(la->config.pio, la->config.sm, false);
    if (la->dma_chan >= 0) {
        dma_channel_abort((uint)la->dma_chan);
    }
    pio_sm_clear_fifos(la->config.pio, la->config.sm);
    pio_sm_restart(la->config.pio, la->config.sm);
}
