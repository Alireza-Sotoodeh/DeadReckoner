// Last Edit: 2026-06-08 11:55:00
// Reason for Last Edit: Migrated proven SD Module diagnostic test to ESP32 Classic (VSPI) with 3.3V power architecture.
// Author: DeadReckoner Mentor

/*
 * =========================================================================
 * PROJECT: Dead Reckoning SD Card Sanity Check
 * VERSION: Commercial Module Test on ESP32 Standard (3.3V Power)
 * * * WIRING DIAGRAM (ESP32 to Dual-Voltage SD Module)
 * -------------------------------------------------------------------------
 * SD Module Pin | ESP32 Pin     | Note / Reasoning
 * -------------------------------------------------------------------------
 * 5V            | NOT CONNECTED | Leave floating (Bypassing regulator)
 * 3.3V          | 3V3           | Powered directly from ESP 3.3V rail
 * GND           | GND           | Common Ground
 * CS            | GPIO 5        | VSPI CS0 (Chip Select)
 * MOSI          | GPIO 23       | VSPI MOSI
 * SCK           | GPIO 18       | VSPI SCK
 * MISO          | GPIO 19       | VSPI MISO
 * -------------------------------------------------------------------------
 * =========================================================================
 */

#include <SPI.h>
#include <SD.h>

// Define Chip Select pin for ESP32 VSPI
const int chipSelect = 5; 

void setup() {
  Serial.begin(115200);
  
  // Allow Serial Monitor to initialize completely
  delay(2000); 

  Serial.println("\n=============================================");
  Serial.println("  STARTING FULL SD CARD DIAGNOSTIC TEST  ");
  Serial.println("  PLATFORM: ESP32 CLASSIC (3.3V TEST)    ");
  Serial.println("=============================================");
  
  // TEST 1: Hardware and Library Initialization
  Serial.print("TEST 1: Initializing SD card... ");
  if (!SD.begin(chipSelect)) {
    Serial.println("FAILED!");
    Serial.println("-> ERROR: Check wiring. If using 3.3V, the module's level shifter might not be powering on.");
    Serial.println("-> FIX: Try moving the power wire from ESP32 3V3 to ESP32 VIN (5V), and module 3.3V to 5V.");
    
    // Halt execution safely to prevent TWDT crash
    while (1) {
      delay(10); 
    }
  }
  Serial.println("PASSED.");

  // TEST 2: Check and remove previous test artifacts
  Serial.print("TEST 2: Checking for old test file... ");
  if (SD.exists("/DR_TEST.txt")) {
    SD.remove("/DR_TEST.txt");
    Serial.println("Found and Deleted.");
  } else {
    Serial.println("Clean state confirmed.");
  }

  // TEST 3: Block Write Test
  Serial.print("TEST 3: Creating and writing to /DR_TEST.txt... ");
  File testFile = SD.open("/DR_TEST.txt", FILE_WRITE);
  if (testFile) {
    testFile.println("--- DEAD RECKONING SD MODULE TEST ---");
    testFile.println("If you can read this, the SPI Write/Read is fully functional on ESP32 at 3.3V!");
    testFile.println("System Check: 100% HEALTHY.");
    testFile.close();
    Serial.println("PASSED.");
  } else {
    Serial.println("FAILED! Could not create file.");
    
    // Halt execution safely
    while (1) {
      delay(10);
    }
  }

  // TEST 4: Block Read Test
  Serial.print("TEST 4: Reading back data from /DR_TEST.txt... ");
  testFile = SD.open("/DR_TEST.txt");
  if (testFile) {
    Serial.println("PASSED.\n");
    Serial.println("------- FILE CONTENT -------");
    
    // Print file contents to Serial Monitor
    while (testFile.available()) {
      Serial.write(testFile.read());
    }
    
    Serial.println("----------------------------\n");
    testFile.close();
  } else {
    Serial.println("FAILED! Could not open file for reading.");
    
    // Halt execution safely
    while (1) {
      delay(10);
    }
  }

  // TEST 5: File Deletion Test
  Serial.print("TEST 5: Deleting test file to clean up... ");
  if (SD.remove("/DR_TEST.txt")) {
    Serial.println("PASSED.");
  } else {
    Serial.println("FAILED! Could not delete file.");
  }

  Serial.println("=============================================");
  Serial.println("  ALL TESTS PASSED SUCCESSFULLY! YAY!  ");
  Serial.println("=============================================");
}

void loop() {
  // Static hardware test does not require loop execution
  delay(100); 
}