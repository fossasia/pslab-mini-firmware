/**
 * @file scpi_test_helpers.c
 * @brief Implementation of common test helper functions for SCPI protocol testing
 *
 * @author PSLab Team
 * @date 2025-10-03
 */

#include "scpi_test_helpers.h"
#include <string.h>

// ============================================================================
// Mock USB callback functions
// ============================================================================

uint32_t scpi_mock_usb_write_capture(USB_Handle *handle, uint8_t const *data, uint32_t len, int cmock_num_calls)
{
    (void)handle;
    (void)cmock_num_calls;

    if (g_scpi_test_captured_response_len + len < SCPI_TEST_RESPONSE_BUFFER_SIZE) {
        memcpy(g_scpi_test_captured_response + g_scpi_test_captured_response_len, data, len);
        g_scpi_test_captured_response_len += len;
        g_scpi_test_captured_response[g_scpi_test_captured_response_len] = '\0';
    }

    return len;
}

uint32_t scpi_mock_usb_read_inject(USB_Handle *handle, uint8_t *buffer, uint32_t max_len, int cmock_num_calls)
{
    (void)handle;
    (void)cmock_num_calls;

    if (g_scpi_test_injected_data_len == 0) {
        return 0;
    }

    uint32_t to_copy = (g_scpi_test_injected_data_len > max_len) ? max_len : (uint32_t)g_scpi_test_injected_data_len;
    memcpy(buffer, g_scpi_test_injected_data, to_copy);

    // Shift remaining data
    memmove(g_scpi_test_injected_data, g_scpi_test_injected_data + to_copy, g_scpi_test_injected_data_len - to_copy);
    g_scpi_test_injected_data_len -= to_copy;

    return to_copy;
}

bool scpi_mock_usb_rx_ready_check(USB_Handle *handle, int cmock_num_calls)
{
    (void)handle;
    (void)cmock_num_calls;
    return g_scpi_test_injected_data_len > 0;
}

// ============================================================================
// Helper functions
// ============================================================================

void scpi_inject_usb_command(const char *command)
{
    size_t cmd_len = strlen(command);
    if (g_scpi_test_injected_data_len + cmd_len < SCPI_TEST_USB_BUFFER_SIZE) {
        memcpy(g_scpi_test_injected_data + g_scpi_test_injected_data_len, command, cmd_len);
        g_scpi_test_injected_data_len += cmd_len;
    }
}

const char *scpi_get_captured_response(void)
{
    return g_scpi_test_captured_response;
}

void scpi_clear_captured_response(void)
{
    g_scpi_test_captured_response_len = 0;
    memset(g_scpi_test_captured_response, 0, sizeof(g_scpi_test_captured_response));
}

void scpi_run_protocol_with_usb_mocks(USB_Handle *usb_handle)
{
    USB_task_Expect(usb_handle);
    USB_rx_ready_StubWithCallback(scpi_mock_usb_rx_ready_check);
    USB_read_StubWithCallback(scpi_mock_usb_read_inject);
    USB_write_StubWithCallback(scpi_mock_usb_write_capture);
    protocol_task();
}