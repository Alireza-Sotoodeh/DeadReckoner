/**
 * Copyright (c) 2015 - present LibDriver All rights reserved
 *
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation- Keep `g_spi_status`, `g_last_addr`, and `g_operation_type` in `driver_mpu9250_interface.c` as they're specific to interface.
 files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * @file      driver_mpu9250_interface.c
 * @brief     driver mpu9250 STM32 SPI interface source file
 * @version   1.0.0
 * @author    Shifeng Li (template); adapted for STM32
 * @date      2022-08-30 (template); 2025-08-02 (adapted)
 *
 * <h3>history</h3>
 * <table>
 * <tr><th>Date        <th>Version  <th>Author      <th>Description
 * <tr><td>2022/08/30  <td>1.0      <td>Shifeng Li  <td>first upload
 * <tr><td>2025/08/02  <td>1.1      <td>[Your Name] <td>adapted for STM32 SPI at 10 MHz with volatile debugging
 * </table>
 */

#include "driver_mpu9250_interface.h"
#include "stm32f4xx_hal.h"  // Your HAL includes

/* Define your SPI handle (from CubeMX-generated code, e.g., main.h or spi.h) */
extern SPI_HandleTypeDef hspi1;  /* Assuming SPI1 is used; change if different */

/* Define CS (Chip Select) pin (adjust based on your CubeMX config, e.g., PA4) */
#define MPU9250_CS_PORT GPIOA
#define MPU9250_CS_PIN  GPIO_PIN_4

/* Volatile variables for Live Expressions debugging in CubeIDE (no prints) */
volatile uint8_t g_spi_status = 0;         /* 0: OK, 1: Init, 2: Read err, 3: Write err (addr), 4: Write err (data), 5: IRQ recv */
volatile uint8_t g_last_addr = 0;          /* Last register address accessed */
volatile uint8_t g_operation_type = 0;     /* 0: None, 1: Read, 2: Write */

/**
 * @brief  interface iic bus init
 * @return status code
 *         - 0 success
 *         - 1 iic init failed
 * @note   none (not used for SPI)
 */
uint8_t mpu9250_interface_iic_init(void)
{
    return 1;  /* Unused for SPI */
}

/**
 * @brief  interface iic bus deinit
 * @return status code
 *         - 0 success
 *         - 1 iic deinit failed
 * @note   none
 */
uint8_t mpu9250_interface_iic_deinit(void)
{
    return 1;  /* Unused for SPI */
}

/**
 * @brief      interface iic bus read
 * @param[in]  addr iic device write address
 * @param[in]  reg iic register address
 * @param[out] *buf pointer to a data buffer
 * @param[in]  len length of the data buffer
 * @return     status code
 *             - 0 success
 *             - 1 read failed
 * @note       none
 */
uint8_t mpu9250_interface_iic_read(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    return 1;  /* Unused for SPI */
}

/**
 * @brief     interface iic bus write
 * @param[in] addr iic device write address
 * @param[in] reg iic register address
 * @param[in] *buf pointer to a data buffer
 * @param[in] len length of the data buffer
 * @return    status code
 *            - 0 success
 *            - 1 write failed
 * @note      none
 */
uint8_t mpu9250_interface_iic_write(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    return 1;  /* Unused for SPI */
}

/**
 * @brief  interface spi bus init
 * @return status code
 *         - 0 success
 *         - 1 spi init failed
 * @note   SPI speed (~10 MHz) set in CubeMX (.ioc): e.g., prescaler 8 for 84 MHz APB2 clock.
 *         CubeMX calls HAL_SPI_Init; this assumes it's done in main.
 */
uint8_t mpu9250_interface_spi_init(void)
{
    /* Set CS high (idle) */
    HAL_GPIO_WritePin(MPU9250_CS_PORT, MPU9250_CS_PIN, GPIO_PIN_SET);
    g_spi_status = 1;  /* Indicate init */
    return 0;  /* Success */
}

/**
 * @brief  interface spi bus deinit
 * @return status code
 *         - 0 success
 *         - 1 spi deinit failed
 * @note   none
 */
uint8_t mpu9250_interface_spi_deinit(void)
{
    HAL_SPI_DeInit(&hspi1);
    g_spi_status = 0;
    return 0;
}

/**
 * @brief      interface spi bus read
 * @param[in]  reg register address
 * @param[out] *buf pointer to a data buffer
 * @param[in]  len length of data buffer
 * @return     status code
 *             - 0 success
 *             - 1 read failed
 * @note       none
 */
uint8_t mpu9250_interface_spi_read(uint8_t reg, uint8_t *buf, uint16_t len)
{
    uint8_t tx_buf[1] = {reg | 0x80};  /* Read bit set */
    g_operation_type = 1;  /* Read */
    g_last_addr = reg;

    HAL_GPIO_WritePin(MPU9250_CS_PORT, MPU9250_CS_PIN, GPIO_PIN_RESET);  /* CS low */
    if (HAL_SPI_Transmit(&hspi1, tx_buf, 1, HAL_MAX_DELAY) != HAL_OK)
    {
        g_spi_status = 2;  /* Read error */
        HAL_GPIO_WritePin(MPU9250_CS_PORT, MPU9250_CS_PIN, GPIO_PIN_SET);
        return 1;
    }
    if (HAL_SPI_Receive(&hspi1, buf, len, HAL_MAX_DELAY) != HAL_OK)
    {
        g_spi_status = 2;  /* Read error */
        HAL_GPIO_WritePin(MPU9250_CS_PORT, MPU9250_CS_PIN, GPIO_PIN_SET);
        return 1;
    }
    HAL_GPIO_WritePin(MPU9250_CS_PORT, MPU9250_CS_PIN, GPIO_PIN_SET);  /* CS high */

    g_spi_status = 0;  /* Success */
    return 0;
}

/**
 * @brief     interface spi bus write
 * @param[in] reg register address
 * @param[in] *buf pointer to a data buffer
 * @param[in] len length of data buffer
 * @return    status code
 *            - 0 success
 *            - 1 write failed
 * @note      none
 */
uint8_t mpu9250_interface_spi_write(uint8_t reg, uint8_t *buf, uint16_t len)
{
    uint8_t tx_buf[1] = {reg & 0x7F};  /* Write bit clear */
    g_operation_type = 2;  /* Write */
    g_last_addr = reg;

    HAL_GPIO_WritePin(MPU9250_CS_PORT, MPU9250_CS_PIN, GPIO_PIN_RESET);  /* CS low */
    if (HAL_SPI_Transmit(&hspi1, tx_buf, 1, HAL_MAX_DELAY) != HAL_OK)
    {
        g_spi_status = 3;  /* Write error (addr) */
        HAL_GPIO_WritePin(MPU9250_CS_PORT, MPU9250_CS_PIN, GPIO_PIN_SET);
        return 1;
    }
    if (HAL_SPI_Transmit(&hspi1, buf, len, HAL_MAX_DELAY) != HAL_OK)
    {
        g_spi_status = 4;  /* Write error (data) */
        HAL_GPIO_WritePin(MPU9250_CS_PORT, MPU9250_CS_PIN, GPIO_PIN_SET);
        return 1;
    }
    HAL_GPIO_WritePin(MPU9250_CS_PORT, MPU9250_CS_PIN, GPIO_PIN_SET);  /* CS high */

    g_spi_status = 0;  /* Success */
    return 0;
}

/**
 * @brief     interface delay ms
 * @param[in] ms time
 * @note      none
 */
void mpu9250_interface_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

/**
 * @brief     interface print format data
 * @param[in] fmt format data
 * @note      Disabled; use Live Expressions for volatiles
 */
void mpu9250_interface_debug_print(const char *const fmt, ...)
{
    /* Intentionally empty */
}
