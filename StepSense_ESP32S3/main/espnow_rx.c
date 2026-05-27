#include "espnow_rx.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "tof_settings.h"
#include "hw_interaction.h"
#include "data_aggregator.h"
#include "status_manager.h"
#include <string.h>
#include "esp_timer.h"

static const char *TAG = "ESPNOW_RX";

static void add_or_update_peer(const uint8_t *mac) {
    esp_now_peer_info_t peer = {};
    peer.channel = 1;
    peer.encrypt = false;
    memcpy(peer.peer_addr, mac, 6);
    if (!esp_now_is_peer_exist(mac)) {
        esp_now_add_peer(&peer);
    }
}

static void on_recv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    const uint8_t *mac = recv_info->src_addr;

    if (len == sizeof(PairPacket_t)) {
        PairPacket_t *req = (PairPacket_t*)data;
        if (req->msg_type == MSG_TYPE_PAIR_REQ) {
            uint8_t sender_mac[6];
            memcpy(sender_mac, mac, 6);
            ESP_LOGI(TAG, "Pairing request from %02X:%02X:%02X:%02X:%02X:%02X, ID: %d",
                     sender_mac[0], sender_mac[1], sender_mac[2],
                     sender_mac[3], sender_mac[4], sender_mac[5],
                     req->identity);

            // Safety Check: Reject unassigned identity
            if (req->identity != IDENTITY_LEFT && req->identity != IDENTITY_RIGHT) {
                ESP_LOGW(TAG, "Ignoring PAIR_REQ with unassigned identity!");
                return;
            }

            TofSettings_t *s = tof_settings_get();
            if (req->identity == IDENTITY_LEFT) memcpy(s->peer_mac_l, sender_mac, 6);
            else memcpy(s->peer_mac_r, sender_mac, 6);
            
            // Request async save instead of blocking commit
            tof_settings_request_save();

            // Add as ESP-NOW peer
            add_or_update_peer(sender_mac);
            
            // Ack back
            PairPacket_t ack = { 
                .msg_type = MSG_TYPE_PAIR_ACK,
                .identity = 0, // Bridge identity
                .uptime_ms = esp_timer_get_time() / 1000
            };
            esp_read_mac(ack.mac_addr, ESP_MAC_WIFI_STA);
            
            esp_now_send(sender_mac, (uint8_t*)&ack, sizeof(ack));
            
            status_manager_update_foot(req->identity);
        }
    } else if (len == sizeof(WirelessDataPacket_t)) {
        WirelessDataPacket_t *pkt = (WirelessDataPacket_t*)data;
        
        // Verify if it's one of our paired feet
        TofSettings_t *s = tof_settings_get();
        bool authorized = (memcmp(s->peer_mac_l, mac, 6) == 0) || (memcmp(s->peer_mac_r, mac, 6) == 0);
        if (!authorized) return;

        status_manager_update_foot(pkt->identity);
        data_aggregator_push_frame(pkt);
    }
}

void espnow_rx_init(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Force specific channel to match C3 and configuration
    uint8_t ch = tof_settings_get()->wifi_ch;
    ESP_ERROR_CHECK(esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE));
    ESP_LOGI(TAG, "WiFi Started on Channel %d", ch);

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(on_recv));
    
    // Load existing peers from settings
    TofSettings_t *s = tof_settings_get();
    uint8_t bcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    if (memcmp(s->peer_mac_l, bcast, 6) != 0) {
        add_or_update_peer(s->peer_mac_l);
        ESP_LOGI(TAG, "Loaded Left Peer: %02X:%02X:%02X:%02X:%02X:%02X", 
                 s->peer_mac_l[0], s->peer_mac_l[1], s->peer_mac_l[2], s->peer_mac_l[3], s->peer_mac_l[4], s->peer_mac_l[5]);
    }
    if (memcmp(s->peer_mac_r, bcast, 6) != 0) {
        add_or_update_peer(s->peer_mac_r);
        ESP_LOGI(TAG, "Loaded Right Peer: %02X:%02X:%02X:%02X:%02X:%02X", 
                 s->peer_mac_r[0], s->peer_mac_r[1], s->peer_mac_r[2], s->peer_mac_r[3], s->peer_mac_r[4], s->peer_mac_r[5]);
    }

    ESP_LOGI(TAG, "ESP-NOW RX started. Passive listening mode.");
    
    // Always add broadcast as a peer for outgoing sync packets
    add_or_update_peer(bcast);
}

int espnow_rx_get_peers(uint8_t macs[2][6]) {
    TofSettings_t *s = tof_settings_get();
    int count = 0;
    uint8_t bcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    if (memcmp(s->peer_mac_l, bcast, 6) != 0) memcpy(macs[count++], s->peer_mac_l, 6);
    if (memcmp(s->peer_mac_r, bcast, 6) != 0) memcpy(macs[count++], s->peer_mac_r, 6);
    return count;
}
