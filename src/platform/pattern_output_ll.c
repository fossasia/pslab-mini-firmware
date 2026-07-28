#include "platform/pattern_output_ll.h"

#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/structs/bus_ctrl.h"
#include "pico/stdlib.h"

#include "platform/platform.h"

enum {
    PATTERN_OUTPUT_INSTRUCTIONS_PER_SAMPLE = 2,
};

static PIO config_pio(PatternOutputLLConfig const *config)
{
    return config && config->pio ? (PIO)config->pio : pio0;
}

void pattern_output_ll_default_config(
    PatternOutputLLConfig *config,
    uint32_t pin_base,
    uint32_t pin_count,
    uint32_t rate_hz
)
{
    if (!config) {
        return;
    }

    *config = (PatternOutputLLConfig){
        .pio = pio0,
        .sm = 1,
        .pin_base = pin_base,
        .pin_count = pin_count,
        .rate_hz = rate_hz,
    };
}

static bool load_program(PatternOutputLL *pg)
{
    PIO pio = config_pio(&pg->config);
    pg->program_instructions[0] = pio_encode_pull(false, true);
    pg->program_instructions[1] = pio_encode_out(pio_pins, pg->config.pin_count);

    struct pio_program program = {
        .instructions = pg->program_instructions,
        .length = 2,
        .origin = -1,
    };

    if (!pio_can_add_program(pio, &program)) {
        return false;
    }

    pg->program_offset = pio_add_program(pio, &program);
    pg->program_loaded = true;
    return true;
}

static void unload_program(PatternOutputLL *pg)
{
    if (!pg->program_loaded) {
        return;
    }

    PIO pio = config_pio(&pg->config);
    struct pio_program program = {
        .instructions = pg->program_instructions,
        .length = 2,
        .origin = -1,
    };
    pio_remove_program(pio, &program, pg->program_offset);
    pg->program_loaded = false;
}

static float rate_to_clkdiv(uint32_t rate_hz)
{
    if (rate_hz == 0) {
        return 0.0f;
    }

    uint32_t sys_hz =
        PLATFORM_get_peripheral_clock_speed(PLATFORM_CLOCK_SYS);
    return (float)sys_hz /
           ((float)rate_hz * (float)PATTERN_OUTPUT_INSTRUCTIONS_PER_SAMPLE);
}

static void save_and_apply_dma_priority(PatternOutputLL *pg)
{
    if (!pg || pg->bus_priority_saved) {
        return;
    }

    pg->saved_bus_priority = bus_ctrl_hw->priority;
    pg->bus_priority_saved = true;
    bus_ctrl_hw->priority = pg->saved_bus_priority |
                            BUSCTRL_BUS_PRIORITY_DMA_W_BITS |
                            BUSCTRL_BUS_PRIORITY_DMA_R_BITS;
}

static void restore_bus_priority(PatternOutputLL *pg)
{
    if (!pg || !pg->bus_priority_saved) {
        return;
    }

    bus_ctrl_hw->priority = pg->saved_bus_priority;
    pg->bus_priority_saved = false;
}

bool pattern_output_ll_configure(
    PatternOutputLL *pg,
    PatternOutputLLConfig const *config
)
{
    if (!pg || !config || config->pin_count == 0 ||
        config->pin_count > PATTERN_OUTPUT_LL_MAX_PIN_COUNT ||
        config->pin_base + config->pin_count > PATTERN_OUTPUT_LL_GPIO_COUNT ||
        config->rate_hz == 0) {
        return false;
    }

    float clk_div = rate_to_clkdiv(config->rate_hz);
    if (clk_div < 1.0f) {
        return false;
    }

    if (pg->initialized && pattern_output_ll_is_busy(pg)) {
        return false;
    }

    PIO old_pio = config_pio(&pg->config);
    if (pg->initialized) {
        pio_sm_set_enabled(old_pio, pg->config.sm, false);
        unload_program(pg);
    }

    pg->config = *config;
    PIO pio = config_pio(&pg->config);
    for (uint32_t pin = 0; pin < config->pin_count; ++pin) {
        pio_gpio_init(pio, config->pin_base + pin);
    }

    if (!load_program(pg)) {
        pg->initialized = false;
        pg->program_loaded = false;
        pg->loop_enabled = false;
        return false;
    }

    pio_sm_config sm_config = pio_get_default_sm_config();
    sm_config_set_out_pins(&sm_config, config->pin_base, config->pin_count);
    sm_config_set_wrap(
        &sm_config,
        pg->program_offset,
        pg->program_offset + 1
    );
    sm_config_set_clkdiv(&sm_config, clk_div);
    sm_config_set_out_shift(&sm_config, true, false, 32);
    sm_config_set_fifo_join(&sm_config, PIO_FIFO_JOIN_TX);
    pio_sm_init(pio, config->sm, pg->program_offset, &sm_config);
    pio_sm_set_consecutive_pindirs(
        pio,
        config->sm,
        config->pin_base,
        config->pin_count,
        true
    );

    pg->initialized = true;
    return true;
}

bool pattern_output_ll_init(
    PatternOutputLL *pg,
    PatternOutputLLConfig const *config
)
{
    if (!pg) {
        return false;
    }

    *pg = (PatternOutputLL){
        .dma_chan = dma_claim_unused_channel(false),
        .ctrl_dma_chan = -1,
    };

    if (pg->dma_chan < 0) {
        return false;
    }

    pg->ctrl_dma_chan = dma_claim_unused_channel(false);
    if (pg->ctrl_dma_chan < 0) {
        dma_channel_unclaim((uint)pg->dma_chan);
        pg->dma_chan = -1;
        return false;
    }

    save_and_apply_dma_priority(pg);

    if (!pattern_output_ll_configure(pg, config)) {
        dma_channel_unclaim((uint)pg->dma_chan);
        dma_channel_unclaim((uint)pg->ctrl_dma_chan);
        pg->dma_chan = -1;
        pg->ctrl_dma_chan = -1;
        restore_bus_priority(pg);
        return false;
    }

    return true;
}

void pattern_output_ll_deinit(PatternOutputLL *pg)
{
    if (!pg) {
        return;
    }

    if (pg->initialized) {
        pattern_output_ll_stop(pg);
    }
    unload_program(pg);
    if (pg->dma_chan >= 0) {
        dma_channel_unclaim((uint)pg->dma_chan);
        pg->dma_chan = -1;
    }
    if (pg->ctrl_dma_chan >= 0) {
        dma_channel_unclaim((uint)pg->ctrl_dma_chan);
        pg->ctrl_dma_chan = -1;
    }
    restore_bus_priority(pg);
    pg->initialized = false;
}

bool pattern_output_ll_start(
    PatternOutputLL *pg,
    uint32_t const *pattern,
    uint32_t word_count,
    bool loop
)
{
    if (!pg || !pg->initialized || !pattern || word_count == 0 ||
        pg->dma_chan < 0 || pg->ctrl_dma_chan < 0 ||
        pattern_output_ll_is_busy(pg)) {
        return false;
    }

    PIO pio = config_pio(&pg->config);
    pio_sm_set_enabled(pio, pg->config.sm, false);
    pio_sm_clear_fifos(pio, pg->config.sm);
    pio_sm_restart(pio, pg->config.sm);
    pg->loop_enabled = loop;

    dma_channel_config dma_config =
        dma_channel_get_default_config((uint)pg->dma_chan);
    channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_32);
    channel_config_set_read_increment(&dma_config, true);
    channel_config_set_write_increment(&dma_config, false);
    channel_config_set_dreq(&dma_config, pio_get_dreq(pio, pg->config.sm, true));
    if (loop) {
        channel_config_set_chain_to(&dma_config, (uint)pg->ctrl_dma_chan);
        channel_config_set_irq_quiet(&dma_config, true);
    }

    dma_channel_configure(
        (uint)pg->dma_chan,
        &dma_config,
        &pio->txf[pg->config.sm],
        pattern,
        word_count,
        false
    );

    if (loop) {
        pg->restart_read_addr = (uintptr_t)pattern;

        dma_channel_config ctrl_config =
            dma_channel_get_default_config((uint)pg->ctrl_dma_chan);
        channel_config_set_transfer_data_size(&ctrl_config, DMA_SIZE_32);
        channel_config_set_read_increment(&ctrl_config, false);
        channel_config_set_write_increment(&ctrl_config, false);

        dma_channel_configure(
            (uint)pg->ctrl_dma_chan,
            &ctrl_config,
            &dma_channel_hw_addr((uint)pg->dma_chan)->al3_read_addr_trig,
            &pg->restart_read_addr,
            1,
            false
        );
    }

    dma_channel_start((uint)pg->dma_chan);
    pio_sm_set_enabled(pio, pg->config.sm, true);
    return true;
}

void pattern_output_ll_stop(PatternOutputLL *pg)
{
    if (!pg || !pg->initialized) {
        return;
    }

    pg->loop_enabled = false;

    PIO pio = config_pio(&pg->config);
    pio_sm_set_enabled(pio, pg->config.sm, false);
    if (pg->dma_chan >= 0) {
        dma_channel_abort((uint)pg->dma_chan);
    }
    if (pg->ctrl_dma_chan >= 0) {
        dma_channel_abort((uint)pg->ctrl_dma_chan);
    }
    pio_sm_clear_fifos(pio, pg->config.sm);
    pio_sm_restart(pio, pg->config.sm);
}

bool pattern_output_ll_is_busy(PatternOutputLL const *pg)
{
    if (!pg || !pg->initialized || pg->dma_chan < 0 ||
        pg->ctrl_dma_chan < 0) {
        return false;
    }

    PIO pio = config_pio(&pg->config);
    return pg->loop_enabled || dma_channel_is_busy((uint)pg->dma_chan) ||
           dma_channel_is_busy((uint)pg->ctrl_dma_chan) ||
           !pio_sm_is_tx_fifo_empty(pio, pg->config.sm);
}
