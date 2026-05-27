#include "hw_interaction.h"
#include "esp_log.h"
#include "led_strip.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>
#include "nvs_flash.h"
#include "esp_system.h"
#include "driver/gpio.h"

static const char *TAG = "S3_HW";

#define IO_WS2812 3  
#define BOOT_BUTTON_PIN 0 // S3 DevKit BOOT button is usually GPIO 0

static led_strip_handle_t led_strip;
static volatile s3_led_state_t current_state = S3_LED_RAINBOW;
static volatile int g_flash_timer = 0; 

static void button_task(void *arg) {
    gpio_reset_pin(BOOT_BUTTON_PIN);
    gpio_set_direction(BOOT_BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_pullup_en(BOOT_BUTTON_PIN);

    int press_ticks = 0;
    while(1) {
        if (gpio_get_level(BOOT_BUTTON_PIN) == 0) {
            press_ticks++;
            if (press_ticks > 30) { // 3 seconds
                ESP_LOGW(TAG, "BOOT BUTTON HELD 3S! Erasing Pairing NVS and Restarting...");
                nvs_flash_deinit();
                nvs_flash_erase();
                esp_restart();
            }
        } else {
            press_ticks = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void breathing_led_task(void *arg) {
    float angle = 0;
    uint32_t hue = 0;
    
    while(1) {
        // Priority 1: Green Flash (Momentary override)
        if (g_flash_timer > 0) {
            led_strip_set_pixel(led_strip, 0, 0, 255, 0); // Solid Bright Green
            led_strip_refresh(led_strip);
            g_flash_timer--;
            vTaskDelay(pdMS_TO_TICKS(30)); 
            continue;
        }

        // Priority 2: Standard Animations
        float intensity = (sin(angle) + 1.0f) / 2.0f; // 0.0 to 1.0
        uint8_t brightness = (uint8_t)(intensity * 150) + 20; // 20 to 170
        uint8_t r=0, g=0, b=0;
        
        switch (current_state) {
            case S3_LED_RAINBOW:
                led_strip_set_pixel_hsv(led_strip, 0, hue, 255, 100);
                hue = (hue + 5) % 360;
                break;
 
            case S3_LED_CYAN_BREATH:
                g = brightness; b = brightness;
                led_strip_set_pixel(led_strip, 0, r, g, b);
                break;
 
            case S3_LED_YELLOW_PULSE:
            case S3_LED_YELLOW_ACTIVE:
                r = brightness; g = (uint8_t)(brightness * 0.8f);
                led_strip_set_pixel(led_strip, 0, r, g, b);
                break;
 
            case S3_LED_GREEN_BREATH:
            case S3_LED_GREEN_ACTIVE:
                g = brightness;
                led_strip_set_pixel(led_strip, 0, r, g, b);
                break;
 
            case S3_LED_PURPLE_BREATH:
            case S3_LED_PURPLE_ACTIVE:
                r = brightness; b = brightness;
                led_strip_set_pixel(led_strip, 0, r, g, b);
                break;
 
            case S3_LED_OFF:
            default:
                led_strip_set_pixel(led_strip, 0, 0, 0, 0);
                break;
        }
 
        led_strip_refresh(led_strip);
        
        // Speed control: Active states breathe much faster
        bool is_active = (current_state == S3_LED_YELLOW_ACTIVE || 
                          current_state == S3_LED_GREEN_ACTIVE || 
                          current_state == S3_LED_PURPLE_ACTIVE);
        
        if (is_active) angle += 0.2f;  // Fast breath
        else angle += 0.05f;           // Normal breath
        
        if (angle > 6.28f) angle = 0;
        
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void hw_interaction_init(void) {
    led_strip_config_t strip_config = {
        .strip_gpio_num = IO_WS2812,
        .max_leds = 1,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };
    
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, 
        .flags.with_dma = false,
    };
    
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    led_strip_clear(led_strip);
    
    xTaskCreate(breathing_led_task, "s3_led_task", 2048, NULL, 5, NULL);
    xTaskCreate(button_task, "s3_btn_task", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "Hardware interactions initialized (RGB LED + Button Task)");
}

void hw_interaction_set_led(s3_led_state_t state) {
    current_state = state;
}

void hw_interaction_trigger_flash(void) {
    g_flash_timer = 3; // ~100ms of green flash
}
