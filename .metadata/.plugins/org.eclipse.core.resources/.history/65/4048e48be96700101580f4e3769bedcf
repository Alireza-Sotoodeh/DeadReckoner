#ifndef MPU9250_H
#define MPU9250_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>
#include <math.h>  // For filter math

// Error codes
typedef enum {
    MPU9250_OK = 0,
    MPU9250_ERR_INIT,
    MPU9250_ERR_COMM,
    MPU9250_ERR_TIMEOUT,
    MPU9250_ERR_INVALID_DATA,
    MPU9250_ERR_DMP_LOAD,
    MPU9250_ERR_CALIB
} MPU9250_Error_t;

// DMP Modes (3 modes as requested)
typedef enum {
    MPU9250_MODE_QUATERNION,   // Fused quaternions (w, x, y, z)
    MPU9250_MODE_EULER,        // Fused Euler angles (yaw, pitch, roll in degrees)
    MPU9250_MODE_LINEAR_ACCEL  // Gravity-compensated linear accel (x, y, z in m/s²)
} MPU9250_Mode_t;

// Data structures for each mode (filtered)
typedef struct {
    float w, x, y, z;  // Quaternions
} MPU9250_Quaternion_t;

typedef struct {
    float yaw, pitch, roll;  // Degrees
} MPU9250_Euler_t;

typedef struct {
    float x, y, z;  // m/s²
} MPU9250_LinearAccel_t;

typedef union {
    MPU9250_Quaternion_t quat;
    MPU9250_Euler_t euler;
    MPU9250_LinearAccel_t lin_accel;
} MPU9250_Data_t;

// Configuration struct (pass to init)
typedef struct {
    SPI_HandleTypeDef *hspi;  // HAL SPI handle (e.g., &hspi1)
    GPIO_TypeDef *cs_port;    // CS GPIO port (e.g., GPIOA)
    uint16_t cs_pin;          // CS pin (e.g., GPIO_PIN_4)
    float sample_rate_hz;     // DMP sample rate (default 200 Hz)
} MPU9250_Config_t;

// Function prototypes
MPU9250_Error_t MPU9250_Init(const MPU9250_Config_t *config);
MPU9250_Error_t MPU9250_Calibrate(void);  // Calibrate sensors (call after init)
MPU9250_Error_t MPU9250_GetData(MPU9250_Mode_t mode, MPU9250_Data_t *data);  // Single command to get filtered data

#endif // MPU9250_H
