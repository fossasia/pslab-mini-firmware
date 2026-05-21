#ifndef LOGIC_ANALYSER_H
#define LOGIC_ANALYSER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "hardware/pio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    PIO pio;
    uint sm;
    uint pin_base;
    uint pin_count;
    float clk_div;
} LogicAnalyserConfig;

typedef struct {
    uint pin_base;
    uint pin_count;
    uint sample_count;
    uint word_count;
    uint bits_per_word;
} LogicAnalyserCaptureInfo;

typedef struct LogicAnalyser LogicAnalyser;

typedef enum {
    LOGIC_ANALYSER_TRIGGER_LEVEL,
    LOGIC_ANALYSER_TRIGGER_EDGE,
} LogicAnalyserTriggerMode;

bool logic_analyser_init(LogicAnalyser *la, LogicAnalyserConfig const *config);
void logic_analyser_deinit(LogicAnalyser *la);
bool logic_analyser_configure(LogicAnalyser *la, LogicAnalyserConfig const *config);
bool logic_analyser_capture(
    LogicAnalyser *la,
    uint trigger_pin,
    bool trigger_level,
    LogicAnalyserTriggerMode trigger_mode,
    uint32_t *capture_buf,
    uint sample_count,
    LogicAnalyserCaptureInfo *info
);
bool logic_analyser_is_busy(LogicAnalyser const *la);
uint logic_analyser_capture_word_count(uint pin_count, uint sample_count);
uint logic_analyser_bits_packed_per_word(uint pin_count);

struct LogicAnalyser {
    LogicAnalyserConfig config;
    int dma_chan;
    uint program_offset;
    uint16_t program_instruction;
    bool program_loaded;
    bool initialized;
};

#ifdef __cplusplus
}
#endif

#endif
