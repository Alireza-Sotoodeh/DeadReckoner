// Last Edit: 2026-06-03 17:07:30
// Reason for Last Edit: Ported to ESP32-S3, updated pin definitions, I2C routing, and EEPROM initialization
// Author: Alireza Sotoodeh

/*
 * =========================================================================
 * PROJECT: Signal-Free Offline Tracking System
 * VERSION: 1.1 (ESP32-S3 Port)
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
#define INTERVAL_MS_PRINT 50 														// Sets the minimum time interval between printing sensor data in the loop to avoid flooding the serial output.
// higher measn slower 
MPU9250 mpu; 																						// handler: allowing access to all library methods
unsigned long lastPrintMillis = 0;
// Tracks the timestamp (from millis()) of the last time sensor data was printed to Serial

//MPU9250 setting
#define MPU9250_Accelerometer_Rang  A2G 								//select: A2G, A4G, A8G, A16G
#define MPU9250_Gyroscope_Rang  G500DPS 								//select: G250DPS, G500DPS, G1000DPS, G2000DPS
#define MPU9250_Magnetometer_resolution  M16BITS 				//select: M14BITS, M16BITS
#define MPU9250_fifo_sample_rate  SMPL_1000HZ 					//select: SMPL_1000HZ, SMPL_500HZ, SMPL_333HZ, SMPL_250HZ, SMPL_200HZ, SMPL_167HZ, SMPL_143HZ, SMPL_125HZ
#define MPU9250_Gyroscope_filter_choice  0x01						//select: 0x00: Enables DLPF with 8kHz sample rate.
// 0x01: Enables DLPF with 1kHz sample rate. 0x02 or 0x03: Bypasses DLPF
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

/*////////////////////////////setup////////////////////////////*/
void setup() 
{ 
	Serial.begin(bud_rate);
  
  // Initialize Hardware I2C for MPU9250 with explicit pins for ESP32-S3
	Wire.begin(I2C_MPU_SDA, I2C_MPU_SCL); 
	
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
} 

/*////////////////////////////loop////////////////////////////*/

void loop() 
{ 
	// Check for button press (active-low)
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50);
    // Debounce delay
    if (digitalRead(BUTTON_PIN) == LOW) {  // Confirm button press
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
      while (digitalRead(BUTTON_PIN) == LOW) {
        delay(10);
        // Wait for button release
      }
    }
  }
  
  // Main sensor data loop
  unsigned long currentMillis = millis();
  if (mpu.update() && currentMillis - lastPrintMillis > INTERVAL_MS_PRINT) {
    // Get quaternions and accelerations
    float qw = mpu.getQuaternionW();
    float qx = mpu.getQuaternionX();
    float qy = mpu.getQuaternionY();
    float qz = mpu.getQuaternionZ();
    float ax = mpu.getLinearAccX();
    // Acceleration in m/s²
    float ay = mpu.getLinearAccY();
    float az = mpu.getLinearAccZ();
    float temp = mpu.getTemperature();
    
    // Print to Serial for MATLAB (qw,qx,qy,qz,ax,ay,az)