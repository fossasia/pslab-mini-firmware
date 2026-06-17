/**
 * @file uart_ll.c
 * @brief UART hardware implementation for RP2040/RP2350
 *
 * This module handles initialization and operation of the UART peripheral of
 * the Raspberry Pi Pico family. It configures the hardware and dispatches UART
 * interrupts to the hardware-independent UART implementation.
 *
 * Implementation Details:
 * - Supports UART0 and UART1
 * - Configured for 115200 baud, 8N1 format
 * - Interrupt-based reception
 * - Interrupt-based transmission
 *
 * The public interface intentionally follows the STM32 low-level UART driver.
 * The function names retain "dma" terminology for compatibility with the
 * hardware-independent system layer, even though this Pico implementation uses
 * UART interrupts at the moment.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "hardware/irq.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"

#include "util/error.h"

#include "uart_ll.h"

enum {
    DEFAULT_UART0_TX_GPIO = 0,
    DEFAULT_UART0_RX_GPIO = 1,
    DEFAULT_UART1_TX_GPIO = 4,
    DEFAULT_UART1_RX_GPIO = 5,
};

/* UART instance configuration */
typedef struct {
    uart_inst_t *uart;
    uint irq;
    uint tx_gpio;
    uint rx_gpio;
    uint8_t *rx_buffer_data;
    uint32_t rx_buffer_size;
    uint32_t volatile rx_head;
    uint8_t *tx_buffer_data;
    uint32_t volatile tx_size;
    uint32_t volatile tx_index;
    bool volatile tx_in_progress;
    UART_LL_TxCompleteCallback tx_complete_callback;
    UART_LL_RxCompleteCallback rx_complete_callback;
    UART_LL_IdleCallback idle_callback;
    bool initialized;
} UARTInstance;

/* Instance array */
static UARTInstance g_uart_instances[UART_BUS_COUNT] = {
    [UART_BUS_0] = {
        .uart = uart0,
        .irq = UART0_IRQ,
        .tx_gpio = DEFAULT_UART0_TX_GPIO,
        .rx_gpio = DEFAULT_UART0_RX_GPIO,
    },
    [UART_BUS_1] = {
        .uart = uart1,
        .irq = UART1_IRQ,
        .tx_gpio = DEFAULT_UART1_TX_GPIO,
        .rx_gpio = DEFAULT_UART1_RX_GPIO,
    },
};

/**
 * @brief Common IRQ handler for a UART bus.
 */
static void uart_irq_handler(UART_Bus bus)
{
    if (bus >= UART_BUS_COUNT || !g_uart_instances[bus].initialized) {
        return;
    }

    UARTInstance *instance = &g_uart_instances[bus];

    while (uart_is_readable(instance->uart)) {
        uint8_t data = uart_getc(instance->uart);
        if (instance->rx_buffer_data && instance->rx_buffer_size > 0) {
            instance->rx_buffer_data[instance->rx_head] = data;
            instance->rx_head = (instance->rx_head + 1) % instance->rx_buffer_size;

            if (instance->rx_head == 0 && instance->rx_complete_callback) {
                instance->rx_complete_callback(bus);
            }
            if (instance->idle_callback) {
                instance->idle_callback(bus, instance->rx_head);
            }
        }
    }

    while (instance->tx_in_progress && uart_is_writable(instance->uart)) {
        if (instance->tx_index >= instance->tx_size) {
            break;
        }
        uart_putc_raw(instance->uart, instance->tx_buffer_data[instance->tx_index]);
        instance->tx_index++;
    }

    if (instance->tx_in_progress && instance->tx_index >= instance->tx_size) {
        uint32_t transferred = instance->tx_size;
        instance->tx_in_progress = false;
        instance->tx_buffer_data = NULL;
        instance->tx_size = 0;
        instance->tx_index = 0;
        uart_set_irq_enables(instance->uart, true, false);

        if (instance->tx_complete_callback) {
            instance->tx_complete_callback(bus, transferred);
        }
    }
}

static void uart0_irq_handler(void) { uart_irq_handler(UART_BUS_0); }

static void uart1_irq_handler(void) { uart_irq_handler(UART_BUS_1); }

/**
 * @brief Initialize the UART peripheral.
 *
 * This function configures the UART hardware, including baud rate, data bits,
 * stop bits, and parity, to prepare it for serial communication.
 */
void UART_LL_init(UART_Bus bus, uint8_t *rx_buf, uint32_t sz)
{
    if (bus >= UART_BUS_COUNT) {
        THROW(ERROR_INVALID_ARGUMENT);
    }

    if (!rx_buf || sz == 0) {
        THROW(ERROR_INVALID_ARGUMENT);
    }

    if (g_uart_instances[bus].initialized) {
        THROW(ERROR_RESOURCE_BUSY);
    }

    UARTInstance *instance = &g_uart_instances[bus];

    uart_init(instance->uart, UART_DEFAULT_BAUDRATE);
    gpio_set_function(instance->tx_gpio, GPIO_FUNC_UART);
    gpio_set_function(instance->rx_gpio, GPIO_FUNC_UART);
    uart_set_format(instance->uart, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(instance->uart, true);

    instance->rx_buffer_data = rx_buf;
    instance->rx_buffer_size = sz;
    instance->rx_head = 0;
    instance->tx_buffer_data = NULL;
    instance->tx_size = 0;
    instance->tx_index = 0;
    instance->tx_in_progress = false;
    instance->initialized = true;

    irq_set_exclusive_handler(
        instance->irq,
        bus == UART_BUS_0 ? uart0_irq_handler : uart1_irq_handler
    );
    irq_set_enabled(instance->irq, true);
    uart_set_irq_enables(instance->uart, true, false);
}

/**
 * @brief Deinitialize the UART peripheral.
 *
 * @param bus UART bus instance to deinitialize
 */
void UART_LL_deinit(UART_Bus bus)
{
    if (bus >= UART_BUS_COUNT) {
        THROW(ERROR_INVALID_ARGUMENT);
    }

    if (!g_uart_instances[bus].initialized) {
        return;
    }

    UARTInstance *instance = &g_uart_instances[bus];
    uart_set_irq_enables(instance->uart, false, false);
    irq_set_enabled(instance->irq, false);
    uart_deinit(instance->uart);

    instance->rx_buffer_data = NULL;
    instance->rx_buffer_size = 0;
    instance->rx_head = 0;
    instance->tx_buffer_data = NULL;
    instance->tx_size = 0;
    instance->tx_index = 0;
    instance->tx_in_progress = false;
    instance->tx_complete_callback = NULL;
    instance->rx_complete_callback = NULL;
    instance->idle_callback = NULL;
    instance->initialized = false;
}

/**
 * @brief Start UART transmission
 *
 * @param bus UART bus instance
 * @param buffer Pointer to data to transmit
 * @param size Number of bytes to transmit
 */
void UART_LL_start_dma_tx(UART_Bus bus, uint8_t *buffer, uint32_t size)
{
    if (bus >= UART_BUS_COUNT) {
        THROW(ERROR_INVALID_ARGUMENT);
    }

    if (!buffer || size == 0) {
        THROW(ERROR_INVALID_ARGUMENT);
    }

    if (!g_uart_instances[bus].initialized) {
        THROW(ERROR_DEVICE_NOT_READY);
    }

    UARTInstance *instance = &g_uart_instances[bus];
    if (instance->tx_in_progress) {
        THROW(ERROR_RESOURCE_BUSY);
    }

    instance->tx_buffer_data = buffer;
    instance->tx_size = size;
    instance->tx_index = 0;
    instance->tx_in_progress = true;

    uart_irq_handler(bus);
    if (instance->tx_in_progress) {
        uart_set_irq_enables(instance->uart, true, true);
    }
}

/**
 * @brief Get current position for RX buffer
 *
 * @param bus UART bus instance
 * @return Current receive position
 */
uint32_t UART_LL_get_dma_position(UART_Bus bus)
{
    if (bus >= UART_BUS_COUNT || !g_uart_instances[bus].initialized) {
        return 0;
    }

    return g_uart_instances[bus].rx_head;
}

/**
 * @brief Check if UART TX is busy
 *
 * @param bus UART bus instance
 * @return true if TX is in progress, false otherwise
 */
bool UART_LL_tx_busy(UART_Bus bus)
{
    if (bus >= UART_BUS_COUNT || !g_uart_instances[bus].initialized) {
        return false;
    }

    return g_uart_instances[bus].tx_in_progress;
}

/**
 * @brief Set the TX complete callback function
 * @param bus UART bus instance
 * @param callback Callback function to call when TX is complete
 */
void UART_LL_set_tx_complete_callback(
    UART_Bus bus,
    UART_LL_TxCompleteCallback callback
)
{
    if (bus >= UART_BUS_COUNT) {
        return;
    }
    g_uart_instances[bus].tx_complete_callback = callback;
}

/**
 * @brief Set the RX complete callback function
 * @param bus UART bus instance
 * @param callback Callback function to call when RX buffer is full
 */
void UART_LL_set_rx_complete_callback(
    UART_Bus bus,
    UART_LL_RxCompleteCallback callback
)
{
    if (bus >= UART_BUS_COUNT) {
        return;
    }
    g_uart_instances[bus].rx_complete_callback = callback;
}

/**
 * @brief Set the idle line callback function
 * @param bus UART bus instance
 * @param callback Callback function to call when idle line is detected
 */
void UART_LL_set_idle_callback(UART_Bus bus, UART_LL_IdleCallback callback)
{
    if (bus >= UART_BUS_COUNT) {
        return;
    }
    g_uart_instances[bus].idle_callback = callback;
}
