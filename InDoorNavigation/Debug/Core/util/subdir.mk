################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/util/inv_mpu.c \
../Core/util/inv_mpu_dmp_motion_driver.c \
../Core/util/stm32_mpu9250_clk.c \
../Core/util/stm32_mpu9250_i2c.c 

OBJS += \
./Core/util/inv_mpu.o \
./Core/util/inv_mpu_dmp_motion_driver.o \
./Core/util/stm32_mpu9250_clk.o \
./Core/util/stm32_mpu9250_i2c.o 

C_DEPS += \
./Core/util/inv_mpu.d \
./Core/util/inv_mpu_dmp_motion_driver.d \
./Core/util/stm32_mpu9250_clk.d \
./Core/util/stm32_mpu9250_i2c.d 


# Each subdirectory must supply rules for building sources it contributes
Core/util/%.o Core/util/%.su Core/util/%.cyclo: ../Core/util/%.c Core/util/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xC -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-util

clean-Core-2f-util:
	-$(RM) ./Core/util/inv_mpu.cyclo ./Core/util/inv_mpu.d ./Core/util/inv_mpu.o ./Core/util/inv_mpu.su ./Core/util/inv_mpu_dmp_motion_driver.cyclo ./Core/util/inv_mpu_dmp_motion_driver.d ./Core/util/inv_mpu_dmp_motion_driver.o ./Core/util/inv_mpu_dmp_motion_driver.su ./Core/util/stm32_mpu9250_clk.cyclo ./Core/util/stm32_mpu9250_clk.d ./Core/util/stm32_mpu9250_clk.o ./Core/util/stm32_mpu9250_clk.su ./Core/util/stm32_mpu9250_i2c.cyclo ./Core/util/stm32_mpu9250_i2c.d ./Core/util/stm32_mpu9250_i2c.o ./Core/util/stm32_mpu9250_i2c.su

.PHONY: clean-Core-2f-util

