#ifndef MPU9250_H
#define MPU9250_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <math.h>
#include <string.h>

// Timeouts and configuration
#define MPU9250_SPI_TIMEOUT 100
#define MPU9250_I2C_TIMEOUT 100
#define MPU9250_MAX_FIFO_SIZE 1024
#define MPU9250_DMP_PACKET_SIZE 28
#define MPU9250_QUAT_SENSITIVITY 1073741824.0f  // 2^30

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
    MPU9250_ERR_DMP_VERIFY,
    MPU9250_ERR_FIFO_OVERFLOW,
    MPU9250_ERR_FIFO_EMPTY,
    MPU9250_ERR_TIMEOUT,
    MPU9250_ERR_INVALID_PARAM,
    MPU9250_ERR_MAG_NOT_READY
} MPU9250_Error_t;

// Output modes
typedef enum {
    MPU9250_MODE_QUAT,
    MPU9250_MODE_EULER,
    MPU9250_MODE_LIN_ACCEL,
    MPU9250_MODE_ALL
} MPU9250_Mode_t;

// DMP features
typedef enum {
    MPU9250_DMP_FEATURE_6X_LP_QUAT       = 0x400,
    MPU9250_DMP_FEATURE_6X_QUAT          = 0x200,
    MPU9250_DMP_FEATURE_3X_QUAT          = 0x100,
    MPU9250_DMP_FEATURE_GYRO_CAL         = 0x020,
    MPU9250_DMP_FEATURE_SEND_RAW_ACCEL   = 0x008,
    MPU9250_DMP_FEATURE_SEND_RAW_GYRO    = 0x004,
    MPU9250_DMP_FEATURE_SEND_CAL_GYRO    = 0x002
} MPU9250_DMP_Feature_t;

// Data structures
typedef struct {
    float w, x, y, z;
} MPU9250_Quat_t;

typedef struct {
    float yaw, pitch, roll;  // In degrees
} MPU9250_Euler_t;

typedef struct {
    float x, y, z;  // In m/s², gravity-compensated
} MPU9250_LinearAccel_t;

typedef struct {
    float x, y, z;  // Raw accelerometer data in g
} MPU9250_Accel_t;

typedef struct {
    float x, y, z;  // Raw gyroscope data in dps
} MPU9250_Gyro_t;

typedef struct {
    float x, y, z;  // Magnetometer data in µT
} MPU9250_Mag_t;

typedef struct {
    MPU9250_Quat_t quat;
    MPU9250_Euler_t euler;
    MPU9250_LinearAccel_t linear_accel;
    MPU9250_Accel_t accel;
    MPU9250_Gyro_t gyro;
    MPU9250_Mag_t mag;
    uint32_t timestamp;
    uint8_t data_ready;
} MPU9250_AllData_t;

typedef union {
    MPU9250_Quat_t quat;
    MPU9250_Euler_t euler;
    MPU9250_LinearAccel_t linear_accel;
    MPU9250_AllData_t all;
} MPU9250_Data_t;

// Configuration
typedef struct {
    SPI_HandleTypeDef *spi_handle;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
    uint16_t sample_rate;        // Hz, up to 1000
    uint8_t gyro_fsr;           // Full scale range (250, 500, 1000, 2000 dps)
    uint8_t accel_fsr;          // Full scale range (2, 4, 8, 16 g)
    uint8_t dlpf_cfg;           // Digital low pass filter (0-6)
    uint8_t calibrate_gyro;     // 1 to enable gyro calib in init
    uint8_t calibrate_accel;    // 1 to enable accel calib in init
    uint8_t calibrate_mag;      // 1 to enable mag calib in init
    float alpha;                // Complementary filter alpha (0.0-1.0, default 0.98)
    uint16_t dmp_features;      // DMP features to enable
} MPU9250_Config_t;

// Calibration data structure
typedef struct {
    float gyro_bias[3];
    float accel_bias[3];
    float mag_bias[3];
    float mag_scale[3];
    uint8_t calibrated;
} MPU9250_Calibration_t;

// API Functions
MPU9250_Error_t MPU9250_Init(MPU9250_Config_t *config);
MPU9250_Error_t MPU9250_CalibrateMag(float *mag_bias, float *mag_scale);
MPU9250_Error_t MPU9250_CalibrateGyro(float *gyro_bias);
MPU9250_Error_t MPU9250_CalibrateAccel(float *accel_bias);
MPU9250_Error_t MPU9250_LoadDMP(void);
MPU9250_Error_t MPU9250_EnableDMP(uint16_t features);
MPU9250_Error_t MPU9250_GetData(MPU9250_Mode_t mode, MPU9250_Data_t *data);
MPU9250_Error_t MPU9250_GetFIFOCount(uint16_t *count);
MPU9250_Error_t MPU9250_ResetFIFO(void);
MPU9250_Error_t MPU9250_GetCalibration(MPU9250_Calibration_t *calib);
MPU9250_Error_t MPU9250_SetCalibration(MPU9250_Calibration_t *calib);

// Utility functions
MPU9250_Error_t MPU9250_SelfTest(uint8_t *result);
MPU9250_Error_t MPU9250_Reset(void);
const char* MPU9250_GetErrorString(MPU9250_Error_t error);

// Future fusion function
MPU9250_Error_t MPU9250_FuseBMP280(float pressure, float altitude);

#endif // MPU9250_H
