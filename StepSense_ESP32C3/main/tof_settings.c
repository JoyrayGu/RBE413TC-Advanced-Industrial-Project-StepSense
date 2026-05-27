#include "tof_settings.h"
#include "tof_protocol.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "TOF_SETTINGS";
static TofSettings_t g_settings;
#define NVS_NAMESPACE "storage"
#define NVS_KEY_STRUCT "tof_cfg"

static void set_defaults(void) {
    g_settings.identity = IDENTITY_UNASSIGNED; // Default to NULL
    memset(g_settings.local_mac, 0, 6);
    memset(g_settings.peer_mac, 0xFF, 6); // Broadcast default
    g_settings.wifi_ch = 1;
    g_settings.uart_baud = 2000000;
    g_settings.mirror_enabled = 0; // Default Mirror OFF
    g_settings.is_paired = 0;      // Default Unpaired
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
    } else if (err == ESP_OK) {
        ESP_LOGI(TAG, "Settings loaded from NVS (Identity: %s, Ch: %d)", 
                 g_settings.identity == 0 ? "Left" : "Right", g_settings.wifi_ch);
    }

    nvs_close(handle);
    return err;
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
