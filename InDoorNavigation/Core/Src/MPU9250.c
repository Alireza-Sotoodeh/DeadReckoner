#include "MPU9250.h"

// Helper function to write to MPU9250 register
static void MPU9250_WriteReg(uint8_t reg, uint8_t data) {
    HAL_GPIO_WritePin(MPU9250_CS_GPIO_PORT, MPU9250_CS_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, &reg, 1, HAL_MAX_DELAY);
    HAL_SPI_Transmit(&hspi1, &data, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(MPU9250_CS_GPIO_PORT, MPU9250_CS_PIN, GPIO_PIN_SET);
}

// Helper function to read from MPU9250 register
static uint8_t MPU9250_ReadReg(uint8_t reg) {
    uint8_t data;
    reg |= 0x80; // Set MSB to 1 for read operation
    HAL_GPIO_WritePin(MPU9250_CS_GPIO_PORT, MPU9250_CS_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, &reg, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi1, &data, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(MPU9250_CS_GPIO_PORT, MPU9250_CS_PIN, GPIO_PIN_SET);
    return data;
}

// Initialize MPU9250
void MPU9250_Init(void) {
    // Reset MPU9250
    MPU9250_WriteReg(0x6B, 0x80);
    HAL_Delay(100);
    // Wake up MPU9250 and select clock source
    MPU9250_WriteReg(0x6B, 0x01); // Use gyro X-axis PLL for better accuracy
    // Configure gyro (±2000°/s)
    MPU9250_WriteReg(0x1B, 0x18);
    // Configure accel (±16g)
    MPU9250_WriteReg(0x1C, 0x18);
    // Enable magnetometer (bypass mode for direct access)
    MPU9250_WriteReg(0x37, 0x02);
    HAL_Delay(10);
    // Configure magnetometer (AK8963) - 16-bit, 100Hz continuous mode
    MPU9250_WriteReg(0x0A, 0x16); // Assumes AK8963 I2C address is handled via bypass
}

// Read gyroscope data
void MPU9250_ReadGyro(float *gx, float *gy, float *gz) {
    uint8_t data[6];
    for (int i = 0; i < 6; i++) {
        data[i] = MPU9250_ReadReg(0x43 + i);
    }
    int16_t raw_gx = (data[0] << 8) | data[1];
    int16_t raw_gy = (data[2] << 8) | data[3];
    int16_t raw_gz = (data[4] << 8) | data[5];
    *gx = raw_gx / 16.4; // Sensitivity for ±2000°/s
    *gy = raw_gy / 16.4;
    *gz = raw_gz / 16.4;
}

// Read accelerometer data
void MPU9250_ReadAccel(float *ax, float *ay, float *az) {
    uint8_t data[6];
    for (int i = 0; i < 6; i++) {
        data[i] = MPU9250_ReadReg(0x3B + i);
    }
    int16_t raw_ax = (data[0] << 8) | data[1];
    int16_t raw_ay = (data[2] << 8) | data[3];
    int16_t raw_az = (data[4] << 8) | data[5];
    *ax = raw_ax / 2048.0; // Sensitivity for ±16g
    *ay = raw_ay / 2048.0;
    *az = raw_az / 2048.0;
}

// Read magnetometer data (AK8963 via bypass mode)
void MPU9250_ReadMag(float *mx, float *my, float *mz) {
    uint8_t data[7]; // 6 bytes data + 1 status byte
    // Check if data is ready (ST1 register)
    data[0] = MPU9250_ReadReg(0x02);
    if (data[0] & 0x01) {
        for (int i = 0; i < 6; i++) {
            data[i] = MPU9250_ReadReg(0x03 + i);
        }
        int16_t raw_mx = (data[1] << 8) | data[0]; // Little-endian
        int16_t raw_my = (data[3] << 8) | data[2];
        int16_t raw_mz = (data[5] << 8) | data[4];
        // Adjust for sensitivity (assume 10-bit default adjusted to 16-bit mode)
        *mx = raw_mx * 0.15; // 4912 µT / 32760 = ~0.15 µT/LSB
        *my = raw_my * 0.15;
        *mz = raw_mz * 0.15;
    }
}

// Read temperature
float MPU9250_ReadTemp(void) {
    uint8_t data[2];
    data[0] = MPU9250_ReadReg(0x41);
    data[1] = MPU9250_ReadReg(0x42);
    int16_t raw_temp = (data[0] << 8) | data[1];
    return (raw_temp / 340.0) + 36.53; // Formula from MPU9250 datasheet
}
