// Last Edit: 2026-06-08 11:30:00
// Reason for Last Edit: Added ESP32 Chip Identification diagnostic tool to verify hardware specs.
// Author: DeadReckoner Mentor

/*
 * =========================================================================
 * PROJECT: Dead Reckoning Hardware Identification
 * VERSION: ESP32 Chip ID Tool
 * * use ESP32 Dev Module option from esp32
 * -------------------------------------------------------------------------
 * NO WIRING REQUIRED FOR THIS TEST. 
 * Connect the ESP32 directly to the PC via USB.
 * -------------------------------------------------------------------------
 * =========================================================================
 */

#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  
  // Allow Serial Monitor to initialize completely
  delay(2000); 

  Serial.println("\n=============================================");
  Serial.println("     ESP32 CHIP IDENTIFICATION TOOL      ");
  Serial.println("=============================================");
  
  // 1. Get Chip Model
  Serial.print("Chip Model: ");
  Serial.println(ESP.getChipModel());
  
  // 2. Get Chip Revision
  Serial.print("Chip Revision: v");
  Serial.println(ESP.getChipRevision());
  
  // 3. Get Number of CPU Cores
  Serial.print("Number of Cores: ");
  Serial.println(ESP.getChipCores());
  
  // 4. Get Flash Memory Size
  Serial.print("Flash Chip Size (MB): ");
  uint32_t flashSize = ESP.getFlashChipSize() / (1024 * 1024);
  Serial.println(flashSize);

  Serial.println("=============================================");
  Serial.println("System Ready.");
}

void loop() {
  // Static diagnostic test does not require loop execution
  delay(1000); 
}