#ifndef MPU9250_H
#define MPU9250_H

#include "stm32f4xx_hal.h"  // STM32 HAL library
#include <stdint.h>
#include <math.h>  // For trig functions in Euler conversion

// Timeouts
#define MPU9250_SPI_TIMEOUT 100
#define MPU9250_I2C_TIMEOUT 100

// Addresses
#define MPU9250_I2C_ADDR 0x68
#define AK8963_I2C_ADDR 0x0C

// Error codes
typedef enum {
    MPU9250_OK = 0,
    MPU9250_ERR_SPI,
    MPU9250_ERR_I2C,
    MPU9250_ERR_WHOAMI,
    MPU9250_ERR_CALIB_FAIL,
    MPU9250_ERR_DMP_LOAD,
    MPU9250_ERR_FIFO_OVERFLOW,
    MPU9250_ERR_TIMEOUT
} MPU9250_Error_t;

// Output modes
typedef enum {
    MPU9250_MODE_QUAT,
    MPU9250_MODE_EULER,
    MPU9250_MODE_LIN_ACCEL
} MPU9250_Mode_t;

// Data structures
typedef struct {
    float w, x, y, z;
} MPU9250_Quat_t;

typedef struct {
    float yaw, pitch, roll;  // In degrees
} MPU9250_Euler_t;

typedef struct {
    float x, y, z;  // In m/s², gravity-compensated
} MPU9250_Accel_t;

typedef union {
    MPU9250_Quat_t quat;
    MPU9250_Euler_t euler;
    MPU9250_Accel_t accel;
} MPU9250_Data_t;

// Configuration
typedef struct {
    SPI_HandleTypeDef *spi_handle;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
    uint16_t sample_rate;  // Hz, up to 1000
    uint8_t gyro_fsr;      // Full scale range (250, 500, 1000, 2000 dps)
    uint8_t accel_fsr;     // Full scale range (2, 4, 8, 16 g)
    uint8_t calibrate_gyro;  // 1 to enable gyro calib in init
    uint8_t calibrate_accel; // 1 to enable accel calib in init
    float alpha;             // Complementary filter alpha (0.0-1.0, default 0.98)
} MPU9250_Config_t;

// API Functions
MPU9250_Error_t MPU9250_Init(MPU9250_Config_t *config);
MPU9250_Error_t MPU9250_CalibrateMag(float *mag_bias, float *mag_scale);
MPU9250_Error_t MPU9250_CalibrateGyro(float *gyro_bias);
MPU9250_Error_t MPU9250_CalibrateAccel(float *accel_bias);
MPU9250_Error_t MPU9250_LoadDMP(const uint8_t *firmware, uint16_t firmware_size);
MPU9250_Error_t MPU9250_GetData(MPU9250_Mode_t mode, MPU9250_Data_t *data);
MPU9250_Error_t MPU9250_FuseBMP280(float pressure, float altitude);  // Placeholder for future fusion

#endif // MPU9250_H
