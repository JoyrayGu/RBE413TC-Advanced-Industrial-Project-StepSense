#include "tof_settings.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "TOF_SETTINGS";
static TofSettings_t g_settings;
static bool g_is_dirty = false;

#define NVS_NAMESPACE "storage"
#define NVS_KEY_STRUCT "tof_cfg"

static void nvs_save_task(void *arg) {
    while (1) {
        if (g_is_dirty) {
            g_is_dirty = false;
            tof_settings_save();
            ESP_LOGI(TAG, "Async NVS Commit Finished.");
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Check every second
    }
}

static void set_defaults(void) {
    memset(g_settings.peer_mac_l, 0xFF, 6); // Broadcast default
    memset(g_settings.peer_mac_r, 0xFF, 6); // Broadcast default
    g_settings.wifi_ch = 1;
    g_settings.uart_baud = 2000000;
    g_settings.mirror_enabled = 0; // Default Mirror OFF
}

esp_err_t tof_settings_init(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    nvs_handle_t handle;
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    size_t size = sizeof(TofSettings_t);
    err = nvs_get_blob(handle, NVS_KEY_STRUCT, &g_settings, &size);
    
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No settings found in NVS, applying defaults...");
        set_defaults();
        err = nvs_set_blob(handle, NVS_KEY_STRUCT, &g_settings, sizeof(TofSettings_t));
        if (err == ESP_OK) nvs_commit(handle);
    } else    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Settings loaded from NVS (L: %02X:%02X..., R: %02X:%02X...)", 
                 g_settings.peer_mac_l[0], g_settings.peer_mac_l[1],
                 g_settings.peer_mac_r[0], g_settings.peer_mac_r[1]);
    }

    nvs_close(handle);

    // Start background save task
    xTaskCreate(nvs_save_task, "nvs_save", 2048, NULL, 1, NULL);

    return err;
}

void tof_settings_request_save(void) {
    g_is_dirty = true;
    ESP_LOGI(TAG, "NVS Save Requested (Async).");
}

TofSettings_t* tof_settings_get(void) {
    return &g_settings;
}

esp_err_t tof_settings_save(void) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    err = nvs_set_blob(handle, NVS_KEY_STRUCT, &g_settings, sizeof(TofSettings_t));
    if (err == ESP_OK) nvs_commit(handle);
    
    nvs_close(handle);
    ESP_LOGI(TAG, "Settings saved to NVS.");
    return err;
}
