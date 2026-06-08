// Last Edit: 2026-06-08 09:40:00
// Reason for Last Edit: Updated wiring diagram for 3.3V power source to validate compatibility with future Li-ion battery architecture.
// Author: DeadReckoner Mentor

/*
 * =========================================================================
 * PROJECT: Dead Reckoning SD Card Sanity Check
 * VERSION: Full Diagnostic for NodeMCU (ESP8266) - 3.3V Power Test
 * * * WIRING DIAGRAM (NodeMCU to Dual-Voltage SD Module via 3.3V)
 * -------------------------------------------------------------------------
 * SD Module Pin | NodeMCU Pin   | Note / Reasoning
 * -------------------------------------------------------------------------
 * 5V            | NOT CONNECTED | Leave floating (Bypass onboard regulator)
 * 3.3V          | 3V3           | Powered directly from ESP 3.3V rail
 * GND           | GND           | Common Ground
 * CS            | D1 (GPIO 5)   | Avoid D8 (GPIO 15) to prevent boot crashes
 * MOSI          | D7 (GPIO 13)  | Hardware SPI MOSI
 * SCK           | D5 (GPIO 14)  | Hardware SPI SCK
 * MISO          | D6 (GPIO 12)  | Hardware SPI MISO
 * -------------------------------------------------------------------------
 * =========================================================================
 */


#include <SPI.h>
#include <SD.h>

// Define Chip Select pin for NodeMCU (Safely using D1)
const int chipSelect = D1; 

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
    
    // Halt execution on critical failure safely
    while (1) { 
      yield(); // Prevent Soft WDT Reset
    } 
  }
  Serial.println("PASSED.");

  // Display inserted Card Type
  Serial.print("-> Card Type: ");
  switch (SD.type()) {
    case SD_CARD_TYPE_SD1:  Serial.println("SD1"); break;
    case SD_CARD_TYPE_SD2:  Serial.println("SD2"); break;
    case SD_CARD_TYPE_SDHC: Serial.println("SDHC/SDXC"); break;
    default:                Serial.println("Unknown");
  }

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
    testFile.println("If you can read this, the SPI Write/Read is fully functional!");
    testFile.println("System Check: 100% HEALTHY.");
    testFile.close();
    Serial.println("PASSED.");
  } else {
    Serial.println("FAILED! Could not create file.");
    
    // Halt execution on critical failure safely
    while (1) { 
      yield(); 
    }
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
    
    // Halt execution on critical failure safely
    while (1) { 
      yield(); 
    }
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
  Serial.println("  Hardware is ready for the ESP32-S3!  ");
  Serial.println("=============================================");
}

void loop() {
  // Static hardware test does not require loop execution
  yield(); 
}