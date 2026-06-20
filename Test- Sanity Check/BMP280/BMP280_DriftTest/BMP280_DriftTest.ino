// Last Edit: 2026-06-20
// Reason for Last Edit: Long-duration drift + altitude validation for BMP280
// Author: Alireza Sotoodeh

/*
 * =========================================================================
 * PROJECT: Hardware Sanity Check (BMP280 — Drift & Altitude Validation)
 * VERSION: Test.2
 * * WIRING DIAGRAM
 * -------------------------------------------------------------------------
 * Component        | Arduino Uno Pin    | Note / Reasoning
 * -------------------------------------------------------------------------
 * BMP280 VCC       | 3V3                | Sensor supply voltage
 * BMP280 GND       | GND                | Common ground
 * BMP280 SCL       | A5 (SCL)           | I2C Clock line
 * BMP280 SDA       | A4 (SDA)           | I2C Data line
 * BMP280 CSB       | 3V3                | High = I2C mode (not SPI)
 * BMP280 SDO       | GND                | Low = I2C address 0x76
 * * DESIGN NOTES:
 * - Logs temperature, pressure, and calculated altitude every second for 300 s.
 * - Reports min / max / mean / sigma for all three channels.
 * - Computes a trend slope from first 30 s vs last 30 s.
 * - Altitude check: compares measured mean altitude against a user-set reference.
 * - For ESP32-S3 integration: SCL -> GPIO 5, SDA -> GPIO 4.
 * =========================================================================
 */

#include <Wire.h>
#include <Adafruit_BMP280.h>

#define BMP280_ADDR 0x76
#define SERIAL_BAUD 115200
#define DURATION_S  300
#define INTERVAL_MS 1000

// Set this to your local altitude in metres for the altitude check.
// Find it via: https://www.latlong.net, Google Maps long-press,
// or the <ele> tag in your GPX logs.
#define REFERENCE_ALTITUDE_M 1741.0

// Sea-level pressure reference for altitude calculation.
// 1013.25 hPa is the standard (ISA) value. Adjust if you know local QNH.
#define SEALEVEL_PRESSURE_HPA 1013.25
#define P_OFFSET 800.0f
#define A_OFFSET 1600.0f

Adafruit_BMP280 bmp;

// Running statistics
static float tMin = 9999, tMax = -9999, tSum = 0, tSumSq = 0;
static float pMin = 999999, pMax = -999999, pSum = 0, pSumSq = 0;
static float aMin = 99999, aMax = -99999, aSum = 0, aSumSq = 0;

// First 30 s / last 30 s averages for trend computation
#define TREND_WINDOW 30
static float tFirstSum = 0, pFirstSum = 0;
static float tLastSum = 0, pLastSum = 0;

void setup() {
  Serial.begin(SERIAL_BAUD);
  while (!Serial) { ; }

  Serial.println(F("==============================================="));
  Serial.println(F(" BMP280 \x97 5-Minute Drift & Altitude Test"));
  Serial.println(F("==============================================="));

  Wire.begin();
  if (!bmp.begin(BMP280_ADDR)) {
    Serial.println(F("ERROR: BMP280 not found on I2C bus."));
    Serial.println(F("ACTION: Run BMP280_Diagnostic.ino first."));
    while (1) { delay(10); }
  }

  // Verify chip ID
  uint8_t chipId = 0;
  Wire.beginTransmission(BMP280_ADDR);
  Wire.write(0xD0);
  Wire.endTransmission(false);
  Wire.requestFrom(BMP280_ADDR, (uint8_t)1, (uint8_t)1);
  chipId = Wire.read();
  if (chipId != 0x58 && chipId != 0x60) {
    Serial.print(F("ERROR: Unexpected Chip ID 0x"));
    Serial.println(chipId, HEX);
    while (1) { delay(10); }
  }

  // Set oversampling to x4/x4, normal mode for stable readings
  Wire.beginTransmission(BMP280_ADDR);
  Wire.write(0xF4);
  Wire.write(0x6D);  // osrs_t=x4, osrs_p=x4, normal mode
  Wire.endTransmission();
  delay(100);

  Serial.println();
  Serial.println(F("Logging for 300 seconds at 1 Hz..."));
  Serial.println(F("Open Tools -> Serial Plotter for live graph."));
  Serial.println();
  Serial.println(F("time_s, temp_C, pressure_hPa, altitude_m"));
}

void loop() {
  static unsigned long startMs = 0;
  static unsigned long lastLogMs = 0;
  static int elapsed = 0;

  if (startMs == 0) {
    startMs = millis();
    lastLogMs = startMs;
  }

  unsigned long now = millis();
  if (now - lastLogMs < INTERVAL_MS) {
    return;
  }
  lastLogMs = now;
  elapsed++;

  // Read sensor
  float temp = bmp.readTemperature();
  float pres = bmp.readPressure() / 100.0F;
  float alt  = bmp.readAltitude(SEALEVEL_PRESSURE_HPA);

  // Print CSV line
  Serial.print(elapsed);
  Serial.print(F(", "));
  Serial.print(temp, 2);
  Serial.print(F(", "));
  Serial.print(pres, 2);
  Serial.print(F(", "));
  Serial.println(alt, 1);

  // Update statistics
  if (temp < tMin) tMin = temp;
  if (temp > tMax) tMax = temp;
  tSum += temp;  tSumSq += temp * temp;

  if (pres < pMin) pMin = pres;
  if (pres > pMax) pMax = pres;
  float pOff = pres - P_OFFSET; pSum += pOff; pSumSq += pOff * pOff;

  if (alt < aMin) aMin = alt;
  if (alt > aMax) aMax = alt;
  float aOff = alt - A_OFFSET; aSum += aOff; aSumSq += aOff * aOff;

  // Trend accumulation
  if (elapsed <= TREND_WINDOW) {
    tFirstSum += temp;
    pFirstSum += pres;
  }
  if (elapsed > DURATION_S - TREND_WINDOW) {
    tLastSum += temp;
    pLastSum += pres;
  }

  // === After 300 s, print summary ==================================
  if (elapsed >= DURATION_S) {
    int n = DURATION_S;
    float tMean = tSum / n;
    float pMeanOff = pSum / n;
    float pMean = P_OFFSET + pMeanOff;
    float aMeanOff = aSum / n;
    float aMean = A_OFFSET + aMeanOff;
    float tVar = tSumSq / n - tMean * tMean;
    float pVar = pSumSq / n - pMeanOff * pMeanOff;
    float aVar = aSumSq / n - aMeanOff * aMeanOff;
    float tSigma = sqrt(tVar < 0 ? 0.0f : tVar);
    float pSigma = sqrt(pVar < 0 ? 0.0f : pVar);
    float aSigma = sqrt(aVar < 0 ? 0.0f : aVar);

    // Trend slope (°C/min and hPa/min)
    float tSlope = (tLastSum / TREND_WINDOW - tFirstSum / TREND_WINDOW) / ((DURATION_S - TREND_WINDOW) / 60.0);
    float pSlope = (pLastSum / TREND_WINDOW - pFirstSum / TREND_WINDOW) / ((DURATION_S - TREND_WINDOW) / 60.0);

    Serial.println();
    Serial.println(F("==============================================="));
    Serial.println(F(" DRIFT TEST SUMMARY"));
    Serial.println(F("==============================================="));
    Serial.print(F("Elapsed: "));
    Serial.print(DURATION_S);
    Serial.println(F(" s"));
    Serial.println();

    Serial.print(F("Temperature [\xB0C]:  min = "));
    Serial.print(tMin, 2);
    Serial.print(F(",  max = "));
    Serial.print(tMax, 2);
    Serial.print(F(",  mean = "));
    Serial.print(tMean, 2);
    Serial.print(F(",  SD = "));
    Serial.print(tSigma, 3);
    Serial.println();

    Serial.print(F("Pressure [hPa]:    min = "));
    Serial.print(pMin, 1);
    Serial.print(F(",  max = "));
    Serial.print(pMax, 1);
    Serial.print(F(",  mean = "));
    Serial.print(pMean, 1);
    Serial.print(F(",  SD = "));
    Serial.print(pSigma, 3);
    Serial.println();

    Serial.print(F("Altitude [m]:      min = "));
    Serial.print(aMin, 1);
    Serial.print(F(",  max = "));
    Serial.print(aMax, 1);
    Serial.print(F(",  mean = "));
    Serial.print(aMean, 1);
    Serial.print(F(",  SD = "));
    Serial.print(aSigma, 2);
    Serial.println();

    Serial.println();
    Serial.print(F("Trend:  temp = "));
    Serial.print(tSlope, 4);
    Serial.print(F(" \xB0C/min"));
    if (fabs(tSlope) < 0.01) {
      Serial.print(F("  (stable)"));
    }
    Serial.println();
    Serial.print(F("Trend:  pressure = "));
    Serial.print(pSlope, 4);
    Serial.print(F(" hPa/min"));
    if (fabs(pSlope) < 0.1) {
      Serial.print(F("  (stable)"));
    }
    Serial.println();

    Serial.println();
    Serial.println(F("-----------------------------------------------"));
    Serial.println(F(" ALTITUDE CHECK"));
    Serial.print(F("  Measured mean altitude:  "));
    Serial.print(aMean, 1);
    Serial.println(F(" m"));
    Serial.print(F("  Reference altitude:      "));
    Serial.print(REFERENCE_ALTITUDE_M, 1);
    Serial.println(F(" m"));
    float delta = fabs(aMean - REFERENCE_ALTITUDE_M);
    Serial.print(F("  Delta:                    "));
    Serial.print(delta, 1);
    Serial.println(F(" m"));
    if (delta > 200.0) {
      Serial.println(F("  VERDICT: FAILED \x97 delta exceeds 200 m."));
      Serial.println(F("  ACTION: Verify REFERENCE_ALTITUDE_M is correct."));
      Serial.println(F("  ACTION: Check SEALEVEL_PRESSURE_HPA (local QNH)."));
    } else {
      Serial.println(F("  VERDICT: PASSED."));
    }
    Serial.println(F("==============================================="));
    Serial.println(F(" TEST COMPLETE."));
    Serial.println(F("==============================================="));

    // Stop logging
    while (1) { delay(1000); }
  }
}
