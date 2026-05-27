#include "hw_interaction.h"
#include "led_strip.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_log.h"
#include <math.h>

#define BOOT_BUTTON_PIN 9
#define WS2812_PIN 10
static const char *TAG = "HW_INT";
// Breathe Effect Configuration
#define BREATHE_MIN         5
#define BREATHE_MAX         180
#define BREATHE_STEP        3
#define BREATHE_INTERVAL_MS 30

static led_strip_handle_t led_strip;
static led_state_t current_state = LED_STATE_RAINBOW;
static led_state_t return_state = LED_STATE_RAINBOW;

static void button_task(void *arg) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BOOT_BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    uint32_t press_count = 0;
    while (1) {
        if (gpio_get_level(BOOT_BUTTON_PIN) == 0) {
            press_count++;
            if (press_count >= 30) { // 3 seconds at 100ms interval
                ESP_LOGW(TAG, "BOOT button long press detected! Erasing NVS and restarting...");
                hw_interaction_set_led(LED_STATE_RED_SOLID);
                vTaskDelay(pdMS_TO_TICKS(500));
                nvs_flash_deinit();
                nvs_flash_erase();
                esp_restart();
            }
        } else {
            press_count = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void led_task(void *arg) {
    uint16_t hue = 0;
    int breathe_val = 0;
    int breathe_dir = 1;
    bool toggle = true;

    while (1) {
        switch (current_state) {
            case LED_STATE_RAINBOW:
                led_strip_set_pixel_hsv(led_strip, 0, hue, 255, 100);
                hue = (hue + 5) % 360;
                vTaskDelay(pdMS_TO_TICKS(20));
                break;
            case LED_STATE_YELLOW_BLINK: // 500ms blink
                toggle = !toggle;
                if (toggle) led_strip_set_pixel(led_strip, 0, 100, 100, 0); // Yellow
                else led_strip_set_pixel(led_strip, 0, 0, 0, 0);
                vTaskDelay(pdMS_TO_TICKS(250));
                break;
            case LED_STATE_GREEN_BREATHE:
                breathe_val += breathe_dir * BREATHE_STEP;
                if (breathe_val >= BREATHE_MAX) {
                    breathe_val = BREATHE_MAX;
                    breathe_dir = -1;
                } else if (breathe_val <= BREATHE_MIN) {
                    breathe_val = BREATHE_MIN;
                    breathe_dir = 1;
                }
                led_strip_set_pixel(led_strip, 0, 0, breathe_val, 0);
                vTaskDelay(pdMS_TO_TICKS(BREATHE_INTERVAL_MS));
                break;
            case LED_STATE_GREEN_FAST_BLINK:
                toggle = !toggle;
                if (toggle) led_strip_set_pixel(led_strip, 0, 0, 150, 0); // Green
                else led_strip_set_pixel(led_strip, 0, 0, 0, 0);
                vTaskDelay(pdMS_TO_TICKS(80)); // Fast blink (~6Hz)
                break;
            case LED_STATE_GREEN_FLASH: // Keep as one-off blip for legacy/other use
                led_strip_set_pixel(led_strip, 0, 0, 255, 0);
                led_strip_refresh(led_strip);
                vTaskDelay(pdMS_TO_TICKS(50));
                led_strip_set_pixel(led_strip, 0, 0, 0, 0);
                led_strip_refresh(led_strip);
                vTaskDelay(pdMS_TO_TICKS(50));
                current_state = return_state; 
                break;
            case LED_STATE_RED_SLOW: // 1s blink
                toggle = !toggle;
                if (toggle) led_strip_set_pixel(led_strip, 0, 100, 0, 0);
                else led_strip_set_pixel(led_strip, 0, 0, 0, 0);
                vTaskDelay(pdMS_TO_TICKS(500));
                break;
            case LED_STATE_RED_SOLID:
                led_strip_set_pixel(led_strip, 0, 255, 0, 0);
                vTaskDelay(pdMS_TO_TICKS(500));
                break;
        }
        if (current_state != LED_STATE_GREEN_FLASH) {
            led_strip_refresh(led_strip);
        }
    }
}

void hw_interaction_init(void) {
    led_strip_config_t strip_config = {
        .strip_gpio_num = WS2812_PIN,
        .max_leds = 1, 
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, 
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    led_strip_clear(led_strip);

    xTaskCreate(button_task, "btn_task", 4096, NULL, 5, NULL);
    xTaskCreate(led_task, "led_task", 2048, NULL, 4, NULL);
}

void hw_interaction_set_led(led_state_t state) {
    if (state == LED_STATE_GREEN_FLASH) {
        if (current_state != LED_STATE_GREEN_FLASH) {
            return_state = current_state;
        }
    }
    current_state = state;
}
