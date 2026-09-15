#include "system/pattern_generator.h"

static bool config_is_valid(PatternGeneratorConfig const *config)
{
    return config && config->pin_count >= 1 &&
           config->pin_count <= PATTERN_OUTPUT_LL_MAX_PIN_COUNT &&
           config->pin_base + config->pin_count <=
               PATTERN_OUTPUT_LL_GPIO_COUNT &&
           config->rate_hz >= 1;
}

static bool configure_platform(
    PatternGenerator *pg,
    PatternGeneratorConfig const *config
)
{
    PatternOutputLLConfig ll_config;
    pattern_output_ll_default_config(
        &ll_config,
        config->pin_base,
        config->pin_count,
        config->rate_hz
    );

    if (pg->initialized) {
        return pattern_output_ll_configure(&pg->platform, &ll_config);
    }

    pg->initialized = pattern_output_ll_init(&pg->platform, &ll_config);
    return pg->initialized;
}

bool pattern_generator_init(
    PatternGenerator *pg,
    PatternGeneratorConfig const *config
)
{
    if (!pg || !config_is_valid(config)) {
        return false;
    }

    *pg = (PatternGenerator){0};
    if (!configure_platform(pg, config)) {
        return false;
    }

    pg->config = *config;
    return true;
}

void pattern_generator_deinit(PatternGenerator *pg)
{
    if (!pg || !pg->initialized) {
        return;
    }

    pattern_output_ll_deinit(&pg->platform);
    pg->initialized = false;
    pg->running = false;
}

bool pattern_generator_configure(
    PatternGenerator *pg,
    PatternGeneratorConfig const *config
)
{
    if (!pg || !config_is_valid(config) || pattern_generator_is_running(pg)) {
        return false;
    }

    if (!configure_platform(pg, config)) {
        return false;
    }

    pg->config = *config;
    return true;
}

bool pattern_generator_start(
    PatternGenerator *pg,
    uint32_t const *pattern,
    uint32_t word_count,
    PatternGeneratorMode mode
)
{
    if (!pg || !pg->initialized || !pattern || word_count == 0 ||
        pattern_generator_is_running(pg)) {
        return false;
    }

    pg->pattern = pattern;
    pg->word_count = word_count;
    pg->mode = mode;
    pg->running = pattern_output_ll_start(
        &pg->platform,
        pattern,
        word_count,
        mode == PATTERN_GENERATOR_MODE_LOOP
    );
    return pg->running;
}

void pattern_generator_stop(PatternGenerator *pg)
{
    if (!pg || !pg->initialized) {
        return;
    }

    pattern_output_ll_stop(&pg->platform);
    pg->running = false;
}

void pattern_generator_task(PatternGenerator *pg)
{
    if (!pg || !pg->initialized) {
        return;
    }

    pattern_output_ll_task(&pg->platform);
    if (!pg->running || pattern_output_ll_is_busy(&pg->platform)) {
        return;
    }

    pattern_generator_stop(pg);
}

bool pattern_generator_is_running(PatternGenerator const *pg)
{
    return pg && pg->running && pattern_output_ll_is_busy(&pg->platform);
}

uint32_t pattern_generator_get_underruns(PatternGenerator *pg)
{
    if (!pg || !pg->initialized) {
        return 0;
    }

    return pattern_output_ll_get_underruns(&pg->platform);
}
