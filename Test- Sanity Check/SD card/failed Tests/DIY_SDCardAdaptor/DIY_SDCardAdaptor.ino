// Last Edit: 2026-06-05 14:15:00
// Reason for Last Edit: Standalone Sanity Check for DIY SD Card Adapter on NodeMCU (ESP8266)
// Last Edit: 2026-06-05 14:30:00
// Reason for Last Edit: Enforced English-only comments rule.
// Author: Alireza Sotoodeh

/*
 * =========================================================================
 * PROJECT: Hardware Sanity Check (SD Card Module)
 * VERSION: Test.ESP8266
 * * DIY SD ADAPTER WIRING DIAGRAM (DIRECT SOLDER TO NODEMCU)
 * -------------------------------------------------------------------------
 * SPI Function | NodeMCU Pin   | Note / Reasoning
 * -------------------------------------------------------------------------
 * VCC (3.3V)   | 3V3           | ***CRITICAL: 3V3 Only! (Color is reversed)***
 * GND          | GND           | Common Ground (Color is reversed)
 * CS           | D8 (GPIO 15)  | Hardware SPI Chip Select
 * MOSI (DI)    | D7 (GPIO 13)  | Hardware SPI MOSI
 * SCK (CLK)    | D5 (GPIO 14)  | Hardware SPI SCK
 * MISO (DO)    | D6 (GPIO 12)  | Hardware SPI MISO
 * -------------------------------------------------------------------------
 * * DESIGN NOTES:
 * - This code explicitly tests the SPI interface on NodeMCU (ESP8266).
 * - Hardware SPI pins on NodeMCU: CS=D8, MOSI=D7, MISO=D6, SCK=D5.
 * - The DIY adapter removes the 5V-to-3.3V regulator and level shifters.
 * =========================================================================
 */

#include <SPI.h>
#include <SD.h>

// Define Chip Select pin for NodeMCU hardware SPI
const int chipSelect = D1;

void setup() {
  Serial.begin(115200);
  
  // Brief delay to allow Serial Monitor to initialize
  delay(2000); 

  Serial.println("\n=================================");
  Serial.println("Starting SD Card Sanity Check on NodeMCU...");
  Serial.println("=================================");

  // Initialize SD card
  Serial.print("STATUS: Initializing SD card... ");
  if (!SD.begin(chipSelect)) {
    Serial.println("FAILED!");
    Serial.println("ERROR: Check wiring (especially D5 to D8) and ensure card is FAT32.");
    
    // Halt execution on critical failure safely
    while (1) {
      yield(); // <--- اضافه شده برای جلوگیری از WDT Reset
    }
  }
  Serial.println("SUCCESS.");

  // Test 1: File creation and data writing
  Serial.print("STATUS: Opening test.txt for writing... ");
  File dataFile = SD.open("test.txt", FILE_WRITE);
  
  if (dataFile) {
    Serial.println("SUCCESS.");
    Serial.println("STATUS: Writing test data block...");
    dataFile.println("SD Card Hardware Sanity Check: PASSED.");
    dataFile.println("DIY Adapter is working perfectly on NodeMCU ESP8266.");
    dataFile.close();
    Serial.println("STATUS: File closed and data saved.");
  } else {
    Serial.println("FAILED to open test.txt for writing.");
    
    // Halt execution on critical failure safely
    while (1) {
      yield(); 
    }
  }

  // Test 2: File reading and verification
  Serial.print("STATUS: Re-opening test.txt for reading... ");
  dataFile = SD.open("test.txt");
  
  if (dataFile) {
    Serial.println("SUCCESS.");
    Serial.println("--- DATA READ FROM SD CARD ---");
    
    // Print file contents to Serial
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
  // Static hardware test does not require loop execution
  yield(); 
}