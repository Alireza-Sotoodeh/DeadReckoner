// Last Edit: 2026-06-04 13:30:00
// Reason for Last Edit: Standalone Sanity Check for OLED 0.91" using Software I2C on ESP32-S3
// Author: Alireza Sotoodeh

/*
 * =========================================================================
 * PROJECT: Hardware Sanity Check (OLED Display)
 * VERSION: Test.2
 * * WIRING DIAGRAM
 * -------------------------------------------------------------------------
 * Component        | ESP32-S3 Pin          | Note / Reasoning
 * -------------------------------------------------------------------------
 * OLED VCC         | 3.3V                  | Logic power
 * OLED GND         | GND                   | Common ground
 * OLED SCL         | GPIO 7                | Software I2C Clock (as per main architecture)
 * OLED SDA         | GPIO 6                | Software I2C Data (as per main architecture)
 * =========================================================================
 */

#include <U8g2lib.h>
#include <Wire.h> // Included for core dependencies

// Initialize OLED using Software I2C exactly as in the main project architecture
U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ 7, /* data=*/ 6, /* reset=*/ U8X8_PIN_NONE);

#define font_8_pixel u8g2_font_helvB08_tf

void setup() {
  Serial.begin(115200);
  Serial.println("=================================");
  Serial.println("Starting OLED Sanity Check...");
  Serial.println("=================================");

  // Initialize display
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(font_8_pixel);
  
  // Draw static text to confirm boot
  u8g2.drawStr(10, 15, "OLED is ONLINE");
  u8g2.drawStr(10, 25, "System Healthy");
  u8g2.sendBuffer();
  
  Serial.println("STATUS: Initialization command sent to OLED.");
  delay(2000); // Hold the message for 2 seconds
}

void loop() {
  static int frameCounter = 0;
  
  u8g2.clearBuffer();
  u8g2.setFont(font_8_pixel);
  
  // Render a dynamic counter to verify refresh rate and stability
  char buf[32];
  snprintf(buf, sizeof(buf), "Test Frame: %d", frameCounter);
  u8g2.drawStr(15, 20, buf);
  
  u8g2.sendBuffer();
  
  // Print to serial monitor for side-by-side verification
  Serial.print("Rendered Frame: ");
  Serial.println(frameCounter);
  
  frameCounter++;
  delay(100); // 10 FPS test
}