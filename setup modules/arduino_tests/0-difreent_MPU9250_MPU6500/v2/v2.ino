#include <Wire.h>

#define MPU_ADDRESS 0x68      // MPU chip address
#define MPU_WHO_AM_I_REG 0x75 // WHO_AM_I register for MPU

#define AK8963_ADDRESS 0x0C   // AK8963 magnetometer address
#define AK8963_WHO_AM_I_REG 0x00 // WHO_AM_I register for AK8963

#define MPU_INT_PIN_CFG 0x37  // Register to enable I2C bypass
#define MPU_PWR_MGMT_1 0x6B   // Power management register

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("IMU Module Model Detector");

  Wire.begin();

  // Wake up the MPU
  writeByte(MPU_ADDRESS, MPU_PWR_MGMT_1, 0x00);
  delay(100);

  // Read MPU WHO_AM_I
  uint8_t mpu_who_am_i = readByte(MPU_ADDRESS, MPU_WHO_AM_I_REG);
  Serial.print("MPU WHO_AM_I: 0x");
  Serial.println(mpu_who_am_i, HEX);

  // Enable I2C bypass mode to access external magnetometer directly
  writeByte(MPU_ADDRESS, MPU_INT_PIN_CFG, 0x02);
  delay(100);

  // Read AK8963 WHO_AM_I (if present)
  uint8_t ak_who_am_i = readByte(AK8963_ADDRESS, AK8963_WHO_AM_I_REG);
  Serial.print("AK8963 WHO_AM_I: 0x");
  Serial.println(ak_who_am_i, HEX);

  // Perform I2C scan to confirm detected devices
  Serial.println("I2C Scan Results:");
  for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("Device found at 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  Serial.println("I2C Scan Complete.");

  // Interpret the results to determine the module model
  Serial.println("Model Interpretation:");
  if (mpu_who_am_i == 0x71 || mpu_who_am_i == 0x73) {
    if (ak_who_am_i == 0x48) {
      Serial.println("This is likely a genuine MPU9250 (9-axis IMU with integrated AK8963 magnetometer).");
      Serial.println(" - MPU9250 WHO_AM_I is 0x71 or 0x73 (standard for authentic chips).");
      Serial.println(" - AK8963 magnetometer is detected and functional.");
    } else {
      Serial.println("This might be an MPU9250 without functional magnetometer (or a defective unit).");
      Serial.println(" - Consider it as 6-axis only (accel + gyro).");
    }
  } else if (mpu_who_am_i == 0x70) {
    if (ak_who_am_i == 0x48) {
      Serial.println("This is likely a GY-9250 or similar clone module (MPU6500 + external AK8963).");
      Serial.println(" - MPU6500 (6-axis) WHO_AM_I is 0x70.");
      Serial.println(" - Separate AK8963 magnetometer is detected at 0x0C, making it 9-axis capable.");
      Serial.println(" - Common in affordable breakout boards; works like MPU9250 but may have slight differences in accuracy.");
    } else {
      Serial.println("This is likely an MPU6500 (6-axis only, no magnetometer).");
      Serial.println(" - MPU WHO_AM_I is 0x70, but no AK8963 detected.");
    }
  } else if (mpu_who_am_i == 0x68) {
    Serial.println("This might be an MPU6050 (6-axis IMU, older model).");
    Serial.println(" - No magnetometer expected.");
  } else {
    Serial.println("Unknown or unsupported module.");
    Serial.print(" - MPU WHO_AM_I: 0x"); Serial.println(mpu_who_am_i, HEX);
    Serial.print(" - AK8963 WHO_AM_I: 0x"); Serial.println(ak_who_am_i, HEX);
    Serial.println(" - Check wiring or try a different module.");
  }

  Serial.println("If the magnetometer is detected (AK8963 WHO_AM_I = 0x48), you can use it for 9DOF applications.");
  Serial.println("For full confirmation, test data readings in a separate sketch.");
}

void loop() {
  // No loop needed; runs once
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
  if (Wire.available()) {
    return Wire.read();
  } else {
    return 0xFF; // Error value
  }
}