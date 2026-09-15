#include "platform/ad9629.h"

#include <stdbool.h>
#include <stdint.h>

enum {
    AD9629_GPIO_UNASSIGNED = 0xffffffffu,
    AD9629_DEFAULT_TIMEOUT_US = 100000,
};

void AD9629_default_config(AD9629_Config *config)
{
    if (!config) {
        return;
    }

    *config = (AD9629_Config){
        .sclk_gpio = AD9629_GPIO_UNASSIGNED,
        .sdio_gpio = AD9629_GPIO_UNASSIGNED,
        .cs_gpio = AD9629_GPIO_UNASSIGNED,
        .timeout_us = AD9629_DEFAULT_TIMEOUT_US,
    };
}

bool AD9629_init(AD9629_Handle *handle, AD9629_Config const *config)
{
    if (!handle || !config) {
        return false;
    }

    handle->config = *config;
    handle->initialized = true;
    return true;
}

void AD9629_deinit(AD9629_Handle *handle)
{
    if (!handle) {
        return;
    }

    handle->initialized = false;
}

bool AD9629_write_register(
    AD9629_Handle *handle,
    AD9629_Register reg,
    uint8_t value
)
{
    (void)handle;
    (void)reg;
    (void)value;
    return false;
}

bool AD9629_read_register(
    AD9629_Handle *handle,
    AD9629_Register reg,
    uint8_t *value
)
{
    (void)handle;
    (void)reg;
    (void)value;
    return false;
}

bool AD9629_update_register(
    AD9629_Handle *handle,
    AD9629_Register reg,
    uint8_t mask,
    uint8_t value
)
{
    (void)handle;
    (void)reg;
    (void)mask;
    (void)value;
    return false;
}

bool AD9629_transfer(AD9629_Handle *handle)
{
    (void)handle;
    return false;
}

bool AD9629_reset(AD9629_Handle *handle)
{
    (void)handle;
    return false;
}

bool AD9629_read_chip_id(AD9629_Handle *handle, uint8_t *chip_id)
{
    (void)handle;
    (void)chip_id;
    return false;
}

bool AD9629_read_speed_grade(
    AD9629_Handle *handle,
    AD9629_SpeedGrade *grade
)
{
    (void)handle;
    (void)grade;
    return false;
}

bool AD9629_probe(AD9629_Handle *handle)
{
    (void)handle;
    return false;
}

bool AD9629_set_power_mode(AD9629_Handle *handle, AD9629_PowerMode mode)
{
    (void)handle;
    (void)mode;
    return false;
}

bool AD9629_set_pin23_function(
    AD9629_Handle *handle,
    AD9629_Pin23Function function
)
{
    (void)handle;
    (void)function;
    return false;
}

bool AD9629_set_or_mode_select(AD9629_Handle *handle, bool or_output_enabled)
{
    (void)handle;
    (void)or_output_enabled;
    return false;
}

bool AD9629_set_clock_divide(AD9629_Handle *handle, AD9629_ClockDivide divide)
{
    (void)handle;
    (void)divide;
    return false;
}

bool AD9629_set_offset_adjust(AD9629_Handle *handle, int8_t offset_lsb)
{
    (void)handle;
    (void)offset_lsb;
    return false;
}

bool AD9629_set_output_format(
    AD9629_Handle *handle,
    AD9629_OutputFormat format
)
{
    (void)handle;
    (void)format;
    return false;
}

bool AD9629_set_output_supply(
    AD9629_Handle *handle,
    AD9629_OutputSupply supply
)
{
    (void)handle;
    (void)supply;
    return false;
}

bool AD9629_set_output_invert(AD9629_Handle *handle, bool enabled)
{
    (void)handle;
    (void)enabled;
    return false;
}

bool AD9629_set_output_disabled(AD9629_Handle *handle, bool disabled)
{
    (void)handle;
    (void)disabled;
    return false;
}

bool AD9629_set_output_drive(
    AD9629_Handle *handle,
    AD9629_OutputDrive data_1v8_drive,
    AD9629_OutputDrive data_3v3_drive,
    AD9629_OutputDrive dco_1v8_drive,
    AD9629_OutputDrive dco_3v3_drive
)
{
    (void)handle;
    (void)data_1v8_drive;
    (void)data_3v3_drive;
    (void)dco_1v8_drive;
    (void)dco_3v3_drive;
    return false;
}

bool AD9629_set_dco_polarity(
    AD9629_Handle *handle,
    AD9629_DcoPolarity polarity
)
{
    (void)handle;
    (void)polarity;
    return false;
}

bool AD9629_set_output_phase(AD9629_Handle *handle, uint8_t input_cycles)
{
    (void)handle;
    (void)input_cycles;
    return false;
}

bool AD9629_set_output_delay(
    AD9629_Handle *handle,
    AD9629_OutputDelay delay,
    bool data_delay_enabled,
    bool dco_delay_enabled
)
{
    (void)handle;
    (void)delay;
    (void)data_delay_enabled;
    (void)dco_delay_enabled;
    return false;
}

bool AD9629_set_test_mode(
    AD9629_Handle *handle,
    AD9629_TestMode mode,
    AD9629_UserPatternMode user_mode
)
{
    (void)handle;
    (void)mode;
    (void)user_mode;
    return false;
}

bool AD9629_reset_pn_generators(AD9629_Handle *handle)
{
    (void)handle;
    return false;
}

bool AD9629_set_user_patterns(
    AD9629_Handle *handle,
    uint16_t pattern1,
    uint16_t pattern2
)
{
    (void)handle;
    (void)pattern1;
    (void)pattern2;
    return false;
}

bool AD9629_start_bist(AD9629_Handle *handle)
{
    (void)handle;
    return false;
}

bool AD9629_bist_passed(AD9629_Handle *handle, bool *passed)
{
    (void)handle;
    (void)passed;
    return false;
}

bool AD9629_set_gclk_detect(AD9629_Handle *handle, bool enabled)
{
    (void)handle;
    (void)enabled;
    return false;
}

bool AD9629_set_gclk_run(AD9629_Handle *handle, bool enabled)
{
    (void)handle;
    (void)enabled;
    return false;
}

bool AD9629_set_sdio_pulldown_disabled(AD9629_Handle *handle, bool disabled)
{
    (void)handle;
    (void)disabled;
    return false;
}
