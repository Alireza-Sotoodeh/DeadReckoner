// Last Edit: 2026-06-20
// Reason for Last Edit: Cold/Warm/Hot start timing test for GY-GPS6Mv2
// Author: Alireza Sotoodeh

/*
 * =========================================================================
 * PROJECT: NEO-6M (GY-GPS6Mv2) — Cold Start Timing & Fix Reliability
 * MCU: Arduino Uno (ATmega328P)
 * =========================================================================
 * WIRING DIAGRAM
 * -------------------------------------------------------------------------
 * GY-GPS6Mv2 Pin | Arduino Uno Pin | Note
 * -------------------------------------------------------------------------
 * VCC            | 5V             | GPS module supply
 * GND            | GND            | Common ground
 * TX             | Pin 2          | Uno RX (GPS → Arduino)
 * RX             | Pin 3          | Uno TX (Arduino → GPS, optional)
 * =========================================================================
 * TEST SEQUENCE:
 *   1. Cold start — power on from full power-off, measure time to first 3D fix
 *   2. Stabilisation — 120 s of 3D fix logging (lat/lon/alt/HDOP/sats)
 *   3. Wait for user to briefly power-cycle GPS (VCC disconnect ~3 s),
 *      then measure re-acquisition time (hot or warm start).
 *   4. Final summary with all statistics.
 * =========================================================================
 */

#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>

#define GPS_RX_PIN 2
#define GPS_TX_PIN 3
#define GPS_BAUDRATE 9600
#define STABILISE_S 120
#define STALE_AGE_MS 2000

TinyGPSPlus gps;
SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);

// Cold start timing
static unsigned long startMs = 0;
static unsigned long ttffMs = 0;

// Stabilisation stats
static unsigned long stabEndMs = 0;
static unsigned long fixCount = 0;
static unsigned long noFixMs = 0;
static unsigned long noFixStart = 0;
static bool hadFix = false;

static long hdopMin = 999, hdopMax = 0, hdopSum = 0, hdopCount = 0;
static int satMin = 99, satMax = 0, satSum = 0, satCount = 0;
static float latSum = 0, lonSum = 0, altSum = 0;
static float latMin = 90, latMax = -90, lonMin = 180, lonMax = -180;
static float altMin = 99999, altMax = -99999;
static int latCount = 0, lonCount = 0, altCount = 0;

// Re-acquisition (hot/warm start)
static bool reacqPhase = false;
static unsigned long reacqStart = 0;
static unsigned long reacqFixMs = 0;
static unsigned long reacqWaitStart = 0;
static bool reacqWaitPrinted = false;

static bool waitPrinted = false;

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println(F("=============================================="));
  Serial.println(F(" NEO-6M — Cold Start Timing & Fix Reliability"));
  Serial.println(F("=============================================="));
  Serial.println();

  gpsSerial.begin(GPS_BAUDRATE);

  Serial.println(F("Phase 1: COLD START — waiting for first 3D fix..."));
  Serial.println(F("(Ensure antenna has clear sky view)"));
  Serial.println();
  startMs = millis();
}

void loop() {
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // Cold start TTFF
  if (ttffMs == 0 && gps.location.isValid() && gps.location.age() < 500
      && gps.altitude.isValid()) {
    ttffMs = millis() - startMs;
    Serial.println();
    Serial.print(F(">>> COLD START TTFF: "));
    Serial.print(ttffMs / 1000);
    Serial.println(F(" sec"));
    Serial.println();

    // Begin stabilisation phase
    stabEndMs = millis() + STABILISE_S * 1000UL;
    Serial.println(F("Phase 2: Stabilisation — logging for 120 s..."));
    Serial.println(F("SATS  HDOP  LAT          LNG           ALT   AGE"));
    Serial.println(F("----  ----  -----------  -----------  -----  ---"));
  }

  // Track fix/no-fix time
  bool hasFix = gps.location.isValid() && gps.location.age() < STALE_AGE_MS;
  if (ttffMs > 0 && !reacqPhase) {
    if (hasFix) {
      if (!hadFix) hadFix = true;
      if (noFixStart > 0) {
        noFixMs += millis() - noFixStart;
        noFixStart = 0;
      }
    } else if (hadFix && noFixStart == 0) {
      noFixStart = millis();
    }
  }

  // Re-acquisition phase
  if (reacqPhase) {
    if (!reacqWaitPrinted && reacqWaitStart > 0 && millis() - reacqWaitStart > 5000) {
      reacqWaitPrinted = true;
      Serial.println(F("Reconnecting GPS... waiting for fix."));
    }
    if (reacqFixMs == 0 && hasFix) {
      reacqFixMs = millis() - reacqStart;
      Serial.println();
      Serial.print(F(">>> RE-ACQUISITION TTFF: "));
      Serial.print(reacqFixMs / 1000);
      Serial.println(F(" sec"));
      Serial.println();
      printSummary();
      while (1) { delay(1000); }
    }
    return;
  }

  // Log fresh fix data during stabilisation
  if (ttffMs > 0 && gps.location.isUpdated() && !reacqPhase) {
    fixCount++;
    int sats = gps.satellites.isValid() ? gps.satellites.value() : 0;
    if (sats < satMin) satMin = sats;
    if (sats > satMax) satMax = sats;
    satSum += sats; satCount++;

    long hdop = gps.hdop.isValid() ? gps.hdop.value() : 999;
    if (hdop < hdopMin) hdopMin = hdop;
    if (hdop > hdopMax) hdopMax = hdop;
    hdopSum += hdop; hdopCount++;

    float lat = gps.location.lat();
    float lon = gps.location.lng();
    latSum += lat; lonSum += lon; latCount++; lonCount++;
    if (lat < latMin) latMin = lat;
    if (lat > latMax) latMax = lat;
    if (lon < lonMin) lonMin = lon;
    if (lon > lonMax) lonMax = lon;

    if (gps.altitude.isValid()) {
      float a = gps.altitude.meters();
      altSum += a; altCount++;
      if (a < altMin) altMin = a;
      if (a > altMax) altMax = a;
    }

    // Print log line
    if (sats < 10) Serial.print(' ');
    Serial.print(sats); Serial.print(F("  "));
    if (hdop < 100) Serial.print(' ');
    if (hdop < 10) Serial.print(' ');
    Serial.print(hdop); Serial.print(F("  "));
    Serial.print(lat, 6); Serial.print(F("  "));
    Serial.print(lon, 6); Serial.print(F("  "));
    if (gps.altitude.isValid()) {
      Serial.print(gps.altitude.meters(), 1);
    } else {
      Serial.print(F("N/A"));
    }
    Serial.print(F("  "));
    Serial.print(gps.location.age());
    Serial.println(F("ms"));
  }

  // Stabilisation done — enter re-acquisition phase
  if (ttffMs > 0 && !reacqPhase && millis() > stabEndMs) {
    reacqPhase = true;
    Serial.println();
    Serial.println(F("=============================================="));
    Serial.println(F("Phase 3: RE-ACQUISITION TEST"));
    Serial.println(F("Power-cycle GPS (disconnect VCC for ~3 s, reconnect)."));
    Serial.println(F("Measuring time to re-acquire 3D fix..."));
    Serial.println(F("=============================================="));
    Serial.println();
    reacqWaitStart = millis();
    reacqStart = millis();
  }
}

static void printSummary() {
  unsigned long elapsed = (millis() - startMs) / 1000;
  unsigned long stabElapsed = STABILISE_S;

  Serial.println();
  Serial.println(F("=============================================="));
  Serial.println(F(" GPS START TEST — FINAL SUMMARY"));
  Serial.println(F("=============================================="));

  Serial.print(F("Cold start TTFF:        "));
  Serial.print(ttffMs / 1000);
  Serial.println(F(" sec"));

  Serial.print(F("Re-acquisition TTFF:    "));
  Serial.print(reacqFixMs / 1000);
  Serial.println(F(" sec"));

  Serial.println();
  Serial.print(F("Fix count:              "));
  Serial.println(fixCount);
  Serial.print(F("Fix uptime:             "));
  if (stabElapsed > 0) {
    Serial.print(100 - (noFixMs / 10) / stabElapsed);
  } else {
    Serial.print(F("N/A"));
  }
  Serial.println(F("%"));

  Serial.println();
  Serial.print(F("Satellites  min="));
  Serial.print(satMin); Serial.print(F("  avg="));
  Serial.print(satCount > 0 ? (float)satSum / satCount : 0, 1);
  Serial.print(F("  max="));
  Serial.println(satMax);

  Serial.print(F("HDOP        min="));
  Serial.print(hdopMin); Serial.print(F("  avg="));
  Serial.print(hdopCount > 0 ? (float)hdopSum / hdopCount : 0, 1);
  Serial.print(F("  max="));
  Serial.println(hdopMax);

  if (latCount > 0) {
    float latMean = latSum / latCount;
    float lonMean = lonSum / lonCount;
    float latSigma = 0, lonSigma = 0;

    // Estimate σ from min/max range
    latSigma = (latMax - latMin) / 6.0f;   // rough 6σ range
    lonSigma = (lonMax - lonMin) / 6.0f;

    // Latitude → meters (111320 m/°)
    float latSigmaM = latSigma * 111320.0f;
    float lonSigmaM = lonSigma * 111320.0f * cos(latMean * 3.14159f / 180.0f);

    Serial.println();
    Serial.println(F("Position stability (2 min stationary):"));
    Serial.print(F("  Lat:  "));
    Serial.print(latMean, 6);
    Serial.print(F("  ± "));
    Serial.print(latSigmaM, 2);
    Serial.println(F(" m"));
    Serial.print(F("  Lon:  "));
    Serial.print(lonMean, 6);
    Serial.print(F("  ± "));
    Serial.print(lonSigmaM, 2);
    Serial.println(F(" m"));
  }

  if (altCount > 0) {
    float aMean = altSum / altCount;
    Serial.print(F("  Alt:  "));
    Serial.print(aMean, 1);
    Serial.print(F(" m  [" ));
    Serial.print(altMin, 1);
    Serial.print(F(" – "));
    Serial.print(altMax, 1);
    Serial.println(F(" m]"));
  }

  Serial.println(F("=============================================="));
  Serial.println(F(" TEST COMPLETE."));
  Serial.println(F("=============================================="));
}
