#ifndef WIFI_PROVISIONING_H
#define WIFI_PROVISIONING_H

#include <stdbool.h>
#include <stdint.h>

#define WIFI_PROVISIONING_MAX_SSID_LEN 32
#define WIFI_PROVISIONING_MAX_PASSWORD_LEN 63

typedef struct {
    char ssid[WIFI_PROVISIONING_MAX_SSID_LEN + 1];
    char password[WIFI_PROVISIONING_MAX_PASSWORD_LEN + 1];
} wifi_provisioning_credentials_t;

bool wifi_provisioning_load_credentials(wifi_provisioning_credentials_t *credentials);

/*
 * Starts the temporary PSLab Pico setup access point and waits for the user to
 * submit station credentials through http://192.168.4.1.
 *
 * The Wi-Fi driver and default AP network interface must already be created.
 * This function stops the access point before returning successfully.
 */
bool wifi_provisioning_run(wifi_provisioning_credentials_t *credentials);

#endif
