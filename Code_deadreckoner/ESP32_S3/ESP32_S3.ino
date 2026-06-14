// Last Edit: 2026-06-12 19:05:00
// Reason for Last Edit: Implemented Union structure for memory optimization and prepared PSRAM integration.
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
 * SD Card CS    | GPIO 15       | FSPI CS0 (High-speed line)
 * SD Card MOSI  | GPIO 11       | FSPI MOSI
 * SD Card SCK   | GPIO 12       | FSPI SCK (Running at 20MHz)
 * SD Card MISO  | GPIO 13       | FSPI MISO
 * -------------------------------------------------------------------------
 * BTN SELECT    | GPIO 1        | Menu Enter/Toggle (Active-Low, Internal Pull-up)
 * BTN UP        | GPIO 2        | Menu Navigation (Active-Low, Internal Pull-up)
 * BTN DOWN      | GPIO 8        | Menu Navigation (Active-Low, Internal Pull-up)
 * BTN TAG       | GPIO 14       | Waypoint Marker (Active-Low, Internal Pull-up)
 * -------------------------------------------------------------------------
 * BUZZER (+)    | GPIO 21       | Active Buzzer (use 100-ohm series resistor)
 * LED RED       | GPIO 17       | Requires 220~330 ohm series resistor
 * LED GREEN     | GPIO 18       | Requires 220~330 ohm series resistor
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
#include "esp_heap_caps.h" // Required for explicit PSRAM memory allocation

/*////////////////////////////defines////////////////////////////*/

// =========================================================================
// pin definition
// =========================================================================
  // Button Pins 
  #define BTN_SELECT_PIN 1
  #define BTN_UP_PIN 2
  #define BTN_DOWN_PIN 8
  #define BTN_TAG_PIN 14
  // MPU9250
  #define I2C_MPU_SDA 4
  #define I2C_MPU_SCL 5
  // 0.91 inch OLED
  #define I2C_OLED_SDA 6
  #define I2C_OLED_SCL 7
  // DIY SD Card 
  #define SD_CS_PIN 15
  #define SD_MOSI_PIN 11
  #define SD_SCK_PIN 12
  #define SD_MISO_PIN 13
  // Notification Pins
  #define BUZZER_PIN 21
  #define LED_RED_PIN 17                                  
  #define LED_GREEN_PIN 18                                
// =========================================================================
// setting definition
// =========================================================================
  // serial print
  #define bud_rate 115200
  // DIY SD card
  #define SPI_FREQ_MHZ 20                                 // max is 26MHZ
  #define Attempt_Runtime_SD_recovery_MS 3000
  #define Recheck_SD_beforeBoot_MS 3000                                
  #define SD_recoverd_signal_MS 50
  // MPU9250 setting
  MPU9250 mpu; // handler: allowing access to all library methods
  #define MPU9250_IMU_ADDRESS 0x68 												// Specifies the I2C slave address of the MPU9250:0x68 GND / 0x69 High
  #define MAGNETIC_DECLINATION 3.4	 											// angle between magnetic north and true north (should be fix for diffrent city)
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
  #define attempt_recovery_MPU9250_MS 2000
  // OLED 
  U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ I2C_OLED_SCL, /* data=*/ I2C_OLED_SDA, /* reset=*/ U8X8_PIN_NONE);
  #define font_10_pixel u8g2_font_t0_15b_me
  #define font_8_pixel u8g2_font_helvB08_tf
  #define font_5_pixel u8g2_font_spleen5x8_me
  #define update_rate_oled 1500
  #define OLED_SLEEP_TIMEOUT_MS 20000                       
  // Notification Timings
  #define ALARM_BEEP_MS 100                                 // Duration of error beeps during SD failure      
  #define TAG_BEEP_MS 50                                    
  #define TAG_BLINK_MS 100                                  
  // buttons 
  #define Press_to_ShutDown_MS 3000
  // menue 
  #define MENU_ITEMS_COUNT 5
// =========================================================================
// PARAMETRIC BANDWIDTH & MEMORY ENGINE
// =========================================================================
  #define DATA_FRAME_SIZE            41     
  #define SAMPLING_RATE_HZ           100    
  #define BYTES_PER_SECOND           (DATA_FRAME_SIZE * SAMPLING_RATE_HZ)
  #define BYTES_PER_HOUR             ((uint64_t)BYTES_PER_SECOND * 3600)
  #define MB_PER_HOUR                ((float)BYTES_PER_HOUR / (1024.0 * 1024.0)) 
  #define QUEUE_LENGTH               50000  
  #define PSRAM_BUFFER_SIZE_MB       ((float)(QUEUE_LENGTH * DATA_FRAME_SIZE) / (1024.0 * 1024.0))
// =========================================================================
// Calabiriation
// =========================================================================
  #define EEPROM_MAGIC_NUMBER       0xDEAD
  #define EEPROM_MAGIC_ADDR         0

/*//////////////////////////// Function prototypes ////////////////////////////*/
void performCalibration();
void print_calibration();
void saveCalibration();
void loadCalibration();
/*//////////////////////////// RTOS Data Structures ////////////////////////////*/

#pragma pack(push, 1) // Force absolute 1-byte alignment for all enclosed structures
// Optimized Parametric Binary structure using Union (Guaranteed Exactly 41 Bytes)
typedef struct {
    uint32_t frame_seq;  // 4 Bytes: Monotonic sequential index for frame drop tracking
    uint32_t timestamp;  // 4 Bytes: Absolute hardware microsecond timestamp from boot-up
    uint8_t event_flag;  // 1 Byte: 0=IMU, 1=TAG, 0xAA=SD_GAP, 0xBB=GPS
    
    // Memory Overlap: Total payload size strictly 32 Bytes
    union {
        struct {
            float q[4];
            float accel[3];
            float temp;
        } imu;
        
        struct {
            double lat;
            double lng;
        } gps;
    } payload;
    
} LogFrame;
#pragma pack(pop) // Restore default compiler alignment

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
  volatile bool sd_critical_error = false;
  volatile bool system_shutdown_requested = false;
  volatile uint32_t global_frame_counter = 0;

// Hardware spinlocks for multi-core thread safety
  portMUX_TYPE frameCounterMux = portMUX_INITIALIZER_UNLOCKED;
  portMUX_TYPE tagEventMux = portMUX_INITIALIZER_UNLOCKED; 

// Tracks the active file to resume appending after failure
  uint16_t global_log_id = 1;      // X: Main Log/Test Number (e.g., 001)
  uint16_t global_recovery_id = 1; // Y: Recovery Instance Number (e.g., 002)
  char current_log_filename[20] = "DR_LOG_001.BIN";

// FreeRTOS Handles & PSRAM Queue
  QueueHandle_t dataQueue;
  uint8_t *queueBuffer;      
  StaticQueue_t *queueStruct;
  TaskHandle_t sensorTaskHandle;
  TaskHandle_t loggingTaskHandle;

// SD Card Handlers
  SdFat sd;
  File logFile;

/*//////////////////////////// FreeRTOS Tasks ////////////////////////////*/

// =========================================================================
// DYNAMIC RECOVERY PROTOCOL
// =========================================================================
void attemptMPURecovery() {
    // 1. Hardware-level I2C Bus Reset
    Wire.end();
    Wire.begin(I2C_MPU_SDA, I2C_MPU_SCL);
    Wire.setClock(400000);
    Wire.setTimeout(50);

    // 2. Re-initialize MPU Settings
    MPU9250Setting setting;
    setting.accel_fs_sel = ACCEL_FS_SEL::MPU9250_Accelerometer_Rang;
    setting.gyro_fs_sel = GYRO_FS_SEL::MPU9250_Gyroscope_Rang;
    setting.mag_output_bits = MAG_OUTPUT_BITS::MPU9250_Magnetometer_resolution; 
    setting.fifo_sample_rate = FIFO_SAMPLE_RATE::MPU9250_fifo_sample_rate; 
    setting.gyro_fchoice = MPU9250_Gyroscope_filter_choice;
    setting.gyro_dlpf_cfg = GYRO_DLPF_CFG::MPU9250_Gyroscope_DLPF_cutoff;
    setting.accel_fchoice = MPU9250_Accelerometer_filter_choice;
    setting.accel_dlpf_cfg = ACCEL_DLPF_CFG::MPU9250_Accelerometer_DLPF_cutoff;

    // 3. Attempt Connection & Apply Filters
    if (mpu.setup(MPU9250_IMU_ADDRESS, setting)) {
        mpu.setMagneticDeclination(MAGNETIC_DECLINATION); 
        mpu.selectFilter(QuatFilterSel::MPU9250_filter_algorithm); 
        mpu.setFilterIterations(MPU9250_filter_iterations);
        
        loadCalibration(); // Re-apply EEPROM values
        mpu_critical_error = false; // Flag system as recovered
    }
}

// =========================================================================
// SD CARD DYNAMIC RECOVERY PROTOCOL
// =========================================================================
bool attemptSDRecovery() {
    logFile.close();
    SPI.end();
    
    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    
    if (sd.begin(SD_CS_PIN, SD_SCK_MHZ(SPI_FREQ_MHZ))) {
        // === FIX ISSUE 9: Generate file name based on current validated recovery index ===
        snprintf(current_log_filename, sizeof(current_log_filename), "%03d%03d.BIN", global_log_id, global_recovery_id);
        
        logFile = sd.open(current_log_filename, FILE_WRITE);
        if (logFile) {
            // Commit on Success: Only increment index after file is verified open
            global_recovery_id++;
            sd_critical_error = false;
            return true;
        }
    }
    return false;
}

// =========================================================================
// Core 0 Task
// Strictly for high-speed sensor reading and mathematical fusion
// =========================================================================
void sensorTask(void *pvParameters) {
  LogFrame frame;
  unsigned long last_mpu_data_time = millis();  // Track last successful read
  unsigned long last_recovery_attempt = 0;      // Tracks MPU9250 recovery intervals
  unsigned long last_sd_recovery_attempt = 0;   // Tracks recovery intervals for SD Card

  for(;;) {

    // === f shutdown is initiated ===
    if (system_shutdown_requested) {
        vTaskDelay(pdMS_TO_TICKS(100));
        continue;
    }

    // === INTERCEPTOR: CRITICAL MPU DISCONNECT ERROR ===
    if (mpu_critical_error) {
        if (millis() - last_recovery_attempt > attempt_recovery_MPU9250_MS) {
            last_recovery_attempt = millis();
            attemptMPURecovery();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
        continue; // Skip sensor reading until recovered
    }

    if (mpu.update()) {
      last_mpu_data_time = millis(); // Reset timeout counter
      portENTER_CRITICAL(&frameCounterMux);
      frame.frame_seq = global_frame_counter++;
      portEXIT_CRITICAL(&frameCounterMux);
      // === FIX ISSUE: Capture absolute high-precision microsecond hardware timestamp ===
      frame.timestamp = (uint32_t)esp_timer_get_time(); 
      frame.event_flag = 0; // Default: 0 marks standard high-speed IMU packet
      
      // Updated syntax to target the optimized overlapping payload union
      frame.payload.imu.q[0] = mpu.getQuaternionW();
      frame.payload.imu.q[1] = mpu.getQuaternionX();
      frame.payload.imu.q[2] = mpu.getQuaternionY();
      frame.payload.imu.q[3] = mpu.getQuaternionZ();
      frame.payload.imu.accel[0] = mpu.getLinearAccX();
      frame.payload.imu.accel[1] = mpu.getLinearAccY();
      frame.payload.imu.accel[2] = mpu.getLinearAccZ();
      frame.payload.imu.temp = mpu.getTemperature();
      // Thread-safe isolation for inter-core Waypoint Tagging flags
      portENTER_CRITICAL(&tagEventMux);
      if (tag_event_triggered) {
          frame.event_flag = 1; // 1 marks user button interaction event
          tag_event_triggered = false; 
      }
      portEXIT_CRITICAL(&tagEventMux);
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

// =========================================================================
// Core 1 Task
// For OLED, Buttons, and heavy Flash/SD writing
// =========================================================================
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
  bool error_handled = false; 
  unsigned long last_sd_recovery_attempt = 0;

  for(;;) {
    unsigned long currentMillis = millis();

    // === INTERCEPTOR: RUNTIME SD CARD FAILURE RECOVERY ===
    if (sd_critical_error) {
        // 1. HARDWARE ALARM LAYER (Absolute Non-Blocking)
        // Rhythmic SOS pattern: Strict 100ms ON every 1000ms cycle
        if ((currentMillis % 1000) < 100) {
            digitalWrite(LED_RED_PIN, HIGH);
            if (!is_muted) digitalWrite(BUZZER_PIN, HIGH);
        } else {
            digitalWrite(LED_RED_PIN, LOW);
            digitalWrite(BUZZER_PIN, LOW);
        }

        // 2. SLOW-RATE RECOVERY LAYER (Executes only once every 3000ms)
        // Prevents the 500ms sd.begin() hardware timeout from starving the alarm rhythm
        if (currentMillis - last_sd_recovery_attempt > Attempt_Runtime_SD_recovery_MS) {
            last_sd_recovery_attempt = currentMillis;
            
            is_oled_sleeping = false;
            u8g2.setPowerSave(0); 
            u8g2.clearBuffer();
            u8g2.drawStr(5, 12, "SD CARD ERR!");
            u8g2.drawStr(0, 26, "RETRIES ACTIVE...");
            u8g2.sendBuffer();
            
            if (attemptSDRecovery()) {
                LogFrame gapFrame;
                
                // FIX ISSUE: Fully clear the entire structure to prevent uninitialized temp/garbage bytes
                memset(&gapFrame, 0, sizeof(LogFrame));
                
                // Assign identifiers after the memory block is sterile
                gapFrame.frame_seq = global_frame_counter;
                gapFrame.timestamp = (uint32_t)esp_timer_get_time();    
                gapFrame.event_flag = 0xAA;        
                
                // Force secure block write of the sterile gap marker
                logFile.write((uint8_t*)&gapFrame, sizeof(LogFrame));
                logFile.sync();
                
                // Reset hardware signaling to safe operational state
                digitalWrite(LED_RED_PIN, LOW);
                digitalWrite(LED_GREEN_PIN, HIGH);
                
                u8g2.clearBuffer();
                char recBuf[32];
                snprintf(recBuf, sizeof(recBuf), "Rec Active: %s", current_log_filename);
                u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(recBuf)) / 2, 20, recBuf);
                u8g2.sendBuffer();
                
                vTaskDelay(pdMS_TO_TICKS(SD_recoverd_signal_MS)); // Non-blocking FreeRTOS delay for user feedback
                digitalWrite(LED_GREEN_PIN, LOW);
                
                force_update_ui = true;
                last_interaction_millis = currentMillis;
                continue; 
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10)); // Yield execution to prevent watchdog starvation
        continue; // Force trap within recovery boundary
    }

    // === INTERCEPTOR: CRITICAL MPU DISCONNECT ERROR ===
    if (mpu_critical_error) {
        if (!error_handled) {
            // Safely close the SD log file to prevent data corruption
            if (logFile) { logFile.sync(); logFile.close(); }
            
            // Force wake OLED
            is_oled_sleeping = false;
            u8g2.setPowerSave(0); 
            u8g2.clearBuffer();
            u8g2.drawStr(5, 15, "CRITICAL ERROR!");
            u8g2.drawStr(0, 28, "MPU DISCONNECTED");
            u8g2.sendBuffer();
            
            error_handled = true;
        }
        
        // Non-blocking SOS Pattern (100ms ON, 100ms OFF)
        if ((currentMillis / 100) % 2 == 0) {
            digitalWrite(LED_RED_PIN, HIGH);
            if (!is_muted) digitalWrite(BUZZER_PIN, HIGH);
        } else {
            digitalWrite(LED_RED_PIN, LOW);
            digitalWrite(BUZZER_PIN, LOW);
        }
        
        vTaskDelay(pdMS_TO_TICKS(20)); // Yield to scheduler
        continue; // BYPASS THE REST OF THE LOOP!
        
    } else if (error_handled) {
        // === DYNAMIC RECOVERY TRIGGERED ===
        error_handled = false;
        digitalWrite(LED_RED_PIN, LOW);
        digitalWrite(BUZZER_PIN, LOW);
        
        // Create a new sequential log file to resume mission safely
        logFile = sd.open(current_log_filename, FILE_WRITE);
        
        // Feedback to User
        u8g2.clearBuffer();
        u8g2.drawStr(10, 15, "MPU RECOVERED!");
        u8g2.drawStr(5, 28, "Resuming Log...");
        u8g2.sendBuffer();
        vTaskDelay(pdMS_TO_TICKS(1500)); // Show message briefly
        
        // Reset UI State to live tracking
        force_update_ui = true;
        currentState = STATE_LIVE_VIEW;
        last_interaction_millis = currentMillis;
    }
    
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
            // Protect writing to shared variable across cores
            portENTER_CRITICAL(&tagEventMux);
            tag_event_triggered = true;
            portEXIT_CRITICAL(&tagEventMux);
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

      // SELECT Button with Smart Long-Press Detection for Safe Shutdown
      if (digitalRead(BTN_SELECT_PIN) == LOW) {
        if (!selectWasPressed) { 
            // Button was just physically pressed down
            selectTriggered = false; // Do not trigger instantly on falling edge
            selectWasPressed = true;
            last_interaction_millis = currentMillis; // Mark press start time
        } else {
            // Button is being continuously HELD down
            if (currentMillis - last_interaction_millis > Press_to_ShutDown_MS) {
                // --- CRITICAL SAFE SHUTDOWN PROTOCOL ---
                system_shutdown_requested = true; // Signals Core 0 to halt data generation
                
                // Force wake OLED and notify user
                is_oled_sleeping = false;
                u8g2.setPowerSave(0); 
                u8g2.clearBuffer();
                u8g2.setFont(font_8_pixel);
                u8g2.drawStr(5, 12, "SAVING DATA...");
                u8g2.drawStr(0, 26, "DO NOT UNPLUG!");
                u8g2.sendBuffer();
                
                // FLUSH LAYER: Drain remaining frames from PSRAM Queue directly to SD Card
                LogFrame flushFrame;
                uint32_t remainingFrames = uxQueueMessagesWaiting(dataQueue);
                Serial.print("Shutdown active. Flushing frames to SD: "); Serial.println(remainingFrames);
                
                uint32_t flushCount = 0; // NEW: Counter to track and feed the Task Watchdog Timer
                while (xQueueReceive(dataQueue, &flushFrame, 0) == pdPASS) {
                    if (logFile) {
                        logFile.write((uint8_t*)&flushFrame, sizeof(LogFrame));
                    }
                    
                    flushCount++;
                    // CRITICAL INDUSTRIAL FIX: Yield every 500 frames to feed the RTOS Watchdog.
                    // Prevents a hard system reset/crash during massive 1.6MB PSRAM buffer flushing.
                    if (flushCount % 500 == 0) {
                        vTaskDelay(pdMS_TO_TICKS(5)); 
                    }
                }
                
                // Close file handler safely to lock file allocation tables
                if (logFile) {
                    logFile.sync();
                    logFile.close();
                }
                
                // Final UI Notification
                u8g2.clearBuffer();
                const char* offMsg = "SAFE TO POWER OFF";
                int xOff = (u8g2.getDisplayWidth() - u8g2.getStrWidth(offMsg)) / 2;
                u8g2.drawStr(xOff, 20, offMsg);
                u8g2.sendBuffer();
                
                // Hardware visual feedback: Solid Green LED signals absolute safety
                digitalWrite(LED_RED_PIN, LOW);
                digitalWrite(LED_GREEN_PIN, HIGH);
                digitalWrite(BUZZER_PIN, LOW);
                
                // Optional: Insert Power Latch GPIO clear command here to cut battery physically
                // digitalWrite(POWER_LATCH_PIN, LOW);
                
                while(1) {
                    vTaskDelay(pdMS_TO_TICKS(1000)); // Lock system safely forever
                }
            }
        }
      } else { 
        if (selectWasPressed) {
            // Button was RELEASED. Verify if it was a valid short press (< 1 second)
            if (currentMillis - last_interaction_millis < 1000) {
                selectTriggered = true;
                force_update_ui = true;
            }
        }
        selectWasPressed = false;
      }

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
                
                // CRITICAL FIX: Cast to 64-bit unsigned integer to prevent arithmetic overflow on large SD cards (>32GB)
                sd_free_mb = (uint32_t)(((uint64_t)freeClusters * sectorsPerCluster) / 2048);
                
                // Fixed parametric bandwidth tracking based on 41-Byte packets
                sd_remain_hours = (float)sd_free_mb / MB_PER_HOUR; 
                
                uint32_t sd_total_mb = (uint32_t)(((uint64_t)totalClusters * sectorsPerCluster) / 2048);
                sd_total_gb = (float)sd_total_mb / 1024.0;

                // === FIX ISSUE 7 & 8: High-Speed Filtered Directory Iteration via openNext() ===
                totalFilesCount = 0;
                File rootDir;
                if (rootDir.open("/", O_RDONLY)) {
                    File file;
                    char nameBuf[25];
                    while (file.openNext(&rootDir, O_RDONLY)) {
                        if (!file.isDir()) {
                            file.getName(nameBuf, sizeof(nameBuf));
                            int len = strlen(nameBuf);
                            
                            // Validate .BIN extension first
                            if (len >= 4 && strcasecmp(nameBuf + len - 4, ".BIN") == 0) {
                                // Filter and count only unique Parent Log sessions starting with "DR_LOG_"
                                if (strncmp(nameBuf, "DR_LOG_", 7) == 0) {
                                    totalFilesCount++;
                                }
                            }
                        }
                        file.close();
                    }
                    rootDir.close();
                }
            } else {
                sd_free_mb = 0; sd_remain_hours = 0.0; sd_total_gb = 0.0; totalFilesCount = 0;
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
          print_calibration();
          saveCalibration();
          xQueueReset(dataQueue);
          vTaskResume(sensorTaskHandle);
          force_update_ui = true; 
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
          char filename[20];
          global_log_id = 1;
          while (true) {
            snprintf(filename, sizeof(filename), "DR_LOG_%03d.BIN", global_log_id);
            if (!sd.exists(filename)) break;
            global_log_id++;
            if (global_log_id > 999) { global_log_id = 999; break; }
          }
          // Reset recovery counter
          global_recovery_id = 1;
          // Thread-safe initialization of sequence tracker
          portENTER_CRITICAL(&frameCounterMux);
          global_frame_counter = 0;
          portEXIT_CRITICAL(&frameCounterMux);

          // CRITICAL FIX: Clear and purge any stale sensor frames accumulated in the queue 
          // during menu interaction, ensuring the new file starts strictly from a sterile buffer state.
          xQueueReset(dataQueue);

          strcpy(current_log_filename, filename); 
          logFile = sd.open(current_log_filename, FILE_WRITE);

          // Render instant feedback to the user on screen
          u8g2.clearBuffer();
          char flashBuf[25];
          snprintf(flashBuf, sizeof(flashBuf), "Created: LOG_%03d", global_log_id);
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
          
          // CRITICAL FIX: Enhanced sweeping protocol to remove BOTH Parent logs and Orphaned recovery fragments
          for (int i = 1; i <= 999; i++) {
            // 1. Delete the Parent Log File
            snprintf(delFilename, sizeof(delFilename), "DR_LOG_%03d.BIN", i);
            if (sd.exists(delFilename)) {
              sd.remove(delFilename);
            }
            
            // 2. Nested Sweep: Sequentially search and destroy all child recovery chunks [i][j].BIN
            int j = 1;
            while (true) {
              snprintf(delFilename, sizeof(delFilename), "%03d%03d.BIN", i, j);
              if (sd.exists(delFilename)) {
                sd.remove(delFilename);
                j++; // Increment to check the next sequential crash fragment
              } else {
                break; // No more recovery fragments exist for this specific log session
              }
            }
          }
          strcpy(current_log_filename, "DR_LOG_001.BIN");
          // Reset X
          global_log_id = 1;
          // Reset Y            
          global_recovery_id = 1;
          // Reset frame counter       
          // Thread-safe initialization of sequence tracker on memory wipe
          portENTER_CRITICAL(&frameCounterMux);
          global_frame_counter = 0;
          portEXIT_CRITICAL(&frameCounterMux);
          // CRITICAL FIX: Purge the queue after memory wipe to prevent frames captured 
          // during formatting from leaking into the new fresh session.
          xQueueReset(dataQueue);     
          logFile = sd.open(current_log_filename, FILE_WRITE);
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
        size_t bytesWritten = logFile.write((uint8_t*)&receivedFrame, sizeof(LogFrame));
        
        // Error Detection: If bytes written do not match the expected struct size, hardware link is broken
        if (bytesWritten != sizeof(LogFrame)) {
            sd_critical_error = true; // Trigger immediate dynamic recovery interceptor
        }
      } else {
          sd_critical_error = true; // File pointer lost, enter error mode
      }
      
      if (currentMillis - lastFlushMillis > 5000) { 
        if (logFile) {
            if (!logFile.sync()) {
                sd_critical_error = true; // Sync failure means card was pulled out
            }
        }
        lastFlushMillis = currentMillis;
      }

      // === PHASE 4: Graphics Rendering ===
      if (!is_oled_sleeping) {
          if ((currentMillis - lastDisplayMillis > update_rate_oled) || force_update_ui) {
            force_update_ui = false;
            u8g2.clearBuffer();
            
            if (currentState == STATE_LIVE_VIEW) {
              char buf[32];
              // Fixed Syntax & Compressed Layout for 128x32 OLED (Two-Column Grid Optimization)
              // Row 1 (Y=8): Quaternions W & X
              snprintf(buf, sizeof(buf), "Qw:%.2f", receivedFrame.payload.imu.q[0]); u8g2.drawStr(0, 8, buf);
              snprintf(buf, sizeof(buf), "Qx:%.2f", receivedFrame.payload.imu.q[1]); u8g2.drawStr(64, 8, buf);
              
              // Row 2 (Y=19): Quaternions Y & Z
              snprintf(buf, sizeof(buf), "Qy:%.2f", receivedFrame.payload.imu.q[2]); u8g2.drawStr(0, 19, buf);
              snprintf(buf, sizeof(buf), "Qz:%.2f", receivedFrame.payload.imu.q[3]); u8g2.drawStr(64, 19, buf);
              
              // Row 3 (Y=31): Real-time run duration & Non-blocking MPU Temperature
              uint32_t total_secs = receivedFrame.timestamp / 1000000; 
              uint32_t mins = total_secs / 60;
              uint32_t secs = total_secs % 60;
              snprintf(buf, sizeof(buf), "T:%03lu:%02lu", mins, secs); u8g2.drawStr(0, 31, buf);
              snprintf(buf, sizeof(buf), "T:%.1fC", receivedFrame.payload.imu.temp); u8g2.drawStr(76, 31, buf);
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
      if (auto_off_enabled && !is_oled_sleeping && (currentMillis - last_interaction_millis > OLED_SLEEP_TIMEOUT_MS)) {
          is_oled_sleeping = true;
          u8g2.setPowerSave(1); 
          currentState = STATE_LIVE_VIEW; 
          force_update_ui = true;
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
	Wire.setTimeout(50); //  Prevents Hardware I2C Bus Hang if wire is pulled

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
    delay(Recheck_SD_beforeBoot_MS);
    digitalWrite(LED_RED_PIN, LOW); 
  } 
  
  // === Dynamic Sequential Logging Architecture ===
  // Render "Scanning SD..." while the ESP computes existing files
  u8g2.clearBuffer();
  u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth("Scanning SD...")) / 2, 20, "Scanning SD...");
  u8g2.sendBuffer();

  char filename[20];
  global_log_id = 1;
  // Scan the SD root directory to find the next available sequential number
  while (true) {
    snprintf(filename, sizeof(filename), "DR_LOG_%03d.BIN", global_log_id);
    if (!sd.exists(filename)) break;
    global_log_id++;
    if (global_log_id > 999) {
      Serial.println("ERROR: Log file limit reached (999). Overwriting DR_LOG_999.BIN");
      global_log_id = 999;
      break;
    }
  }
  
  global_recovery_id = 1;           // Reset recovery counter 
  global_frame_counter = 0;         // Reset frame counter
  strcpy(current_log_filename, filename); 
  logFile = sd.open(current_log_filename, FILE_WRITE);

  if (logFile) {
    Serial.print("Success: Opened new log file -> ");
    Serial.println(filename);
    
    // Render dynamic SD statistics before mission starts
    u8g2.clearBuffer();
    char scanBuf[25];
    snprintf(scanBuf, sizeof(scanBuf), "Found: %d Logs", global_log_id - 1);
    u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(scanBuf)) / 2, 12, scanBuf);
    
    snprintf(scanBuf, sizeof(scanBuf), "Next: LOG_%03d", global_log_id);
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
  // =========================================================================
  // PSRAM ALLOCATION & QUEUE CREATION
  // =========================================================================
  // 1. Allocate the 1.6MB data buffer strictly in the external PSRAM
  queueBuffer = (uint8_t *)heap_caps_malloc(QUEUE_LENGTH * sizeof(LogFrame), MALLOC_CAP_SPIRAM);
  
  // 2. Allocate the Queue Manager struct strictly in internal 8-bit RAM (for scheduler speed)
  queueStruct = (StaticQueue_t *)heap_caps_malloc(sizeof(StaticQueue_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

  if (queueBuffer == NULL || queueStruct == NULL) {
      Serial.println("CRITICAL: Failed to allocate PSRAM for Queue! System Halted.");
      u8g2.clearBuffer();
      u8g2.drawStr(10, 15, "PSRAM ERROR!");
      u8g2.sendBuffer();
      while(1); // Trap the system here if PSRAM is defective or disabled in Arduino settings
  }

  // 3. Create the Static Queue using the allocated memory
  dataQueue = xQueueCreateStatic(QUEUE_LENGTH, sizeof(LogFrame), queueBuffer, queueStruct);
  Serial.print("SUCCESS: Buffer allocated in PSRAM. Total size (MB): ");
  Serial.println(PSRAM_BUFFER_SIZE_MB);

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
  // Write the confirmation sentinel first to validate future boots
  uint16_t magic = EEPROM_MAGIC_NUMBER;
  EEPROM.put(EEPROM_MAGIC_ADDR, magic);
  
  // FIX ISSUE 2: Offset the data allocation by the size of the magic number
  int addr = sizeof(uint16_t);
  
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
  uint16_t loadedMagic = 0;
  EEPROM.get(EEPROM_MAGIC_ADDR, loadedMagic);
  
  // Defensive Check: Validate if EEPROM has been calibrated before
  if (loadedMagic != EEPROM_MAGIC_NUMBER) {
    Serial.println("WARNING: No valid calibration found in EEPROM. Using factory defaults.");
    mpu.setAccBias(0.0, 0.0, 0.0);
    mpu.setGyroBias(0.0, 0.0, 0.0);
    mpu.setMagBias(0.0, 0.0, 0.0);
    mpu.setMagScale(1.0, 1.0, 1.0);
    return; // Abort loading to protect the Madgwick filter from corrupt floats
  }

  int addr = sizeof(uint16_t);
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