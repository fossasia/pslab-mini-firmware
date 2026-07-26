#include "application/gateway/uart_commands.h"

#include <string.h>

#include "system/bus/uart.h"
#include "platform/platform.h"
#include "util/error.h"
#include "util/util.h"

enum {
    UART_GATEWAY_RX_BUFFER_SIZE = 512,
    UART_GATEWAY_TX_BUFFER_SIZE = 512,
    UART_GATEWAY_MIN_BAUD = 1200,
    UART_GATEWAY_MAX_BAUD = 3000000,
    UART_GATEWAY_MAX_TIMEOUT_MS = 60000,
    UART_GATEWAY_TRANSACTION_IDLE_MS = 5,
};

static UART_Handle *gateway_uart;
static uint8_t rx_data[UART_GATEWAY_RX_BUFFER_SIZE];
static uint8_t tx_data[UART_GATEWAY_TX_BUFFER_SIZE];
static CircularBuffer rx_buffer;
static CircularBuffer tx_buffer;

static struct {
    uint32_t bus;
    uint32_t baud;
    uint32_t timeout_ms;
} state = {
    .bus = UART_GATEWAY_DEFAULT_BUS,
    .baud = UART_GATEWAY_DEFAULT_BAUD,
    .timeout_ms = UART_GATEWAY_DEFAULT_TIMEOUT_MS,
};

static bool bus_is_allowed(uint32_t bus)
{
    /*
     * UART0 is currently owned by the firmware logging/stdout path.
     * Keep the gateway on UART1 until bus ownership is made explicit.
     */
    return bus == 1 && bus < UART_get_bus_count();
}

bool uart_gateway_set_bus(uint32_t bus)
{
    if (gateway_uart || !bus_is_allowed(bus)) {
        return false;
    }

    state.bus = bus;
    return true;
}

uint32_t uart_gateway_get_bus(void) { return state.bus; }

bool uart_gateway_set_baud(uint32_t baud)
{
    if (gateway_uart || baud < UART_GATEWAY_MIN_BAUD ||
        baud > UART_GATEWAY_MAX_BAUD) {
        return false;
    }

    state.baud = baud;
    return true;
}

uint32_t uart_gateway_get_baud(void) { return state.baud; }

bool uart_gateway_set_timeout(uint32_t timeout_ms)
{
    if (timeout_ms > UART_GATEWAY_MAX_TIMEOUT_MS) {
        return false;
    }

    state.timeout_ms = timeout_ms;
    return true;
}

uint32_t uart_gateway_get_timeout(void) { return state.timeout_ms; }

bool uart_gateway_open(void)
{
    if (gateway_uart) {
        return true;
    }

    if (!bus_is_allowed(state.bus)) {
        return false;
    }

    circular_buffer_init(&rx_buffer, rx_data, sizeof(rx_data));
    circular_buffer_init(&tx_buffer, tx_data, sizeof(tx_data));

    Error err = ERROR_NONE;
    TRY
    {
        gateway_uart = UART_init_with_baud(
            state.bus,
            &rx_buffer,
            &tx_buffer,
            state.baud
        );
    }
    CATCH(err)
    {
        gateway_uart = NULL;
        (void)err;
        return false;
    }

    return gateway_uart != NULL;
}

void uart_gateway_close(void)
{
    if (!gateway_uart) {
        return;
    }

    UART_Handle *handle = gateway_uart;
    gateway_uart = NULL;

    Error err = ERROR_NONE;
    TRY { UART_deinit(handle); }
    CATCH(err)
    {
        (void)err;
    }
}

bool uart_gateway_is_open(void) { return gateway_uart != NULL; }

uint32_t uart_gateway_write(uint8_t const *data, size_t len)
{
    if (!gateway_uart || !data || len == 0) {
        return 0;
    }

    if (len > UART_GATEWAY_MAX_TRANSFER) {
        len = UART_GATEWAY_MAX_TRANSFER;
    }

    return UART_write(gateway_uart, data, (uint32_t)len);
}

uint32_t uart_gateway_read(uint8_t *data, size_t max_len)
{
    if (!gateway_uart || !data || max_len == 0) {
        return 0;
    }

    if (max_len > UART_GATEWAY_MAX_TRANSFER) {
        max_len = UART_GATEWAY_MAX_TRANSFER;
    }

    return UART_read(gateway_uart, data, (uint32_t)max_len);
}

uint32_t uart_gateway_available(void)
{
    return gateway_uart ? UART_rx_available(gateway_uart) : 0;
}

void uart_gateway_clear(void)
{
    uint8_t scratch[64];

    if (!gateway_uart) {
        return;
    }

    while (UART_rx_available(gateway_uart) > 0) {
        if (UART_read(gateway_uart, scratch, sizeof(scratch)) == 0) {
            break;
        }
    }
}

bool uart_gateway_flush(void)
{
    return gateway_uart && UART_flush(gateway_uart, state.timeout_ms);
}

uint32_t uart_gateway_transact(
    uint8_t const *tx_data,
    size_t tx_len,
    uint8_t *rx_data_out,
    size_t rx_max_len
)
{
    if (!gateway_uart || !tx_data || tx_len == 0 || !rx_data_out ||
        rx_max_len == 0) {
        return 0;
    }

    uart_gateway_clear();

    uint32_t written = uart_gateway_write(tx_data, tx_len);
    if (written != tx_len || !uart_gateway_flush()) {
        return 0;
    }

    uint32_t start = PLATFORM_get_tick();
    uint32_t last_change = start;
    uint32_t last_available = 0;

    while (true) {
        uint32_t now = PLATFORM_get_tick();
        uint32_t available = uart_gateway_available();

        if (available != last_available) {
            last_available = available;
            last_change = now;
        }

        if (available >= rx_max_len ||
            (available > 0 &&
             (now - last_change) >= UART_GATEWAY_TRANSACTION_IDLE_MS)) {
            break;
        }

        if (state.timeout_ms && (now - start) >= state.timeout_ms) {
            break;
        }
    }

    return uart_gateway_read(rx_data_out, rx_max_len);
}
