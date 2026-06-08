// Last Edit: 2026-06-08 12:45:00
// Reason for Last Edit: Ported external SdFat diagnostic script to ESP32 architecture for DIY Adapter testing.
// Author: DeadReckoner Mentor

/*
 * =========================================================================
 * PROJECT: Dead Reckoning Hardware Diagnostics
 * VERSION: Advanced SdFat Info Dump (ESP32 Classic)
 * * * WIRING DIAGRAM (ESP32 to DIY SD Adapter)
 * -------------------------------------------------------------------------
 * DIY Adapter Pin| ESP32 Pin     | Note / Reasoning
 * -------------------------------------------------------------------------
 * VCC (3.3V)     | 3V3           | DIRECT 3.3V ONLY (No onboard regulator)
 * GND            | GND           | Common Ground
 * CS             | GPIO 5        | VSPI CS0 (Chip Select)
 * MOSI           | GPIO 23       | VSPI MOSI
 * SCK            | GPIO 18       | VSPI SCK
 * MISO           | GPIO 19       | VSPI MISO
 * -------------------------------------------------------------------------
 * =========================================================================
 */

#include <SdFat.h>
#include <SPI.h>

// PIN DEFINITIONS FOR ESP32 VSPI
#define SD_CHIP_SELECT_PIN  5
#define SD_MOSI_PIN         23
#define SD_MISO_PIN         19
#define SD_SCK_PIN          18

// VARIABLES
SdFat sd;
File myFile;
File myDir;

uint32_t cardSize;
uint32_t eraseSize;

// FUNCTION PROTOTYPES
void initSDCard();
void dumpSDInfo();
void createDirectory();
void writeToFile();
void readFromFile();
void readDirectory();
void removeFolder();
uint8_t dumpCIDInformation();
uint8_t dumpCSDInformation();
uint8_t dumpPartitionInformation();
void dumpVolumeInformation();

void setup() {
  Serial.begin(115200);
  
  // Wait for Serial to initialize
  while (!Serial) {
    yield();
  }
  delay(2000);

  Serial.println("\n=============================================");
  Serial.println("  ADVANCED SDFAT DIAGNOSTIC (ESP32 / DIY)    ");
  Serial.println("=============================================");

  initSDCard();
  
  // Execute diagnostic routines
  dumpSDInfo();
  createDirectory();
  writeToFile();
  readFromFile();
  readDirectory();
  removeFolder();

  Serial.println("=============================================");
  Serial.println("  DIAGNOSTIC SEQUENCE COMPLETE.              ");
  Serial.println("=============================================");
}

void loop() {
  // Static test, no loop required
  delay(100);
}

void initSDCard() {
  uint32_t t = millis();
  
  Serial.print("Initializing SD Card at high speed... ");
  // Initialize at the highest speed supported by the board that is not over 50 MHz.
  if (!sd.begin(SD_CHIP_SELECT_PIN, SD_SCK_MHZ(10))) {
    Serial.println("FAILED!");
    Serial.println("-> Try a lower speed (e.g., SD_SCK_MHZ(10)) if SPI errors occur.");
    return;
  }
  Serial.println("PASSED.");

  t = millis() - t;
  cardSize = sd.card()->cardSize();
  
  if (cardSize == 0) {
    Serial.println("ERROR: cardSize failed to read.");
    return;
  }
  
  Serial.print("Initialization Time: ");
  Serial.print(t);
  Serial.println(" ms");
}

void dumpSDInfo() {
  Serial.print("SdFat Version: ");
  Serial.println(SD_FAT_VERSION); 
  
  Serial.print("\nCard Type: ");
  switch (sd.card()->type()) {
    case SD_CARD_TYPE_SD1:  Serial.println("SD1"); break;
    case SD_CARD_TYPE_SD2:  Serial.println("SD2"); break;
    case SD_CARD_TYPE_SDHC:
      if (cardSize < 70000000) {
        Serial.println("SDHC");
      } else {
        Serial.println("SDXC");
      }
      break;
    default: Serial.println("Unknown");
  }
  
  if (!dumpCIDInformation()) return;
  if (!dumpCSDInformation()) return;

  uint32_t ocr;
  if (!sd.card()->readOCR(&ocr)) {
    Serial.println("\nreadOCR failed");
    return;
  }
  
  Serial.print("OCR: 0X");
  Serial.println(ocr, HEX);
  
  if (!dumpPartitionInformation()) return;
  
  if (!sd.fsBegin()) {
    Serial.println("\nFile System initialization failed.");
    return;
  }

  dumpVolumeInformation();
}

void createDirectory() {
  Serial.println("\n--- Directory Creation Test ---");
  if (!sd.exists("testFolder")) {
    if (myDir.open("/")) {
      if (sd.mkdir("testFolder")) {
        Serial.println("Created 'testFolder' successfully.");
      } else {
        Serial.println("FAILED to create 'testFolder'.");
      }
    } else {
      Serial.println("FAILED to open root directory.");
    }  
  } else {
    Serial.println("Folder 'testFolder' already exists.");
  }
}

void writeToFile() {
  Serial.println("\n--- File Write Test ---");
  myFile = sd.open("testFolder/test.txt", FILE_WRITE);
  
  if (myFile) {
    Serial.print("Writing data... ");
    myFile.println("testing 1, 2, 3.");
    myFile.close();
    Serial.println("Write Complete.");
  } else {
    Serial.println("ERROR opening test.txt for writing.");
  }
}

void readFromFile() {
  Serial.println("\n--- File Read Test ---");
  myFile = sd.open("testFolder/test.txt");
  
  if (myFile) {
    Serial.println("Content of 'testFolder/test.txt':");
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    myFile.close();
  } else {
    Serial.println("ERROR opening 'testFolder/test.txt' for reading.");
  }
}

void readDirectory() {
  Serial.println("\n--- Directory Read Test ---");
  if (myDir.open("/testFolder")) {
    while (myFile.openNext(&myDir, O_RDONLY)) {
      myFile.printFileSize(&Serial);
      Serial.write(' ');
      myFile.printModifyDateTime(&Serial);
      Serial.write(' ');
      myFile.printName(&Serial);
      if (myFile.isDir()) {
        Serial.write('/');
      }
      Serial.println();
      myFile.close();
    }
  } else {
    Serial.println("ERROR opening myDir.");
  }  
}

void removeFolder() {
  Serial.println("\n--- Folder Removal Test ---");
  if (sd.rmdir("testFolder")) {
    Serial.println("Removal of 'testFolder' completed.");
  } else {
    Serial.println("Removal of 'testFolder' failed.");
  }
}

uint8_t dumpCIDInformation() {
  cid_t cid;
  if (!sd.card()->readCID(&cid)) {
    Serial.println("readCID failed");
    return false;
  }
  
  Serial.print("\nManufacturer ID: 0X");
  Serial.println(int(cid.mid), HEX);
  Serial.print("Product: ");
  for (uint8_t i = 0; i < 5; i++) {
    Serial.print(cid.pnm[i]);
  }
  Serial.println();
  return true;
}

uint8_t dumpCSDInformation() {
  csd_t csd;
  uint8_t eraseSingleBlock;
  
  if (!sd.card()->readCSD(&csd)) {
    Serial.println("readCSD failed");
    return false;
  }
  
  if (csd.v1.csd_ver == 0) {
    eraseSingleBlock = csd.v1.erase_blk_en;
    eraseSize = (csd.v1.sector_size_high << 1) | csd.v1.sector_size_low;
  } else if (csd.v2.csd_ver == 1) {
    eraseSingleBlock = csd.v2.erase_blk_en;
    eraseSize = (csd.v2.sector_size_high << 1) | csd.v2.sector_size_low;
  } else {
    Serial.println("CSD version error");
    return false;
  }
  
  eraseSize++;
  Serial.print("Card Size: ");
  Serial.print(0.000512 * cardSize);
  Serial.println(" MB");
  return true;
}

uint8_t dumpPartitionInformation() {
  mbr_t mbr;
  if (!sd.card()->readBlock(0, (uint8_t*)&mbr)) {
    Serial.println("read MBR failed");
    return false;
  }
  Serial.println("Partition info extracted successfully.");
  return true;
}

void dumpVolumeInformation() {
  Serial.print("\nVolume is FAT");
  Serial.println(int(sd.vol()->fatType()));
  
  uint32_t volFree = sd.vol()->freeClusterCount();
  float fs = 0.000512 * volFree * sd.vol()->blocksPerCluster();
  
  Serial.print("Free Space: ");
  Serial.print(fs);
  Serial.println(" MB");
}