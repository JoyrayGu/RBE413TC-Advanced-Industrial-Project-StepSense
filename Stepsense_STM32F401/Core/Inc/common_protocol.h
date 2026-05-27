/**
 ******************************************************************************
 * @file    common_protocol.h
 * @brief   Protocol definitions for STM32F401RET6 to ESP32-C3 communication
 ******************************************************************************
 */

#ifndef INC_COMMON_PROTOCOL_H_
#define INC_COMMON_PROTOCOL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Sensor IDs */
#define SENSOR_ID_FRONT  1
#define SENSOR_ID_REAR   2
#define SENSOR_ID_TOE    3

#define PACKET_PAYLOAD_LEN 136

/* Payload structures - ensuring proper alignment */
#pragma pack(push, 1)

typedef struct __attribute__((packed)) {
    uint32_t timestamp;   // Timestamp from HAL_GetTick()
    uint32_t frame_id;    // Auto-incremented frame ID shared across sensors
    uint8_t  sensor_id;   // 1:Front, 2:Rear, 3:Toe
    uint8_t  payload_len; // Fixed to 136
    uint16_t checksum;    // Checksum of payload + header (excluding checksum itself)
} PacketHeader_t;

typedef struct __attribute__((packed)) {
    PacketHeader_t header;
    uint8_t payload[PACKET_PAYLOAD_LEN];
} UartPacket_t;

#pragma pack(pop)

/* Checksum utility function */
static inline uint16_t Compute_Checksum(const UartPacket_t* packet) {
    uint16_t sum = 0;
    const uint8_t* ptr = (const uint8_t*)packet;
    
    // Sum header bytes excluding the checksum field itself (offset 10, length 2)
    for (int i = 0; i < 10; i++) {
        sum += ptr[i];
    }
    
    // Sum payload bytes
    for (int i = 0; i < PACKET_PAYLOAD_LEN; i++) {
        sum += packet->payload[i];
    }
    
    return sum;
}

#ifdef __cplusplus
}
#endif

#endif /* INC_COMMON_PROTOCOL_H_ */
