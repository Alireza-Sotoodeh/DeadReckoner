/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "driver_mpu6500.h"
#include "driver_mpu6500_dmp.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>

/* Private define ------------------------------------------------------------*/
#define SAMPLE_RATE_HZ 200  // increased update rate
#define ACCEL_RANGE MPU6500_ACCELEROMETER_RANGE_8G
#define GYRO_RANGE MPU6500_GYROSCOPE_RANGE_1000DPS
#define PI 3.141592653589793238462643383279502884197f
#define MAX_INIT_RETRIES 3
#define INIT_RETRY_DELAY_MS 500

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart1;
static mpu6500_handle_t gs_handle;

// Variables for sensor data
float accel_g[3] = {0};      // Acceleration in g
float gyro_dps[3] = {0};     // Gyro in degrees per second
int32_t quat[4] = {0};       // Quaternion
float pitch = 0, roll = 0, yaw = 0;  // Euler angles
uint32_t step_count = 0;     // Pedometer step count
float position[3] = {0};     // Estimated position (x,y,z)
float velocity[3] = {0};     // Estimated velocity

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);
void mpu6500_receive_callback(uint8_t type);
void mpu6500_tap_callback(uint8_t count, uint8_t direction);
void mpu6500_orient_callback(uint8_t orientation);
void update_navigation_estimates(void);
void send_data_via_uart(void);
void debug_log(const char *format, ...);

/* Private user code ---------------------------------------------------------*/

// Debug logging function
void debug_log(const char *format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    HAL_UART_Transmit(&huart1, (uint8_t *)buffer, strlen(buffer), HAL_MAX_DELAY);
}

// Callback for MPU6500 data ready interrupt and FIFO overflow
void mpu6500_receive_callback(uint8_t type) {
    if (type == MPU6500_INTERRUPT_DATA_READY) {
        uint16_t len = 1;
        int16_t accel_raw[3];
        int16_t gyro_raw[3];

        if (mpu6500_dmp_read_all(&accel_raw, &accel_g, &gyro_raw, &gyro_dps, &quat, &pitch, &roll, &yaw, &len) == 0) {
            // Validate sensor data
            for (int i = 0; i < 3; i++) {
                if (fabs(accel_g[i]) > 4.0f || fabs(gyro_dps[i]) > 500.0f) {
                    debug_log("Invalid sensor data: accel_g[%d]=%.2f, gyro_dps[%d]=%.2f\r\n", i, accel_g[i], i, gyro_dps[i]);
                    return;
                }
            }
            update_navigation_estimates();
            send_data_via_uart();
        } else {
            debug_log("Failed to read DMP data\r\n");
        }
    } else if (type == MPU6500_INTERRUPT_FIFO_OVERFLOW) {
        debug_log("FIFO overflow detected, resetting FIFO\r\n");
        mpu6500_force_fifo_reset(&gs_handle);
    }
}

// EXTI interrupt handler for MPU6500
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == EXT_Pin) {
        if (mpu6500_dmp_irq_handler() != 0) {
            debug_log("MPU6500 IRQ handler failed\r\n");
        }
    }
}

// Tap detection callback
void mpu6500_tap_callback(uint8_t count, uint8_t direction) {
    debug_log("Tap detected: count=%d, dir=%d\r\n", count, direction);
}

// Orientation callback
void mpu6500_orient_callback(uint8_t orientation) {
    debug_log("Orientation changed: %d\r\n", orientation);
}

// Simple navigation update (dead reckoning)
void update_navigation_estimates(void) {
    static uint32_t last_time = 0;
    uint32_t current_time = HAL_GetTick();
    float dt = (current_time - last_time) / 1000.0f; // Convert to seconds

    if (last_time != 0 && dt > 0) {
        for (int i = 0; i < 3; i++) {
            float gravity_component = 0;
            if (i == 0) gravity_component = sinf(roll * PI / 180.0f);
            if (i == 1) gravity_component = -sinf(pitch * PI / 180.0f);
            if (i == 2) gravity_component = -1.0f;

            float linear_accel = accel_g[i] - gravity_component;
            velocity[i] += linear_accel * 9.81f * dt; // Convert g to m/s²
            position[i] += velocity[i] * dt;
        }
    }
    last_time = current_time;
}

// Send data via UART for debugging
void send_data_via_uart(void) {
    static uint32_t last_send_time = 0;
    uint32_t current_time = HAL_GetTick();

    if (current_time - last_send_time >= 100) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), // @suppress("Float formatting support")
                 "Pitch: %.2f, Roll: %.2f, Yaw: %.2f | "
                 "Accel: %.2f, %.2f, %.2f g | "
                 "Gyro: %.2f, %.2f, %.2f dps | "
                 "Pos: %.2f, %.2f, %.2f m\r\n",
                 pitch, roll, yaw,
                 accel_g[0], accel_g[1], accel_g[2],
                 gyro_dps[0], gyro_dps[1], gyro_dps[2],
                 position[0], position[1], position[2]);
        HAL_UART_Transmit(&huart1, (uint8_t *)buffer, strlen(buffer), HAL_MAX_DELAY);
        last_send_time = current_time;
    }
}

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void) {
    /* MCU Configuration--------------------------------------------------------*/
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_USART1_UART_Init();

    /* Initialize MPU6500 with retry logic */
    uint8_t res;
    uint8_t retries = 0;
    while (retries < MAX_INIT_RETRIES) {
        res = mpu6500_dmp_init(MPU6500_INTERFACE_SPI, MPU6500_ADDRESS_AD0_LOW,
                               mpu6500_receive_callback,
                               mpu6500_tap_callback,
                               mpu6500_orient_callback);
        if (res == 0) {
            debug_log("MPU6500 initialized successfully!\r\n");
            break;
        }
        debug_log("MPU6500 init failed, attempt %d/%d\r\n", retries + 1, MAX_INIT_RETRIES);
        HAL_Delay(INIT_RETRY_DELAY_MS);
        retries++;
    }

    if (res != 0) {
        debug_log("MPU6500 initialization failed after %d retries!\r\n", MAX_INIT_RETRIES);
        while (1); // Consider recovery mode instead of halting
    }

    /* Configure MPU6500 settings */
    mpu6500_set_sample_rate_divider(&gs_handle, (1000 / SAMPLE_RATE_HZ) - 1);
    mpu6500_set_accelerometer_range(&gs_handle, ACCEL_RANGE);
    mpu6500_set_gyroscope_range(&gs_handle, GYRO_RANGE);

    /* Infinite loop */
    while (1) {
        HAL_Delay(100);
        if (mpu6500_dmp_get_pedometer_counter(&step_count) == 0) {
            debug_log("Steps: %lu\r\n", step_count);
        } else {
            debug_log("Failed to read pedometer counter\r\n");
        }
    }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 80;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief SPI1 Initialization Function
  * @retval None
  */
static void MX_SPI1_Init(void) {
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi1) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief USART1 Initialization Function
  * @retval None
  */
static void MX_USART1_UART_Init(void) {
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief GPIO Initialization Function
  * @retval None
  */
static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    HAL_GPIO_WritePin(MPU6500_CS_GPIO_Port, MPU6500_CS_Pin, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = MPU6500_CS_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(MPU6500_CS_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = EXT_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(EXT_GPIO_Port, &GPIO_InitStruct);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void) {
    __disable_irq();
    while (1) {
    }
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line) {
    debug_log("Assert failed: file %s on line %d\r\n", file, line);
}
#endif /* USE_FULL_ASSERT */
