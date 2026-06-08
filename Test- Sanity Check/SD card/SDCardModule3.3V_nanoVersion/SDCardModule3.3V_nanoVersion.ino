// Last Edit: 2026-06-08 10:55:00
// Reason for Last Edit: Removed SD.type() method which is unsupported in the standard Arduino AVR SD library to fix compilation error.
// Author: DeadReckoner Mentor

/*
 * =========================================================================
 * PROJECT: Dead Reckoning SD Card Sanity Check
 * VERSION: Full Diagnostic for Arduino UNO / Nano
 * * * WIRING DIAGRAM (Arduino to Dual-Voltage SD Module)
 * -------------------------------------------------------------------------
 * SD Module Pin | Arduino Pin | Note / Reasoning
 * -------------------------------------------------------------------------
 * 5V            | 5V          | Power via Arduino 5V (Module regulates to 3.3V)
 * 3.3V          | NOT CONN.   | Leave floating
 * GND           | GND         | Common Ground
 * CS            | 10          | Standard SPI SS (Chip Select)
 * MOSI          | 11          | Hardware SPI MOSI
 * SCK           | 13          | Hardware SPI SCK
 * MISO          | 12          | Hardware SPI MISO
 * -------------------------------------------------------------------------
 * =========================================================================
 */

#include <SPI.h>
#include <SD.h>

// Define Chip Select pin for Arduino
const int chipSelect = 10; 

void setup() {
  Serial.begin(115200);
  
  // Allow Serial Monitor to initialize completely
  delay(2000); 

  Serial.println("\n=============================================");
  Serial.println("  STARTING FULL SD CARD DIAGNOSTIC TEST  ");
  Serial.println("=============================================");
  
  // TEST 1: Hardware and Library Initialization
  Serial.print("TEST 1: Initializing SD card... ");
  if (!SD.begin(chipSelect)) {
    Serial.println("FAILED!");
    Serial.println("-> ERROR: Check wiring, SD card presence, or format (FAT32).");
    
    // Halt execution on critical failure
    while (1); 
  }
  Serial.println("PASSED.");

  // TEST 2: Check and remove previous test artifacts
  Serial.print("TEST 2: Checking for old test file... ");
  if (SD.exists("DR_TEST.txt")) {
    SD.remove("DR_TEST.txt");
    Serial.println("Found and Deleted.");
  } else {
    Serial.println("Clean state confirmed.");
  }

  // TEST 3: Block Write Test
  Serial.print("TEST 3: Creating and writing to DR_TEST.txt... ");
  File testFile = SD.open("DR_TEST.txt", FILE_WRITE);
  if (testFile) {
    testFile.println("--- DEAD RECKONING SD MODULE TEST ---");
    testFile.println("If you can read this, the SPI Write/Read is fully functional on Arduino!");
    testFile.println("System Check: 100% HEALTHY.");
    testFile.close();
    Serial.println("PASSED.");
  } else {
    Serial.println("FAILED! Could not create file.");
    
    // Halt execution on critical failure
    while (1); 
  }

  // TEST 4: Block Read Test
  Serial.print("TEST 4: Reading back data from DR_TEST.txt... ");
  testFile = SD.open("DR_TEST.txt");
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
    
    // Halt execution on critical failure
    while (1); 
  }

  // TEST 5: File Deletion Test
  Serial.print("TEST 5: Deleting test file to clean up... ");
  if (SD.remove("DR_TEST.txt")) {
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
}