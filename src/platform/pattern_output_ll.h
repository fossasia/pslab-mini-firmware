#ifndef PATTERN_OUTPUT_LL_H
#define PATTERN_OUTPUT_LL_H

#include <stdbool.h>
#include <stdint.h>

enum {
    PATTERN_OUTPUT_LL_MAX_PIN_COUNT = 8,
    PATTERN_OUTPUT_LL_GPIO_COUNT = 30,
};

typedef struct {
    void *pio;
    uint32_t sm;
    uint32_t pin_base;
    uint32_t pin_count;
    uint32_t rate_hz;
} PatternOutputLLConfig;

typedef struct {
    PatternOutputLLConfig config;
    int dma_chan;
    int ctrl_dma_chan;
    uint32_t program_offset;
    uintptr_t restart_read_addr;
    uint32_t saved_bus_priority;
    uint32_t underrun_count;
    uint16_t program_instructions[2];
    bool program_loaded;
    bool loop_enabled;
    bool bus_priority_saved;
    bool initialized;
} PatternOutputLL;

void pattern_output_ll_default_config(
    PatternOutputLLConfig *config,
    uint32_t pin_base,
    uint32_t pin_count,
    uint32_t rate_hz
);
bool pattern_output_ll_init(
    PatternOutputLL *pg,
    PatternOutputLLConfig const *config
);
void pattern_output_ll_deinit(PatternOutputLL *pg);
bool pattern_output_ll_configure(
    PatternOutputLL *pg,
    PatternOutputLLConfig const *config
);
bool pattern_output_ll_start(
    PatternOutputLL *pg,
    uint32_t const *pattern,
    uint32_t word_count,
    bool loop
);
void pattern_output_ll_stop(PatternOutputLL *pg);
bool pattern_output_ll_is_busy(PatternOutputLL const *pg);
void pattern_output_ll_task(PatternOutputLL *pg);
uint32_t pattern_output_ll_get_underruns(PatternOutputLL *pg);

#endif
