#ifndef LOGIC_ANALYSER_H
#define LOGIC_ANALYSER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "platform/logic_analyser_ll.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t pin_base;
    uint32_t pin_count;
    float clk_div;
} LogicAnalyserConfig;

typedef struct {
    uint32_t pin_base;
    uint32_t pin_count;
    uint32_t sample_count;
    uint32_t word_count;
    uint32_t bits_per_word;
} LogicAnalyserCaptureInfo;

typedef struct LogicAnalyser LogicAnalyser;

typedef enum {
    LOGIC_ANALYSER_TRIGGER_AUTO,
    LOGIC_ANALYSER_TRIGGER_LEVEL,
    LOGIC_ANALYSER_TRIGGER_EDGE,
} LogicAnalyserTriggerMode;

bool logic_analyser_init(LogicAnalyser *la, LogicAnalyserConfig const *config);
void logic_analyser_deinit(LogicAnalyser *la);
bool logic_analyser_configure(LogicAnalyser *la, LogicAnalyserConfig const *config);
bool logic_analyser_capture(
    LogicAnalyser *la,
    uint32_t trigger_pin,
    bool trigger_level,
    LogicAnalyserTriggerMode trigger_mode,
    uint32_t *capture_buf,
    uint32_t sample_count,
    LogicAnalyserCaptureInfo *info
);
bool logic_analyser_capture_start(
    LogicAnalyser *la,
    uint32_t trigger_pin,
    bool trigger_level,
    LogicAnalyserTriggerMode trigger_mode,
    uint32_t *capture_buf,
    uint32_t sample_count,
    LogicAnalyserCaptureInfo *info,
    bool wait_for_trigger
);
bool logic_analyser_capture_arm(
    LogicAnalyser *la,
    uint32_t trigger_pin,
    bool trigger_level,
    LogicAnalyserTriggerMode trigger_mode,
    uint32_t *capture_buf,
    uint32_t sample_count,
    LogicAnalyserCaptureInfo *info,
    bool wait_for_trigger
);
bool logic_analyser_capture_start_armed(LogicAnalyser *la);
void logic_analyser_capture_wait(LogicAnalyser *la);
bool logic_analyser_capture_complete(LogicAnalyser *la);
void logic_analyser_capture_abort(LogicAnalyser *la);
bool logic_analyser_is_busy(LogicAnalyser const *la);
void logic_analyser_wait_for_trigger(
    uint32_t trigger_pin,
    bool trigger_level,
    LogicAnalyserTriggerMode trigger_mode
);
bool logic_analyser_wait_for_trigger_timeout(
    uint32_t trigger_pin,
    bool trigger_level,
    LogicAnalyserTriggerMode trigger_mode,
    uint32_t timeout_us
);
uint32_t logic_analyser_capture_word_count(
    uint32_t pin_count,
    uint32_t sample_count
);
uint32_t logic_analyser_bits_packed_per_word(uint32_t pin_count);

struct LogicAnalyser {
    LogicAnalyserConfig config;
    LogicAnalyserLL platform;
    bool initialized;
};

#ifdef __cplusplus
}
#endif

#endif
