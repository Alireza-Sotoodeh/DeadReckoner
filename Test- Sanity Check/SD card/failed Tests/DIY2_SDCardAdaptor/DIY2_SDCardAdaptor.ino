// Last Edit: 2026-06-08 10:25:00
// Reason for Last Edit: Adapted standard SD datalogger for ESP8266 NodeMCU to test DIY Adapter.
// Author: DeadReckoner Mentor

/*
 * =========================================================================
 * PROJECT: Dead Reckoning SD Card Sanity Check
 * VERSION: DIY Adapter Datalogger Test (ESP8266)
 * * * WIRING DIAGRAM (NodeMCU to DIY SD Adapter)
 * -------------------------------------------------------------------------
 * SD Adapter Pin| NodeMCU Pin   | Note / Reasoning
 * -------------------------------------------------------------------------
 * VCC (3.3V)    | 3V3           | DIRECT 3.3V ONLY (No onboard regulator)
 * GND           | GND           | Common Ground
 * CS            | D1 (GPIO 5)   | Safe Chip Select pin for ESP8266
 * MOSI          | D7 (GPIO 13)  | Hardware SPI MOSI
 * SCK           | D5 (GPIO 14)  | Hardware SPI SCK
 * MISO          | D6 (GPIO 12)  | Hardware SPI MISO
 * -------------------------------------------------------------------------
 * =========================================================================
 */

#include <SPI.h>
#include <SD.h>

const int chipSelect = D1;

void setup() {
  Serial.begin(115200);
  delay(2000); // Allow Serial Monitor to initialize

  Serial.println("\n--- DIY SD ADAPTER DATALOGGER TEST ---");
  Serial.print("STATUS: Initializing SD card... ");

  if (!SD.begin(chipSelect)) {
    Serial.println("FAILED!");
    Serial.println("ERROR: Check wiring, ensure card is FAT32, and check connections.");
    
    // Halt execution safely to prevent WDT resets
    while (1) {
      yield();
    }
  }
  
  Serial.println("SUCCESS. Card initialized.");
}

void loop() {
  // Assemble the data string to log
  String dataString = "DR_Test_Log, ";
  
  // Read the single available analog pin on ESP8266 (A0)
  int sensorValue = analogRead(A0);
  dataString += "A0_Value: ";
  dataString += String(sensorValue);
  dataString += ", Uptime_ms: ";
  dataString += String(millis());

  // Open the file (only one file can be open at a time)
  File dataFile = SD.open("datalog.txt", FILE_WRITE);

  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    
    // Echo to serial port for verification
    Serial.print("LOGGED: ");
    Serial.println(dataString);
  } else {
    Serial.println("ERROR: Failed to open datalog.txt for writing.");
  }

  // Delay to prevent WDT crash and control logging rate (1 Hz)
  delay(1000); 
}