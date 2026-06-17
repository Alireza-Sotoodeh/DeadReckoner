/*
 * =========================================================================
 * PROJECT: NEO-6M (GY-GPS6Mv2) Hardware Diagnostic & Validation Tool
 * MCU: Arduino Uno (ATmega328P)
 * =========================================================================
 * WIRING DIAGRAM (Software Serial Communication)
 * -------------------------------------------------------------------------
 * GY-GPS6Mv2 Pin | Arduino Uno Pin | Note
 * -------------------------------------------------------------------------
 * VCC            | 5V or 3.3V      | Connected to Uno 5V rail
 * GND            | GND             | Common system ground
 * TX             | Pin 2           | Uno RX (Receives data from GPS TX)
 * RX             | Pin 3           | Uno TX (Sends config to GPS RX - Optional)
 * =========================================================================
 */

#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>

#define GPS_RX_PIN 2
#define GPS_TX_PIN 3
#define GPS_BAUDRATE 9600

TinyGPSPlus gps;
SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);

void setup() {
    // Initialize primary hardware serial for computer monitoring
    Serial.begin(115200);
    delay(1000); 

    Serial.println(F("\n=============================================="));
    Serial.println(F("   NEO-6M GPS Diagnostic Tool (Arduino Uno)   "));
    Serial.println(F("==============================================\n"));

    Serial.println(F("[*] Initializing Software Serial for GPS on Pins 2(RX), 3(TX)..."));
    gpsSerial.begin(GPS_BAUDRATE);

    Serial.println(F("[+] SUCCESS: Software Serial Port Active."));
    Serial.println(F("[*] Waiting for satellite lock..."));
    Serial.println(F("[!] NOTE: Antenna must have a clear, direct view of the sky."));
    Serial.println(F("[!] A Cold Start may take 5 to 15 minutes to secure a fix."));
    Serial.println(F("\n>>> Starting Live GPS Data Stream <<<\n"));
}

void loop() {
    // Read incoming characters from Software Serial stream safely
    while (gpsSerial.available() > 0) {
        gps.encode(gpsSerial.read());
    }

    // Process and print values only when a fresh, valid location sentence is decoded
    if (gps.location.isUpdated()) {
        Serial.print(F("SATS: "));
        Serial.print(gps.satellites.value());
        
        Serial.print(F("  |  LAT: "));
        Serial.print(gps.location.lat(), 6);
        
        Serial.print(F("  |  LNG: "));
        Serial.print(gps.location.lng(), 6);
        
        Serial.print(F("  |  ALT: "));
        Serial.print(gps.altitude.meters(), 1);
        
        Serial.print(F("m  |  SPEED: "));
        Serial.print(gps.speed.kmph(), 1);
        
        Serial.print(F("km/h  |  TIME: "));
        if (gps.time.isValid()) {
            if (gps.time.hour() < 10) Serial.print("0");
            Serial.print(gps.time.hour()); Serial.print(":");
            if (gps.time.minute() < 10) Serial.print("0");
            Serial.print(gps.time.minute()); Serial.print(":");
            if (gps.time.second() < 10) Serial.print("0");
            Serial.print(gps.time.second());
        } else {
            Serial.print(F("N/A"));
        }
        Serial.println();
    }

    // Hardware Timeout Trap to capture bad connections or wiring swaps
    if (millis() > 5000 && gps.charsProcessed() < 10) {
        Serial.println(F("[-] CRITICAL ERROR: No data from GPS. Check Pin 2 and Pin 3 wiring."));
        delay(2000);
    }
}