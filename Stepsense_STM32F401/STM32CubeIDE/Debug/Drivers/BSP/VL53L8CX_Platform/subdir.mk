################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L8CX_Platform/vl53l8cx_platform.c 

OBJS += \
./Drivers/BSP/VL53L8CX_Platform/vl53l8cx_platform.o 

C_DEPS += \
./Drivers/BSP/VL53L8CX_Platform/vl53l8cx_platform.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/BSP/VL53L8CX_Platform/vl53l8cx_platform.o: E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L8CX_Platform/vl53l8cx_platform.c Drivers/BSP/VL53L8CX_Platform/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xE -c -I../../Core/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../Drivers/CMSIS/Include -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX/inc" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L8CX/inc" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX_Platform" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L8CX_Platform" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Core/Src" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-BSP-2f-VL53L8CX_Platform

clean-Drivers-2f-BSP-2f-VL53L8CX_Platform:
	-$(RM) ./Drivers/BSP/VL53L8CX_Platform/vl53l8cx_platform.cyclo ./Drivers/BSP/VL53L8CX_Platform/vl53l8cx_platform.d ./Drivers/BSP/VL53L8CX_Platform/vl53l8cx_platform.o ./Drivers/BSP/VL53L8CX_Platform/vl53l8cx_platform.su

.PHONY: clean-Drivers-2f-BSP-2f-VL53L8CX_Platform

