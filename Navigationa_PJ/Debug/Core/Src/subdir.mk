################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/driver_mpu9250.c \
../Core/Src/driver_mpu9250_basic.c \
../Core/Src/driver_mpu9250_dmp.c \
../Core/Src/driver_mpu9250_fifo.c \
../Core/Src/driver_mpu9250_interface.c \
../Core/Src/driver_mpu9250_register_test.c \
../Core/Src/main.c \
../Core/Src/stm32f4xx_hal_msp.c \
../Core/Src/stm32f4xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32f4xx.c 

OBJS += \
./Core/Src/driver_mpu9250.o \
./Core/Src/driver_mpu9250_basic.o \
./Core/Src/driver_mpu9250_dmp.o \
./Core/Src/driver_mpu9250_fifo.o \
./Core/Src/driver_mpu9250_interface.o \
./Core/Src/driver_mpu9250_register_test.o \
./Core/Src/main.o \
./Core/Src/stm32f4xx_hal_msp.o \
./Core/Src/stm32f4xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32f4xx.o 

C_DEPS += \
./Core/Src/driver_mpu9250.d \
./Core/Src/driver_mpu9250_basic.d \
./Core/Src/driver_mpu9250_dmp.d \
./Core/Src/driver_mpu9250_fifo.d \
./Core/Src/driver_mpu9250_interface.d \
./Core/Src/driver_mpu9250_register_test.d \
./Core/Src/main.d \
./Core/Src/stm32f4xx_hal_msp.d \
./Core/Src/stm32f4xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32f4xx.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xC -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/driver_mpu9250.cyclo ./Core/Src/driver_mpu9250.d ./Core/Src/driver_mpu9250.o ./Core/Src/driver_mpu9250.su ./Core/Src/driver_mpu9250_basic.cyclo ./Core/Src/driver_mpu9250_basic.d ./Core/Src/driver_mpu9250_basic.o ./Core/Src/driver_mpu9250_basic.su ./Core/Src/driver_mpu9250_dmp.cyclo ./Core/Src/driver_mpu9250_dmp.d ./Core/Src/driver_mpu9250_dmp.o ./Core/Src/driver_mpu9250_dmp.su ./Core/Src/driver_mpu9250_fifo.cyclo ./Core/Src/driver_mpu9250_fifo.d ./Core/Src/driver_mpu9250_fifo.o ./Core/Src/driver_mpu9250_fifo.su ./Core/Src/driver_mpu9250_interface.cyclo ./Core/Src/driver_mpu9250_interface.d ./Core/Src/driver_mpu9250_interface.o ./Core/Src/driver_mpu9250_interface.su ./Core/Src/driver_mpu9250_register_test.cyclo ./Core/Src/driver_mpu9250_register_test.d ./Core/Src/driver_mpu9250_register_test.o ./Core/Src/driver_mpu9250_register_test.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/stm32f4xx_hal_msp.cyclo ./Core/Src/stm32f4xx_hal_msp.d ./Core/Src/stm32f4xx_hal_msp.o ./Core/Src/stm32f4xx_hal_msp.su ./Core/Src/stm32f4xx_it.cyclo ./Core/Src/stm32f4xx_it.d ./Core/Src/stm32f4xx_it.o ./Core/Src/stm32f4xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32f4xx.cyclo ./Core/Src/system_stm32f4xx.d ./Core/Src/system_stm32f4xx.o ./Core/Src/system_stm32f4xx.su

.PHONY: clean-Core-2f-Src

