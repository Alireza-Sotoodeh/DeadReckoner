// Last Edit: 2026-06-08 18:45:00
// Purpose: Fine-grained SPI Sweep (1MHz steps) to pinpoint the exact failure threshold.

#include <SdFat.h>
#include <SPI.h>

#define SD_CHIP_SELECT_PIN 5
#define TEST_FILE "SWEEP.BIN"
#define BUF_SIZE 16384 

SdFat sd;
File file;
uint8_t buffer[BUF_SIZE];

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n--- HIGH-PRECISION SPI SWEEP (1MHz Steps) ---");
  Serial.println("Freq(MHz) | Result | Speed(KB/s)");
  Serial.println("------------------------------------");

  // حلقه 1 مگاهرتزی از 1 تا 40
  for (int freq = 1; freq <= 40; freq++) {
    Serial.print(freq);
    Serial.print(" MHz    | ");
    
    if (runTest(freq)) {
      // ادامه تست
    } else {
      Serial.println("FAILED | N/A");
      Serial.println("--- LIMIT REACHED. SYSTEM UNSTABLE ABOVE THIS FREQ. ---");
      break; 
    }
  }
  Serial.println("--- SWEEP FINISHED ---");
}

bool runTest(int freq) {
  if (!sd.begin(SD_CHIP_SELECT_PIN, SD_SCK_MHZ(freq))) return false;

  sd.remove(TEST_FILE);
  file = sd.open(TEST_FILE, FILE_WRITE);
  if (!file) return false;

  uint32_t start = millis();
  for(int i = 0; i < 32; i++) {
    if (file.write(buffer, BUF_SIZE) != BUF_SIZE) {
      file.close();
      return false;
    }
  }
  uint32_t duration = millis() - start;
  file.close();
  
  float speed = (512.0) / (duration / 1000.0);
  Serial.print("PASSED | "); 
  Serial.print(speed); 
  Serial.println(" KB/s");
  return true;
}

void loop() {}