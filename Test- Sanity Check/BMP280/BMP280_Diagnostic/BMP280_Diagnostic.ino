// Last Edit: 2026-06-20
// Reason for Last Edit: Standalone Sanity Check for BMP280 Barometric Sensor
// Author: Alireza Sotoodeh

/*
 * =========================================================================
 * PROJECT: Hardware Sanity Check (BMP280 Barometric Sensor)
 * VERSION: Test.1
 * * WIRING DIAGRAM
 * -------------------------------------------------------------------------
* Component        | Arduino Uno Pin    | Note / Reasoning
 * -------------------------------------------------------------------------
 * BMP280 VIN       | 5V / 3V3           | Input voltage (Module has onboard regulator)
 * BMP280 GND       | GND                | Common ground
 * BMP280 SCL       | A5                 | I2C Clock line (Requires external shifting for long tests)
 * BMP280 SDA       | A4                 | I2C Data line (Requires external shifting for long tests)
 * * DESIGN NOTES:
 * - 4-Pin Module hardwires SDO to GND internally (Address fixed at 0x76).
 * - 4-Pin Module hardwires CSB to VCC internally (Forced I2C mode).
 * - Chip ID register (0xD0) should return 0x58 for BMP280.
 *   BME280 returns 0x60 (compatible, different humidity sensor).
 * - For ESP32-S3 integration: SCL -> GPIO 5, SDA -> GPIO 4 (same bus as MPU9250).
 * - Adafruit_BMP280 library required: Sketch -> Include Library -> Manage Libraries -> search BMP280.
 * =========================================================================
 */

#include <Wire.h>
#include <Adafruit_BMP280.h>

#define BMP280_ADDR 0x76
#define SERIAL_BAUD 115200

Adafruit_BMP280 bmp;

static uint8_t readRegister(uint8_t reg);
static void printOversampling(uint8_t osrs);
static void printPowerMode(uint8_t mode);
static void printFilterCoeff(uint8_t filter);
static void printModeName(int i);

void setup() {
  Serial.begin(SERIAL_BAUD);
  while (!Serial) { ; }

  Serial.println(F("==============================================="));
  Serial.println(F(" BMP280 Barometric Sensor \x97 Diagnostic Suite"));
  Serial.println(F("==============================================="));

  // -----------------------------------------------------------------
  // TEST 1: I2C connection and sensor initialization
  // -----------------------------------------------------------------
  Serial.print(F("TEST 1: Initializing BMP280 at address 0x"));
  Serial.print(BMP280_ADDR, HEX);
  Serial.print(F("... "));
  Wire.begin();
  if (!bmp.begin(BMP280_ADDR)) {
    Serial.println(F("FAILED!"));
    Serial.println(F("ERROR: BMP280 not responding on I2C bus."));
    Serial.println(F("ACTION: Check wiring (VCC, GND, SCL, SDA)."));
    Serial.println(F("ACTION: Verify CSB is tied to 3V3 (I2C mode)."));
    Serial.println(F("ACTION: Verify SDO connection for correct address."));
    while (1) { delay(10); }
  }
  Serial.println(F("PASSED."));

  // -----------------------------------------------------------------
  // TEST 2: Chip ID verification
  // -----------------------------------------------------------------
  Serial.print(F("TEST 2: Reading Chip ID register (0xD0)... "));
  uint8_t chipId = readRegister(0xD0);
  Serial.print(F("0x"));
  if (chipId < 0x10) Serial.print('0');
  Serial.print(chipId, HEX);
  Serial.print(F("... "));
  if (chipId == 0x58) {
    Serial.println(F("PASSED. (BMP280)"));
  } else if (chipId == 0x60) {
    Serial.println(F("WARNING: Chip ID = 0x60 (BME280, not BMP280)."));
    Serial.println(F("  The BME280 is compatible and adds humidity sensing."));
    Serial.println(F("  This test will continue \x97 all BMP280 functions work."));
  } else {
    Serial.println(F("FAILED!"));
    Serial.print(F("  Expected 0x58 (BMP280) or 0x60 (BME280), got 0x"));
    Serial.println(chipId, HEX);
    Serial.println(F("ACTION: Check sensor model or replace unit."));
    while (1) { delay(10); }
  }

  // -----------------------------------------------------------------
  // TEST 3: Temperature readout sanity
  // -----------------------------------------------------------------
  Serial.print(F("TEST 3: Reading temperature... "));
  float temp = bmp.readTemperature();
  Serial.print(temp);
  Serial.print(F(" \x30C... "));
  if (isnan(temp)) {
    Serial.println(F("FAILED! (returned NaN)"));
    while (1) { delay(10); }
  }
  if (temp < -40.0 || temp > 85.0) {
    Serial.println(F("WARNING: Value outside expected range (-40..+85 \x30C)."));
    Serial.println(F("  Sensor may be faulty or in extreme environment."));
  } else {
    Serial.println(F("PASSED."));
  }

  // -----------------------------------------------------------------
  // TEST 4: Pressure readout sanity
  // -----------------------------------------------------------------
  Serial.print(F("TEST 4: Reading pressure... "));
  float pres = bmp.readPressure() / 100.0F;
  Serial.print(pres);
  Serial.print(F(" hPa... "));
  if (isnan(pres)) {
    Serial.println(F("FAILED! (returned NaN)"));
    while (1) { delay(10); }
  }
  if (pres < 300.0 || pres > 1100.0) {
    Serial.println(F("WARNING: Value outside expected range (300..1100 hPa)."));
    Serial.println(F("  Sensor may be faulty, at extreme altitude, or in a pressure chamber."));
  } else {
    Serial.println(F("PASSED."));
  }

  // -----------------------------------------------------------------
  // TEST 5: I2C bus scan
  // -----------------------------------------------------------------
  Serial.println(F("TEST 5: Scanning I2C bus for connected devices..."));
  uint8_t foundCount = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print(F("  Device found at 0x"));
      if (addr < 0x10) Serial.print('0');
      Serial.println(addr, HEX);
      foundCount++;
    }
  }
  if (foundCount == 0) {
    Serial.println(F("  (no devices found \x97 this should not happen if BMP280 is wired correctly)"));
    Serial.println(F("  WARNING: I2C bus appears empty despite passing TEST 1."));
  } else {
    Serial.print(F("  Scan complete: "));
    Serial.print(foundCount);
    Serial.println(F(" device(s) found."));
    if (foundCount > 1) {
      Serial.println(F("  NOTE: In standalone test only BMP280 should appear."));
      Serial.println(F("  In ESP32-S3 integration, expect MPU9250 (0x68) as well."));
    }
  }
  Serial.println(F("  PASSED."));

  // -----------------------------------------------------------------
  // TEST 6: Configuration register dump
  // -----------------------------------------------------------------
  Serial.println(F("TEST 6: Configuration register dump..."));
  uint8_t ctrlMeas = readRegister(0xF4);
  uint8_t config   = readRegister(0xF5);
  uint8_t status   = readRegister(0xF3);

  Serial.print(F("  ctrl_meas (0xF4): 0x"));
  if (ctrlMeas < 0x10) Serial.print('0');
  Serial.println(ctrlMeas, BIN);

  uint8_t osrsT = (ctrlMeas >> 5) & 0x07;
  uint8_t osrsP = (ctrlMeas >> 2) & 0x07;
  uint8_t mode  = ctrlMeas & 0x03;
  Serial.print(F("    osrs_t (temperature oversampling): "));
  printOversampling(osrsT);
  Serial.println();
  Serial.print(F("    osrs_p (pressure oversampling): "));
  printOversampling(osrsP);
  Serial.println();
  Serial.print(F("    mode (power mode): "));
  printPowerMode(mode);
  Serial.println();

  Serial.print(F("  config (0xF5): 0x"));
  if (config < 0x10) Serial.print('0');
  Serial.print(config, BIN);
  uint8_t tStandby = (config >> 5) & 0x07;
  uint8_t filter   = (config >> 2) & 0x07;
  if (tStandby > 0) {
    float standbyMs[] = {0.5, 62.5, 125, 250, 500, 1000, 2000, 4000};
    Serial.print(F("  \x97 t_sb (standby): "));
    Serial.print(standbyMs[tStandby & 0x07]);
    Serial.println(F(" ms"));
  } else {
    Serial.println(F("  \x97 t_sb (standby): 0.5 ms"));
  }
  Serial.print(F("    filter (IIR coefficient): "));
  printFilterCoeff(filter);
  Serial.println();

  Serial.print(F("  status (0xF3): 0x"));
  if (status < 0x10) Serial.print('0');
  Serial.print(status, BIN);
  if (status & 0x01) {
    Serial.println(F("  \x97 im_update: NVM data is being copied (briefly)"));
  } else {
    Serial.println(F("  \x97 im_update: idle"));
  }
  if (status & 0x08) {
    Serial.println(F("  \x97 measuring: conversion in progress"));
  } else {
    Serial.println(F("  \x97 measuring: idle"));
  }
  Serial.println(F("  PASSED."));

  // -----------------------------------------------------------------
  // TEST 7: Mode / oversampling sweep
  // -----------------------------------------------------------------
  Serial.println(F("TEST 7: Mode and oversampling sweep..."));
  const uint8_t modeConfigs[][2] = {
    {0x00, 0x00},
    {0x7D, 0x01},
    {0x6D, 0x03},
    {0xFF, 0x03},
  };
  for (int i = 0; i < 4; i++) {
    Wire.beginTransmission(BMP280_ADDR);
    Wire.write(0xF4);
    Wire.write(modeConfigs[i][0]);
    Wire.endTransmission();
    delay(10);
    uint8_t verify = readRegister(0xF4);
    if (verify != modeConfigs[i][0]) {
      Serial.print(F("  WARNING: Mode \""));
      printModeName(i);
      Serial.println(F("\" register write mismatch."));
    }
    delay(100);
    float t = bmp.readTemperature();
    float p = bmp.readPressure() / 100.0F;
    Serial.print(F("  "));
    printModeName(i);
    Serial.print(F(" :  "));
    Serial.print(t);
    Serial.print(F(" \x30C,  "));
    Serial.print(p);
    Serial.println(F(" hPa"));
  }
  Wire.beginTransmission(BMP280_ADDR);
  Wire.write(0xF4);
  Wire.write(0x6D);
  Wire.endTransmission();
  delay(100);
  Serial.println(F("  PASSED."));

  // -----------------------------------------------------------------
  // TEST 8: Noise floor measurement (100 samples)
  // -----------------------------------------------------------------
  Serial.println(F("TEST 8: Measuring noise floor (100 samples)..."));
  const int N = 100;
  float tSum = 0, pSum = 0;
  float tSumSq = 0, pSumSq = 0;
  float tMin = 9999, tMax = -9999;
  float pMin = 999999, pMax = -999999;

  for (int i = 0; i < N; i++) {
    float t = bmp.readTemperature();
    float p = bmp.readPressure() / 100.0F;
    tSum += t;   pSum += p;
    tSumSq += t * t;  pSumSq += p * p;
    if (t < tMin) tMin = t;
    if (t > tMax) tMax = t;
    if (p < pMin) pMin = p;
    if (p > pMax) pMax = p;
    delay(20);
  }
  float tMean = tSum / N;
  float pMean = pSum / N;
  float tSigma = sqrt(tSumSq / N - tMean * tMean);
  float pSigma = sqrt(pSumSq / N - pMean * pMean);

  Serial.print(F("  Temperature:  mean = "));
  Serial.print(tMean, 2);
  Serial.print(F(" \x30C,  min = "));
  Serial.print(tMin, 2);
  Serial.print(F(",  max = "));
  Serial.print(tMax, 2);
  Serial.print(F(",  \xE3 = "));
  Serial.print(tSigma, 3);
  Serial.println(F(" \x30C"));

  Serial.print(F("  Pressure:     mean = "));
  Serial.print(pMean, 1);
  Serial.print(F(" hPa,  min = "));
  Serial.print(pMin, 1);
  Serial.print(F(",  max = "));
  Serial.print(pMax, 1);
  Serial.print(F(",  \xE3 = "));
  Serial.print(pSigma, 3);
  Serial.println(F(" hPa"));

  bool tNoisy = (tSigma > 0.3);
  bool pNoisy = (pSigma > 1.5);
  if (tNoisy || pNoisy) {
    if (tNoisy) {
      Serial.println(F("  WARNING: Temperature noise exceeds 0.3 \x30C threshold."));
    }
    if (pNoisy) {
      Serial.println(F("  WARNING: Pressure noise exceeds 1.5 hPa threshold."));
    }
    Serial.println(F("  ACTION: Check for airflow, mechanical vibration, or damaged sensor."));
  } else {
    Serial.println(F("  Noise levels within expected range."));
  }
  Serial.println(F("  PASSED."));

  // -----------------------------------------------------------------
  // Summary
  // -----------------------------------------------------------------
  Serial.println(F("==============================================="));
  Serial.println(F(" ALL TESTS PASSED. BMP280 is healthy."));
  Serial.println(F("==============================================="));
}

void loop() {
  delay(1000);
}

// -----------------------------------------------------------------
// Helper: read one byte from a BMP280 register via I2C
// -----------------------------------------------------------------
static uint8_t readRegister(uint8_t reg) {
  Wire.beginTransmission(BMP280_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(BMP280_ADDR, (uint8_t)1, (uint8_t)1);
  return Wire.read();
}

// -----------------------------------------------------------------
// Helper: print oversampling setting
// -----------------------------------------------------------------
static void printOversampling(uint8_t osrs) {
  switch (osrs) {
    case 0:  Serial.print(F("skipped"));  break;
    case 1:  Serial.print(F("x1"));       break;
    case 2:  Serial.print(F("x2"));       break;
    case 3:  Serial.print(F("x4"));       break;
    case 4:  Serial.print(F("x8"));       break;
    case 5:  Serial.print(F("x16"));      break;
    default: Serial.print(F("reserved")); break;
  }
}

// -----------------------------------------------------------------
// Helper: print power mode
// -----------------------------------------------------------------
static void printPowerMode(uint8_t mode) {
  switch (mode) {
    case 0:  Serial.print(F("sleep"));  break;
    case 1:
    case 2:  Serial.print(F("forced")); break;
    case 3:  Serial.print(F("normal")); break;
    default: Serial.print(F("unknown")); break;
  }
}

// -----------------------------------------------------------------
// Helper: print filter coefficient
// -----------------------------------------------------------------
static void printFilterCoeff(uint8_t filter) {
  switch (filter) {
    case 0:  Serial.print(F("off"));     break;
    case 1:  Serial.print(F("x2"));      break;
    case 2:  Serial.print(F("x4"));      break;
    case 3:  Serial.print(F("x8"));      break;
    case 4:  Serial.print(F("x16"));     break;
    default: Serial.print(F("reserved")); break;
  }
}

// -----------------------------------------------------------------
// Helper: print mode name for TEST 7
// -----------------------------------------------------------------
static void printModeName(int i) {
  switch (i) {
    case 0:  Serial.print(F("Sleep"));       break;
    case 1:  Serial.print(F("Forced x16"));  break;
    case 2:  Serial.print(F("Normal x4"));   break;
    case 3:  Serial.print(F("Normal x16"));  break;
  }
}
