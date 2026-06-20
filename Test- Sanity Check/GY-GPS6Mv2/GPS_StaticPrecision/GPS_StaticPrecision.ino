// Last Edit: 2026-06-20
// Reason for Last Edit: 1-hour stationary precision test for GY-GPS6Mv2
// Author: Alireza Sotoodeh

/*
 * =========================================================================
 * PROJECT: NEO-6M (GY-GPS6Mv2) — 1-Hour Static Precision Test
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
 * MEASUREMENTS:
 *   - Lat/Lon standard deviation (metres) — position scatter
 *   - Altitude min/max/mean/σ
 *   - HDOP and satellite count stats
 *   - 1 Hz CSV logging for optional external post-processing
 *   - Requires clear sky view for entire 1-hour duration
 * =========================================================================
 */

#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>
#include <math.h>

#define GPS_RX_PIN 2
#define GPS_TX_PIN 3
#define GPS_BAUDRATE 9600
#define DURATION_S 3600
#define INTERVAL_MS 1000
#define STALE_AGE_MS 3000

TinyGPSPlus gps;
SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);

// Offsets (set at first fix)
static float latRef = 0, lonRef = 0;
static float latOffSum = 0, latOffSumSq = 0;
static float lonOffSum = 0, lonOffSumSq = 0;
static float altOffSum = 0, altOffSumSq = 0;
static float altRef = 0;
static int latN = 0, lonN = 0, altN = 0;

// HDOP + satellites
static long hdopMin = 999, hdopMax = 0, hdopSum = 0, hdopCount = 0;
static int satMin = 99, satMax = 0, satSum = 0, satCount = 0;

// Global stats
static float latAbsMin = 90, latAbsMax = -90;
static float lonAbsMin = 180, lonAbsMax = -180;
static float altMin = 99999, altMax = -99999;

// Timing
static unsigned long startMs = 0;
static unsigned long lastLogMs = 0;
static int elapsed = 0;
static bool refSet = false;

// Fix drop tracking
static unsigned long noFixMs = 0;
static unsigned long noFixStart = 0;
static bool hadFix = false;

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println(F("=============================================="));
  Serial.println(F(" NEO-6M — 1-Hour Static Precision Test"));
  Serial.println(F("=============================================="));
  Serial.println();

  gpsSerial.begin(GPS_BAUDRATE);

  Serial.println(F("Waiting for 3D fix before starting..."));
  Serial.println(F("(Ensure clear sky view — cold start may take 5-15 min)"));
}

void loop() {
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // Wait for first 3D fix
  if (!refSet) {
    if (gps.location.isValid() && gps.altitude.isValid() && gps.location.age() < 1000) {
      refSet = true;
      latRef = gps.location.lat();
      lonRef = gps.location.lng();
      altRef = gps.altitude.meters();
      startMs = millis();
      lastLogMs = startMs;
      Serial.println(F("3D fix acquired. Starting 1-hour log..."));
      Serial.println();
      Serial.println(F("time_s, lat, lon, alt_m, hdop, sats"));
    }
    return;
  }

  unsigned long now = millis();
  if (now - lastLogMs < INTERVAL_MS) {
    return;
  }
  lastLogMs = now;
  elapsed++;

  bool hasFix = gps.location.isValid() && gps.location.age() < STALE_AGE_MS;

  // Track fix/no-fix time
  if (hasFix) {
    if (noFixStart > 0) {
      noFixMs += now - noFixStart;
      noFixStart = 0;
    }
    hadFix = true;
  } else if (hadFix && noFixStart == 0) {
    noFixStart = now;
  }

  if (!hasFix) {
    Serial.print(elapsed);
    Serial.println(F(", NO_FIX"));
    if (elapsed >= DURATION_S) {
      printSummary();
      while (1) { delay(1000); }
    }
    return;
  }

  float lat = gps.location.lat();
  float lon = gps.location.lng();
  float alt = gps.altitude.meters();
  long hdop = gps.hdop.isValid() ? gps.hdop.value() : 999;
  int sats = gps.satellites.isValid() ? gps.satellites.value() : 0;

  // CSV output
  Serial.print(elapsed);
  Serial.print(F(", "));
  Serial.print(lat, 6);
  Serial.print(F(", "));
  Serial.print(lon, 6);
  Serial.print(F(", "));
  Serial.print(alt, 1);
  Serial.print(F(", "));
  Serial.print(hdop);
  Serial.print(F(", "));
  Serial.println(sats);

  // Update position stats (offset from reference)
  float latOff = (lat - latRef) * 111320.0f;  // metres
  float lonOff = (lon - lonRef) * 111320.0f * cos(latRef * 3.14159f / 180.0f);

  latOffSum += latOff; latOffSumSq += latOff * latOff; latN++;
  lonOffSum += lonOff; lonOffSumSq += lonOff * lonOff; lonN++;
  altOffSum += alt; altOffSumSq += alt * alt; altN++;

  // Absolute lat/lon bounds
  if (lat < latAbsMin) latAbsMin = lat;
  if (lat > latAbsMax) latAbsMax = lat;
  if (lon < lonAbsMin) lonAbsMin = lon;
  if (lon > lonAbsMax) lonAbsMax = lon;

  // Altitude bounds
  if (alt < altMin) altMin = alt;
  if (alt > altMax) altMax = alt;

  // HDOP
  if (hdop < hdopMin) hdopMin = hdop;
  if (hdop > hdopMax) hdopMax = hdop;
  hdopSum += hdop; hdopCount++;

  // Satellites
  if (sats < satMin) satMin = sats;
  if (sats > satMax) satMax = sats;
  satSum += sats; satCount++;

  // End
  if (elapsed >= DURATION_S) {
    printSummary();
    while (1) { delay(1000); }
  }
}

static void printSummary() {
  Serial.println();
  Serial.println(F("=============================================="));
  Serial.println(F(" STATIC PRECISION — FINAL SUMMARY"));
  Serial.println(F("=============================================="));

  Serial.print(F("Duration: "));
  Serial.print(elapsed);
  Serial.println(F(" s"));

  Serial.print(F("Fix uptime: "));
  unsigned long uptime = elapsed * 1000UL - noFixMs;
  Serial.print(uptime / 10 / elapsed);
  Serial.println(F("%"));

  // Position scatter
  if (latN > 0 && lonN > 0) {
    float latMeanOff = latOffSum / latN;
    float lonMeanOff = lonOffSum / lonN;
    float latSigma = sqrt((latOffSumSq / latN - latMeanOff * latMeanOff) < 0 ? 0 : (latOffSumSq / latN - latMeanOff * latMeanOff));
    float lonSigma = sqrt((lonOffSumSq / lonN - lonMeanOff * lonMeanOff) < 0 ? 0 : (lonOffSumSq / lonN - lonMeanOff * lonMeanOff));
    float cep = 0.589f * (latSigma + lonSigma);  // Circular Error Probable (50%)

    Serial.println();
    Serial.println(F("Position scatter (relative to first fix):"));
    Serial.print(F("  Lat σ: "));
    Serial.print(latSigma, 3);
    Serial.println(F(" m"));
    Serial.print(F("  Lon σ: "));
    Serial.print(lonSigma, 3);
    Serial.println(F(" m"));
    Serial.print(F("  CEP (50%): "));
    Serial.print(cep, 3);
    Serial.println(F(" m"));

    float latDrift = latMeanOff / (elapsed / 3600.0f);
    float lonDrift = lonMeanOff / (elapsed / 3600.0f);
    Serial.print(F("  Lat drift: "));
    Serial.print(latDrift, 3);
    Serial.println(F(" m/h"));
    Serial.print(F("  Lon drift: "));
    Serial.print(lonDrift, 3);
    Serial.println(F(" m/h"));
  }

  if (altN > 0) {
    float altMean = altOffSum / altN;
    float altSigma = sqrt((altOffSumSq / altN - altMean * altMean) < 0 ? 0 : (altOffSumSq / altN - altMean * altMean));
    Serial.println();
    Serial.print(F("Altitude:  mean="));
    Serial.print(altMean, 1);
    Serial.print(F("  σ="));
    Serial.print(altSigma, 1);
    Serial.print(F("  min="));
    Serial.print(altMin, 1);
    Serial.print(F("  max="));
    Serial.println(altMax, 1);
  }

  Serial.println();
  Serial.print(F("HDOP:  min="));
  Serial.print(hdopMin);
  Serial.print(F("  avg="));
  Serial.print(hdopCount > 0 ? (float)hdopSum / hdopCount : 0, 1);
  Serial.print(F("  max="));
  Serial.println(hdopMax);

  Serial.print(F("SATS:  min="));
  Serial.print(satMin);
  Serial.print(F("  avg="));
  Serial.print(satCount > 0 ? (float)satSum / satCount : 0, 1);
  Serial.print(F("  max="));
  Serial.println(satMax);

  Serial.println(F("=============================================="));
  Serial.println(F(" TEST COMPLETE."));
  Serial.println(F("=============================================="));
}
