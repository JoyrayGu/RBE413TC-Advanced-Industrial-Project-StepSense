################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX/src/vl53l5cx_api.c \
E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX/src/vl53l5cx_plugin_detection_thresholds.c \
E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX/src/vl53l5cx_plugin_motion_indicator.c \
E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX/src/vl53l5cx_plugin_xtalk.c 

OBJS += \
./Drivers/BSP/VL53L5CX/src/vl53l5cx_api.o \
./Drivers/BSP/VL53L5CX/src/vl53l5cx_plugin_detection_thresholds.o \
./Drivers/BSP/VL53L5CX/src/vl53l5cx_plugin_motion_indicator.o \
./Drivers/BSP/VL53L5CX/src/vl53l5cx_plugin_xtalk.o 

C_DEPS += \
./Drivers/BSP/VL53L5CX/src/vl53l5cx_api.d \
./Drivers/BSP/VL53L5CX/src/vl53l5cx_plugin_detection_thresholds.d \
./Drivers/BSP/VL53L5CX/src/vl53l5cx_plugin_motion_indicator.d \
./Drivers/BSP/VL53L5CX/src/vl53l5cx_plugin_xtalk.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/BSP/VL53L5CX/src/vl53l5cx_api.o: E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX/src/vl53l5cx_api.c Drivers/BSP/VL53L5CX/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xE -c -I../../Core/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../Drivers/CMSIS/Include -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX/inc" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L8CX/inc" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX_Platform" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L8CX_Platform" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Core/Src" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Drivers/BSP/VL53L5CX/src/vl53l5cx_plugin_detection_thresholds.o: E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX/src/vl53l5cx_plugin_detection_thresholds.c Drivers/BSP/VL53L5CX/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xE -c -I../../Core/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../Drivers/CMSIS/Include -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX/inc" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L8CX/inc" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX_Platform" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L8CX_Platform" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Core/Src" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Drivers/BSP/VL53L5CX/src/vl53l5cx_plugin_motion_indicator.o: E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX/src/vl53l5cx_plugin_motion_indicator.c Drivers/BSP/VL53L5CX/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xE -c -I../../Core/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../Drivers/CMSIS/Include -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX/inc" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L8CX/inc" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX_Platform" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L8CX_Platform" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Core/Src" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Drivers/BSP/VL53L5CX/src/vl53l5cx_plugin_xtalk.o: E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX/src/vl53l5cx_plugin_xtalk.c Drivers/BSP/VL53L5CX/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xE -c -I../../Core/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../Drivers/CMSIS/Include -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX/inc" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L8CX/inc" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX_Platform" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L8CX_Platform" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Core/Src" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-BSP-2f-VL53L5CX-2f-src

clean-Drivers-2f-BSP-2f-VL53L5CX-2f-src:
	-$(RM) ./Drivers/BSP/VL53L5CX/src/vl53l5cx_api.cyclo ./Drivers/BSP/VL53L5CX/src/vl53l5cx_api.d ./Drivers/BSP/VL53L5CX/src/vl53l5cx_api.o ./Drivers/BSP/VL53L5CX/src/vl53l5cx_api.su ./Drivers/BSP/VL53L5CX/src/vl53l5cx_plugin_detection_thresholds.cyclo ./Drivers/BSP/VL53L5CX/src/vl53l5cx_plugin_detection_thresholds.d ./Drivers/BSP/VL53L5CX/src/vl53l5cx_plugin_detection_thresholds.o ./Drivers/BSP/VL53L5CX/src/vl53l5cx_plugin_detection_thresholds.su ./Drivers/BSP/VL53L5CX/src/vl53l5cx_plugin_motion_indicator.cyclo ./Drivers/BSP/VL53L5CX/src/vl53l5cx_plugin_motion_indicator.d ./Drivers/BSP/VL53L5CX/src/vl53l5cx_plugin_motion_indicator.o ./Drivers/BSP/VL53L5CX/src/vl53l5cx_plugin_motion_indicator.su ./Drivers/BSP/VL53L5CX/src/vl53l5cx_plugin_xtalk.cyclo ./Drivers/BSP/VL53L5CX/src/vl53l5cx_plugin_xtalk.d ./Drivers/BSP/VL53L5CX/src/vl53l5cx_plugin_xtalk.o ./Drivers/BSP/VL53L5CX/src/vl53l5cx_plugin_xtalk.su

.PHONY: clean-Drivers-2f-BSP-2f-VL53L5CX-2f-src

