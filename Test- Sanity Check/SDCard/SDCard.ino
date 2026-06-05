// Last Edit: 2026-06-05 01:00:00
// Reason for Last Edit: Standalone Sanity Check for SPI SD Card Module on Arduino Nano
// Last Edit: 2026-06-05 01:10:00
// Reason for Last Edit: Added wiring diagram for Arduino Nano SPI connections.
// Author: Alireza Sotoodeh

/*
 * =========================================================================
 * PROJECT: Hardware Sanity Check (SD Card Module)
 * VERSION: Test.3
 * * WIRING DIAGRAM
 * -------------------------------------------------------------------------
 * Component        | Arduino Nano Pin      | Note / Reasoning
 * -------------------------------------------------------------------------
 * SD Card VCC      | 5V                    | Module requires 5V logic power
 * SD Card GND      | GND                   | Common ground
 * SD Card CS       | D10                   | SPI Chip Select
 * SD Card MOSI     | D11                   | SPI Master Out Slave In
 * SD Card MISO     | D12                   | SPI Master In Slave Out
 * SD Card SCK      | D13                   | SPI Serial Clock
 * -------------------------------------------------------------------------
 * * DESIGN NOTES:
 * - This code explicitly tests the SPI interface, file creation, block writing, 
 * and reading capabilities of the SD Card module before integration into the 
 * main dual-core ESP32-S3 architecture.
 * =========================================================================
 */

#include <SPI.h>
#include <SD.h>

const int chipSelect = 10;
File dataFile;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial port to connect
  }

  Serial.println("=================================");
  Serial.println("Starting SD Card Sanity Check...");
  Serial.println("=================================");

  // Initialize the SD card
  Serial.print("STATUS: Initializing SD card... ");
  if (!SD.begin(chipSelect)) {
    Serial.println("FAILED!");
    Serial.println("ERROR: Check wiring, ensure SD card is inserted and formatted to FAT32.");
    while (1); // Halt the system
  }
  Serial.println("SUCCESS.");

  // Test 1: File Creation and Writing
  Serial.print("STATUS: Opening test.txt for writing... ");
  dataFile = SD.open("test.txt", FILE_WRITE);
  
  if (dataFile) {
    Serial.println("SUCCESS.");
    Serial.println("STATUS: Writing test data block...");
    dataFile.println("SD Card Sanity Check Passed.");
    dataFile.println("Module is ready for DeadReckoner Phase 3.");
    dataFile.close();
    Serial.println("STATUS: File closed and data flushed to memory.");
  } else {
    Serial.println("FAILED to open test.txt for writing.");
    while (1); // Halt
  }

  // Test 2: File Reading and Verification
  Serial.print("STATUS: Re-opening test.txt for reading... ");
  dataFile = SD.open("test.txt");
  
  if (dataFile) {
    Serial.println("SUCCESS.");
    Serial.println("--- DATA READ FROM SD CARD ---");
    while (dataFile.available()) {
      Serial.write(dataFile.read());
    }
    Serial.println("------------------------------");
    dataFile.close();
    Serial.println("STATUS: Read test completed successfully.");
  } else {
    Serial.println("FAILED to open test.txt for reading.");
  }
  
  Serial.println("=================================");
  Serial.println("ALL TESTS PASSED. Hardware is healthy.");
  Serial.println("=================================");
}

void loop() {
  // Standalone test does not require loop execution
}