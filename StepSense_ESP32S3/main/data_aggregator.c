#include "data_aggregator.h"
#include "usb_cdc_bridge.h"
#include "tof_settings.h"
#include <string.h>

void data_aggregator_init(void) {
    // Nothing to initialize yet
}

static uint16_t calc_checksum(const UartPacket_t *pkt) {
    uint16_t sum = 0;
    const uint8_t *hptr = (const uint8_t*)&pkt->header;
    for (int i = 0; i < 12; i++) sum += hptr[i];
    for (int i = 0; i < 136; i++) sum += pkt->payload[i];
    return sum;
}

void data_aggregator_push_frame(const WirelessDataPacket_t *in) {
    UartPacket_t out_pkt;
    
    if (!tof_settings_get()->mirror_enabled) return;
    
    for (int i = 0; i < 3; i++) {
        if (in->sensor_mask & (1 << i)) {
            out_pkt.header.magic = 0x55AA;
            out_pkt.header.timestamp = in->sync_anchor + in->sync_offset;
            out_pkt.header.frame_id = in->frame_id;
            // Multiplex mapping strategy:
            // SENSOR_ID_TOE = 1, FRONT = 2, REAR = 3
            // If Left foot (identity == 0), ID is 1, 2, 3
            // If Right foot (identity == 1), ID is 11, 12, 13
            out_pkt.header.sensor_id = (in->identity * 10) + (i + 1); 
            out_pkt.header.payload_len = PACKET_PAYLOAD_LEN;
            
            // Copy 128 bytes data
            memcpy(out_pkt.payload, in->distances[i], 128); 
            memset(out_pkt.payload + 128, 0, PACKET_PAYLOAD_LEN - 128); 
            
            out_pkt.header.checksum = calc_checksum(&out_pkt);
            
            usb_cdc_bridge_send((const uint8_t *)&out_pkt, sizeof(UartPacket_t));
        }
    }
}
