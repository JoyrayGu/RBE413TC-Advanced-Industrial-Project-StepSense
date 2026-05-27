/**
 ******************************************************************************
 * @file    sensing_engine.h
 * @brief   Header for the Sensing Engine Module using VL53L5CX and VL53L8CX
 ******************************************************************************
 */

#ifndef INC_SENSING_ENGINE_H_
#define INC_SENSING_ENGINE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "common_protocol.h"

/* Sensor Configurations */
#define SENSOR1_I2C_ADDR 0x54  // Front (VL53L5CX)
#define SENSOR2_I2C_ADDR 0x56  // Rear (VL53L5CX)
#define SENSOR3_I2C_ADDR 0x58  // Toe (VL53L8CX)

/* Resolution Constants */
#define SENSOR_ZONES_8X8 64
#define SENSOR_ZONES_4X4 16
#define MAX_ZONES        64  // We support up to 64 zones

/* --------------------------------------------------------------------------
 * Function Prototypes
 * -------------------------------------------------------------------------- */

/**
 * @brief Initialize all 3 ToF sensors (remap addresses & download firmware).
 * @retval 0 on success, non-zero on error.
 */
int Sensing_Engine_Init(void);

/**
 * @brief Non-blocking check for new data and updates the global packets if ready.
 */
void Sensing_Engine_Update(void);

/**
 * @brief Push available data via UART2 DMA.
 */
void Sensing_Engine_Push(void);

/**
 * @brief Debug print function mapped to UART1 output
 * @param format printf-style format string
 */
void Sensing_Debug_Print(const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif /* INC_SENSING_ENGINE_H_ */
