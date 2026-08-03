#ifndef PATTERN_GENERATOR_H
#define PATTERN_GENERATOR_H

#include <stdbool.h>
#include <stdint.h>

#include "platform/pattern_output_ll.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PATTERN_GENERATOR_MODE_ONCE,
    PATTERN_GENERATOR_MODE_LOOP,
} PatternGeneratorMode;

typedef struct {
    uint32_t pin_base;
    uint32_t pin_count;
    uint32_t rate_hz;
} PatternGeneratorConfig;

typedef struct PatternGenerator PatternGenerator;

bool pattern_generator_init(
    PatternGenerator *pg,
    PatternGeneratorConfig const *config
);
void pattern_generator_deinit(PatternGenerator *pg);
bool pattern_generator_configure(
    PatternGenerator *pg,
    PatternGeneratorConfig const *config
);
bool pattern_generator_start(
    PatternGenerator *pg,
    uint32_t const *pattern,
    uint32_t word_count,
    PatternGeneratorMode mode
);
void pattern_generator_stop(PatternGenerator *pg);
void pattern_generator_task(PatternGenerator *pg);
bool pattern_generator_is_running(PatternGenerator const *pg);
uint32_t pattern_generator_get_underruns(PatternGenerator *pg);

struct PatternGenerator {
    PatternGeneratorConfig config;
    PatternOutputLL platform;
    uint32_t const *pattern;
    uint32_t word_count;
    PatternGeneratorMode mode;
    bool initialized;
    bool running;
};

#ifdef __cplusplus
}
#endif

#endif
