#ifndef TOF_PROTOCOL_H
#define TOF_PROTOCOL_H

#include <stdint.h>

#define SENSOR_ID_SYSTEM 0
#define SENSOR_ID_TOE    1
#define SENSOR_ID_FRONT  2
#define SENSOR_ID_REAR   3

#define IDENTITY_LEFT       0
#define IDENTITY_RIGHT      1
#define IDENTITY_UNASSIGNED 0xFF

typedef enum {
    STATE_INIT = 0,
    STATE_DISCOVERY,
    STATE_BINDING,
    STATE_LOCKED,
    STATE_TIMEOUT
} SystemState_t;

#define MSG_TYPE_PAIR_REQ 0x10
#define MSG_TYPE_PAIR_ACK 0x11
#define MSG_TYPE_DATA     0x12
#define MSG_TYPE_SYNC     0x13

#define PACKET_PAYLOAD_LEN 136 
#define PACKET_HEADER_LEN 14
#define PACKET_TOTAL_LEN 150

typedef struct __attribute__((packed)) {
  uint16_t magic;      
  uint32_t timestamp;  
  uint32_t frame_id;   
  uint8_t sensor_id;   
  uint8_t payload_len; 
  uint16_t checksum;   
} PacketHeader_t;

typedef struct __attribute__((packed)) {
  PacketHeader_t header;
  uint8_t payload[PACKET_PAYLOAD_LEN];
} UartPacket_t;

typedef struct {
  uint16_t distance_mm;
  uint8_t status;
} TofZone_t;

static inline TofZone_t tof_parse_zone(const uint8_t *payload, int zone_idx) {
  uint16_t raw = payload[zone_idx * 2] | (payload[zone_idx * 2 + 1] << 8);
  TofZone_t zone;
  zone.distance_mm = raw & 0x0FFF;
  zone.status = (raw >> 12) & 0x0F;
  return zone;
}

typedef struct __attribute__((packed)) {
    uint16_t msg_type;    
    uint8_t identity;     
    uint8_t mac_addr[6];  
    uint32_t uptime_ms;   
} PairPacket_t;

typedef struct __attribute__((packed)) {
    uint16_t msg_type;    // MSG_TYPE_SYNC
    uint32_t s3_sync_time; 
} SyncPacket_t;

typedef struct __attribute__((packed)) {
    uint32_t frame_id;
    uint32_t sync_anchor; // Last S3 global time received
    uint32_t sync_offset; // Local offset from anchor to sample
    uint8_t identity;     
    uint8_t sensor_mask;  
    uint16_t distances[3][64]; 
} WirelessDataPacket_t;

#endif // TOF_PROTOCOL_H
