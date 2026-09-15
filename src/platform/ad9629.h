/**
 * @file ad9629.h
 * @brief AD9629 external ADC register/control interface.
 *
 * This module contains the platform-level register definitions and typed
 * configuration API for the AD9629 parallel-output ADC. It only covers the SPI
 * control plane; parallel sample capture is handled by a separate backend.
 */

#ifndef PSLAB_AD9629_H
#define PSLAB_AD9629_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    AD9629_CHIP_ID = 0x70,
    AD9629_SAMPLE_WIDTH_BITS = 12,
};

typedef enum {
    AD9629_REG_SPI_CONFIG = 0x000,
    AD9629_REG_CHIP_ID = 0x001,
    AD9629_REG_CHIP_GRADE = 0x002,
    AD9629_REG_MODES = 0x008,
    AD9629_REG_CLOCK_DIVIDE = 0x00B,
    AD9629_REG_TEST_MODE = 0x00D,
    AD9629_REG_BIST_ENABLE = 0x00E,
    AD9629_REG_OFFSET_ADJUST = 0x010,
    AD9629_REG_OUTPUT_MODE = 0x014,
    AD9629_REG_OUTPUT_ADJUST = 0x015,
    AD9629_REG_OUTPUT_PHASE = 0x016,
    AD9629_REG_OUTPUT_DELAY = 0x017,
    AD9629_REG_USER_PATTERN1_LSB = 0x019,
    AD9629_REG_USER_PATTERN1_MSB = 0x01A,
    AD9629_REG_USER_PATTERN2_LSB = 0x01B,
    AD9629_REG_USER_PATTERN2_MSB = 0x01C,
    AD9629_REG_BIST_SIGNATURE_LSB = 0x024,
    AD9629_REG_OR_MODE_SELECT = 0x02A,
    AD9629_REG_TRANSFER = 0x0FF,
    AD9629_REG_USR2 = 0x101,
} AD9629_Register;

enum {
    AD9629_SPI_CONFIG_LSB_FIRST = (1u << 6) | (1u << 1),
    AD9629_SPI_CONFIG_SOFT_RESET = (1u << 5) | (1u << 2),

    AD9629_TRANSFER_SYNC = 1u << 0,

    AD9629_MODES_POWER_MASK = 0x03u,
    AD9629_MODES_PIN23_FUNCTION_MASK = 0x03u << 5,
    AD9629_MODES_PIN23_ENABLE = 1u << 7,

    AD9629_CLOCK_DIVIDE_MASK = 0x07u,

    AD9629_TEST_MODE_PATTERN_MASK = 0x0Fu,
    AD9629_TEST_MODE_RESET_PN_SHORT = 1u << 4,
    AD9629_TEST_MODE_RESET_PN_LONG = 1u << 5,
    AD9629_TEST_MODE_USER_MODE_MASK = 0x03u << 6,

    AD9629_BIST_ENABLE = 1u << 0,
    AD9629_BIST_INIT = 1u << 2,

    AD9629_OUTPUT_MODE_FORMAT_MASK = 0x03u,
    AD9629_OUTPUT_MODE_INVERT = 1u << 2,
    AD9629_OUTPUT_MODE_DISABLE = 1u << 4,
    AD9629_OUTPUT_MODE_DRVDD_MASK = 0x03u << 6,

    AD9629_OUTPUT_ADJUST_DATA_DRIVE_1V8_MASK = 0x03u,
    AD9629_OUTPUT_ADJUST_DATA_DRIVE_3V3_MASK = 0x03u << 2,
    AD9629_OUTPUT_ADJUST_DCO_DRIVE_1V8_MASK = 0x03u << 4,
    AD9629_OUTPUT_ADJUST_DCO_DRIVE_3V3_MASK = 0x03u << 6,

    AD9629_OUTPUT_PHASE_MASK = 0x07u,
    AD9629_OUTPUT_PHASE_DCO_INVERT = 1u << 7,

    AD9629_OUTPUT_DELAY_MASK = 0x07u,
    AD9629_OUTPUT_DELAY_ENABLE_DATA = 1u << 5,
    AD9629_OUTPUT_DELAY_ENABLE_DCO = 1u << 7,

    AD9629_BIST_SIGNATURE_PASS = 1u << 0,

    AD9629_OR_MODE_SELECT_OR_OUTPUT = 1u << 0,

    AD9629_USR2_DISABLE_SDIO_PULLDOWN = 1u << 0,
    AD9629_USR2_RUN_GCLK = 1u << 2,
    AD9629_USR2_ENABLE_GCLK_DETECT = 1u << 3,
};

typedef enum {
    AD9629_SPEED_GRADE_20_MSPS = 0,
    AD9629_SPEED_GRADE_40_MSPS = 1,
    AD9629_SPEED_GRADE_65_MSPS = 2,
    AD9629_SPEED_GRADE_80_MSPS = 3,
    AD9629_SPEED_GRADE_UNKNOWN = 0xFF,
} AD9629_SpeedGrade;

typedef enum {
    AD9629_POWER_RUN = 0,
    AD9629_POWER_FULL_DOWN = 1,
    AD9629_POWER_STANDBY = 2,
    AD9629_POWER_DIGITAL_RESET = 3,
} AD9629_PowerMode;

typedef enum {
    AD9629_PIN23_FULL_POWER_DOWN = 0,
    AD9629_PIN23_STANDBY = 1,
    AD9629_PIN23_OUTPUT_DISABLED = 2,
    AD9629_PIN23_OUTPUT_ENABLED = 3,
} AD9629_Pin23Function;

typedef enum {
    AD9629_CLOCK_DIVIDE_1 = 0,
    AD9629_CLOCK_DIVIDE_2 = 1,
    AD9629_CLOCK_DIVIDE_4 = 3,
} AD9629_ClockDivide;

typedef enum {
    AD9629_OUTPUT_FORMAT_OFFSET_BINARY = 0,
    AD9629_OUTPUT_FORMAT_TWOS_COMPLEMENT = 1,
    AD9629_OUTPUT_FORMAT_GRAY_CODE = 2,
} AD9629_OutputFormat;

typedef enum {
    AD9629_OUTPUT_SUPPLY_3V3_CMOS = 0,
    AD9629_OUTPUT_SUPPLY_1V8_CMOS = 2,
} AD9629_OutputSupply;

typedef enum {
    AD9629_OUTPUT_DRIVE_1_STRIPE = 0,
    AD9629_OUTPUT_DRIVE_2_STRIPES = 1,
    AD9629_OUTPUT_DRIVE_3_STRIPES = 2,
    AD9629_OUTPUT_DRIVE_4_STRIPES = 3,
} AD9629_OutputDrive;

typedef enum {
    AD9629_DCO_NORMAL = 0,
    AD9629_DCO_INVERTED = 1,
} AD9629_DcoPolarity;

typedef enum {
    AD9629_TEST_OFF = 0x0,
    AD9629_TEST_MIDSCALE_SHORT = 0x1,
    AD9629_TEST_POSITIVE_FULL_SCALE = 0x2,
    AD9629_TEST_NEGATIVE_FULL_SCALE = 0x3,
    AD9629_TEST_ALTERNATING_CHECKERBOARD = 0x4,
    AD9629_TEST_PN23 = 0x5,
    AD9629_TEST_PN9 = 0x6,
    AD9629_TEST_ONE_ZERO_WORD_TOGGLE = 0x7,
    AD9629_TEST_USER_INPUT = 0x8,
    AD9629_TEST_ONE_ZERO_BIT_TOGGLE = 0x9,
    AD9629_TEST_1X_SYNC = 0xA,
    AD9629_TEST_ONE_BIT_HIGH = 0xB,
    AD9629_TEST_MIXED_BIT_FREQUENCY = 0xC,
} AD9629_TestMode;

typedef enum {
    AD9629_USER_PATTERN_SINGLE = 0,
    AD9629_USER_PATTERN_ALTERNATE = 1,
    AD9629_USER_PATTERN_SINGLE_ONCE = 2,
    AD9629_USER_PATTERN_ALTERNATE_ONCE = 3,
} AD9629_UserPatternMode;

typedef enum {
    AD9629_OUTPUT_DELAY_0_56_NS = 0,
    AD9629_OUTPUT_DELAY_1_12_NS = 1,
    AD9629_OUTPUT_DELAY_1_68_NS = 2,
    AD9629_OUTPUT_DELAY_2_24_NS = 3,
    AD9629_OUTPUT_DELAY_2_80_NS = 4,
    AD9629_OUTPUT_DELAY_3_36_NS = 5,
    AD9629_OUTPUT_DELAY_3_92_NS = 6,
    AD9629_OUTPUT_DELAY_4_48_NS = 7,
} AD9629_OutputDelay;

typedef struct {
    uint32_t sclk_gpio;
    uint32_t sdio_gpio;
    uint32_t cs_gpio;
    uint32_t timeout_us;
} AD9629_Config;

/**
 * @brief AD9629 driver handle.
 *
 * The caller owns the handle storage and may allocate it statically or on the
 * stack. AD9629_init() copies the supplied configuration into this handle.
 */
typedef struct {
    AD9629_Config config;
    bool initialized;
} AD9629_Handle;

/**
 * @brief Fill config with conservative board-independent defaults.
 */
void AD9629_default_config(AD9629_Config *config);

/**
 * @brief Initialize handle state without probing hardware.
 *
 * Commit 1 only scaffolds the control API. This function validates pointers,
 * copies the configuration, and marks the handle usable by later operations.
 * SDIO transport setup and chip probing are implemented by later commits.
 *
 * @return true if handle/config are valid and copied.
 */
bool AD9629_init(AD9629_Handle *handle, AD9629_Config const *config);

/**
 * @brief Clear handle initialization state.
 */
void AD9629_deinit(AD9629_Handle *handle);

/**
 * @brief Write one AD9629 SPI register.
 *
 * @return true when the bus transaction succeeds. Stubbed false in commit 1.
 */
bool AD9629_write_register(
    AD9629_Handle *handle,
    AD9629_Register reg,
    uint8_t value
);

/**
 * @brief Read one AD9629 SPI register.
 *
 * @return true when value was read. Stubbed false in commit 1.
 */
bool AD9629_read_register(
    AD9629_Handle *handle,
    AD9629_Register reg,
    uint8_t *value
);

/**
 * @brief Update selected bits in one AD9629 SPI register.
 *
 * @return true when the read-modify-write succeeds. Stubbed false in commit 1.
 */
bool AD9629_update_register(
    AD9629_Handle *handle,
    AD9629_Register reg,
    uint8_t mask,
    uint8_t value
);

/**
 * @brief Apply shadowed register writes via register 0xFF.
 *
 * @return true when the transfer write succeeds. Stubbed false in commit 1.
 */
bool AD9629_transfer(AD9629_Handle *handle);

bool AD9629_reset(AD9629_Handle *handle);
bool AD9629_read_chip_id(AD9629_Handle *handle, uint8_t *chip_id);
bool AD9629_read_speed_grade(
    AD9629_Handle *handle,
    AD9629_SpeedGrade *grade
);
bool AD9629_probe(AD9629_Handle *handle);

bool AD9629_set_power_mode(AD9629_Handle *handle, AD9629_PowerMode mode);
bool AD9629_set_pin23_function(
    AD9629_Handle *handle,
    AD9629_Pin23Function function
);
bool AD9629_set_or_mode_select(AD9629_Handle *handle, bool or_output_enabled);
bool AD9629_set_clock_divide(AD9629_Handle *handle, AD9629_ClockDivide divide);
bool AD9629_set_offset_adjust(AD9629_Handle *handle, int8_t offset_lsb);
bool AD9629_set_output_format(
    AD9629_Handle *handle,
    AD9629_OutputFormat format
);
bool AD9629_set_output_supply(
    AD9629_Handle *handle,
    AD9629_OutputSupply supply
);
bool AD9629_set_output_invert(AD9629_Handle *handle, bool enabled);
bool AD9629_set_output_disabled(AD9629_Handle *handle, bool disabled);
bool AD9629_set_output_drive(
    AD9629_Handle *handle,
    AD9629_OutputDrive data_1v8_drive,
    AD9629_OutputDrive data_3v3_drive,
    AD9629_OutputDrive dco_1v8_drive,
    AD9629_OutputDrive dco_3v3_drive
);
bool AD9629_set_dco_polarity(
    AD9629_Handle *handle,
    AD9629_DcoPolarity polarity
);
bool AD9629_set_output_phase(AD9629_Handle *handle, uint8_t input_cycles);
bool AD9629_set_output_delay(
    AD9629_Handle *handle,
    AD9629_OutputDelay delay,
    bool data_delay_enabled,
    bool dco_delay_enabled
);
bool AD9629_set_test_mode(
    AD9629_Handle *handle,
    AD9629_TestMode mode,
    AD9629_UserPatternMode user_mode
);
bool AD9629_reset_pn_generators(AD9629_Handle *handle);
bool AD9629_set_user_patterns(
    AD9629_Handle *handle,
    uint16_t pattern1,
    uint16_t pattern2
);
bool AD9629_start_bist(AD9629_Handle *handle);
bool AD9629_bist_passed(AD9629_Handle *handle, bool *passed);
bool AD9629_set_gclk_detect(AD9629_Handle *handle, bool enabled);
bool AD9629_set_gclk_run(AD9629_Handle *handle, bool enabled);
bool AD9629_set_sdio_pulldown_disabled(AD9629_Handle *handle, bool disabled);

#ifdef __cplusplus
}
#endif

#endif
