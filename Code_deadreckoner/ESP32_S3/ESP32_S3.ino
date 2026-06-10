// Last Edit: 2026-06-10 15:25:00
// Reason for Last Edit: Migrated to Dual-State UI, added 4-button navigation, non-blocking debouncing, and Tag/Waypoint system.
// Author: Alireza Sotoodeh

/*
 * =========================================================================
 * PROJECT: Signal-Free Offline Tracking System
 * VERSION: 1.4 (Interactive UI + Waypoint Tagging)
 * * WIRING DIAGRAM
 * -------------------------------------------------------------------------
 * Component        | ESP32-S3 Pin          | Note / Reasoning
 * -------------------------------------------------------------------------
 * MPU9250 VCC      | 3.3V                  | MPU9250 is 3.3V tolerant logic
 * MPU9250 GND      | GND                   | Common ground
 * MPU9250 SCL      | GPIO 5                | Hardware I2C (Wire) Clock (Requires external 4.7k pull-up to 3.3V)
 * MPU9250 SDA      | GPIO 4                | Hardware I2C (Wire) Data (Requires external 4.7k pull-up to 3.3V)
 * MPU9250 AD0      | GND                   | Sets I2C Address to 0x68
 * MPU9250 NCS      | 3.3V                  | Chip Select: HIGH forces I2C Mode
 * MPU9250 FSYNC    | GND                   | Frame Sync: Not used, tied to GND to prevent noise
 * MPU9250 INT      | NC (Not Connected)    | Interrupt: Not used, polling via mpu.update()
 * MPU9250 ECL      | NC (Not Connected)    | Aux I2C Clock: Not used
 * MPU9250 EDA      | NC (Not Connected)    | Aux I2C Data: Not used
 * -------------------------------------------------------------------------
 * OLED VCC         | 3.3V                  | 
 * OLED GND         | GND                   | Common ground
 * OLED SCL         | GPIO 7                | Software I2C
 * OLED SDA         | GPIO 6                | Software I2C
 * -------------------------------------------------------------------------
 * SD Adapter 3V3   | 3.3V                  | DIRECT 3.3V ONLY (No onboard regulator)
 * SD Adapter GND   | GND                   | Common Ground
 * SD Adapter CS    | GPIO 10               | FSPI CS0
 * SD Adapter MOSI  | GPIO 11               | FSPI MOSI
 * SD Adapter SCK   | GPIO 12               | FSPI SCK
 * SD Adapter MISO  | GPIO 13               | FSPI MISO
 * -------------------------------------------------------------------------
 * BTN SELECT       | GPIO 1                | Menu Enter/Open (Active-Low)
 * BTN UP           | GPIO 2                | Menu Navigation (Active-Low)
 * BTN DOWN         | GPIO 8                | Menu Navigation (Active-Low)
 * BTN TAG          | GPIO 9                | Waypoint Event Trigger (Active-Low)
 * =========================================================================
 */
 
/*////////////////////////////includes////////////////////////////*/
#include "MPU9250.h" //for setting up MPU9250
#include <EEPROM.h>  // ESP32 EEPROM wrapper library for loading calibration
#include <U8g2lib.h> // U8g2 library for OLED
#include <Wire.h>    // I2C library
#include <SPI.h>     // SPI library for SD Card
#include <SdFat.h>   // SdFat library for high-speed logging

/*////////////////////////////defines////////////////////////////*/
#define bud_rate 115200

// UI and Button Pins (Active-Low, Internal Pull-up)
#define BTN_SELECT_PIN 1
#define BTN_UP_PIN 2
#define BTN_DOWN_PIN 8
#define BTN_TAG_PIN 9

#define I2C_MPU_SDA 4
#define I2C_MPU_SCL 5
#define I2C_OLED_SDA 6
#define I2C_OLED_SCL 7

// SPI Pins for SD Card (ESP32-S3 specific to avoid GPIO 5 collision)
#define SD_CS_PIN 10
#define SD_MOSI_PIN 11
#define SD_SCK_PIN 12
#define SD_MISO_PIN 13
#define SPI_FREQ_MHZ 20 // Optimal speed derived from hardware sweep

//MpU9205
#define MPU9250_IMU_ADDRESS 0x68 												// Specifies the I2C slave address of the MPU9250:0x68 GND / 0x69 High
#define MAGNETIC_DECLINATION 3.4	 											// angle between magnetic north and true north
#define INTERVAL_MS_PRINT 50 														// Sets the minimum time interval between printing sensor data.

MPU9250 mpu; // handler: allowing access to all library methods
unsigned long lastPrintMillis = 0; // Tracks the timestamp (from millis()) of the last time sensor data was printed to Serial

//MPU9250 setting
#define MPU9250_Accelerometer_Rang  A2G 								//select: A2G, A4G, A8G, A16G
#define MPU9250_Gyroscope_Rang  G500DPS 								//select: G250DPS, G500DPS, G1000DPS, G2000DPS
#define MPU9250_Magnetometer_resolution  M16BITS 				//select: M14BITS, M16BITS
#define MPU9250_fifo_sample_rate  SMPL_1000HZ 					//select: SMPL_1000HZ, SMPL_500HZ, SMPL_333HZ, SMPL_250HZ, SMPL_200HZ, SMPL_167HZ, SMPL_143HZ, SMPL_125HZ
#define MPU9250_Gyroscope_filter_choice  0x01						//select: 0x00: Enables DLPF with 8kHz sample rate|0x01: Enables DLPF with 1kHz sample rate|0x02 or 0x03: Bypasses DLPF
#define MPU9250_Gyroscope_DLPF_cutoff  DLPF_5HZ 				//select: DLPF_250HZ, DLPF_184HZ, DLPF_92HZ, DLPF_41HZ, DLPF_20HZ, DLPF_10HZ, DLPF_5HZ, DLPF_3600HZ
#define MPU9250_Accelerometer_filter_choice  0x01				//select: 0x01 Enable, 0x00 bypass
#define MPU9250_Accelerometer_DLPF_cutoff  DLPF_5HZ 		//select: DLPF_218HZ_0, DLPF_218HZ_1, DLPF_99HZ, DLPF_45HZ, DLPF_21HZ, DLPF_10HZ, DLPF_5HZ, DLPF_420HZ
#define MPU9250_filter_algorithm	MADGWICK 							//select: MADGWICK, MAHONY, NONE
#define MPU9250_filter_iterations	10										//select: 1-50 higher better but may slow down

// OLED setup (0.91-inch SSD1306, 128x32, Software I2C)
// Reverted to SW_I2C to resolve library conflicts on ESP32-S3 secondary buses. Safe on Core 1.
U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ I2C_OLED_SCL, /* data=*/ I2C_OLED_SDA, /* reset=*/ U8X8_PIN_NONE); // Software I2C on custom ESP32-S3 pins to avoid bus congestion

#define font_10_pixel u8g2_font_t0_15b_me
#define font_8_pixel u8g2_font_helvB08_tf
#define font_5_pixel u8g2_font_spleen5x8_me
#define update_rate_oled 1500

/*//////////////////////////// RTOS Data Structures ////////////////////////////*/

// Binary structure to hold one frame of sensor data safely (49 Bytes)
typedef struct {
    uint32_t timestamp;
    float q[4];
    float accel[3];
    double gps_lat;
    double gps_lng;
    uint8_t event_flag; // NEW: 1 if TAG button was pressed, 0 otherwise
} LogFrame;

// UI State Machine Definitions
enum UIState {
    STATE_LIVE_VIEW,
    STATE_MENU,
    STATE_SUBMENU_MSG,  // A generic state to show temporary messages
    STATE_SUBMENU_SD_INFO    // Updated to match internal loggingTask state naming
};
volatile UIState currentState = STATE_LIVE_VIEW;

#define MENU_ITEMS_COUNT 5
const char* menuItems[MENU_ITEMS_COUNT] = {
    "1.Display Mode",
    "2.Step Feedback",
    "3.SD Card Info",
    "4.Calibration",
    "5.Exit Menu"
};
int8_t menuCursor = 0; // Tracks selected menu item
char subMenuMsg[20] = ""; 


// Inter-Core Communication Flags
volatile bool tag_event_triggered = false;

// FreeRTOS Handles
QueueHandle_t dataQueue;
TaskHandle_t sensorTaskHandle;
TaskHandle_t loggingTaskHandle;

// SD Card Handlers
SdFat sd;
File logFile;

// Function prototypes to avoid scope issues
void performCalibration();
void print_calibration();
void saveCalibration();
void loadCalibration();

/*//////////////////////////// FreeRTOS Tasks ////////////////////////////*/

// Core 0 Task: Strictly for high-speed sensor reading and mathematical fusion
void sensorTask(void *pvParameters) {
  LogFrame frame;
  for(;;) {
    if (mpu.update()) {
      // Pack the struct with highest precision possible
      frame.timestamp = millis();
      frame.q[0] = mpu.getQuaternionW();
      frame.q[1] = mpu.getQuaternionX();
      frame.q[2] = mpu.getQuaternionY();
      frame.q[3] = mpu.getQuaternionZ();
      frame.accel[0] = mpu.getLinearAccX();
      frame.accel[1] = mpu.getLinearAccY();
      frame.accel[2] = mpu.getLinearAccZ();
      frame.gps_lat = 0.0; // Ready for S6MV2 GPS module
      frame.gps_lng = 0.0;
      
      // Thread-safe check for Waypoint Tagging
      if (tag_event_triggered) {
          frame.event_flag = 1;
          tag_event_triggered = false; // Reset the flag after recording
      } else {
          frame.event_flag = 0;
      }
      // Send to queue without blocking. If queue is full, frame drops (keeps real-time integrity)
      xQueueSend(dataQueue, &frame, 0);
    }
    // Yield to scheduler to avoid Watchdog timeout
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

// Core 1 Task: For OLED, Buttons, and heavy Flash/SD writing
void loggingTask(void *pvParameters) {
  LogFrame receivedFrame;
  unsigned long lastDisplayMillis = 0;
  unsigned long lastFlushMillis = 0;
  unsigned long lastBtnCheckMillis = 0; 
  
  // State tracking variables for Edge Detection
  bool selectWasPressed = false;
  bool upWasPressed = false;
  bool downWasPressed = false;
  bool tagWasPressed = false;
  
  // Flag for instant UI rendering
  bool force_update_ui = false;
  
  // Storage variables for SD calculations (Calculated only once)
  uint32_t sd_free_mb = 0;
  float sd_remain_hours = 0.0;

  for(;;) {
    unsigned long currentMillis = millis();

    // === PHASE 1: Button Edge Detection & Debouncing ===
    // Scan buttons every 50ms (faster response, but still debounced)
    bool selectTriggered = false, upTriggered = false, downTriggered = false;
    
    if (currentMillis - lastBtnCheckMillis > 50) {
      
      // TAG Button
      if (digitalRead(BTN_TAG_PIN) == LOW) {
        if (!tagWasPressed) { tag_event_triggered = true; tagWasPressed = true; }
      } else { tagWasPressed = false; }

      // SELECT Button
      if (digitalRead(BTN_SELECT_PIN) == LOW) {
        if (!selectWasPressed) { selectTriggered = true; force_update_ui = true; selectWasPressed = true; }
      } else { selectWasPressed = false; }

      // UP Button
      if (digitalRead(BTN_UP_PIN) == LOW) {
        if (!upWasPressed) { upTriggered = true; force_update_ui = true; upWasPressed = true; }
      } else { upWasPressed = false; }

      // DOWN Button
      if (digitalRead(BTN_DOWN_PIN) == LOW) {
        if (!downWasPressed) { downTriggered = true; force_update_ui = true; downWasPressed = true; }
      } else { downWasPressed = false; }

      lastBtnCheckMillis = currentMillis;
    }

    // === PHASE 2: UI State Machine ===
    if (currentState == STATE_LIVE_VIEW) {
      if (selectTriggered) {
        currentState = STATE_MENU;
        menuCursor = 0;
      }
    } 
    else if (currentState == STATE_MENU) {
      if (upTriggered) {
        menuCursor--;
        if (menuCursor < 0) menuCursor = MENU_ITEMS_COUNT - 1;
      }
      if (downTriggered) {
        menuCursor++;
        if (menuCursor >= MENU_ITEMS_COUNT) menuCursor = 0;
      }
      
      if (selectTriggered) {
        // Handle Menu Actions
        if (menuCursor == 0 || menuCursor == 1) {
            // Future Sub-menus Placeholder (Display Mode & Step Feedback)
            snprintf(subMenuMsg, sizeof(subMenuMsg), "Under Construct!");
            currentState = STATE_SUBMENU_MSG;
            force_update_ui = true;
        }
        else if (menuCursor == 2) {
            // Action: SD Card Info
            u8g2.clearBuffer();
            u8g2.drawStr(10, 20, "Calculating...");
            u8g2.sendBuffer();
            
            // Heavy SPI calculation: Done ONLY ONCE upon entry!
            if (sd.card()->errorCode() == 0) { // Check if SD is actually healthy
                uint32_t freeClusters = sd.vol()->freeClusterCount();
                uint32_t sectorsPerCluster = sd.vol()->sectorsPerCluster();
                
                // 1 Sector = 512 Bytes. 2048 Sectors = 1 Megabyte
                sd_free_mb = (freeClusters * sectorsPerCluster) / 2048; 
                
                // Logging Rate: ~5.6 KB/s = ~20.16 MB/Hour
                sd_remain_hours = (float)sd_free_mb / 20.16;
            } else {
                sd_free_mb = 0;
                sd_remain_hours = 0.0;
            }
            
            currentState = STATE_SUBMENU_SD_INFO;
            force_update_ui = true;
        }
        else if (menuCursor == 3) { 
          // Action: Calibration
          vTaskSuspend(sensorTaskHandle);
          performCalibration();
          saveCalibration();
          vTaskResume(sensorTaskHandle);
          currentState = STATE_LIVE_VIEW; 
        } 
        else if (menuCursor == 4) {
          // Action: Exit Menu
          currentState = STATE_LIVE_VIEW;
          force_update_ui = true;
        }
      }
    }
    // Handle returning to Menu from Sub-menus
    else if (currentState == STATE_SUBMENU_MSG || currentState == STATE_SUBMENU_SD_INFO) {
        // Pressing SELECT in any submenu returns to Menu
        if (selectTriggered) {
            currentState = STATE_MENU;
            force_update_ui = true;
        }
    }

    // === PHASE 3: Pull Data from Queue & SD Logging ===
    if (xQueueReceive(dataQueue, &receivedFrame, pdMS_TO_TICKS(10)) == pdPASS) {
      if (logFile) {
        logFile.write((uint8_t*)&receivedFrame, sizeof(LogFrame));
      }
      if (currentMillis - lastFlushMillis > 5000) { 
        if (logFile) logFile.sync();
        lastFlushMillis = currentMillis;
      }
      
      // === PHASE 4: Graphics Rendering ===
      // Render if time has passed OR if user pressed a button (force update)
      if ((currentMillis - lastDisplayMillis > update_rate_oled) || force_update_ui) {
        force_update_ui = false; // Reset the flag immediately
        u8g2.clearBuffer();
        
        if (currentState == STATE_LIVE_VIEW) {
          char buf[32];
          snprintf(buf, sizeof(buf), "Qw: %.2f", receivedFrame.q[0]);
          u8g2.drawStr(0, 7, buf);
          snprintf(buf, sizeof(buf), "Qx: %.2f", receivedFrame.q[1]);
          u8g2.drawStr(0, 15, buf);
          snprintf(buf, sizeof(buf), "Qy: %.2f", receivedFrame.q[2]);
          u8g2.drawStr(64, 7, buf);
          snprintf(buf, sizeof(buf), "Qz: %.2f", receivedFrame.q[3]);
          u8g2.drawStr(64, 15, buf);
          
          snprintf(buf, sizeof(buf), "T: %.1fC", mpu.getTemperature());
          u8g2.drawStr(0, 28, buf);
        } 
        else if (currentState == STATE_MENU) {
          u8g2.drawStr(0, 8, "--- MENU ---");
          u8g2.drawStr(10, 20, menuItems[menuCursor]);
          u8g2.drawStr(0, 20, ">"); // Cursor
        }
        else if (currentState == STATE_SUBMENU_MSG) {
          u8g2.drawStr(5, 15, subMenuMsg);
          u8g2.drawStr(5, 28, "[Select] to Back");
        }
        else if (currentState == STATE_SUBMENU_SD_INFO) {
          char buf[32];
          snprintf(buf, sizeof(buf), "Free: %lu MB", sd_free_mb);
          u8g2.drawStr(0, 10, buf);
          
          snprintf(buf, sizeof(buf), "Time: %.1f Hrs", sd_remain_hours);
          u8g2.drawStr(0, 20, buf);
          
          u8g2.drawStr(0, 30, "> [Select] to Back");
        }
        
        u8g2.sendBuffer();
        lastDisplayMillis = currentMillis;
      }
    }
  }
}

/*////////////////////////////setup////////////////////////////*/
void setup() 
{ 
	Serial.begin(bud_rate);
	
  // Initialize Hardware I2C for MPU9250 with explicit pins for ESP32-S3
	Wire.begin(I2C_MPU_SDA, I2C_MPU_SCL); 
  Wire.setClock(400000); // Boost I2C to 400kHz for maximum IMU read speed

  // Initialize Secondary Hardware I2C (Wire1) specifically for OLED
  Wire1.begin(I2C_OLED_SDA, I2C_OLED_SCL);
  Wire1.setClock(400000);
	
  // Initialize Buttons with internal pull-ups
  pinMode(BTN_SELECT_PIN, INPUT_PULLUP);
  pinMode(BTN_UP_PIN, INPUT_PULLUP);
  pinMode(BTN_DOWN_PIN, INPUT_PULLUP);
  pinMode(BTN_TAG_PIN, INPUT_PULLUP);
  
  // Initialize OLED
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(font_10_pixel);
  u8g2.drawStr(15, 25, "<< Boot up >>");
  u8g2.sendBuffer();
  delay(1000);
  u8g2.setFont(font_8_pixel); 

  // Initialize SD Card
  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  if (!sd.begin(SD_CS_PIN, SD_SCK_MHZ(SPI_FREQ_MHZ))) {
    Serial.println("SD Card Mount Failed!");
    u8g2.clearBuffer();
    u8g2.drawStr(10, 20, "SD Card Error!");
    u8g2.sendBuffer();
    delay(3000);
  } else {
    Serial.println("SD Card Mounted Successfully.");
    // Open binary log file
    logFile = sd.open("DR_LOG.BIN", FILE_WRITE);
  }

	MPU9250Setting setting;
	// Initialize MPU9250 
	// Sample rate must be at least 2x DLPF rate 
	setting.accel_fs_sel = ACCEL_FS_SEL::MPU9250_Accelerometer_Rang;
  setting.gyro_fs_sel = GYRO_FS_SEL::MPU9250_Gyroscope_Rang;
	setting.mag_output_bits = MAG_OUTPUT_BITS::MPU9250_Magnetometer_resolution; 
	setting.fifo_sample_rate = FIFO_SAMPLE_RATE::MPU9250_fifo_sample_rate; 
	setting.gyro_fchoice = MPU9250_Gyroscope_filter_choice; 
	setting.gyro_dlpf_cfg = GYRO_DLPF_CFG::MPU9250_Gyroscope_DLPF_cutoff; 
	setting.accel_fchoice = MPU9250_Accelerometer_filter_choice;
  setting.accel_dlpf_cfg = ACCEL_DLPF_CFG::MPU9250_Accelerometer_DLPF_cutoff;
  
  //start to setup the MPU based on setting and address
  while (!mpu.setup(MPU9250_IMU_ADDRESS, setting)) {
    Serial.println("MPU connection failed. Retrying in 5 seconds...");
    u8g2.clearBuffer();
    u8g2.drawStr(20, 15, "MPU9250 Failed!");
    u8g2.drawStr(20, 28, "Retrying...");
    u8g2.sendBuffer();
    delay(5000); 
  }							
	mpu.setMagneticDeclination(MAGNETIC_DECLINATION); 
	mpu.selectFilter(QuatFilterSel::MPU9250_filter_algorithm); 
	mpu.setFilterIterations(MPU9250_filter_iterations);
  
  // Initialize EEPROM for ESP32
  EEPROM.begin(128); // Allocate 128 bytes for EEPROM wrapper

  // Load calibration from EEPROM on startup
  Serial.println("Loading calibration from EEPROM...");
  u8g2.clearBuffer();
  u8g2.drawStr(25, 20, "Loading calibration");
  u8g2.drawStr(35, 30, "from EEPROM");
  u8g2.sendBuffer();
  loadCalibration();
  print_calibration();
  u8g2.clearBuffer();
  u8g2.drawStr(25, 15, "Press Button");
  u8g2.drawStr(35, 25, "to Calibrate");
  u8g2.sendBuffer();
  u8g2.setFont(font_5_pixel); 

  // Create queue capable of buffering 300 frames (~3 seconds of data at 100Hz)
  dataQueue = xQueueCreate(300, sizeof(LogFrame));
  
  // Pin Sensor Task to Core 0 (Highest Priority)
  xTaskCreatePinnedToCore(sensorTask, "SensorTask", 4096, NULL, 2, &sensorTaskHandle, 0);
  
  // Pin Logging Task to Core 1
  xTaskCreatePinnedToCore(loggingTask, "LoggingTask", 8192, NULL, 1, &loggingTaskHandle, 1);
} 

/*////////////////////////////loop////////////////////////////*/
void loop() 
{ 
  // Main loop is intentionally left empty.
  // FreeRTOS tasks now manage the entire system architecture.
  vTaskDelete(NULL);
}

// =========================================================================
// CALIBRATION FUNCTIONS 
// =========================================================================

void performCalibration() {
  u8g2.clearBuffer();
  u8g2.drawStr(0, 15, "Accel/Gyro Cal");
  u8g2.drawStr(0, 25, "Keep Still 5s");
  u8g2.sendBuffer();
  mpu.verbose(true);
  delay(5000);
  mpu.calibrateAccelGyro();
  u8g2.clearBuffer();
  u8g2.drawStr(0, 15, "Mag Cal");
  u8g2.drawStr(0, 25, "Figure 8 - 5s");
  u8g2.sendBuffer();
  delay(5000);
  mpu.calibrateMag();
  mpu.verbose(false);
}

void print_calibration() {
  Serial.println("< calibration parameters >");
  Serial.println("accel bias [g]: ");
  Serial.print(mpu.getAccBiasX() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY); Serial.print(", ");
  Serial.print(mpu.getAccBiasY() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY); Serial.print(", ");
  Serial.println(mpu.getAccBiasZ() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY);
  Serial.println("gyro bias [deg/s]: ");
  Serial.print(mpu.getGyroBiasX() / (float)MPU9250::CALIB_GYRO_SENSITIVITY); Serial.print(", ");
  Serial.print(mpu.getGyroBiasY() / (float)MPU9250::CALIB_GYRO_SENSITIVITY); Serial.print(", ");
  Serial.println(mpu.getGyroBiasZ() / (float)MPU9250::CALIB_GYRO_SENSITIVITY);
  Serial.println("mag bias [mG]: ");
  Serial.print(mpu.getMagBiasX()); Serial.print(", ");
  Serial.print(mpu.getMagBiasY()); Serial.print(", ");
  Serial.println(mpu.getMagBiasZ());
  Serial.println("mag scale []: ");
  Serial.print(mpu.getMagScaleX()); Serial.print(", ");
  Serial.print(mpu.getMagScaleY()); Serial.print(", ");
  Serial.println(mpu.getMagScaleZ());
  
  u8g2.clearBuffer();
  char buf[32];
  snprintf(buf, sizeof(buf), "Acc: %.2f,%.2f,%.2f",
           mpu.getAccBiasX() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY,
           mpu.getAccBiasY() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY,
           mpu.getAccBiasZ() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY);
  u8g2.drawStr(0, 15, buf);
  snprintf(buf, sizeof(buf), "Gyr: %.1f,%.1f,%.1f",
           mpu.getGyroBiasX() / (float)MPU9250::CALIB_GYRO_SENSITIVITY,
           mpu.getGyroBiasY() / (float)MPU9250::CALIB_GYRO_SENSITIVITY,
           mpu.getGyroBiasZ() / (float)MPU9250::CALIB_GYRO_SENSITIVITY);
  u8g2.drawStr(0, 25, buf);
  u8g2.sendBuffer();
  delay(2000); 
}

void saveCalibration() {
  int addr = 0;
  EEPROM.put(addr, mpu.getAccBiasX()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getAccBiasY()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getAccBiasZ()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getGyroBiasX()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getGyroBiasY()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getGyroBiasZ()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getMagBiasX()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getMagBiasY()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getMagBiasZ()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getMagScaleX()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getMagScaleY()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getMagScaleZ()); addr += sizeof(float);
  EEPROM.commit();
}

void loadCalibration() {
  // Since setter methods are unavailable, we only read and verify calibration data
  // UPDATE: Applying values using library setters to ensure Madgwick filter runs accurately
  int addr = 0;
  float accBiasX, accBiasY, accBiasZ;
  float gyroBiasX, gyroBiasY, gyroBiasZ;
  float magBiasX, magBiasY, magBiasZ;
  float magScaleX, magScaleY, magScaleZ;

  EEPROM.get(addr, accBiasX); addr += sizeof(float);
  EEPROM.get(addr, accBiasY); addr += sizeof(float);
  EEPROM.get(addr, accBiasZ); addr += sizeof(float);
  EEPROM.get(addr, gyroBiasX); addr += sizeof(float);
  EEPROM.get(addr, gyroBiasY); addr += sizeof(float);
  EEPROM.get(addr, gyroBiasZ); addr += sizeof(float);
  EEPROM.get(addr, magBiasX); addr += sizeof(float);
  EEPROM.get(addr, magBiasY); addr += sizeof(float);
  EEPROM.get(addr, magBiasZ); addr += sizeof(float);
  EEPROM.get(addr, magScaleX); addr += sizeof(float);
  EEPROM.get(addr, magScaleY); addr += sizeof(float);
  EEPROM.get(addr, magScaleZ); addr += sizeof(float);

  // Apply the loaded biases to the MPU object
  mpu.setAccBias(accBiasX, accBiasY, accBiasZ);
  mpu.setGyroBias(gyroBiasX, gyroBiasY, gyroBiasZ);
  mpu.setMagBias(magBiasX, magBiasY, magBiasZ);
  mpu.setMagScale(magScaleX, magScaleY, magScaleZ);
  
  // Print loaded values for verification
  Serial.println("Loaded calibration values from EEPROM:");
  Serial.print("Acc Bias X: "); Serial.println(accBiasX);
  Serial.print("Acc Bias Y: "); Serial.println(accBiasY);
  Serial.print("Acc Bias Z: "); Serial.println(accBiasZ);
  Serial.print("Gyro Bias X: "); Serial.println(gyroBiasX);
  Serial.print("Gyro Bias Y: "); Serial.println(gyroBiasY);
  Serial.print("Gyro Bias Z: "); Serial.println(gyroBiasZ);
  Serial.print("Mag Bias X: "); Serial.println(magBiasX);
  Serial.print("Mag Bias Y: "); Serial.println(magBiasY);
  Serial.print("Mag Bias Z: "); Serial.println(magBiasZ);
  Serial.print("Mag Scale X: "); Serial.println(magScaleX);
  Serial.print("Mag Scale Y: "); Serial.println(magScaleY);
  Serial.print("Mag Scale Z: "); Serial.println(magScaleZ);
  
  Serial.println("STATUS: Calibration successfully applied to internal filter.");
}