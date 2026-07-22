#ifndef LOGIC_ANALYSER_LL_H
#define LOGIC_ANALYSER_LL_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    void *pio;
    uint32_t sm;
    uint32_t pin_base;
    uint32_t pin_count;
    float clk_div;
} LogicAnalyserLLConfig;

typedef struct {
    uint32_t pin_base;
    uint32_t pin_count;
    uint32_t sample_count;
    uint32_t word_count;
    uint32_t bits_per_word;
} LogicAnalyserLLCaptureInfo;

typedef struct {
    LogicAnalyserLLConfig config;
    int dma_chan;
    uint32_t program_offset;
    uint16_t program_instruction;
    bool program_loaded;
    bool initialized;
} LogicAnalyserLL;

void logic_analyser_ll_default_config(
    LogicAnalyserLLConfig *config,
    uint32_t pin_base,
    uint32_t pin_count,
    float clk_div
);
bool logic_analyser_ll_init(
    LogicAnalyserLL *la,
    LogicAnalyserLLConfig const *config
);
void logic_analyser_ll_deinit(LogicAnalyserLL *la);
bool logic_analyser_ll_configure(
    LogicAnalyserLL *la,
    LogicAnalyserLLConfig const *config
);
bool logic_analyser_ll_is_busy(LogicAnalyserLL const *la);
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
);
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
);
bool logic_analyser_ll_capture_start_armed(LogicAnalyserLL *la);
void logic_analyser_ll_wait_for_trigger(
    uint32_t trigger_pin,
    bool trigger_level,
    bool edge_trigger
);
bool logic_analyser_ll_capture_complete(LogicAnalyserLL *la);
void logic_analyser_ll_capture_wait(LogicAnalyserLL *la);
void logic_analyser_ll_capture_abort(LogicAnalyserLL *la);

#endif
