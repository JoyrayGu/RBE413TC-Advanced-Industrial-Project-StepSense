#include "usb_cdc_bridge.h"
#include "esp_log.h"
#include "driver/usb_serial_jtag.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hw_interaction.h"
#include "status_manager.h"

static const char *TAG = "USB_CDC";
static volatile bool g_is_connected = false;

static void usb_monitor_task(void *arg) {
    uint8_t rx_buf[64];
    while(1) {
        // 1. Check for incoming data (direct proof of host interaction)
        int rx_len = usb_serial_jtag_read_bytes(rx_buf, sizeof(rx_buf), 0);
        
        if (rx_len > 0) {
            if (!g_is_connected) {
                ESP_LOGI(TAG, "Host detected (Data received)");
                g_is_connected = true;
                status_manager_update_usb(true);
            }
            
            // Look for handshake 'A'
            for(int i=0; i<rx_len; i++){
                if (rx_buf[i] == 'A') {
                    const char ack = 'A';
                    usb_serial_jtag_write_bytes(&ack, 1, 0);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void usb_cdc_bridge_init(void) {
    // Driver installed by esp_console (tof_cli)
    xTaskCreate(usb_monitor_task, "usb_monitor", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "USB CDC Bridging Initialized (Shared with Console)");
}

void usb_cdc_bridge_send(const uint8_t *data, size_t len) {
    if (len > 0) {
        // Try to write. If it takes too long (>10ms), assume host isn't consuming
        int written = usb_serial_jtag_write_bytes(data, len, pdMS_TO_TICKS(10));
        
        if (written < len) {
            if (g_is_connected) {
                ESP_LOGW(TAG, "Buffer full/Host not consuming. Assuming disconnected.");
                g_is_connected = false;
                status_manager_update_usb(false);
            }
        } else {
            if (!g_is_connected) {
                g_is_connected = true;
                status_manager_update_usb(true);
            }
        }
    }
}

bool usb_cdc_is_connected(void) {
    return g_is_connected;
}
