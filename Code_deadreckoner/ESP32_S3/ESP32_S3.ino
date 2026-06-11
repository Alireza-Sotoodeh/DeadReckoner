// Last Edit: 2026-06-11 18:50:00  
// Reason for Last Edit: Patched trailing standalone brace, fully integrated dynamic 7-item scrolling SD menu with absolute redirection and feedback.
// Author: Alireza Sotoodeh

/*
 * =========================================================================
 * PROJECT: DeadReckoner
 * VERSION: 1.9 (Advanced SD Manager UI + Stealth Mode + CC LED & Active Buzzer)
 * * WIRING DIAGRAM
 * -------------------------------------------------------------------------
 * Component Pin | MCU Pin       | Note / Hardware Reasoning
 * -------------------------------------------------------------------------
 * MPU9250 VCC   | 3.3V          | Clean 3.3V power rail
 * MPU9250 GND   | GND           | Common system ground
 * MPU9250 SCL   | GPIO 5        | Hardware I2C (Wire) Clock (Requires external 4.7k pull-up)
 * MPU9250 SDA   | GPIO 4        | Hardware I2C (Wire) Data (Requires external 4.7k pull-up)
 * MPU9250 AD0   | GND           | Forces I2C Address to 0x68
 * MPU9250 NCS   | 3.3V          | SPI disable, forces I2C mode
 * MPU9250 FSYNC | GND           | Tied to GND to prevent floating noise
 * -------------------------------------------------------------------------
 * OLED VCC      | 3.3V          | 0.91-inch SSD1306 Power
 * OLED GND      | GND           | Common system ground
 * OLED SCL      | GPIO 7        | Software I2C (U8g2 Bit-bang on Core 1)
 * OLED SDA      | GPIO 6        | Software I2C (U8g2 Bit-bang on Core 1)
 * -------------------------------------------------------------------------
 * SD Card 3V3   | 3.3V          | DIRECT 3.3V ONLY (Bypassing 5V regulators)
 * SD Card GND   | GND           | Common system ground
 * SD Card CS    | GPIO 10       | FSPI CS0 (High-speed line)
 * SD Card MOSI  | GPIO 11       | FSPI MOSI
 * SD Card SCK   | GPIO 12       | FSPI SCK (Running at 20MHz)
 * SD Card MISO  | GPIO 13       | FSPI MISO
 * -------------------------------------------------------------------------
 * BTN SELECT    | GPIO 1        | Menu Enter/Toggle (Active-Low, Internal Pull-up)
 * BTN UP        | GPIO 2        | Menu Navigation (Active-Low, Internal Pull-up)
 * BTN DOWN      | GPIO 8        | Menu Navigation (Active-Low, Internal Pull-up)
 * BTN TAG       | GPIO 9        | Waypoint Marker (Active-Low, Internal Pull-up)
 * -------------------------------------------------------------------------
 * BUZZER (+)    | GPIO 21       | Active Buzzer (Driven via digitalWrite HIGH, use 100-ohm series resistor)
 * BUZZER (-)    | GND           | Common ground return
 * LED RED       | GPIO 17       | Critical Error Warning (Requires 220~330 ohm series resistor)
 * LED GREEN     | GPIO 18       | TAG Event Indicator (Requires 220~330 ohm series resistor)
 * LED CATHODE   | GND           | Center long pin of the 3-pin Common Cathode LED
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

// MPU9250
#define MPU9250_IMU_ADDRESS 0x68 												// Specifies the I2C slave address of the MPU9250:0x68 GND / 0x69 High
#define MAGNETIC_DECLINATION 3.4	 											// angle between magnetic north and true north
#define INTERVAL_MS_PRINT 50 														// Sets the minimum time interval between printing sensor data.
// Notification Pins
#define BUZZER_PIN 21
#define LED_RED_PIN 17    // Connect via 330-ohm resistor! (Common Cathode)
#define LED_GREEN_PIN 18  // Connect via 330-ohm resistor! (Common Cathode)

MPU9250 mpu; // handler: allowing access to all library methods
unsigned long lastPrintMillis = 0; // Tracks the timestamp (from millis()) of the last time sensor data was printed to Serial

// MPU9250 setting
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

// OLED setup (0.91-inch SSD1306, 128x32, Software I2C) Safe on Core 1.
U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ I2C_OLED_SCL, /* data=*/ I2C_OLED_SDA, /* reset=*/ U8X8_PIN_NONE);
#define font_10_pixel u8g2_font_t0_15b_me
#define font_8_pixel u8g2_font_helvB08_tf
#define font_5_pixel u8g2_font_spleen5x8_me
#define update_rate_oled 1500
#define OLED_SLEEP_TIMEOUT_MS 20000                       // default Time in milliseconds before OLED sleeps

// Notification Timings
#define ALARM_BEEP_MS 100       // Duration of error beeps during SD failure      
#define TAG_BEEP_MS 50          // Duration of short beep for Waypoint TAG
#define TAG_BLINK_MS 100        // Duration of Green LED flash for TAG
#define Recheck_SD_MS 3000      // Wait time before retrying SD initialization

/*//////////////////////////// RTOS Data Structures ////////////////////////////*/

// Binary structure to hold one frame of sensor data safely (49 Bytes)
typedef struct {
    uint32_t timestamp;
    float q[4];
    float accel[3];
    double gps_lat;
    double gps_lng;
    uint8_t event_flag; // 1 if TAG button was pressed, 0 otherwise
} LogFrame;
// UI State Machine Definitions
enum UIState {
    STATE_LIVE_VIEW,
    STATE_MENU,
    STATE_SUBMENU_MSG,  
    STATE_SUBMENU_SD_INFO,    
    STATE_SUBMENU_DISPLAY,
    STATE_SUBMENU_MUTE,
    STATE_CONFIRM_FORMAT,
    STATE_CONFIRM_CREATE_FILE      
};
volatile UIState currentState = STATE_LIVE_VIEW;

#define MENU_ITEMS_COUNT 5
const char* menuItems[MENU_ITEMS_COUNT] = {
    "1.Display Mode",
    "2.Mute Sounds",
    "3.SD Card Info",
    "4.Calibration",
    "5.Exit Menu"
};
int8_t menuCursor = 0; // Tracks selected menu item
char subMenuMsg[20] = ""; 

// Inter-Core Communication Flags
volatile bool tag_event_triggered = false;
volatile bool mpu_critical_error = false;
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
  unsigned long last_mpu_data_time = millis(); // Track last successful read

  for(;;) {
    // If a critical error was flagged, halt sensor reading to save CPU
    if (mpu_critical_error) {
        vTaskDelay(pdMS_TO_TICKS(100));
        continue; 
    }

    if (mpu.update()) {
      last_mpu_data_time = millis(); // Reset timeout counter
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
    } else {
      // DETECT SENSOR DISCONNECTION: No new data for 500ms
      if (millis() - last_mpu_data_time > 500) {
          mpu_critical_error = true;
      }
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
  int8_t displayCursor = 0; // Tracks selection inside Display Mode submenu
  int8_t muteCursor = 0; // Tracks selection inside Mute Sounds submenu
  int8_t confirmCursor = 1; // Tracks choice in Format Confirm menu (Default: 1 = NO)
  int8_t sdMenuCursor = 0; // Tracks selection inside the 7-item SD Submenu
  int8_t sdScrollOffset = 0; // Manages the scrolling window for 128x32 OLED
  uint16_t totalFilesCount = 0; // Stores computed BIN files count on SD

  // State tracking variables for Edge Detection
  bool selectWasPressed = false;
  bool upWasPressed = false;
  bool downWasPressed = false;
  bool tagWasPressed = false;
  // Flag for instant UI rendering
  bool force_update_ui = false;
  // Storage variables for SD calculations
  uint32_t sd_free_mb = 0;
  float sd_remain_hours = 0.0;
  float sd_total_gb = 0.0;
  // === Power Management Variables ===
  bool is_oled_sleeping = false;
  bool auto_off_enabled = true; // Default: OLED sleeps after 20s
  unsigned long last_interaction_millis = millis();
  // === Sound & Notification Management Variables ===
  bool is_muted = false; // Default: Sounds are ON
  unsigned long buzzer_turn_off_time = 0;
  unsigned long led_turn_off_time = 0;
  bool is_buzzer_on = false;
  bool is_led_on = false;
  
  for(;;) {
    // === INTERCEPTOR: CRITICAL MPU DISCONNECT ERROR ===
    if (mpu_critical_error) {
        // 1. Safely close the SD log file to prevent data corruption
        if (logFile) {
            logFile.sync();
            logFile.close(); 
        }
        
        // 2. Force wake OLED and display absolute error
        is_oled_sleeping = false;
        u8g2.setPowerSave(0); 
        u8g2.clearBuffer();
        u8g2.drawStr(5, 15, "CRITICAL ERROR!");
        u8g2.drawStr(0, 28, "MPU DISCONNECTED");
        u8g2.sendBuffer();

        // 3. Infinite SOS Trap Loop (100ms ON / 50ms OFF)
        while(true) {
            digitalWrite(LED_RED_PIN, HIGH);
            digitalWrite(BUZZER_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(100)); // Non-blocking RTOS delay
            digitalWrite(LED_RED_PIN, LOW);
            digitalWrite(BUZZER_PIN, LOW);
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
    unsigned long currentMillis = millis();
    // === PHASE 0: Non-Blocking Hardware Notifications ===
    if (is_buzzer_on && (currentMillis >= buzzer_turn_off_time)) {
        digitalWrite(BUZZER_PIN, LOW);
        is_buzzer_on = false;
    }
    if (is_led_on && (currentMillis >= led_turn_off_time)) {
        digitalWrite(LED_GREEN_PIN, LOW); // Turn off Green LED
        is_led_on = false;
    }

    // === PHASE 1: Button Edge Detection & Debouncing ===
    bool selectTriggered = false, upTriggered = false, downTriggered = false;
    if (currentMillis - lastBtnCheckMillis > 50) {
      
      // TAG Button (Independent of OLED Sleep state)
      if (digitalRead(BTN_TAG_PIN) == LOW) {
        if (!tagWasPressed) { 
            tag_event_triggered = true;
            tagWasPressed = true;
            last_interaction_millis = currentMillis; 
            
            // Trigger Fast Feedback (Non-Blocking)
            if (!is_muted) { 
                digitalWrite(BUZZER_PIN, HIGH);
                is_buzzer_on = true;
                buzzer_turn_off_time = currentMillis + TAG_BEEP_MS; 
            }
            
            // Green LED always flashes for TAG, even in stealth mode
            digitalWrite(LED_GREEN_PIN, HIGH);
            is_led_on = true;
            led_turn_off_time = currentMillis + TAG_BLINK_MS; 
        }
      } else { tagWasPressed = false; }

      // SELECT Button
      if (digitalRead(BTN_SELECT_PIN) == LOW) {
        if (!selectWasPressed) { 
            selectTriggered = true;
            force_update_ui = true; selectWasPressed = true;
            last_interaction_millis = currentMillis; 
        }
      } else { selectWasPressed = false; }

      // UP Button
      if (digitalRead(BTN_UP_PIN) == LOW) {
        if (!upWasPressed) { 
            upTriggered = true;
            force_update_ui = true; upWasPressed = true;
            last_interaction_millis = currentMillis; 
        }
      } else { upWasPressed = false; }

      // DOWN Button
      if (digitalRead(BTN_DOWN_PIN) == LOW) {
        if (!downWasPressed) { 
            downTriggered = true;
            force_update_ui = true; downWasPressed = true;
            last_interaction_millis = currentMillis; 
        }
      } else { downWasPressed = false; }

      lastBtnCheckMillis = currentMillis;
    }

    // === INTERCEPTOR: The First-Press Trap Solver ===
    if (is_oled_sleeping && (selectTriggered || upTriggered || downTriggered)) {
        // WAKE UP SEQUENCE
        is_oled_sleeping = false;
        u8g2.setPowerSave(0); // Hardware wake-up command
        currentState = STATE_LIVE_VIEW; // Return to Home Principle
        force_update_ui = true;
        // CONSUME the button presses so they don't leak into Phase 2 (Menu)
        selectTriggered = false;
        upTriggered = false;
        downTriggered = false;
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
        if (menuCursor == 0) {
            // Action: Enter Display Mode Submenu instead of toggling instantly
            currentState = STATE_SUBMENU_DISPLAY;
            displayCursor = auto_off_enabled ? 1 : 0;
            force_update_ui = true;
            last_interaction_millis = currentMillis;
        }
        else if (menuCursor == 1) {
            // Action: Enter Mute Sounds Submenu instead of toggling instantly
            currentState = STATE_SUBMENU_MUTE;
            muteCursor = is_muted ? 1 : 0; // Focus on current system state
            force_update_ui = true;
            last_interaction_millis = currentMillis;
        }
        else if (menuCursor == 2) {
            // Action: Enter SD Manager Submenu & Compute statistics dynamically
            u8g2.clearBuffer();
            u8g2.drawStr(10, 20, "Calculating...");
            u8g2.sendBuffer();
            
            if (sd.card()->errorCode() == 0) { 
                uint32_t freeClusters = sd.vol()->freeClusterCount();
                uint32_t totalClusters = sd.vol()->clusterCount();
                uint32_t sectorsPerCluster = sd.vol()->sectorsPerCluster();
                sd_free_mb = (freeClusters * sectorsPerCluster) / 2048;
                sd_remain_hours = (float)sd_free_mb / 20.16;
                uint32_t sd_total_mb = (totalClusters * sectorsPerCluster) / 2048;
                sd_total_gb = (float)sd_total_mb / 1024.0; 
                // Count binary logs safely
                totalFilesCount = 0;
                char checkBuf[20];
                for (int i = 1; i <= 999; i++) {
                    snprintf(checkBuf, sizeof(checkBuf), "DR_LOG_%03d.BIN", i);
                    if (sd.exists(checkBuf)) totalFilesCount++;
                }
            } else {
                sd_free_mb = 0;
                sd_remain_hours = 0.0; totalFilesCount = 0;
            }
            
            currentState = STATE_SUBMENU_SD_INFO;
            sdMenuCursor = 0; // Reset scroll cursors
            sdScrollOffset = 0;
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
    else if (currentState == STATE_SUBMENU_MSG) {
        if (selectTriggered) {
            currentState = STATE_MENU;
            force_update_ui = true;
        }
    }
    else if (currentState == STATE_SUBMENU_SD_INFO) {
      // Navigation inside the advanced 7-item SD menu
      if (upTriggered) {
        sdMenuCursor--;
        if (sdMenuCursor < 0) sdMenuCursor = 6; // Loop back to 7th item (Back)
        force_update_ui = true;
      }
      if (downTriggered) {
        sdMenuCursor++;
        if (sdMenuCursor > 6) sdMenuCursor = 0; // Loop back to 1st item (Total)
        force_update_ui = true;
      }
      
      // Calculate scroll offset dynamically for 3-line display window
      if (sdMenuCursor < sdScrollOffset) {
        sdScrollOffset = sdMenuCursor;
      } else if (sdMenuCursor >= sdScrollOffset + 3) {
        sdScrollOffset = sdMenuCursor - 2;
      }

      // SELECT Actions based on cursor position
      if (selectTriggered) {
        if (sdMenuCursor >= 0 && sdMenuCursor <= 3) {
          // Lines 1 to 4: Any info selection jumps directly to Live View
          currentState = STATE_LIVE_VIEW;
          force_update_ui = true;
          last_interaction_millis = currentMillis;
        }
        else if (sdMenuCursor == 4) {
          // Option 5: Create New File -> Enter confirmation trap
          currentState = STATE_CONFIRM_CREATE_FILE;
          confirmCursor = 1; // Default focus on NO for safety
          force_update_ui = true;
        }
        else if (sdMenuCursor == 5) {
          // Option 6: Format / Clear -> Enter confirmation trap
          currentState = STATE_CONFIRM_FORMAT;
          confirmCursor = 1; // Default focus on NO for safety
          force_update_ui = true;
        }
        else if (sdMenuCursor == 6) {
          // Option 7: Back to Menu -> Safe escape to main menu
          currentState = STATE_MENU;
          menuCursor = 2; // Keep main menu cursor on "3.SD Card Info"
          force_update_ui = true;
        }
      }
    }
    else if (currentState == STATE_SUBMENU_DISPLAY) {
      if (upTriggered || downTriggered) {
        displayCursor = (displayCursor == 0) ? 1 : 0;
        force_update_ui = true;
      }
      
      if (selectTriggered) {
        if (displayCursor == 0) {
          auto_off_enabled = false; // Always ON
        } else {
          auto_off_enabled = true; // Auto Off 20s
        }
        
        currentState = STATE_LIVE_VIEW; // Direct redirect to Home
        force_update_ui = true;
        last_interaction_millis = currentMillis;
      }
    }
    else if (currentState == STATE_SUBMENU_MUTE) {
      // Navigation inside Mute Menu
      if (upTriggered || downTriggered) {
        muteCursor = (muteCursor == 0) ? 1 : 0;
        force_update_ui = true;
      }
      
      // Selection inside Mute Menu
      if (selectTriggered) {
        if (muteCursor == 0) {
          is_muted = false; // Sounds: ON
        } else {
          is_muted = true; // Sounds: MUTED
        }
        
        currentState = STATE_LIVE_VIEW; // Direct redirect to Home
        force_update_ui = true;
        last_interaction_millis = currentMillis;
      }
    } 
    else if (currentState == STATE_CONFIRM_CREATE_FILE) {
      // Confirmation trap for creating a new file
      if (upTriggered || downTriggered) {
        confirmCursor = (confirmCursor == 0) ? 1 : 0;
        force_update_ui = true;
      }
      if (selectTriggered) {
        if (confirmCursor == 1) {
          currentState = STATE_SUBMENU_SD_INFO; // Cancel -> Back to SD Info
          force_update_ui = true;
        } else {
          // Execute Auto-Sequential New File Creation
          if (logFile) logFile.close();
          char filename[20] = "DR_LOG_001.BIN";
          int fileNum = 1;
          while (sd.exists(filename)) {
            fileNum++;
            snprintf(filename, sizeof(filename), "DR_LOG_%03d.BIN", fileNum);
          }
          logFile = sd.open(filename, FILE_WRITE);
          // Render instant feedback to the user on screen
          u8g2.clearBuffer();
          char flashBuf[25];
          snprintf(flashBuf, sizeof(flashBuf), "Created: LOG_%03d", fileNum);
          u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(flashBuf)) / 2, 20, flashBuf);
          u8g2.sendBuffer();
          // Sound effect verification
          if (!is_muted) {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(100); digitalWrite(BUZZER_PIN, LOW);
          } else { delay(1000); }
          
          currentState = STATE_LIVE_VIEW; // Direct redirect to Home
          force_update_ui = true;
          last_interaction_millis = currentMillis;
        }
      }
    }
    else if (currentState == STATE_CONFIRM_FORMAT) {
      // Confirmation trap for formatting/deleting logs
      if (upTriggered || downTriggered) {
        confirmCursor = (confirmCursor == 0) ? 1 : 0;
        force_update_ui = true;
      }
      if (selectTriggered) {
        if (confirmCursor == 1) {
          currentState = STATE_SUBMENU_SD_INFO; // Cancel -> Back to SD Info
          force_update_ui = true;
        } else {
          // Execute Smart Delete Protocol
          u8g2.clearBuffer();
          u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth("Clearing Logs...")) / 2, 20, "Clearing Logs...");
          u8g2.sendBuffer();
          
          if (logFile) logFile.close();
          char delFilename[20];
          for (int i = 1; i <= 999; i++) {
            snprintf(delFilename, sizeof(delFilename), "DR_LOG_%03d.BIN", i);
            if (sd.exists(delFilename)) {
              sd.remove(delFilename);
            }
          }
          logFile = sd.open("DR_LOG_001.BIN", FILE_WRITE);
          u8g2.clearBuffer();
          u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth("All Logs Cleared!")) / 2, 20, "All Logs Cleared!");
          u8g2.sendBuffer();
          delay(1200);
          
          currentState = STATE_LIVE_VIEW; // Direct redirect to Home after format
          force_update_ui = true;
          last_interaction_millis = currentMillis;
        }
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
      
      // === POWER MANAGEMENT TRIGGER ===
      if (auto_off_enabled && !is_oled_sleeping && (currentMillis - last_interaction_millis > OLED_SLEEP_TIMEOUT_MS)) {
          is_oled_sleeping = true;
          u8g2.setPowerSave(1); // Hardware sleep command
          currentState = STATE_LIVE_VIEW;
      }

      // === PHASE 4: Graphics Rendering ===
      if (!is_oled_sleeping) {
          if ((currentMillis - lastDisplayMillis > update_rate_oled) || force_update_ui) {
            force_update_ui = false;
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
              u8g2.drawStr(0, 20, ">");
            }
            else if (currentState == STATE_SUBMENU_MSG) {
              u8g2.drawStr(5, 15, subMenuMsg);
              u8g2.drawStr(5, 28, "[Select] to Back");
            }
            else if (currentState == STATE_SUBMENU_SD_INFO) {
              // Construct the 7 lines array in memory dynamically
              char lines[7][32];
              snprintf(lines[0], 32, "1.Total: %.1f GB", sd_total_gb); 
              snprintf(lines[1], 32, "2.Free: %lu MB", sd_free_mb);
              snprintf(lines[2], 32, "3.Time: %.1f Hrs", sd_remain_hours);
              snprintf(lines[3], 32, "4.Files Count: %u", totalFilesCount);
              snprintf(lines[4], 32, "5.Create New File");
              snprintf(lines[5], 32, "6.Format / Clear");
              snprintf(lines[6], 32, "7.Back to Menu");
              // Render only the 3 lines inside the scrolling window matrix
              for (int i = 0; i < 3; i++) {
                int lineIndex = sdScrollOffset + i;
                int yPos = 10 + (i * 10); // Spacing for 32px display
                u8g2.drawStr(12, yPos, lines[lineIndex]);
                // Draw cursor arrow on the correctly focused item
                if (lineIndex == sdMenuCursor) {
                  u8g2.drawStr(2, yPos, ">");
                }
              }
            }
            else if (currentState == STATE_CONFIRM_CREATE_FILE) {
              u8g2.drawStr(0, 8, "Create New File?");
              u8g2.drawStr(20, 22, "YES");
              u8g2.drawStr(20, 32, "NO");
              u8g2.drawStr(8, (confirmCursor == 0) ? 22 : 32, ">");
            }
            else if (currentState == STATE_CONFIRM_FORMAT) {
              u8g2.drawStr(0, 8, "Clear All Logs?");
              u8g2.drawStr(20, 22, "YES");
              u8g2.drawStr(20, 32, "NO");
              u8g2.drawStr(8, (confirmCursor == 0) ? 22 : 32, ">");
            }
            else if (currentState == STATE_SUBMENU_DISPLAY) {
              u8g2.drawStr(0, 8, "Disp Mode:");
              u8g2.drawStr(15, 20, "Always ON");
              u8g2.drawStr(15, 30, "Auto Off 20s");
              u8g2.drawStr(3, (displayCursor == 0) ? 20 : 30, ">");
            }
            else if (currentState == STATE_SUBMENU_MUTE) {
              u8g2.drawStr(0, 8, "Sound Opt:");
              u8g2.drawStr(15, 20, "Sounds: ON");
              u8g2.drawStr(15, 30, "Sounds: MUTED");
              u8g2.drawStr(3, (muteCursor == 0) ? 20 : 30, ">");
            }
            
            u8g2.sendBuffer();
            lastDisplayMillis = currentMillis;
          }
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

  // Initialize Notification Hardware (Standard LED & Buzzer) FIRST!
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  pinMode(LED_RED_PIN, OUTPUT);
  digitalWrite(LED_RED_PIN, LOW);
  pinMode(LED_GREEN_PIN, OUTPUT);
  digitalWrite(LED_GREEN_PIN, LOW);

  // Initialize SD Card with Critical Error Feedback (Blocking Loop)
  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  // Trap the system here indefinitely until the SD card is successfully mounted
  while (!sd.begin(SD_CS_PIN, SD_SCK_MHZ(SPI_FREQ_MHZ))) {
    Serial.println("CRITICAL: SD Card Mount Failed! Waiting for SD...");
    // Render Error Message on OLED (Center-Aligned)
    u8g2.clearBuffer();
    const char* line1 = "SD Card Error!";
    const char* line2 = "Insert/Check SD";
    int x1 = (u8g2.getDisplayWidth() - u8g2.getStrWidth(line1)) / 2;
    int x2 = (u8g2.getDisplayWidth() - u8g2.getStrWidth(line2)) / 2;
    u8g2.drawStr(x1, 12, line1);
    u8g2.drawStr(x2, 28, line2);
    u8g2.sendBuffer();
    // Hardware Alarm: Red LED + Beeps using global parameter
    digitalWrite(LED_RED_PIN, HIGH);
    for(int i = 0; i < 2; i++) {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(ALARM_BEEP_MS);
      digitalWrite(BUZZER_PIN, LOW);
      delay(ALARM_BEEP_MS);
    }
    
    delay(Recheck_SD_MS);
    digitalWrite(LED_RED_PIN, LOW); // Reset LED state for next check cycle
  } 
  
  // === Dynamic Sequential Logging Architecture ===
  // Render "Scanning SD..." while the ESP computes existing files
  u8g2.clearBuffer();
  u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth("Scanning SD...")) / 2, 20, "Scanning SD...");
  u8g2.sendBuffer();

  char filename[20] = "DR_LOG_001.BIN";
  int fileNum = 1;

  // Scan the SD root directory to find the next available sequential number
  while (sd.exists(filename)) {
    fileNum++;
    if (fileNum > 999) {
      Serial.println("ERROR: Log file limit reached (999). Overwriting DR_LOG_999.BIN");
      break;
    }
    snprintf(filename, sizeof(filename), "DR_LOG_%03d.BIN", fileNum);
  }

  // Open/Create the new unique sequential file
  logFile = sd.open(filename, FILE_WRITE);
  if (logFile) {
    Serial.print("Success: Opened new log file -> ");
    Serial.println(filename);
    
    // Render dynamic SD statistics before mission starts
    u8g2.clearBuffer();
    char scanBuf[25];
    snprintf(scanBuf, sizeof(scanBuf), "Found: %d Logs", fileNum - 1);
    u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(scanBuf)) / 2, 12, scanBuf);
    
    snprintf(scanBuf, sizeof(scanBuf), "Next: LOG_%03d", fileNum);
    u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(scanBuf)) / 2, 28, scanBuf);
    u8g2.sendBuffer();
    delay(2000); // 2-second delay to let the user read the info
    
  } else {
    Serial.println("CRITICAL ERROR: Failed to create sequential log file!");
  }

  MPU9250Setting setting;
  setting.accel_fs_sel = ACCEL_FS_SEL::MPU9250_Accelerometer_Rang;
  setting.gyro_fs_sel = GYRO_FS_SEL::MPU9250_Gyroscope_Rang;
  setting.mag_output_bits = MAG_OUTPUT_BITS::MPU9250_Magnetometer_resolution; 
  setting.fifo_sample_rate = FIFO_SAMPLE_RATE::MPU9250_fifo_sample_rate; 
  setting.gyro_fchoice = MPU9250_Gyroscope_filter_choice; 
  setting.gyro_dlpf_cfg = GYRO_DLPF_CFG::MPU9250_Gyroscope_DLPF_cutoff;
  setting.accel_fchoice = MPU9250_Accelerometer_filter_choice;
  setting.accel_dlpf_cfg = ACCEL_DLPF_CFG::MPU9250_Accelerometer_DLPF_cutoff;

  while (!mpu.setup(MPU9250_IMU_ADDRESS, setting)) {
    Serial.println("CRITICAL: MPU connection failed. Check Wiring!");
    u8g2.clearBuffer();
    u8g2.drawStr(15, 15, "MPU9250 Failed!");
    u8g2.drawStr(20, 28, "Check Wiring");
    u8g2.sendBuffer();
    
    // Fast SOS pattern for Boot-time MPU Error
    for(int i = 0; i < 15; i++) {
        digitalWrite(LED_RED_PIN, HIGH);
        digitalWrite(BUZZER_PIN, HIGH);
        delay(100);
        digitalWrite(LED_RED_PIN, LOW);
        digitalWrite(BUZZER_PIN, LOW);
        delay(50);
    }
    delay(1000); // Brief pause before retrying
  }						
  mpu.setMagneticDeclination(MAGNETIC_DECLINATION); 
  mpu.selectFilter(QuatFilterSel::MPU9250_filter_algorithm); 
  mpu.setFilterIterations(MPU9250_filter_iterations);
  // Initialize EEPROM for ESP32 (Allocate 128 bytes)
  EEPROM.begin(128);
  // Load calibration from EEPROM on startup
  Serial.println("Loading calibration from EEPROM...");

  loadCalibration();
  print_calibration();
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
  // Main loop is intentionally left empty. FreeRTOS tasks now manage the entire system architecture.
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
  Serial.print(mpu.getGyroBiasX() / (float)MPU9250::CALIB_GYRO_SENSITIVITY);
  Serial.print(", ");
  Serial.print(mpu.getGyroBiasY() / (float)MPU9250::CALIB_GYRO_SENSITIVITY); Serial.print(", ");
  Serial.println(mpu.getGyroBiasZ() / (float)MPU9250::CALIB_GYRO_SENSITIVITY);
  Serial.println("mag bias [mG]: ");
  Serial.print(mpu.getMagBiasX()); Serial.print(", ");
  Serial.print(mpu.getMagBiasY()); Serial.print(", ");
  Serial.println(mpu.getMagBiasZ());
  Serial.println("mag scale []: ");
  Serial.print(mpu.getMagScaleX());
  Serial.print(", ");
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
  EEPROM.put(addr, mpu.getAccBiasY());
  addr += sizeof(float);
  EEPROM.put(addr, mpu.getAccBiasZ()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getGyroBiasX()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getGyroBiasY()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getGyroBiasZ());
  addr += sizeof(float);
  EEPROM.put(addr, mpu.getMagBiasX()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getMagBiasY()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getMagBiasZ()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getMagScaleX());
  addr += sizeof(float);
  EEPROM.put(addr, mpu.getMagScaleY()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getMagScaleZ()); addr += sizeof(float);
  EEPROM.commit();
}

void loadCalibration() {
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

  mpu.setAccBias(accBiasX, accBiasY, accBiasZ);
  mpu.setGyroBias(gyroBiasX, gyroBiasY, gyroBiasZ);
  mpu.setMagBias(magBiasX, magBiasY, magBiasZ);
  mpu.setMagScale(magScaleX, magScaleY, magScaleZ);
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