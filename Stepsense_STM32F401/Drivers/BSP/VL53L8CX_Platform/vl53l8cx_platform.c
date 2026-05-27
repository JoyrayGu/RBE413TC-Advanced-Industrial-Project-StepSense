/**
 *
 * Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#include "vl53l8cx_platform.h"
#include "i2c.h"

extern I2C_HandleTypeDef hi2c1;

uint8_t VL53L8CX_RdByte(VL53L8CX_Platform *p_platform, uint16_t RegisterAdress,
                        uint8_t *p_value) {
  uint8_t status;
  status = HAL_I2C_Mem_Read(&hi2c1, p_platform->address, RegisterAdress,
                            I2C_MEMADD_SIZE_16BIT, p_value, 1, HAL_MAX_DELAY);
  return (status == HAL_OK) ? 0 : status;
}

uint8_t VL53L8CX_WrByte(VL53L8CX_Platform *p_platform, uint16_t RegisterAdress,
                        uint8_t value) {
  uint8_t status;
  status = HAL_I2C_Mem_Write(&hi2c1, p_platform->address, RegisterAdress,
                             I2C_MEMADD_SIZE_16BIT, &value, 1, HAL_MAX_DELAY);
  return (status == HAL_OK) ? 0 : status;
}

uint8_t VL53L8CX_WrMulti(VL53L8CX_Platform *p_platform, uint16_t RegisterAdress,
                         uint8_t *p_values, uint32_t size) {
  uint8_t status;
  status = HAL_I2C_Mem_Write(&hi2c1, p_platform->address, RegisterAdress,
                             I2C_MEMADD_SIZE_16BIT, p_values, (uint16_t)size,
                             HAL_MAX_DELAY);
  return (status == HAL_OK) ? 0 : status;
}

uint8_t VL53L8CX_RdMulti(VL53L8CX_Platform *p_platform, uint16_t RegisterAdress,
                         uint8_t *p_values, uint32_t size) {
  uint8_t status;
  status = HAL_I2C_Mem_Read(&hi2c1, p_platform->address, RegisterAdress,
                            I2C_MEMADD_SIZE_16BIT, p_values, (uint16_t)size,
                            HAL_MAX_DELAY);
  return (status == HAL_OK) ? 0 : status;
}

uint8_t VL53L8CX_Reset_Sensor(VL53L8CX_Platform *p_platform) {
  return 0; // Handled in main.c during StepSense_Init()
}

void VL53L8CX_SwapBuffer(uint8_t *buffer, uint16_t size) {
  uint32_t i, tmp;

  for (i = 0; i < size; i = i + 4) {
    tmp = (buffer[i] << 24) | (buffer[i + 1] << 16) | (buffer[i + 2] << 8) |
          (buffer[i + 3]);

    memcpy(&(buffer[i]), &tmp, 4);
  }
}

uint8_t VL53L8CX_WaitMs(VL53L8CX_Platform *p_platform, uint32_t TimeMs) {
  HAL_Delay(TimeMs);
  return 0;
}
