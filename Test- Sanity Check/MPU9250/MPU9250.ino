// Last Edit: 2026-06-04
// Reason for Last Edit: Basic I2C sanity check and WHO_AM_I verification for MPU9250 on Arduino Nano
// Author: Alireza Sotoodeh

/*
 * =========================================================================
 * PROJECT: Hardware Sanity Check (MPU9250)
 * VERSION: Test.1
 * * WIRING DIAGRAM
 * -------------------------------------------------------------------------
 * Component        | Arduino Nano Pin      | Note / Reasoning
 * -------------------------------------------------------------------------
 * MPU9250 VCC      | 3V3                   | CRITICAL: MPU9250 logic is 3.3V.
 * MPU9250 GND      | GND                   | Common ground
 * MPU9250 SCL      | A5                    | Hardware I2C Clock for Arduino Nano
 * MPU9250 SDA      | A4                    | Hardware I2C Data for Arduino Nano
 * MPU9250 AD0      | GND                   | Sets I2C Address to 0x68
 * =========================================================================
 */

#include <Wire.h>

#define MPU_ADDR 0x68         // I2C address of the MPU-9250
#define WHO_AM_I_REG 0x75     // Register that contains the device ID
#define PWR_MGMT_1_REG 0x6B   // Power management register
#define ACCEL_XOUT_H 0x3B     // First register for Accelerometer data

void setup() {
  Serial.begin(115200);
  Wire.begin();

  Serial.println("=================================");
  Serial.println("Starting MPU9250 Health Check...");
  Serial.println("=================================");

  // 1. Check I2C Connection and read WHO_AM_I register
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(WHO_AM_I_REG);
  
  // End transmission but keep the connection active
  if (Wire.endTransmission(false) != 0) {
    Serial.println("STATUS: ERROR! MPU9250 not found on I2C bus.");
    Serial.println("ACTION: Check wiring (SDA to A4, SCL to A5) and power.");
    while (1); // Halt execution if device is not found
  }

  // Request 1 byte from the WHO_AM_I register
  Wire.requestFrom(MPU_ADDR, 1, true);

  if (Wire.available()) {
    uint8_t whoAmI = Wire.read();
    Serial.print("WHO_AM_I Register Value: 0x");
    Serial.println(whoAmI, HEX);

    // MPU9250 typically returns 0x71 or 0x73
    if (whoAmI == 0x71 || whoAmI == 0x73) {
      Serial.println("STATUS: MPU9250 is ONLINE and HEALTHY.");
    } else {
      Serial.println("STATUS: WARNING! Device found, but ID does not match MPU9250.");
      Serial.println("ACTION: It might be a fake chip or an MPU6050 (0x68).");
    }
  }

  // 2. Wake up the sensor (clear sleep mode bit in power management register)
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(PWR_MGMT_1_REG);
  Wire.write(0x00); 
  Wire.endTransmission(true);
  
  Serial.println("Sensor awakened. Reading raw data...");
  delay(1000);
}

void loop() {
  // 3. Read raw acceleration data (6 bytes: X_H, X_L, Y_H, Y_L, Z_H, Z_L)
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(ACCEL_XOUT_H); 
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true); 

  if (Wire.available() == 6) {
    // Bitwise shift and bitwise OR to combine high and low bytes
    int16_t accX = Wire.read() << 8 | Wire.read();
    int16_t accY = Wire.read() << 8 | Wire.read();
    int16_t accZ = Wire.read() << 8 | Wire.read();

    Serial.print("Raw Accel -> X: "); Serial.print(accX);
    Serial.print(" | Y: "); Serial.print(accY);
    Serial.print(" | Z: "); Serial.println(accZ);
  } else {
    Serial.println("Error reading data.");
  }

  delay(500); // Wait half a second before next read
}