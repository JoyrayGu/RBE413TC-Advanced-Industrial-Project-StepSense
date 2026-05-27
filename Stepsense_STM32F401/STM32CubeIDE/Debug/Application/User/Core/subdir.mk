################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
E:/STM32CubeIDE_Workplace/Stepsense_260307/Core/Src/gpio.c \
E:/STM32CubeIDE_Workplace/Stepsense_260307/Core/Src/i2c.c \
E:/STM32CubeIDE_Workplace/Stepsense_260307/Core/Src/main.c \
E:/STM32CubeIDE_Workplace/Stepsense_260307/Core/Src/sensing_engine.c \
E:/STM32CubeIDE_Workplace/Stepsense_260307/Core/Src/stm32f4xx_hal_msp.c \
E:/STM32CubeIDE_Workplace/Stepsense_260307/Core/Src/stm32f4xx_it.c \
../Application/User/Core/syscalls.c \
../Application/User/Core/sysmem.c \
E:/STM32CubeIDE_Workplace/Stepsense_260307/Core/Src/usart.c 

OBJS += \
./Application/User/Core/gpio.o \
./Application/User/Core/i2c.o \
./Application/User/Core/main.o \
./Application/User/Core/sensing_engine.o \
./Application/User/Core/stm32f4xx_hal_msp.o \
./Application/User/Core/stm32f4xx_it.o \
./Application/User/Core/syscalls.o \
./Application/User/Core/sysmem.o \
./Application/User/Core/usart.o 

C_DEPS += \
./Application/User/Core/gpio.d \
./Application/User/Core/i2c.d \
./Application/User/Core/main.d \
./Application/User/Core/sensing_engine.d \
./Application/User/Core/stm32f4xx_hal_msp.d \
./Application/User/Core/stm32f4xx_it.d \
./Application/User/Core/syscalls.d \
./Application/User/Core/sysmem.d \
./Application/User/Core/usart.d 


# Each subdirectory must supply rules for building sources it contributes
Application/User/Core/gpio.o: E:/STM32CubeIDE_Workplace/Stepsense_260307/Core/Src/gpio.c Application/User/Core/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xE -c -I../../Core/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../Drivers/CMSIS/Include -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX/inc" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L8CX/inc" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX_Platform" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L8CX_Platform" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Core/Src" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Application/User/Core/i2c.o: E:/STM32CubeIDE_Workplace/Stepsense_260307/Core/Src/i2c.c Application/User/Core/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xE -c -I../../Core/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../Drivers/CMSIS/Include -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX/inc" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L8CX/inc" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX_Platform" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L8CX_Platform" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Core/Src" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Application/User/Core/main.o: E:/STM32CubeIDE_Workplace/Stepsense_260307/Core/Src/main.c Application/User/Core/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xE -c -I../../Core/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../Drivers/CMSIS/Include -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX/inc" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L8CX/inc" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX_Platform" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L8CX_Platform" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Core/Src" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Application/User/Core/sensing_engine.o: E:/STM32CubeIDE_Workplace/Stepsense_260307/Core/Src/sensing_engine.c Application/User/Core/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xE -c -I../../Core/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../Drivers/CMSIS/Include -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX/inc" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L8CX/inc" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX_Platform" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L8CX_Platform" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Core/Src" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Application/User/Core/stm32f4xx_hal_msp.o: E:/STM32CubeIDE_Workplace/Stepsense_260307/Core/Src/stm32f4xx_hal_msp.c Application/User/Core/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xE -c -I../../Core/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../Drivers/CMSIS/Include -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX/inc" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L8CX/inc" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX_Platform" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L8CX_Platform" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Core/Src" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Application/User/Core/stm32f4xx_it.o: E:/STM32CubeIDE_Workplace/Stepsense_260307/Core/Src/stm32f4xx_it.c Application/User/Core/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xE -c -I../../Core/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../Drivers/CMSIS/Include -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX/inc" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L8CX/inc" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX_Platform" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L8CX_Platform" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Core/Src" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Application/User/Core/%.o Application/User/Core/%.su Application/User/Core/%.cyclo: ../Application/User/Core/%.c Application/User/Core/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xE -c -I../../Core/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../Drivers/CMSIS/Include -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX/inc" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L8CX/inc" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX_Platform" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L8CX_Platform" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Core/Src" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Application/User/Core/usart.o: E:/STM32CubeIDE_Workplace/Stepsense_260307/Core/Src/usart.c Application/User/Core/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xE -c -I../../Core/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../Drivers/CMSIS/Include -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX/inc" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L8CX/inc" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L5CX_Platform" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Drivers/BSP/VL53L8CX_Platform" -I"E:/STM32CubeIDE_Workplace/Stepsense_260307/Core/Src" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Application-2f-User-2f-Core

clean-Application-2f-User-2f-Core:
	-$(RM) ./Application/User/Core/gpio.cyclo ./Application/User/Core/gpio.d ./Application/User/Core/gpio.o ./Application/User/Core/gpio.su ./Application/User/Core/i2c.cyclo ./Application/User/Core/i2c.d ./Application/User/Core/i2c.o ./Application/User/Core/i2c.su ./Application/User/Core/main.cyclo ./Application/User/Core/main.d ./Application/User/Core/main.o ./Application/User/Core/main.su ./Application/User/Core/sensing_engine.cyclo ./Application/User/Core/sensing_engine.d ./Application/User/Core/sensing_engine.o ./Application/User/Core/sensing_engine.su ./Application/User/Core/stm32f4xx_hal_msp.cyclo ./Application/User/Core/stm32f4xx_hal_msp.d ./Application/User/Core/stm32f4xx_hal_msp.o ./Application/User/Core/stm32f4xx_hal_msp.su ./Application/User/Core/stm32f4xx_it.cyclo ./Application/User/Core/stm32f4xx_it.d ./Application/User/Core/stm32f4xx_it.o ./Application/User/Core/stm32f4xx_it.su ./Application/User/Core/syscalls.cyclo ./Application/User/Core/syscalls.d ./Application/User/Core/syscalls.o ./Application/User/Core/syscalls.su ./Application/User/Core/sysmem.cyclo ./Application/User/Core/sysmem.d ./Application/User/Core/sysmem.o ./Application/User/Core/sysmem.su ./Application/User/Core/usart.cyclo ./Application/User/Core/usart.d ./Application/User/Core/usart.o ./Application/User/Core/usart.su

.PHONY: clean-Application-2f-User-2f-Core

