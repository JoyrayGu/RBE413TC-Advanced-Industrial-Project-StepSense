/**
 ******************************************************************************
 * @file    sensing_engine.c
 * @brief   Implementation of the Sensing Engine using VL53L5CX, VL53L8CX
 ******************************************************************************
 */

#include "sensing_engine.h"
#include "main.h"
#include "vl53l5cx_api.h"
#include "vl53l8cx_api.h"
#include "usart.h"
#include "i2c.h"
#include "gpio.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>

/* Sensor Instance Configurations */
static VL53L5CX_Configuration sensor1_cfg; // Front
static VL53L5CX_Configuration sensor2_cfg; // Rear
static VL53L8CX_Configuration sensor3_cfg; // Toe

/* Global State */
static uint32_t current_frame_id = 0;
UartPacket_t packet_front;
UartPacket_t packet_rear;
UartPacket_t packet_toe;

/* Live Expression Debug Variables */
uint16_t live_front_distance[64];
uint8_t live_front_status[64];
uint16_t live_rear_distance[64];
uint8_t live_rear_status[64];
uint16_t live_toe_distance[64];
uint8_t live_toe_status[64];

/* DMA Push State Machine */
typedef enum {
    PUSH_IDLE = 0,
    PUSH_FRONT,
    PUSH_REAR,
    PUSH_TOE,
    PUSH_DONE
} PushState_t;

static PushState_t push_state = PUSH_IDLE;
static uint8_t sensors_initialized = 0; // Bitmask: bit0=Front, bit1=Rear, bit2=Toe

/* --------------------------------------------------------------------------
 * UART1 Debug Helper
 * -------------------------------------------------------------------------- */
void Sensing_Debug_Print(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
}

/* --------------------------------------------------------------------------
 * I2C Recovery Helper
 * -------------------------------------------------------------------------- */
static void I2C_Recover(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Enable GPIOB clock (assuming I2C1 is on PB6/PB7)
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // Configure PB6 (SCL) and PB7 (SDA) as General Purpose Output Open-Drain
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // Set SDA high
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
    HAL_Delay(1);

    // Clock SCL 9 times to free SDA
    for (int i = 0; i < 9; i++) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
        HAL_Delay(1);
    }

    // Generate STOP condition
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
    HAL_Delay(1);

    Sensing_Debug_Print("[StepSense] I2C Bus Recovered via Software.\r\n");
    
    // Re-initialize hardware I2C peripheral
    MX_I2C1_Init();
}

/* --------------------------------------------------------------------------
 * Initialization Sequence
 * -------------------------------------------------------------------------- */
int Sensing_Engine_Init(void) {
    uint8_t status = 0;
    uint8_t isAlive = 0;
    
    Sensing_Debug_Print("[StepSense] Sensing Engine Init Start...\r\n");

    /* 1. Reset Phase */
    HAL_GPIO_WritePin(LPN_1_GPIO_Port, LPN_1_Pin, GPIO_PIN_RESET); 
    HAL_GPIO_WritePin(LPN_2_GPIO_Port, LPN_2_Pin, GPIO_PIN_RESET); 
    HAL_GPIO_WritePin(LPN_3_GPIO_Port, LPN_3_Pin, GPIO_PIN_RESET); 

    HAL_GPIO_WritePin(TOFRST_1_GPIO_Port, TOFRST_1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(TOFRST_2_GPIO_Port, TOFRST_2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SPI_I2C_N_GPIO_Port, SPI_I2C_N_Pin, GPIO_PIN_RESET);
    HAL_Delay(10); 

    Sensing_Debug_Print("[StepSense] All sensors mapped to Reset/Sleep\r\n");

    /* 2. Init Sensor 1 (Front, L5CX) -> 0x54 */
    Sensing_Debug_Print("[StepSense] Waking Front (Sensor 1)\r\n");
    HAL_GPIO_WritePin(LPN_1_GPIO_Port, LPN_1_Pin, GPIO_PIN_SET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(TOFRST_1_GPIO_Port, TOFRST_1_Pin, GPIO_PIN_SET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(TOFRST_1_GPIO_Port, TOFRST_1_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);

    sensor1_cfg.platform.address = VL53L5CX_DEFAULT_I2C_ADDRESS;
    vl53l5cx_is_alive(&sensor1_cfg, &isAlive);
    if (isAlive) {
        status = vl53l5cx_set_i2c_address(&sensor1_cfg, SENSOR1_I2C_ADDR);
        if (status == 0) {
            sensor1_cfg.platform.address = SENSOR1_I2C_ADDR;
            Sensing_Debug_Print("[StepSense] Sensor 1 Remap to 0x54 OK.\r\n");
        }
    } else {
        // Probe fallback
        sensor1_cfg.platform.address = SENSOR1_I2C_ADDR;
        vl53l5cx_is_alive(&sensor1_cfg, &isAlive);
    }

    if (isAlive) {
        status = vl53l5cx_init(&sensor1_cfg);
        if (status == 0) {
            // Configure 8x8 mode, 15Hz
            vl53l5cx_set_resolution(&sensor1_cfg, VL53L5CX_RESOLUTION_8X8);
            vl53l5cx_set_ranging_frequency_hz(&sensor1_cfg, 15);
            vl53l5cx_start_ranging(&sensor1_cfg);
            sensors_initialized |= (1 << 0);
            Sensing_Debug_Print("[StepSense] Sensor 1 Ready & Ranging.\r\n");
        } else {
            Sensing_Debug_Print("[StepSense] Sensor 1 L5CX Init Failed (err %d).\r\n", status);
        }
    } else {
        Sensing_Debug_Print("[StepSense] Sensor 1 not found!\r\n");
        I2C_Recover();
    }

    /* 3. Init Sensor 2 (Rear, L5CX) -> 0x56 */
    Sensing_Debug_Print("[StepSense] Waking Rear (Sensor 2)\r\n");
    HAL_GPIO_WritePin(LPN_2_GPIO_Port, LPN_2_Pin, GPIO_PIN_SET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(TOFRST_2_GPIO_Port, TOFRST_2_Pin, GPIO_PIN_SET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(TOFRST_2_GPIO_Port, TOFRST_2_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);

    sensor2_cfg.platform.address = VL53L5CX_DEFAULT_I2C_ADDRESS;
    isAlive = 0;
    vl53l5cx_is_alive(&sensor2_cfg, &isAlive);
    if (isAlive) {
        status = vl53l5cx_set_i2c_address(&sensor2_cfg, SENSOR2_I2C_ADDR);
        if (status == 0) {
            sensor2_cfg.platform.address = SENSOR2_I2C_ADDR;
            Sensing_Debug_Print("[StepSense] Sensor 2 Remap to 0x56 OK.\r\n");
        }
    } else {
        sensor2_cfg.platform.address = SENSOR2_I2C_ADDR;
        vl53l5cx_is_alive(&sensor2_cfg, &isAlive);
    }

    if (isAlive) {
        status = vl53l5cx_init(&sensor2_cfg);
        if (status == 0) {
            vl53l5cx_set_resolution(&sensor2_cfg, VL53L5CX_RESOLUTION_8X8);
            vl53l5cx_set_ranging_frequency_hz(&sensor2_cfg, 15);
            vl53l5cx_start_ranging(&sensor2_cfg);
            sensors_initialized |= (1 << 1);
            Sensing_Debug_Print("[StepSense] Sensor 2 Ready & Ranging.\r\n");
        } else {
            Sensing_Debug_Print("[StepSense] Sensor 2 L5CX Init Failed (err %d).\r\n", status);
        }
    } else {
        Sensing_Debug_Print("[StepSense] Sensor 2 not found!\r\n");
        I2C_Recover();
    }

    /* 4. Init Sensor 3 (Toe, L8CX) -> 0x58 */
    Sensing_Debug_Print("[StepSense] Waking Toe (Sensor 3)\r\n");
    HAL_GPIO_WritePin(LPN_3_GPIO_Port, LPN_3_Pin, GPIO_PIN_SET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(SPI_I2C_N_GPIO_Port, SPI_I2C_N_Pin, GPIO_PIN_SET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(SPI_I2C_N_GPIO_Port, SPI_I2C_N_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);

    sensor3_cfg.platform.address = VL53L8CX_DEFAULT_I2C_ADDRESS;
    isAlive = 0;
    vl53l8cx_is_alive(&sensor3_cfg, &isAlive);
    if (isAlive) {
        status = vl53l8cx_set_i2c_address(&sensor3_cfg, SENSOR3_I2C_ADDR);
        if (status == 0) {
            sensor3_cfg.platform.address = SENSOR3_I2C_ADDR;
            Sensing_Debug_Print("[StepSense] Sensor 3 Remap to 0x58 OK.\r\n");
        }
    } else {
        sensor3_cfg.platform.address = SENSOR3_I2C_ADDR;
        vl53l8cx_is_alive(&sensor3_cfg, &isAlive);
    }

    if (isAlive) {
        status = vl53l8cx_init(&sensor3_cfg);
        if (status == 0) {
            vl53l8cx_set_resolution(&sensor3_cfg, VL53L8CX_RESOLUTION_8X8);
            vl53l8cx_set_ranging_frequency_hz(&sensor3_cfg, 15);
            vl53l8cx_start_ranging(&sensor3_cfg);
            sensors_initialized |= (1 << 2);
            Sensing_Debug_Print("[StepSense] Sensor 3 Ready & Ranging.\r\n");
        } else {
            Sensing_Debug_Print("[StepSense] Sensor 3 L8CX Init Failed (err %d).\r\n", status);
        }
    } else {
        Sensing_Debug_Print("[StepSense] Sensor 3 not found!\r\n");
    }

    return (sensors_initialized == 0x07) ? 0 : -1;
}

/* --------------------------------------------------------------------------
 * Data Parsing Helper (Compress zones into 136-byte payload)
 * -------------------------------------------------------------------------- */
/**
 * Distance is mostly 12-bits (max 4000mm roughly)
 * Status is 4-bits.
 * We pack: (Distance & 0x0FFF) | ((Status & 0x0F) << 12)
 * into a single uint16_t array.
 */
static void Pack_Payload_L5CX(UartPacket_t* pkt, VL53L5CX_ResultsData* res, uint16_t* live_dist, uint8_t* live_status) {
    uint16_t* u16_payload = (uint16_t*)pkt->payload;
    for (int i = 0; i < SENSOR_ZONES_8X8; i++) {
        uint16_t dist = res->distance_mm[VL53L5CX_NB_TARGET_PER_ZONE * i];
        uint8_t stat = res->target_status[VL53L5CX_NB_TARGET_PER_ZONE * i];
        if (live_dist) live_dist[i] = dist;
        if (live_status) live_status[i] = stat;
        u16_payload[i] = (dist & 0x0FFF) | ((stat & 0x0F) << 12);
    }
    // Padding bytes up to 136 handled by memset beforehand
}

static void Pack_Payload_L8CX(UartPacket_t* pkt, VL53L8CX_ResultsData* res, uint16_t* live_dist, uint8_t* live_status) {
    uint16_t* u16_payload = (uint16_t*)pkt->payload;
    for (int i = 0; i < SENSOR_ZONES_8X8; i++) {
        uint16_t dist = res->distance_mm[VL53L8CX_NB_TARGET_PER_ZONE * i];
        uint8_t stat = res->target_status[VL53L8CX_NB_TARGET_PER_ZONE * i];
        if (live_dist) live_dist[i] = dist;
        if (live_status) live_status[i] = stat;
        u16_payload[i] = (dist & 0x0FFF) | ((stat & 0x0F) << 12);
    }
}

static void Setup_Header(UartPacket_t* pkt, uint8_t sensor_id, uint32_t ts) {
    pkt->header.timestamp = ts;
    pkt->header.frame_id = current_frame_id;
    pkt->header.sensor_id = sensor_id;
    pkt->header.payload_len = PACKET_PAYLOAD_LEN;
    pkt->header.checksum = 0; 
    pkt->header.checksum = Compute_Checksum(pkt); // compute final
}

/* --------------------------------------------------------------------------
 * Update Engine (Poll Data)
 * -------------------------------------------------------------------------- */
void Sensing_Engine_Update(void) {
    uint8_t ready = 0;
    VL53L5CX_ResultsData res_l5;
    VL53L8CX_ResultsData res_l8;
    
    // Check if new frame is warranted logically (we might trigger off one sensor, e.g Front)
    // To simplify: we check if all connected sensors have data. If yes, grab it and increment frame.
    uint8_t data_available = 0;

    if (sensors_initialized & (1 << 0)) {
        vl53l5cx_check_data_ready(&sensor1_cfg, &ready);
        if (ready) data_available |= (1 << 0);
    }
    if (sensors_initialized & (1 << 1)) {
        ready = 0;
        vl53l5cx_check_data_ready(&sensor2_cfg, &ready);
        if (ready) data_available |= (1 << 1);
    }
    if (sensors_initialized & (1 << 2)) {
        ready = 0;
        vl53l8cx_check_data_ready(&sensor3_cfg, &ready);
        if (ready) data_available |= (1 << 2);
    }

    // Process only if the available sensors match initialized ones (sync) or partially
    // We update them individually, but tag with same frame_id logically.
    // For extreme sync, trigger off one. Let's process whenever available:

    uint32_t ts = HAL_GetTick();
    uint8_t new_data = 0;

    if (data_available & (1 << 0)) {
        vl53l5cx_get_ranging_data(&sensor1_cfg, &res_l5);
        memset(packet_front.payload, 0, PACKET_PAYLOAD_LEN);
        Pack_Payload_L5CX(&packet_front, &res_l5, live_front_distance, live_front_status);
        Setup_Header(&packet_front, SENSOR_ID_FRONT, ts);
        new_data = 1;
    }

    if (data_available & (1 << 1)) {
        vl53l5cx_get_ranging_data(&sensor2_cfg, &res_l5);
        memset(packet_rear.payload, 0, PACKET_PAYLOAD_LEN);
        Pack_Payload_L5CX(&packet_rear, &res_l5, live_rear_distance, live_rear_status);
        Setup_Header(&packet_rear, SENSOR_ID_REAR, ts);
        new_data = 1;
    }

    if (data_available & (1 << 2)) {
        vl53l8cx_get_ranging_data(&sensor3_cfg, &res_l8);
        memset(packet_toe.payload, 0, PACKET_PAYLOAD_LEN);
        Pack_Payload_L8CX(&packet_toe, &res_l8, live_toe_distance, live_toe_status);
        Setup_Header(&packet_toe, SENSOR_ID_TOE, ts);
        new_data = 1;
    }

    if (new_data) {
        current_frame_id++;
        // If push is idle, trigger transmission
        if (push_state == PUSH_IDLE) {
            push_state = PUSH_FRONT; 
        }
    }
}

/* --------------------------------------------------------------------------
 * UART Push Engine
 * -------------------------------------------------------------------------- */
void Sensing_Engine_Push(void) {
    if (push_state == PUSH_IDLE || push_state == PUSH_DONE) {
        push_state = PUSH_IDLE;
        return;
    }

    // Only push if UART is ready
    if (huart2.gState != HAL_UART_STATE_READY) {
        return; // wait for DMA
    }

    switch (push_state) {
        case PUSH_FRONT:
            if (sensors_initialized & (1 << 0)) {
                HAL_UART_Transmit_DMA(&huart2, (uint8_t*)&packet_front, sizeof(UartPacket_t));
            }
            push_state = PUSH_REAR;
            break;
            
        case PUSH_REAR:
            if (sensors_initialized & (1 << 1)) {
                HAL_UART_Transmit_DMA(&huart2, (uint8_t*)&packet_rear, sizeof(UartPacket_t));
            }
            push_state = PUSH_TOE;
            break;
            
        case PUSH_TOE:
            if (sensors_initialized & (1 << 2)) {
                HAL_UART_Transmit_DMA(&huart2, (uint8_t*)&packet_toe, sizeof(UartPacket_t));
            }
            push_state = PUSH_DONE;
            break;
            
        default:
            push_state = PUSH_IDLE;
            break;
    }
}
