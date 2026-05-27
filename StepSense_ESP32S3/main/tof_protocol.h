#ifndef TOF_PROTOCOL_H
#define TOF_PROTOCOL_H

#include <stdint.h>

/**
 * @brief Sensor ID Definitions
 */
#define SENSOR_ID_SYSTEM 0
#define SENSOR_ID_TOE    1
#define SENSOR_ID_FRONT  2
#define SENSOR_ID_REAR   3

#define IDENTITY_LEFT       0
#define IDENTITY_RIGHT      1
#define IDENTITY_UNASSIGNED 0xFF

/**
 * @brief System States for Pairing
 */
typedef enum {
    STATE_INIT = 0,
    STATE_DISCOVERY,
    STATE_BINDING,
    STATE_LOCKED,
    STATE_TIMEOUT
} SystemState_t;

/**
 * @brief Wireless Communication Types
 */
#define MSG_TYPE_PAIR_REQ 0x10
#define MSG_TYPE_PAIR_ACK 0x11
#define MSG_TYPE_DATA     0x12
#define MSG_TYPE_SYNC     0x13

/**
 * @brief Protocol Constants
 */
#define PACKET_PAYLOAD_LEN 136 
#define PACKET_HEADER_LEN 14
#define PACKET_TOTAL_LEN 150 // Header (14B) + Payload (136B)

/**
 * @brief Packet Header Structure
 * Packed to match STM32 memory layout.
 */
typedef struct __attribute__((packed)) {
  uint16_t magic;      // 0x55AA Sync Word
  uint32_t timestamp;  // STM32 HAL_GetTick()
  uint32_t frame_id;   // Frame ID for synchronization
  uint8_t sensor_id;   // 1, 2, or 3
  uint8_t payload_len; // Fixed at 136
  uint16_t checksum;   // Sum of Header (0-11) + Payload (136B)
} PacketHeader_t;

/**
 * @brief Full UART Packet
 */
typedef struct __attribute__((packed)) {
  PacketHeader_t header;
  uint8_t payload[PACKET_PAYLOAD_LEN];
} UartPacket_t;

/**
 * @brief Zone Data Extraction
 */
typedef struct {
  uint16_t distance_mm;
  uint8_t status;
} TofZone_t;

/**
 * @brief Helper to parse a zone from 2 bytes
 * Format: 12 bits distance (0-4095) | 4 bits status (shifted 12)
 */
static inline TofZone_t tof_parse_zone(const uint8_t *payload, int zone_idx) {
  uint16_t raw = payload[zone_idx * 2] | (payload[zone_idx * 2 + 1] << 8);
  TofZone_t zone;
  zone.distance_mm = raw & 0x0FFF;
  zone.status = (raw >> 12) & 0x0F;
  return zone;
}

/**
 * @brief Pairing Packet Structure (ESP-NOW)
 */
typedef struct __attribute__((packed)) {
    uint16_t msg_type;    // MSG_TYPE_PAIR_REQ or MSG_TYPE_PAIR_ACK
    uint8_t identity;     // 0=Left, 1=Right
    uint8_t mac_addr[6];  // Sender's MAC
    uint32_t uptime_ms;   // Status check
} PairPacket_t;

typedef struct __attribute__((packed)) {
    uint16_t msg_type;    // MSG_TYPE_SYNC
    uint32_t s3_sync_time; 
} SyncPacket_t;

/**
 * @brief Wireless Frame Container (Synched ToF data)
 * Sent from C3 to S3.
 */
typedef struct __attribute__((packed)) {
    uint32_t frame_id;
    uint32_t sync_anchor; // Last S3 global time received
    uint32_t sync_offset; // Local offset from anchor to sample
    uint8_t identity;     
    uint8_t sensor_mask;  
    uint16_t distances[3][64]; 
} WirelessDataPacket_t;

#endif // TOF_PROTOCOL_H
