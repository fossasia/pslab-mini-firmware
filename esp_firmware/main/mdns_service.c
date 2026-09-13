#include "mdns_service.h"

#include <stddef.h>

#include "esp_err.h"
#include "esp_log.h"
#include "mdns.h"

#define MDNS_HOSTNAME "pslab-pico"
#define MDNS_INSTANCE_NAME "PSLab Pico"
#define MDNS_SERVICE_TYPE "_pslab"
#define MDNS_SERVICE_PROTOCOL "_tcp"

static char const *const TAG = "mdns_service";
static bool service_started;

bool mdns_service_start(uint16_t scpi_port)
{
    if (service_started) {
        return true;
    }

    esp_err_t result = mdns_init();
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "mdns_init failed: %s", esp_err_to_name(result));
        return false;
    }

    result = mdns_hostname_set(MDNS_HOSTNAME);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "mdns_hostname_set failed: %s", esp_err_to_name(result));
        mdns_free();
        return false;
    }

    result = mdns_instance_name_set(MDNS_INSTANCE_NAME);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "mdns_instance_name_set failed: %s", esp_err_to_name(result));
        mdns_free();
        return false;
    }

    mdns_txt_item_t txt[] = {
        {"model", "pslab-pico"},
        {"control", "scpi"},
        {"waveform", "udp"},
    };
    result = mdns_service_add(
        MDNS_INSTANCE_NAME,
        MDNS_SERVICE_TYPE,
        MDNS_SERVICE_PROTOCOL,
        scpi_port,
        txt,
        sizeof(txt) / sizeof(txt[0])
    );
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "mdns_service_add failed: %s", esp_err_to_name(result));
        mdns_free();
        return false;
    }

    service_started = true;
    ESP_LOGI(TAG, "mDNS ready: %s.local (%s.%s:%u)",
             MDNS_HOSTNAME,
             MDNS_SERVICE_TYPE,
             MDNS_SERVICE_PROTOCOL,
             (unsigned)scpi_port);
    return true;
}
