#include "mpu9250.h"

// Register definitions (MPU9250)
#define MPU9250_REG_WHO_AM_I 0x75
#define MPU9250_REG_PWR_MGMT_1 0x6B
#define MPU9250_REG_GYRO_CONFIG 0x1B
#define MPU9250_REG_ACCEL_CONFIG 0x1C
#define MPU9250_REG_ACCEL_CONFIG2 0x1D
#define MPU9250_REG_SMPLRT_DIV 0x19
#define MPU9250_REG_CONFIG 0x1A
#define MPU9250_REG_INT_PIN_CFG 0x37
#define MPU9250_REG_USER_CTRL 0x6A
#define MPU9250_REG_FIFO_EN 0x23
#define MPU9250_REG_FIFO_COUNT_H 0x72
#define MPU9250_REG_FIFO_R_W 0x74
#define MPU9250_REG_MEM_BANK 0x6D
#define MPU9250_REG_MEM_START_ADDR 0x6E
#define MPU9250_REG_MEM_R_W 0x6F

// AK8963 registers
#define AK8963_REG_WHO_AM_I 0x00
#define AK8963_REG_CNTL1 0x0A
#define AK8963_REG_CNTL2 0x0B
#define AK8963_REG_ASAX 0x10
#define AK8963_REG_HXL 0x03

// Driver globals
static SPI_HandleTypeDef *spi_h = NULL;
static GPIO_TypeDef *cs_p = NULL;
static uint16_t cs_pin_val = 0;
static uint16_t sample_rate_hz = 0;
static float gyro_scale = 0.0f;
static float accel_scale = 0.0f;
static float mag_bias[3] = {0};
static float mag_scale[3] = {1.0f, 1.0f, 1.0f};
static float gyro_bias[3] = {0};
static float accel_bias[3] = {0};
static float filter_alpha = 0.98f;  // Default complementary filter alpha

// DMP firmware (assume externally defined)
extern const uint8_t dmp_firmware[];
extern const uint16_t dmp_firmware_size;

// Helper functions
static void CS_LOW(void) {
    HAL_GPIO_WritePin(cs_p, cs_pin_val, GPIO_PIN_RESET);
}

static void CS_HIGH(void) {
    HAL_GPIO_WritePin(cs_p, cs_pin_val, GPIO_PIN_SET);
}

static MPU9250_Error_t write_reg(uint8_t reg, uint8_t val) {
    uint8_t tx[2] = {reg, val};
    CS_LOW();
    HAL_StatusTypeDef status = HAL_SPI_Transmit(spi_h, tx, 2, MPU9250_SPI_TIMEOUT);
    CS_HIGH();
    return (status == HAL_OK) ? MPU9250_OK : MPU9250_ERR_SPI;
}

static MPU9250_Error_t read_regs(uint8_t reg, uint8_t *buf, uint16_t len) {
    uint8_t tx = reg | 0x80;  // Read bit
    CS_LOW();
    HAL_SPI_Transmit(spi_h, &tx, 1, MPU9250_SPI_TIMEOUT);
    HAL_StatusTypeDef status = HAL_SPI_Receive(spi_h, buf, len, MPU9250_SPI_TIMEOUT);
    CS_HIGH();
    return (status == HAL_OK) ? MPU9250_OK : MPU9250_ERR_SPI;
}

static MPU9250_Error_t write_ak8963(uint8_t reg, uint8_t val) {
    // Use MPU's aux I2C to write to AK8963
    if (write_reg(0x25, AK8963_I2C_ADDR << 1) != MPU9250_OK) return MPU9250_ERR_I2C;
    if (write_reg(0x26, reg) != MPU9250_OK) return MPU9250_ERR_I2C;
    if (write_reg(0x63, val) != MPU9250_OK) return MPU9250_ERR_I2C;
    return write_reg(0x24, 0x01);  // Trigger write
}

static MPU9250_Error_t read_ak8963(uint8_t reg, uint8_t *buf, uint16_t len) {
    // Use MPU's aux I2C to read from AK8963
    if (write_reg(0x25, (AK8963_I2C_ADDR << 1) | 0x01) != MPU9250_OK) return MPU9250_ERR_I2C;
    if (write_reg(0x26, reg) != MPU9250_OK) return MPU9250_ERR_I2C;
    if (write_reg(0x27, len | 0x80) != MPU9250_OK) return MPU9250_ERR_I2C;
    return read_regs(0x49, buf, len);
}

static MPU9250_Error_t set_mem_bank(uint8_t bank) {
    return write_reg(MPU9250_REG_MEM_BANK, bank);
}

static MPU9250_Error_t set_mem_start(uint8_t addr) {
    return write_reg(MPU9250_REG_MEM_START_ADDR, addr);
}

static MPU9250_Error_t write_mem_chunk(uint16_t addr, const uint8_t *data, uint16_t len) {
    set_mem_bank(addr >> 8);
    set_mem_start(addr & 0xFF);
    for (uint16_t i = 0; i < len; i++) {
        if (write_reg(MPU9250_REG_MEM_R_W, data[i]) != MPU9250_OK) return MPU9250_ERR_SPI;
    }
    return MPU9250_OK;
}

static MPU9250_Error_t flush_fifo(void) {
    return write_reg(MPU9250_REG_USER_CTRL, 0x04);  // Reset FIFO
}

// Init function
MPU9250_Error_t MPU9250_Init(MPU9250_Config_t *config) {
    spi_h = config->spi_handle;
    cs_p = config->cs_port;
    cs_pin_val = config->cs_pin;
    sample_rate_hz = config->sample_rate;
    filter_alpha = config->alpha;

    // Set scales
    switch (config->gyro_fsr) {
        case 250: gyro_scale = 250.0f / 32768.0f; break;
        case 500: gyro_scale = 500.0f / 32768.0f; break;
        case 1000: gyro_scale = 1000.0f / 32768.0f; break;
        case 2000: gyro_scale = 2000.0f / 32768.0f; break;
        default: return MPU9250_ERR_CALIB_FAIL;
    }
    switch (config->accel_fsr) {
        case 2: accel_scale = 2.0f / 32768.0f; break;
        case 4: accel_scale = 4.0f / 32768.0f; break;
        case 8: accel_scale = 8.0f / 32768.0f; break;
        case 16: accel_scale = 16.0f / 32768.0f; break;
        default: return MPU9250_ERR_CALIB_FAIL;
    }

    // Reset MPU
    if (write_reg(MPU9250_REG_PWR_MGMT_1, 0x80) != MPU9250_OK) return MPU9250_ERR_SPI;
    HAL_Delay(100);

    // Wake up and set clock
    if (write_reg(MPU9250_REG_PWR_MGMT_1, 0x01) != MPU9250_OK) return MPU9250_ERR_SPI;

    // Config gyro/accel
    if (write_reg(MPU9250_REG_GYRO_CONFIG, (config->gyro_fsr / 250) << 3) != MPU9250_OK) return MPU9250_ERR_SPI;
    if (write_reg(MPU9250_REG_ACCEL_CONFIG, (config->accel_fsr / 2) << 3) != MPU9250_OK) return MPU9250_ERR_SPI;
    if (write_reg(MPU9250_REG_ACCEL_CONFIG2, 0x00) != MPU9250_OK) return MPU9250_ERR_SPI;  // DLPF off for high speed

    // Sample rate
    if (write_reg(MPU9250_REG_SMPLRT_DIV, (1000 / sample_rate_hz) - 1) != MPU9250_OK) return MPU9250_ERR_SPI;

    // Enable I2C bypass for AK8963
    if (write_reg(MPU9250_REG_INT_PIN_CFG, 0x02) != MPU9250_OK) return MPU9250_ERR_SPI;

    // Verify WHO_AM_I
    uint8_t whoami;
    if (read_regs(MPU9250_REG_WHO_AM_I, &whoami, 1) != MPU9250_OK || whoami != 0x71) return MPU9250_ERR_WHOAMI;

    // Optional calibrations
    if (config->calibrate_gyro) MPU9250_CalibrateGyro(gyro_bias);
    if (config->calibrate_accel) MPU9250_CalibrateAccel(accel_bias);

    // Basic mag setup (power on for calib)
    write_ak8963(AK8963_REG_CNTL1, 0x16);  // Continuous mode 2, 16-bit
    HAL_Delay(10);

    return MPU9250_OK;
}

// Calibrate magnetometer (improved with hard/soft iron correction)
MPU9250_Error_t MPU9250_CalibrateMag(float *mag_bias_out, float *mag_scale_out) {
    const int samples = 500;  // Collect 500 samples
    float mag_min[3] = {+1e9, +1e9, +1e9};
    float mag_max[3] = {-1e9, -1e9, -1e9};
    int16_t raw[3];

    // Instruct user: "Rotate sensor in figure-8 for 10 seconds"
    // (Assume user handles this; in real app, add UART prompt)

    for (int i = 0; i < samples; i++) {
        uint8_t buf[6];
        if (read_ak8963(AK8963_REG_HXL, buf, 6) != MPU9250_OK) return MPU9250_ERR_I2C;
        raw[0] = (int16_t)((buf[1] << 8) | buf[0]);
        raw[1] = (int16_t)((buf[3] << 8) | buf[2]);
        raw[2] = (int16_t)((buf[5] << 8) | buf[4]);

        for (int j = 0; j < 3; j++) {
            if (raw[j] < mag_min[j]) mag_min[j] = raw[j];
            if (raw[j] > mag_max[j]) mag_max[j] = raw[j];
        }
        HAL_Delay(20);  // ~50Hz sampling
    }

    // Compute bias and scale
    for (int j = 0; j < 3; j++) {
        mag_bias[j] = (mag_max[j] + mag_min[j]) / 2.0f;
        mag_scale[j] = (mag_max[j] - mag_min[j]) / 2.0f;
    }
    float avg_scale = (mag_scale[0] + mag_scale[1] + mag_scale[2]) / 3.0f;
    for (int j = 0; j < 3; j++) mag_scale[j] = avg_scale / mag_scale[j];

    // Copy out
    for (int j = 0; j < 3; j++) {
        mag_bias_out[j] = mag_bias[j];
        mag_scale_out[j] = mag_scale[j];
    }

    // Power down mag after calib
    write_ak8963(AK8963_REG_CNTL1, 0x00);
    return MPU9250_OK;
}

// Calibrate gyro (average offsets when still)
MPU9250_Error_t MPU9250_CalibrateGyro(float *gyro_bias_out) {
    const int samples = 200;
    float sum[3] = {0};
    int16_t raw[3];

    for (int i = 0; i < samples; i++) {
        uint8_t buf[6];
        read_regs(0x43, buf, 6);  // Gyro raw
        raw[0] = (int16_t)((buf[0] << 8) | buf[1]);
        raw[1] = (int16_t)((buf[2] << 8) | buf[3]);
        raw[2] = (int16_t)((buf[4] << 8) | buf[5]);
        for (int j = 0; j < 3; j++) sum[j] += raw[j] * gyro_scale;
        HAL_Delay(5);
    }
    for (int j = 0; j < 3; j++) {
        gyro_bias[j] = sum[j] / samples;
        gyro_bias_out[j] = gyro_bias[j];
    }
    return MPU9250_OK;
}

// Calibrate accel (average offsets when level)
MPU9250_Error_t MPU9250_CalibrateAccel(float *accel_bias_out) {
    const int samples = 200;
    float sum[3] = {0};
    int16_t raw[3];

    for (int i = 0; i < samples; i++) {
        uint8_t buf[6];
        read_regs(0x3B, buf, 6);  // Accel raw
        raw[0] = (int16_t)((buf[0] << 8) | buf[1]);
        raw[1] = (int16_t)((buf[2] << 8) | buf[3]);
        raw[2] = (int16_t)((buf[4] << 8) | buf[5]);
        for (int j = 0; j < 3; j++) sum[j] += raw[j] * accel_scale;
        HAL_Delay(5);
    }
    for (int j = 0; j < 3; j++) {
        accel_bias[j] = sum[j] / samples;
        accel_bias_out[j] = accel_bias[j];
    }
    // Assume Z is gravity (9.81 m/s²)
    accel_bias[2] -= 9.81f;
    return MPU9250_OK;
}

// Load DMP firmware with checksum verification
MPU9250_Error_t MPU9250_LoadDMP(const uint8_t *firmware, uint16_t firmware_size) {
    uint16_t addr = 0;
    const uint16_t chunk_size = 16;

    while (addr < firmware_size) {
        uint16_t len = (firmware_size - addr > chunk_size) ? chunk_size : (firmware_size - addr);
        if (write_mem_chunk(addr, &firmware[addr], len) != MPU9250_OK) return MPU9250_ERR_DMP_LOAD;
        addr += len;
    }

    // Verify (simple checksum: sum all bytes)
    uint8_t verify_buf[chunk_size];
    uint32_t expected_sum = 0, actual_sum = 0;
    for (uint16_t i = 0; i < firmware_size; i++) expected_sum += firmware[i];
    addr = 0;
    while (addr < firmware_size) {
        uint16_t len = (firmware_size - addr > chunk_size) ? chunk_size : (firmware_size - addr);
        set_mem_bank(addr >> 8);
        set_mem_start(addr & 0xFF);
        read_regs(MPU9250_REG_MEM_R_W, verify_buf, len);
        for (uint16_t j = 0; j < len; j++) actual_sum += verify_buf[j];
        addr += len;
    }
    if (expected_sum != actual_sum) return MPU9250_ERR_DMP_LOAD;

    // Enable DMP
    if (write_reg(MPU9250_REG_USER_CTRL, 0x08) != MPU9250_OK) return MPU9250_ERR_SPI;  // DMP_EN
    if (write_reg(MPU9250_REG_FIFO_EN, 0x00) != MPU9250_OK) return MPU9250_ERR_SPI;    // Disable all FIFO
    flush_fifo();
    return MPU9250_OK;
}

// Get data with modes and filtering
MPU9250_Error_t MPU9250_GetData(MPU9250_Mode_t mode, MPU9250_Data_t *data) {
    uint8_t fifo_count_buf[2];
    if (read_regs(MPU9250_REG_FIFO_COUNT_H, fifo_count_buf, 2) != MPU9250_OK) return MPU9250_ERR_SPI;
    uint16_t fifo_count = (fifo_count_buf[0] << 8) | fifo_count_buf[1];
    if (fifo_count < 16) return MPU9250_ERR_FIFO_OVERFLOW;  // Wait for full quaternion packet

    uint8_t dmp_data[16];
    if (read_regs(MPU9250_REG_FIFO_R_W, dmp_data, 16) != MPU9250_OK) return MPU9250_ERR_SPI;

    // Parse quaternion (scaled to 2^30)
    MPU9250_Quat_t q;
    q.w = (int32_t)((dmp_data[0] << 24) | (dmp_data[1] << 16) | (dmp_data[2] << 8) | dmp_data[3]) / (float)(1 << 30);
    q.x = (int32_t)((dmp_data[4] << 24) | (dmp_data[5] << 16) | (dmp_data[6] << 8) | dmp_data[7]) / (float)(1 << 30);
    q.y = (int32_t)((dmp_data[8] << 24) | (dmp_data[9] << 16) | (dmp_data[10] << 8) | dmp_data[11]) / (float)(1 << 30);
    q.z = (int32_t)((dmp_data[12] << 24) | (dmp_data[13] << 16) | (dmp_data[14] << 8) | dmp_data[15]) / (float)(1 << 30);

    // Normalize quaternion
    float norm = sqrtf(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
    if (norm > 0) {
        q.w /= norm; q.x /= norm; q.y /= norm; q.z /= norm;
    }

    switch (mode) {
        case MPU9250_MODE_QUAT:
            data->quat = q;
            break;

        case MPU9250_MODE_EULER: {
            // Quaternion to Euler (Yaw, Pitch, Roll)
            // Math: $$ \phi = \atan2(2(q_w q_x + q_y q_z), 1 - 2(q_x^2 + q_y^2)) $$ (roll)
            // $$ \theta = \asin(2(q_w q_y - q_z q_x)) $$ (pitch)
            // $$ \psi = \atan2(2(q_w q_z + q_x q_y), 1 - 2(q_y^2 + q_z^2)) $$ (yaw)
            float sinr_cosp = 2 * (q.w * q.x + q.y * q.z);
            float cosr_cosp = 1 - 2 * (q.x * q.x + q.y * q.y);
            data->euler.roll = atan2f(sinr_cosp, cosr_cosp) * (180.0f / M_PI);

            float sinp = 2 * (q.w * q.y - q.z * q.x);
            data->euler.pitch = (fabsf(sinp) >= 1) ? copysignf(M_PI / 2, sinp) : asinf(sinp) * (180.0f / M_PI);

            float siny_cosp = 2 * (q.w * q.z + q.x * q.y);
            float cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
            data->euler.yaw = atan2f(siny_cosp, cosy_cosp) * (180.0f / M_PI);

            // Apply simple complementary filter (fuse with gyro for stability)
            static MPU9250_Euler_t prev_euler = {0};
            uint8_t gyro_buf[6];
            read_regs(0x43, gyro_buf, 6);
            float gyro[3] = {
                ((int16_t)((gyro_buf[0] << 8) | gyro_buf[1]) * gyro_scale) - gyro_bias[0],
                ((int16_t)((gyro_buf[2] << 8) | gyro_buf[3]) * gyro_scale) - gyro_bias[1],
                ((int16_t)((gyro_buf[4] << 8) | gyro_buf[5]) * gyro_scale) - gyro_bias[2]
            };
            float dt = 1.0f / sample_rate_hz;
            data->euler.roll = filter_alpha * (prev_euler.roll + gyro[0] * dt) + (1 - filter_alpha) * data->euler.roll;
            data->euler.pitch = filter_alpha * (prev_euler.pitch + gyro[1] * dt) + (1 - filter_alpha) * data->euler.pitch;
            data->euler.yaw = filter_alpha * (prev_euler.yaw + gyro[2] * dt) + (1 - filter_alpha) * data->euler.yaw;
            prev_euler = data->euler;
            break;
        }

        case MPU9250_MODE_LIN_ACCEL: {
            // Read raw accel
            uint8_t accel_buf[6];
            read_regs(0x3B, accel_buf, 6);
            float accel_raw[3] = {
                ((int16_t)((accel_buf[0] << 8) | accel_buf[1]) * accel_scale) - accel_bias[0],
                ((int16_t)((accel_buf[2] << 8) | accel_buf[3]) * accel_scale) - accel_bias[1],
                ((int16_t)((accel_buf[4] << 8) | accel_buf[5]) * accel_scale) - accel_bias[2]
            };

            // Gravity vector from quaternion: $$ g = [0, 0, -9.81] $$ rotated by q
            float gravity[3] = {
                2 * (q.x * q.z - q.w * q.y) * 9.81f,
                2 * (q.w * q.x + q.y * q.z) * 9.81f,
                (q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z) * 9.81f
            };

            // Subtract gravity for linear accel
            data->accel.x = accel_raw[0] - gravity[0];
            data->accel.y = accel_raw[1] - gravity[1];
            data->accel.z = accel_raw[2] - gravity[2];
            break;
        }
    }
    return MPU9250_OK;
}

// Placeholder for BMP280 fusion (e.g., correct altitude/pressure for navigation)
MPU9250_Error_t MPU9250_FuseBMP280(float pressure, float altitude) {
    // TODO: Implement Kalman or complementary filter to fuse with IMU data
    // Example: Adjust linear accel Z with altitude derivative
    return MPU9250_OK;
}
