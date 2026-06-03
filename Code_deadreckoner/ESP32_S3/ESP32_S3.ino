// Last Edit: 2026-06-03 18:30:00
// Reason for Last Edit: Finalized FreeRTOS dual-core architecture (Version 1.2) and cleared versioning confusion.
// Author: Alireza Sotoodeh

/*
 * =========================================================================
 * PROJECT: Signal-Free Offline Tracking System
 * VERSION: 1.2 (FreeRTOS Dual-Core)
 * * WIRING DIAGRAM
 * -------------------------------------------------------------------------
 * Component        | ESP32-S3 Pin          | Note / Reasoning
 * -------------------------------------------------------------------------
 * MPU9250 VCC      | 3.3V                  | MPU9250 is 3.3V tolerant logic
 * MPU9250 GND      | GND                   | Common ground
 * MPU9250 SCL      | GPIO 5                | Hardware I2C (Wire) Clock for fast IMU reads
 * MPU9250 SDA      | GPIO 4                | Hardware I2C (Wire) Data for fast IMU reads
 * -------------------------------------------------------------------------
 * OLED VCC         | 3.3V                  | 
 * OLED GND         | GND                   | Common ground
 * OLED SCL         | GPIO 7                | Software I2C to avoid bus congestion
 * OLED SDA         | GPIO 6                | Software I2C with the IMU
 * -------------------------------------------------------------------------
 * Push Button      | GPIO 0 (Boot Btn)     | Input Pullup; Connects to GND when pressed
 * -------------------------------------------------------------------------
 * * DESIGN NOTES:
 * - OLED is intentionally separated onto Software I2C to prevent display 
 * updates from blocking high-speed sensor fusion reads from the MPU9250.
 * =========================================================================
 */
 
/*////////////////////////////includes////////////////////////////*/
#include "MPU9250.h" //for setting up MPU9250
#include <EEPROM.h>  // ESP32 EEPROM wrapper library for loading calibration
#include <U8g2lib.h> // U8g2 library for OLED
#include <Wire.h>    // I2C library

/*////////////////////////////defines////////////////////////////*/
#define bud_rate 115200

//pins (Updated for ESP32-S3)
#define BUTTON_PIN 0             											// Using onboard BOOT button (GPIO 0)
#define I2C_MPU_SDA 4
#define I2C_MPU_SCL 5
#define I2C_OLED_SDA 6
#define I2C_OLED_SCL 7

//MpU9205
#define MPU9250_IMU_ADDRESS 0x68 												// Specifies the I2C slave address of the MPU9250:0x68 GND / 0x69 High
#define MAGNETIC_DECLINATION 3.4	 											// angle between magnetic north and true north
#define INTERVAL_MS_PRINT 50 														// Sets the minimum time interval between printing sensor data.
MPU9250 mpu;
// handler: allowing access to all library methods
unsigned long lastPrintMillis = 0;
// Tracks the timestamp (from millis()) of the last time sensor data was printed to Serial

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

// OLED setup (0.91-inch SSD1306, 128x32, Software I2C updated pins)
U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ I2C_OLED_SCL, /* data=*/ I2C_OLED_SDA, /* reset=*/ U8X8_PIN_NONE);
// Software I2C on custom ESP32-S3 pins to avoid bus congestion

#define font_10_pixel u8g2_font_t0_15b_me
#define font_8_pixel u8g2_font_helvB08_tf
#define font_5_pixel u8g2_font_spleen5x8_me
#define update_rate_oled 1500

/*//////////////////////////// RTOS Data Structures ////////////////////////////*/

// Binary structure to hold one frame of sensor data safely (48 Bytes)
typedef struct {
    uint32_t timestamp;
    float q[4];
    float accel[3];
    double gps_lat;
    double gps_lng;
} LogFrame;

// FreeRTOS Handles
QueueHandle_t dataQueue;
TaskHandle_t sensorTaskHandle;
TaskHandle_t loggingTaskHandle;

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
  // unsigned long lastFlushMillis = 0; // Will be used for the 5-second flush strategy
  
  for(;;) {
    // Check for button press (active-low) for calibration
    if (digitalRead(BUTTON_PIN) == LOW) {
      vTaskDelay(pdMS_TO_TICKS(50));
      // Debounce delay
      if (digitalRead(BUTTON_PIN) == LOW) {  // Confirm button press
        
        // CRITICAL: Suspend Core 0 so I2C bus isn't interrupted during calibration
        vTaskSuspend(sensorTaskHandle);
        
        u8g2.setFont(font_8_pixel);
        Serial.println("Button pressed. Starting calibration...");
        u8g2.clearBuffer();
        u8g2.drawStr(25, 15, "Calibrating...");
        u8g2.sendBuffer();
        
        performCalibration();
        saveCalibration();
        print_calibration();
        
        Serial.println("Calibration saved to EEPROM. Press button to recalibrate.");
        u8g2.clearBuffer();
        u8g2.drawStr(25, 15, "Calibration Done");
        u8g2.drawStr(25, 25, "Press to Recal");
        u8g2.sendBuffer();
        u8g2.setFont(font_5_pixel);
        
        vTaskDelay(pdMS_TO_TICKS(2000));
        
        // Resume Core 0 to continue normal operation
        vTaskResume(sensorTaskHandle);
        
        while (digitalRead(BUTTON_PIN) == LOW) {
          vTaskDelay(pdMS_TO_TICKS(10));
          // Wait for button release
        }
      }
    }

    // Pull data from the Queue
    if (xQueueReceive(dataQueue, &receivedFrame, pdMS_TO_TICKS(10)) == pdPASS) {
      
      // === PHASE 3 SD CARD / LITTLEFS WRITING WILL GO HERE ===
      // file.write((uint8_t*)&receivedFrame, sizeof(LogFrame));
      // if (millis() - lastFlushMillis > 5000) { file.flush(); lastFlushMillis = millis(); }
      
      unsigned long currentMillis = millis();
      if (currentMillis - lastDisplayMillis > update_rate_oled) {
        // Print to Serial for MATLAB visualization
        Serial.print(receivedFrame.q[0], 6); Serial.print(",");
        Serial.print(receivedFrame.q[1], 6); Serial.print(",");
        Serial.print(receivedFrame.q[2], 6); Serial.print(",");
        Serial.println(receivedFrame.q[3], 6);
        
        // Update OLED
        u8g2.clearBuffer();
        char buf[32];
        snprintf(buf, sizeof(buf), "Qw: %.3f", receivedFrame.q[0]);
        u8g2.drawStr(0, 7, buf);
        snprintf(buf, sizeof(buf), "Qx: %.3f", receivedFrame.q[1]);
        u8g2.drawStr(0, 15, buf);
        snprintf(buf, sizeof(buf), "Qy: %.3f", receivedFrame.q[2]);
        u8g2.drawStr(64, 7, buf);
        snprintf(buf, sizeof(buf), "Qz: %.3f", receivedFrame.q[3]);
        u8g2.drawStr(64, 15, buf);
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
	
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  // Button with internal pull-up (active-low)
  
  // Initialize OLED
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(font_10_pixel);
  u8g2.drawStr(15, 25, "<< Boot up >>");
  u8g2.sendBuffer();
  delay(1000);
  u8g2.setFont(font_8_pixel); 
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
  EEPROM.begin(128);
  // Allocate 128 bytes for EEPROM wrapper

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
  Serial.print(mpu.getAccBiasX() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY);
  Serial.print(", ");
  Serial.print(mpu.getAccBiasY() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY);
  Serial.print(", ");
  Serial.print(mpu.getAccBiasZ() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY);
  Serial.println();
  Serial.println("gyro bias [deg/s]: ");
  Serial.print(mpu.getGyroBiasX() / (float)MPU9250::CALIB_GYRO_SENSITIVITY);
  Serial.print(", ");
  Serial.print(mpu.getGyroBiasY() / (float)MPU9250::CALIB_GYRO_SENSITIVITY);
  Serial.print(", ");
  Serial.print(mpu.getGyroBiasZ() / (float)MPU9250::CALIB_GYRO_SENSITIVITY);
  Serial.println();
  Serial.println("mag bias [mG]: ");
  Serial.print(mpu.getMagBiasX());
  Serial.print(", ");
  Serial.print(mpu.getMagBiasY());
  Serial.print(", ");
  Serial.print(mpu.getMagBiasZ());
  Serial.println();
  Serial.println("mag scale []: ");
  Serial.print(mpu.getMagScaleX());
  Serial.print(", ");
  Serial.print(mpu.getMagScaleY());
  Serial.print(", ");
  Serial.print(mpu.getMagScaleZ());
  Serial.println();
  
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
  EEPROM.put(addr, mpu.getAccBiasX());
  addr += sizeof(float);
  EEPROM.put(addr, mpu.getAccBiasY()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getAccBiasZ()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getGyroBiasX()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getGyroBiasY());
  addr += sizeof(float);
  EEPROM.put(addr, mpu.getGyroBiasZ()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getMagBiasX()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getMagBiasY()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getMagBiasZ());
  addr += sizeof(float);
  EEPROM.put(addr, mpu.getMagScaleX()); addr += sizeof(float);
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

  EEPROM.get(addr, accBiasX);
  addr += sizeof(float);
  EEPROM.get(addr, accBiasY); addr += sizeof(float);
  EEPROM.get(addr, accBiasZ); addr += sizeof(float);
  EEPROM.get(addr, gyroBiasX); addr += sizeof(float);
  EEPROM.get(addr, gyroBiasY);
  addr += sizeof(float);
  EEPROM.get(addr, gyroBiasZ); addr += sizeof(float);
  EEPROM.get(addr, magBiasX); addr += sizeof(float);
  EEPROM.get(addr, magBiasY); addr += sizeof(float);
  EEPROM.get(addr, magBiasZ);
  addr += sizeof(float);
  EEPROM.get(addr, magScaleX); addr += sizeof(float);
  EEPROM.get(addr, magScaleY); addr += sizeof(float);
  EEPROM.get(addr, magScaleZ); addr += sizeof(float);
  
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
}