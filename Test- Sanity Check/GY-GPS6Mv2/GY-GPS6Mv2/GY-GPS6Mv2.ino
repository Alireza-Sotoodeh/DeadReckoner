// Last Edit: 2026-06-20
// Reason for Last Edit: Diagnostic live monitor + cold start + HDOP + heartbeat + summary
// Author: Alireza Sotoodeh

/*
 * =========================================================================
 * PROJECT: NEO-6M (GY-GPS6Mv2) Hardware Diagnostic & Validation Tool
 * MCU: Arduino Uno (ATmega328P)
 * =========================================================================
 * WIRING DIAGRAM (Software Serial Communication)
 * -------------------------------------------------------------------------
 * GY-GPS6Mv2 Pin | Arduino Uno Pin | Note
 * -------------------------------------------------------------------------
 * VCC            | 5V             | GPS module supply
 * GND            | GND            | Common ground
 * TX             | Pin 2          | Uno RX (GPS → Arduino)
 * RX             | Pin 3          | Uno TX (Arduino → GPS, optional)
 * =========================================================================
 */

#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>

#define GPS_RX_PIN 2
#define GPS_TX_PIN 3
#define GPS_BAUDRATE 9600
#define HEARTBEAT_MS 10000
#define SUMMARY_S 300
#define STALE_AGE_MS 2000

TinyGPSPlus gps;
SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);

// Statistics
static unsigned long startMs = 0;
static unsigned long ttffMs = 0;
static unsigned long lastHeartbeat = 0;
static unsigned long lastSummary = 0;
static unsigned long fixCount = 0;
static unsigned long noFixMs = 0;
static unsigned long noFixStart = 0;
static bool hadFixEver = false;
static bool errorReported = false;
static long hdopMin = 999, hdopMax = 0, hdopSum = 0, hdopCount = 0;
static int satMin = 99, satMax = 0, satSum = 0, satCount = 0;

static void printTime();
static void printFixQuality();

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println(F("=============================================="));
  Serial.println(F(" NEO-6M GPS Diagnostic Tool (Arduino Uno)"));
  Serial.println(F("=============================================="));
  Serial.println();

  Serial.print(F("Initialising SoftwareSerial on RX="));
  Serial.print(GPS_RX_PIN);
  Serial.print(F(", TX="));
  Serial.println(GPS_TX_PIN);
  gpsSerial.begin(GPS_BAUDRATE);
  Serial.println(F("OK. Waiting for satellite lock..."));
  Serial.println(F("NOTE: Antenna needs clear sky view."));
  Serial.println(F("Cold start may take 5-15 minutes."));
  Serial.println();
  Serial.println(F("SATS  QLT  HDOP  LAT          LNG              ALT       SPEED    TIME"));
  Serial.println(F("----  ---  ----  -----------  --------------  --------  -------  --------"));
  startMs = millis();
}

void loop() {
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // Timeout trap — print once, keep reading
  if (!errorReported && millis() > 5000 && gps.charsProcessed() < 10) {
    Serial.println();
    Serial.println(F("CRITICAL: No data from GPS. Check wiring (Pin 2 ↔ GPS TX)."));
    errorReported = true;
  }

  // Time to first fix
  if (ttffMs == 0 && gps.location.isValid() && gps.location.age() < 500) {
    ttffMs = millis() - startMs;
    Serial.println();
    Serial.print(F("TIME TO FIRST FIX: "));
    Serial.print(ttffMs / 1000);
    Serial.println(F(" sec"));
  }

  // Track fix/no-fix time
  bool hasFix = gps.location.isValid() && gps.location.age() < STALE_AGE_MS;

  if (!hadFixEver && hasFix) {
    hadFixEver = true;
  }

  if (!hasFix && hadFixEver && noFixStart == 0) {
    noFixStart = millis();
  }
  if (hasFix && noFixStart > 0) {
    noFixMs += millis() - noFixStart;
    noFixStart = 0;
  }

  // Fresh location update
  if (gps.location.isUpdated()) {
    fixCount++;

    // Satellite count
    int sats = gps.satellites.isValid() ? gps.satellites.value() : 0;
    if (sats < satMin) satMin = sats;
    if (sats > satMax) satMax = sats;
    satSum += sats; satCount++;

    // HDOP
    long hdop = gps.hdop.isValid() ? gps.hdop.value() : 999;
    if (hdop < hdopMin) hdopMin = hdop;
    if (hdop > hdopMax) hdopMax = hdop;
    hdopSum += hdop; hdopCount++;

    // Print line
    Serial.print(sats < 10 ? F("  ") : sats < 100 ? F(" ") : F(""));
    Serial.print(sats);
    Serial.print(F("  "));
    printFixQuality();
    Serial.print(F("  "));
    if (hdop == 999) { Serial.print(F("----")); }
    else {
      if (hdop < 10) Serial.print(' ');
      if (hdop < 100) Serial.print(' ');
      Serial.print(hdop);
    }
    Serial.print(F("  "));
    Serial.print(gps.location.lat(), 6);
    Serial.print(F("  "));
    Serial.print(gps.location.lng(), 6);
    Serial.print(F("  "));
    if (gps.altitude.isValid()) {
      Serial.print(gps.altitude.meters(), 1);
      Serial.print('m');
    } else {
      Serial.print(F("  N/A  "));
    }
    Serial.print(F("  "));
    if (gps.speed.isValid()) {
      Serial.print(gps.speed.kmph(), 1);
      Serial.print(F("km/h"));
    } else {
      Serial.print(F("  N/A  "));
    }
    Serial.print(F("  "));
    printTime();
    Serial.println();
  }

  // Heartbeat — periodic status even without new fix
  if (millis() - lastHeartbeat > HEARTBEAT_MS) {
    lastHeartbeat = millis();
    if (!hasFix) {
      int sats = gps.satellites.isValid() ? gps.satellites.value() : 0;
      Serial.print(F("... Searching ...  SATS: "));
      Serial.print(sats);
      Serial.print(F("  CHARS: "));
      Serial.print(gps.charsProcessed());
      Serial.print(F("  AGE: "));
      if (gps.location.isValid()) {
        Serial.print(gps.location.age());
        Serial.println(F("ms"));
      } else {
        Serial.println(F("N/A"));
      }
    }
  }

  // Summary every 5 minutes
  if (millis() - lastSummary > SUMMARY_S * 1000UL) {
    lastSummary = millis();
    unsigned long elapsed = (millis() - startMs) / 1000;
    unsigned long uptimePct = 100;
    if (fixCount > 0 && elapsed > 0) {
      uptimePct = 100 - (noFixMs / 10) / elapsed;
      if (uptimePct > 100) uptimePct = 100;
    }
    Serial.println();
    Serial.println(F("--- 5 min Summary ---"));
    Serial.print(F("Elapsed: "));
    Serial.print(elapsed);
    Serial.println(F("s"));
    Serial.print(F("Fixes: "));
    Serial.print(fixCount);
    Serial.print(F("  Uptime: "));
    Serial.print(uptimePct);
    Serial.println(F("%"));
    Serial.print(F("SATS  min="));
    Serial.print(satMin);
    Serial.print(F("  avg="));
    Serial.print(satCount > 0 ? (float)satSum / satCount : 0, 1);
    Serial.print(F("  max="));
    Serial.println(satMax);
    Serial.print(F("HDOP  min="));
    Serial.print(hdopMin);
    Serial.print(F("  avg="));
    Serial.print(hdopCount > 0 ? (float)hdopSum / hdopCount : 0, 1);
    Serial.print(F("  max="));
    Serial.println(hdopMax);
    Serial.println(F("---------------------"));
    Serial.println();
  }
}

static void printFixQuality() {
  if (!gps.location.isValid()) {
    Serial.print(F("NO FX"));
  } else if (gps.altitude.isValid()) {
    Serial.print(F("3D  "));
  } else {
    Serial.print(F("2D  "));
  }
}

static void printTime() {
  if (gps.time.isValid()) {
    if (gps.time.hour() < 10) Serial.print('0');
    Serial.print(gps.time.hour());
    Serial.print(':');
    if (gps.time.minute() < 10) Serial.print('0');
    Serial.print(gps.time.minute());
    Serial.print(':');
    if (gps.time.second() < 10) Serial.print('0');
    Serial.print(gps.time.second());
  } else {
    Serial.print(F("N/A"));
  }
}
