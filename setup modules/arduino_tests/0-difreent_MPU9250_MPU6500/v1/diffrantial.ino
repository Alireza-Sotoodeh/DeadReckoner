#include <Wire.h>

#define MPU9250_ADDRESS 0x68

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("I2C Scanner with MPU Bypass");

  Wire.begin();

  // Wake up MPU and enable bypass mode
  writeByte(MPU9250_ADDRESS, 0x6B, 0x00);  // PWR_MGMT_1
  writeByte(MPU9250_ADDRESS, 0x37, 0x02);  // INT_PIN_CFG for bypass

  // Now scan
  byte error, address;
  int nDevices = 0;
  Serial.println("Scanning...");
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("Device found at 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      nDevices++;
    }
  }
  if (nDevices == 0) Serial.println("No I2C devices found");
  else Serial.println("Scan complete");
}

void loop() {}

void writeByte(byte address, byte subAddress, byte data) {
  Wire.beginTransmission(address);
  Wire.write(subAddress);
  Wire.write(data);
  Wire.endTransmission();
}
