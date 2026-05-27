#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_now.h"
#include "tof_settings.h"
#include "hw_interaction.h"
#include "status_manager.h"
#include "usb_cdc_bridge.h"
#include "data_aggregator.h"
#include "espnow_rx.h"
#include "tof_cli.h"

static const char *TAG = "S3_MAIN";

static void status_evaluation_task(void *arg) {
    while(1) {
        s3_led_state_t recommended = status_manager_get_recommended_led();
        hw_interaction_set_led(recommended);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void sync_broadcast_task(void *arg) {
    const uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    SyncPacket_t sync_pkt = {
        .msg_type = MSG_TYPE_SYNC
    };
    
    // Note: Logging removed to prevent CLI flooding at 10Hz
    while(1) {
        sync_pkt.s3_sync_time = esp_timer_get_time() / 1000;
        esp_now_send(broadcast_mac, (uint8_t *)&sync_pkt, sizeof(sync_pkt));
        vTaskDelay(pdMS_TO_TICKS(100)); // 10Hz
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "StepSense Bridge Unit (ESP32-S3) Booting");
    
    // 1. Core Systems Init
    ESP_ERROR_CHECK(tof_settings_init());
    status_manager_init();
    
    // 2. Hardware / Indicators
    hw_interaction_init();

    // Reset mirror on boot for clean CLI prompt
    tof_settings_get()->mirror_enabled = 0;
    ESP_ERROR_CHECK(tof_cli_init());

    // 3. Components
    data_aggregator_init();
    usb_cdc_bridge_init();

    // 4. Wireless Stack
    espnow_rx_init();

    // 5. Background Monitor
    xTaskCreate(status_evaluation_task, "status_eval", 2048, NULL, 5, NULL);
    xTaskCreate(sync_broadcast_task, "sync_bcast", 4096, NULL, 5, NULL);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
