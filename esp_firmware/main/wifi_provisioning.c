#include "wifi_provisioning.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs.h"

#define PROVISIONING_NVS_NAMESPACE "wifi"
#define PROVISIONING_NVS_SSID_KEY "ssid"
#define PROVISIONING_NVS_PASSWORD_KEY "password"
#define PROVISIONING_COMPLETE_BIT BIT0
#define PROVISIONING_FORM_OVERHEAD_LEN (sizeof("ssid=&password=") - 1u)
#define PROVISIONING_MAX_FORM_LEN \
    (3u * (WIFI_PROVISIONING_MAX_SSID_LEN + WIFI_PROVISIONING_MAX_PASSWORD_LEN) + \
     PROVISIONING_FORM_OVERHEAD_LEN + 1u)
#define PROVISIONING_MAX_RECEIVE_TIMEOUTS 3u

static char const *const TAG = "wifi_provisioning";

static EventGroupHandle_t provisioning_events;
static httpd_handle_t provisioning_server;
static wifi_provisioning_credentials_t pending_credentials;

static char const setup_page[] =
    "<!doctype html><html><head><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<title>PSLab Pico Setup</title></head><body><h1>PSLab Pico Wi-Fi Setup</h1>"
    "<p>Enter the Wi-Fi network that PSLab Pico should join.</p>"
    "<form method=\"post\" action=\"/configure\">"
    "<label>Wi-Fi name<br><input name=\"ssid\" maxlength=\"32\" required></label><br><br>"
    "<label>Password<br><input type=\"password\" name=\"password\" maxlength=\"63\" required></label><br><br>"
    "<button type=\"submit\">Connect</button></form></body></html>";

static int hex_value(char value)
{
    if (value >= '0' && value <= '9') {
        return value - '0';
    }
    if (value >= 'a' && value <= 'f') {
        return value - 'a' + 10;
    }
    if (value >= 'A' && value <= 'F') {
        return value - 'A' + 10;
    }
    return -1;
}

static bool decode_form_value(char const *input, size_t input_len, char *output, size_t output_len)
{
    size_t out = 0;
    for (size_t in = 0; in < input_len; ++in) {
        char value = input[in];
        if (value == '+') {
            value = ' ';
        } else if (value == '%') {
            if (in + 2 >= input_len) {
                return false;
            }
            int high = hex_value(input[++in]);
            int low = hex_value(input[++in]);
            if (high < 0 || low < 0) {
                return false;
            }
            value = (char)((high << 4) | low);
        }
        if (out + 1 >= output_len || value == '\0') {
            return false;
        }
        output[out++] = value;
    }
    output[out] = '\0';
    return true;
}

static bool get_form_value(
    char const *form,
    size_t form_len,
    char const *key,
    char *value,
    size_t value_len
)
{
    size_t key_len = strlen(key);
    size_t offset = 0;
    while (offset < form_len) {
        size_t end = offset;
        while (end < form_len && form[end] != '&') {
            ++end;
        }
        if (end > offset + key_len &&
            memcmp(&form[offset], key, key_len) == 0 &&
            form[offset + key_len] == '=') {
            return decode_form_value(
                &form[offset + key_len + 1],
                end - offset - key_len - 1,
                value,
                value_len
            );
        }
        offset = end + 1;
    }
    return false;
}

static bool credentials_are_valid(wifi_provisioning_credentials_t const *credentials)
{
    size_t password_len = strlen(credentials->password);
    return credentials->ssid[0] != '\0' &&
           password_len >= 8 && password_len <= WIFI_PROVISIONING_MAX_PASSWORD_LEN;
}

static bool save_credentials(wifi_provisioning_credentials_t const *credentials)
{
    nvs_handle_t handle;
    if (nvs_open(PROVISIONING_NVS_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) {
        return false;
    }

    esp_err_t result = nvs_set_str(handle, PROVISIONING_NVS_SSID_KEY, credentials->ssid);
    if (result == ESP_OK) {
        result = nvs_set_str(handle, PROVISIONING_NVS_PASSWORD_KEY, credentials->password);
    }
    if (result == ESP_OK) {
        result = nvs_commit(handle);
    }
    nvs_close(handle);
    return result == ESP_OK;
}

bool wifi_provisioning_load_credentials(wifi_provisioning_credentials_t *credentials)
{
    if (!credentials) {
        return false;
    }

    nvs_handle_t handle;
    if (nvs_open(PROVISIONING_NVS_NAMESPACE, NVS_READONLY, &handle) != ESP_OK) {
        return false;
    }

    size_t ssid_len = sizeof(credentials->ssid);
    size_t password_len = sizeof(credentials->password);
    esp_err_t ssid_result = nvs_get_str(handle, PROVISIONING_NVS_SSID_KEY, credentials->ssid, &ssid_len);
    esp_err_t password_result = nvs_get_str(
        handle,
        PROVISIONING_NVS_PASSWORD_KEY,
        credentials->password,
        &password_len
    );
    nvs_close(handle);

    return ssid_result == ESP_OK && password_result == ESP_OK && credentials_are_valid(credentials);
}

static esp_err_t setup_page_handler(httpd_req_t *request)
{
    httpd_resp_set_type(request, "text/html");
    return httpd_resp_send(request, setup_page, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t configure_handler(httpd_req_t *request)
{
    if (request->content_len <= 0 || request->content_len >= PROVISIONING_MAX_FORM_LEN) {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Invalid configuration request");
        return ESP_FAIL;
    }

    char form[PROVISIONING_MAX_FORM_LEN];
    size_t received = 0;
    unsigned receive_timeouts = 0;
    while (received < (size_t)request->content_len) {
        int bytes = httpd_req_recv(request, form + received, request->content_len - received);
        if (bytes == HTTPD_SOCK_ERR_TIMEOUT) {
            if (++receive_timeouts < PROVISIONING_MAX_RECEIVE_TIMEOUTS) {
                continue;
            }
            httpd_resp_send_err(request, HTTPD_408_REQ_TIMEOUT, "Timed out reading configuration");
            return ESP_FAIL;
        }
        if (bytes <= 0) {
            httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not read configuration");
            return ESP_FAIL;
        }
        receive_timeouts = 0;
        received += (size_t)bytes;
    }
    form[received] = '\0';

    wifi_provisioning_credentials_t credentials = {0};
    if (!get_form_value(form, received, "ssid", credentials.ssid, sizeof(credentials.ssid)) ||
        !get_form_value(form, received, "password", credentials.password, sizeof(credentials.password)) ||
        !credentials_are_valid(&credentials)) {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Use a Wi-Fi name and password with at least 8 characters");
        return ESP_FAIL;
    }
    if (!save_credentials(&credentials)) {
        httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not save Wi-Fi configuration");
        return ESP_FAIL;
    }

    pending_credentials = credentials;
    httpd_resp_set_type(request, "text/html");
    httpd_resp_sendstr(request, "<h1>Saved</h1><p>PSLab Pico is joining the selected Wi-Fi network.</p>");
    xEventGroupSetBits(provisioning_events, PROVISIONING_COMPLETE_BIT);
    return ESP_OK;
}

static bool start_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    if (httpd_start(&provisioning_server, &config) != ESP_OK) {
        return false;
    }

    httpd_uri_t const setup_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = setup_page_handler,
    };
    httpd_uri_t const configure_uri = {
        .uri = "/configure",
        .method = HTTP_POST,
        .handler = configure_handler,
    };
    if (httpd_register_uri_handler(provisioning_server, &setup_uri) != ESP_OK ||
        httpd_register_uri_handler(provisioning_server, &configure_uri) != ESP_OK) {
        httpd_stop(provisioning_server);
        provisioning_server = NULL;
        return false;
    }
    return true;
}

static void stop_server(void)
{
    if (provisioning_server) {
        httpd_stop(provisioning_server);
        provisioning_server = NULL;
    }
}

bool wifi_provisioning_run(wifi_provisioning_credentials_t *credentials)
{
    if (!credentials || strlen(CONFIG_ESP_BRIDGE_PROVISIONING_PASSWORD) < 8) {
        return false;
    }
    if (!provisioning_events) {
        provisioning_events = xEventGroupCreate();
    }
    if (!provisioning_events) {
        return false;
    }
    xEventGroupClearBits(provisioning_events, PROVISIONING_COMPLETE_BIT);

    uint8_t mac[6];
    if (esp_read_mac(mac, ESP_MAC_WIFI_STA) != ESP_OK) {
        return false;
    }

    char ap_ssid[sizeof(((wifi_config_t *)0)->ap.ssid)] = {0};
    snprintf(ap_ssid, sizeof(ap_ssid), "PSLab-Pico-%02X%02X", mac[4], mac[5]);

    wifi_config_t ap_config = {0};
    strlcpy((char *)ap_config.ap.ssid, ap_ssid, sizeof(ap_config.ap.ssid));
    strlcpy(
        (char *)ap_config.ap.password,
        CONFIG_ESP_BRIDGE_PROVISIONING_PASSWORD,
        sizeof(ap_config.ap.password)
    );
    ap_config.ap.ssid_len = strlen(ap_ssid);
    ap_config.ap.channel = 1;
    ap_config.ap.max_connection = 4;
    ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;

    if (esp_wifi_set_mode(WIFI_MODE_AP) != ESP_OK ||
        esp_wifi_set_config(WIFI_IF_AP, &ap_config) != ESP_OK ||
        esp_wifi_start() != ESP_OK ||
        !start_server()) {
        stop_server();
        esp_wifi_stop();
        return false;
    }

    ESP_LOGI(TAG, "setup network: %s", ap_ssid);
    ESP_LOGI(TAG, "open http://192.168.4.1 to configure Wi-Fi");
    xEventGroupWaitBits(
        provisioning_events,
        PROVISIONING_COMPLETE_BIT,
        pdTRUE,
        pdFALSE,
        portMAX_DELAY
    );

    stop_server();
    if (esp_wifi_stop() != ESP_OK) {
        return false;
    }
    *credentials = pending_credentials;
    return true;
}
