#ifndef ESP_LL_H
#define ESP_LL_H

typedef enum { ESP_EN, ESP_BOOT, ESP_DATA_READY, ESP_GPIO_NUM } ESP_Pin;

/**
 * @brief Initialize the ESP32 interface pins
 */
void ESP_LL_init(void);

/**
 * @brief Set the state of ESP32 interface pins
 *
 * @param state The state to set (high/low)
 */
void ESP_LL_set(ESP_Pin pin, bool state);

#endif // ESP_LL_H
