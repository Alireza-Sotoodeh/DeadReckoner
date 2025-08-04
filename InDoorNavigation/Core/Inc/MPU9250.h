#ifndef MPU9250_H
#define MPU9250_H

#include "stm32f4xx_hal.h"

// Define SPI handle (assuming SPI1 is used)
extern SPI_HandleTypeDef hspi1;

// Define CS pin
#define MPU9250_CS_GPIO_PORT GPIOA
#define MPU9250_CS_PIN GPIO_PIN_4

// MPU9250 registers and constants
#define MPU9250_WHO_AM_I 0x75
#define MPU9250_ADDRESS 0x68

// Function prototypes
void MPU9250_Init(void);
void MPU9250_ReadGyro(float *gx, float *gy, float *gz);
void MPU9250_ReadAccel(float *ax, float *ay, float *az);
void MPU9250_ReadMag(float *mx, float *my, float *mz);
float MPU9250_ReadTemp(void);

#endif // MPU9250_H
