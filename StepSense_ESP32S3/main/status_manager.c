#include "status_manager.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "STATUS_MGR";
static int64_t last_recv_us[2] = {0, 0};
static bool usb_is_open = false;
static bool is_booting = true;
static int64_t boot_time_us = 0;

#define RECV_TIMEOUT_US (2000000) // 2 seconds
#define BOOT_DURATION_US (3000000) // 3 seconds for Rainbow

void status_manager_init(void) {
    boot_time_us = esp_timer_get_time();
    is_booting = true;
    ESP_LOGI(TAG, "Status manager initialized (Booting state active)");
}

void status_manager_update_foot(int identity) {
    if (identity >= 0 && identity < 2) {
        last_recv_us[identity] = esp_timer_get_time();
    }
}

void status_manager_update_usb(bool open) {
    usb_is_open = open;
}

s3_led_state_t status_manager_get_recommended_led(void) {
    int64_t now = esp_timer_get_time();

    // 1. Rainbow for Booting
    if (is_booting) {
        if (now - boot_time_us > BOOT_DURATION_US) {
            is_booting = false;
        } else {
            return S3_LED_RAINBOW;
        }
    }

    bool left_active = (now - last_recv_us[FOOT_LEFT] < RECV_TIMEOUT_US) && (last_recv_us[FOOT_LEFT] > 0);
    bool right_active = (now - last_recv_us[FOOT_RIGHT] < RECV_TIMEOUT_US) && (last_recv_us[FOOT_RIGHT] > 0);
    
    // Check if either foot is CURRENTLY streaming (received in last 300ms)
    bool data_flowing = ((now - last_recv_us[FOOT_LEFT] < 300000) && (last_recv_us[FOOT_LEFT] > 0)) ||
                        ((now - last_recv_us[FOOT_RIGHT] < 300000) && (last_recv_us[FOOT_RIGHT] > 0));

    // 2. Both feet connected
    if (left_active && right_active) {
        if (usb_is_open) {
            return data_flowing ? S3_LED_GREEN_ACTIVE : S3_LED_GREEN_BREATH;
        } else {
            return data_flowing ? S3_LED_PURPLE_ACTIVE : S3_LED_PURPLE_BREATH;
        }
    }

    // 3. Single foot connected
    if (left_active || right_active) {
        return data_flowing ? S3_LED_YELLOW_ACTIVE : S3_LED_YELLOW_PULSE;
    }

    // 4. Default: Listening
    return S3_LED_CYAN_BREATH;
}
