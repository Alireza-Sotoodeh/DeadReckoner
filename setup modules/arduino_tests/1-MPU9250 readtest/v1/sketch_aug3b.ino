#include <Wire.h>

#define MPU9250_ADDRESS 0x68
#define AK8963_ADDRESS 0x0C
#define AK8963_WHO_AM_I 0x00

#define MPU_INT_PIN_CFG 0x37
#define MPU_PWR_MGMT_1 0x6B
#define MPU_ACCEL_XOUT_H 0x3B
#define MPU_GYRO_XOUT_H 0x43
#define AK8963_ST1 0x02
#define AK8963_HXL 0x03
#define AK8963_CNTL 0x0A
#define AK8963_ASAX 0x10

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("MPU9250 9-Axis Test");

  Wire.begin();

  // Wake MPU and enable I2C bypass
  writeByte(MPU9250_ADDRESS, MPU_PWR_MGMT_1, 0x00);
  writeByte(MPU9250_ADDRESS, MPU_INT_PIN_CFG, 0x02);  // Bypass enable

  // Test main chip WHO_AM_I
  uint8_t mpu_id = readByte(MPU9250_ADDRESS, 0x75);
  Serial.print("MPU WHO_AM_I: 0x"); Serial.println(mpu_id, HEX);

  // Init and test AK8963
  writeByte(AK8963_ADDRESS, AK8963_CNTL, 0x00);  // Power down mag
  delay(10);
  writeByte(AK8963_ADDRESS, AK8963_CNTL, 0x0F);  // Fuse ROM access mode
  delay(10);

  // Read sensitivity adjustment values (for calibration)
  uint8_t rawData[3];
  readBytes(AK8963_ADDRESS, AK8963_ASAX, 3, rawData);
  float mag_cal_x = (float)(rawData[0] - 128) / 256.0f + 1.0f;
  float mag_cal_y = (float)(rawData[1] - 128) / 256.0f + 1.0f;
  float mag_cal_z = (float)(rawData[2] - 128) / 256.0f + 1.0f;
  Serial.print("Mag Calibration: "); Serial.print(mag_cal_x); Serial.print(", "); Serial.print(mag_cal_y); Serial.print(", "); Serial.println(mag_cal_z);

  // Back to power down and then continuous mode
  writeByte(AK8963_ADDRESS, AK8963_CNTL, 0x00);
  delay(10);
  writeByte(AK8963_ADDRESS, AK8963_CNTL, 0x16);  // Continuous mode 2 (100Hz)
  delay(10);

  // Test AK8963 WHO_AM_I
  uint8_t ak_id = readByte(AK8963_ADDRESS, AK8963_WHO_AM_I);
  Serial.print("AK8963 WHO_AM_I: 0x"); Serial.println(ak_id, HEX);

  if (ak_id != 0x48) {
    Serial.println("AK8963 not detected!");
    while (1);
  }
  Serial.println("Setup complete. Reading data...");
}

void loop() {
  // Read accel
  int16_t accel[3];
  readAccel(accel);
  Serial.print("Accel: "); Serial.print(accel[0]); Serial.print(", "); Serial.print(accel[1]); Serial.print(", "); Serial.println(accel[2]);

  // Read gyro
  int16_t gyro[3];
  readGyro(gyro);
  Serial.print("Gyro: "); Serial.print(gyro[0]); Serial.print(", "); Serial.print(gyro[1]); Serial.print(", "); Serial.println(gyro[2]);

  // Read mag (with readiness check)
  if (readByte(AK8963_ADDRESS, AK8963_ST1) & 0x01) {
    uint8_t mag_data[7];
    readBytes(AK8963_ADDRESS, AK8963_HXL, 7, mag_data);
    int16_t mx = (int16_t)(((uint16_t)mag_data[1] << 8) | mag_data[0]);
    int16_t my = (int16_t)(((uint16_t)mag_data[3] << 8) | mag_data[2]);
    int16_t mz = (int16_t)(((uint16_t)mag_data[5] << 8) | mag_data[4]);
    Serial.print("Mag: "); Serial.print(mx); Serial.print(", "); Serial.print(my); Serial.print(", "); Serial.println(mz);
  } else {
    Serial.println("Mag: Not ready");
  }

  delay(500);
}

// Helper functions
void writeByte(uint8_t address, uint8_t subAddress, uint8_t data) {
  Wire.beginTransmission(address);
  Wire.write(subAddress);
  Wire.write(data);
  Wire.endTransmission();
}

uint8_t readByte(uint8_t address, uint8_t subAddress) {
  Wire.beginTransmission(address);
  Wire.write(subAddress);
  Wire.endTransmission();
  Wire.requestFrom(address, (uint8_t)1);
  return Wire.read();
}

void readBytes(uint8_t address, uint8_t subAddress, uint8_t count, uint8_t *dest) {
  Wire.beginTransmission(address);
  Wire.write(subAddress);
  Wire.endTransmission();
  Wire.requestFrom(address, count);
  for (uint8_t i = 0; i < count; i++) dest[i] = Wire.read();
}

void readAccel(int16_t *dest) {
  uint8_t raw[6];
  readBytes(MPU9250_ADDRESS, MPU_ACCEL_XOUT_H, 6, raw);
  dest[0] = (int16_t)((raw[0] << 8) | raw[1]);
  dest[1] = (int16_t)((raw[2] << 8) | raw[3]);
  dest[2] = (int16_t)((raw[4] << 8) | raw[5]);
}

void readGyro(int16_t *dest) {
  uint8_t raw[6];
  readBytes(MPU9250_ADDRESS, MPU_GYRO_XOUT_H, 6, raw);
  dest[0] = (int16_t)((raw[0] << 8) | raw[1]);
  dest[1] = (int16_t)((raw[2] << 8) | raw[3]);
  dest[2] = (int16_t)((raw[4] << 8) | raw[5]);
}
