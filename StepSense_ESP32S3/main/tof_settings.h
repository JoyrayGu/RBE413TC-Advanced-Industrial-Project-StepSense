#ifndef TOF_SETTINGS_H
#define TOF_SETTINGS_H

#include "esp_err.h"
#include <stdint.h>

/**
 * @brief System Settings Structure (Stored in NVS)
 */
typedef struct {
    uint8_t  peer_mac_l[6];  // Left foot MAC
    uint8_t  peer_mac_r[6];  // Right foot MAC
    uint8_t  wifi_ch;       // WiFi Channel (1-13)
    uint32_t uart_baud;    // UART1 Baud (Link to STM32)
    uint8_t  mirror_enabled; // 0: OFF, 1: ON (Forward raw to PC)
} TofSettings_t;

/**
 * @brief Initialize NVS and load settings. 
 * If no settings exist, it will write defaults.
 */
esp_err_t tof_settings_init(void);

/**
 * @brief Get global settings pointer
 */
TofSettings_t* tof_settings_get(void);

/**
 * @brief Save current settings to NVS
 */
esp_err_t tof_settings_save(void);

/**
 * @brief Request an asynchronous NVS save from a background task.
 * Safe to call from ESP-NOW callbacks.
 */
void tof_settings_request_save(void);

#endif // TOF_SETTINGS_H
