#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "hw_interaction.h"
#include "espnow_sm.h"
#include "tof_uart_rx.h"
#include "usb_cdc_bridge.h"
#include "tof_settings.h"
#include "tof_cli.h"

#define TOF_UART_TX_IO 7
#define TOF_UART_RX_IO 6
#define TOF_BAUD_RATE 2000000

static const char *TAG = "APP_MAIN";

/**
 * @brief Handle received packet from STM32F401
 */
static void on_packet_received(const UartPacket_t *packet) {
    // 1. Send to S3 via ESP-NOW
    espnow_sm_send_data(packet);

    // 2. Stream to PC via USB Type-C ONLY if mirror enabled
    if (tof_settings_get()->mirror_enabled) {
        usb_cdc_bridge_send(packet);
    }
}

void app_main(void) {
    // 1. Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(tof_settings_init());
    
    // Force mirror_enabled off on boot for clean CLI prompt
    tof_settings_get()->mirror_enabled = 0;

    // 1.5 Boot GPIO config (Wait for UART driver to handle pins)
    ESP_LOGI(TAG, "Starting StepSense ESP32-C3 Firmware");

    // 2. Initialize Hardware Interactions (LED, Boot Button)
    hw_interaction_init();
    
    // 3. Initialize USB CDC Bridge & CLI
    usb_cdc_bridge_init();
    ESP_ERROR_CHECK(tof_cli_init());

    // 4. Initialize ESP-NOW State Machine
    espnow_sm_init();

    // 5. Initialize UART to STM32, wait for matching packets
    tof_uart_rx_set_callback(on_packet_received);
    tof_uart_rx_init(TOF_UART_TX_IO, TOF_UART_RX_IO, TOF_BAUD_RATE);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
