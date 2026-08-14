idf_component_register(
    SRCS "main.c" "wifi_provisioning.c" "mdns_service.c"
    INCLUDE_DIRS "."
    REQUIRES
        driver
        esp_event
        esp_http_server
        esp_hw_support
        esp_netif
        esp_timer
        esp_wifi
        lwip
        mdns
        nvs_flash
)
