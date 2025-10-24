#ifndef ESP_H
#define ESP_H

/**
 * @brief Initialize the ESP32 interface
 */
void ESP_init(void);

/**
 * @brief Reset the ESP32
 */
void ESP_reset(void);

/**
 * @brief Enter the ESP32 bootloader
 */
void ESP_enter_bootloader(void);

#endif // ESP_H
