#include "espnow_sm.h"
#include "hw_interaction.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tof_settings.h"
#include "tof_protocol.h"
#include <string.h>

static const char *TAG = "ESPNOW_SM";
static SystemState_t current_state = STATE_INIT;
static void save_target_mac(const uint8_t *mac);
static bool load_target_mac(void);

static bool has_target_mac = false;
static int tx_fail_count = 0;
static uint32_t last_data_time = 0;

/* --- Time Sync Stats --- */
static uint32_t last_s3_sync_time = 0;
static uint32_t c3_tick_at_sync = 0;

static void save_target_mac(const uint8_t *mac) {
    TofSettings_t *s = tof_settings_get();
    memcpy(s->peer_mac, mac, 6);
    tof_settings_save();
    has_target_mac = true;
}

static bool load_target_mac() {
    TofSettings_t *s = tof_settings_get();
    uint8_t zero_mac[6] = {0};
    uint8_t bcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    if (memcmp(s->peer_mac, zero_mac, 6) != 0 && memcmp(s->peer_mac, bcast_mac, 6) != 0) {
        has_target_mac = true;
    }
    return has_target_mac;
}

static void on_recv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    const uint8_t *mac = recv_info->src_addr;
    if (len == sizeof(PairPacket_t)) {
        PairPacket_t *pkt = (PairPacket_t *)data;
        if (pkt->msg_type == MSG_TYPE_PAIR_ACK && current_state == STATE_DISCOVERY) {
            ESP_LOGI(TAG, "Received PAIR_ACK from %02X:%02X:%02X:%02X:%02X:%02X. Binding...",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            save_target_mac(mac);
            
            // Add as peer
            esp_now_peer_info_t peer = {};
            peer.channel = 1;
            peer.encrypt = false;
            memcpy(peer.peer_addr, mac, 6);
            if (!esp_now_is_peer_exist(mac)) {
                esp_now_add_peer(&peer);
            }
            
            current_state = STATE_LOCKED;
            hw_interaction_set_led(LED_STATE_GREEN_FAST_BLINK);
            tx_fail_count = 0;
        } else if (pkt->msg_type == MSG_TYPE_SYNC) {
            SyncPacket_t *s_pkt = (SyncPacket_t *)data;
            last_s3_sync_time = s_pkt->s3_sync_time;
            c3_tick_at_sync = xTaskGetTickCount() * portTICK_PERIOD_MS;
        }
    }
}

static void on_send(const esp_now_send_info_t *mac_info, esp_now_send_status_t status) {
    if (current_state == STATE_LOCKED) {
        if (status != ESP_NOW_SEND_SUCCESS) {
            tx_fail_count++;
            if (tx_fail_count >= 10) {
                ESP_LOGW(TAG, "10 consecutive TX failures. Disconnected. Returning to DISCOVERY.");
                current_state = STATE_DISCOVERY;
                hw_interaction_set_led(LED_STATE_RED_SLOW);
            }
        } else {
            tx_fail_count = 0;
        }
    }
}

static void broadcast_task(void *arg) {
    uint8_t bcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    esp_now_peer_info_t peer = {};
    peer.channel = 1;
    peer.encrypt = false;
    memcpy(peer.peer_addr, bcast_mac, 6);
    if (!esp_now_is_peer_exist(bcast_mac)) {
        esp_now_add_peer(&peer);
    }

    while (1) {
        if (current_state == STATE_DISCOVERY) {
            TofSettings_t *settings = tof_settings_get();
            
            // Block pairing if identity is not assigned
            if (settings->identity == IDENTITY_UNASSIGNED) {
                hw_interaction_set_led(LED_STATE_RED_SLOW);
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }

            hw_interaction_set_led(LED_STATE_YELLOW_BLINK);
            
            PairPacket_t req = {
                .msg_type = MSG_TYPE_PAIR_REQ,
                .identity = settings->identity,
                .uptime_ms = esp_timer_get_time() / 1000
            };
            esp_wifi_get_mac(WIFI_IF_STA, req.mac_addr);
            esp_now_send(bcast_mac, (uint8_t *)&req, sizeof(req));
            hw_interaction_set_led(LED_STATE_YELLOW_BLINK);
        } else if (current_state == STATE_LOCKED) {
            // Watchdog for Data Flow -> LED
            uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
            if (now - last_data_time > 1000) { // 1 second timeout
                hw_interaction_set_led(LED_STATE_GREEN_FAST_BLINK);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void espnow_sm_init(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Force specific channel to match S3 and configuration
    uint8_t ch = tof_settings_get()->wifi_ch;
    ESP_ERROR_CHECK(esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE));
    ESP_LOGI(TAG, "WiFi Started on Channel %d", ch);

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(on_recv));
    ESP_ERROR_CHECK(esp_now_register_send_cb(on_send));

    // Initialize State Machine
    if (load_target_mac()) {
        TofSettings_t *s = tof_settings_get();
        ESP_LOGI(TAG, "Found Target MAC in NVS. Direct to LOCKED.");
        esp_now_peer_info_t peer = {};
        peer.channel = 1;
        peer.encrypt = false;
        memcpy(peer.peer_addr, s->peer_mac, 6);
        if (!esp_now_is_peer_exist(s->peer_mac)) {
            esp_now_add_peer(&peer);
        }
        current_state = STATE_LOCKED;
        hw_interaction_set_led(LED_STATE_GREEN_FAST_BLINK);
    } else {
        ESP_LOGI(TAG, "No Target MAC. Entering DISCOVERY.");
        current_state = STATE_DISCOVERY;
        hw_interaction_set_led(LED_STATE_YELLOW_BLINK);
    }

    xTaskCreate(broadcast_task, "bcast_task", 2048, NULL, 3, NULL);
}

void espnow_sm_send_data(const UartPacket_t *packet) {
    if (current_state == STATE_LOCKED && has_target_mac) {
        // Construct WirelessDataPacket_t
        WirelessDataPacket_t w_packet = {0};
        w_packet.frame_id = packet->header.frame_id;
        
        // Calculate synchronized timestamp (S3 Master Clock Context)
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (last_s3_sync_time > 0) {
            w_packet.sync_anchor = last_s3_sync_time;
            w_packet.sync_offset = now - c3_tick_at_sync;
        } else {
            // Fallback to local uptime if no sync received yet
            w_packet.sync_anchor = 0;
            w_packet.sync_offset = now;
        }

        w_packet.identity = tof_settings_get()->identity; 
        w_packet.sensor_mask = (1 << (packet->header.sensor_id - 1));
        
        // C3 has 1 payload in UartPacket_t, representing Toe, Front, or Rear based on sensor_id
        // We pack the 64 distance zones into the matrix
        uint16_t *packed_src = (uint16_t*)packet->payload;
        int idx = packet->header.sensor_id > 0 && packet->header.sensor_id <= 3 ? packet->header.sensor_id - 1 : 0;
        memcpy(w_packet.distances[idx], packed_src, 128);

        TofSettings_t *s = tof_settings_get();
        esp_err_t err = esp_now_send(s->peer_mac, (uint8_t *)&w_packet, sizeof(w_packet));
        if (err == ESP_OK) {
            last_data_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
            hw_interaction_set_led(LED_STATE_GREEN_BREATHE);
        }
    }
}
