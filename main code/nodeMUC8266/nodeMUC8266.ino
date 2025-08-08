/*////////////////////////////includes////////////////////////////*/
#include "MPU9250.h" //for setting up MPU9250
#include <EEPROM.h>  // ESP8266 EEPROM library for loading calibration
#include <U8g2lib.h> // U8g2 library for OLED
/*////////////////////////////defines////////////////////////////*/
#define bud_rate 115200
//pins
#define BUTTON_PIN 14             											// Push button on D5 (GPIO 14) for NodeMCU
//MpU9205
#define MPU9250_IMU_ADDRESS 0x68 												// Specifies the I2C slave address of the MPU9250:0x68 GND / 0x69 High
#define MAGNETIC_DECLINATION 3.4	 											// angle between magnetic north and true north
#define INTERVAL_MS_PRINT 50 														// Sets the minimum time interval between printing sensor data in the loop to avoid flooding the serial output. higher measn slower 
MPU9250 mpu; 																						// handler: allowing access to all library methods
unsigned long lastPrintMillis = 0; 											// Tracks the timestamp (from millis()) of the last time sensor data was printed to Serial

//MPU9250 setting
#define MPU9250_Accelerometer_Rang  A4G 								//select: A2G, A4G, A8G, A16G
#define MPU9250_Gyroscope_Rang  G500DPS 								//select: G250DPS, G500DPS, G1000DPS, G2000DPS
#define MPU9250_Magnetometer_resolution  M16BITS 				//select: M14BITS, M16BITS
#define MPU9250_fifo_sample_rate  SMPL_1000HZ 						//select: SMPL_1000HZ, SMPL_500HZ, SMPL_333HZ, SMPL_250HZ, SMPL_200HZ, SMPL_167HZ, SMPL_143HZ, SMPL_125HZ
#define MPU9250_Gyroscope_filter_choice  0x01						//select: 0x00: Enables DLPF with 8kHz sample rate. 0x01: Enables DLPF with 1kHz sample rate. 0x02 or 0x03: Bypasses DLPF
#define MPU9250_Gyroscope_DLPF_cutoff  DLPF_20HZ 				//select: DLPF_250HZ, DLPF_184HZ, DLPF_92HZ, DLPF_41HZ, DLPF_20HZ, DLPF_10HZ, DLPF_5HZ, DLPF_3600HZ
#define MPU9250_Accelerometer_filter_choice  0x01				//select: 0x01 Enable, 0x00 bypass
#define MPU9250_Accelerometer_DLPF_cutoff  DLPF_5HZ 		//select: DLPF_218HZ_0, DLPF_218HZ_1, DLPF_99HZ, DLPF_45HZ, DLPF_21HZ, DLPF_10HZ, DLPF_5HZ, DLPF_420HZ
#define MPU9250_filter_algorithm	MADGWICK 							  //select: MADGWICK, MAHONY, NONE
#define MPU9250_filter_iterations	10										//select: 1-50 higher better but may slow down

// OLED setup (0.91-inch SSD1306, 128x32, Software I2C on D3, D4)
U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ 2, /* data=*/ 0, /* reset=*/ U8X8_PIN_NONE); // Software I2C on D4 (SCL, GPIO 2), D3 (SDA, GPIO 0)
#define font_10_pixel u8g2_font_t0_15b_me
#define font_8_pixel u8g2_font_helvB08_tf
#define font_5_pixel u8g2_font_spleen5x8_me
#define update_rate_oled 1500
void setup() 
{ 
	Serial.begin(bud_rate); 
	Wire.begin(); // Hardware I2C for MPU9250 on D1 (SCL, GPIO 5), D2 (SDA, GPIO 4)
	pinMode(BUTTON_PIN, INPUT_PULLUP);	 // Button with internal pull-up (active-low)
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
	
	// Initialize EEPROM for ESP8266
  #if defined(ESP_PLATFORM) || defined(ESP8266)
    EEPROM.begin(0x80);  // Allocate 128 bytes for EEPROM
  #endif
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

void loop() 
{ 
	// Check for button press (active-low)
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50);  // Debounce delay
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
        delay(10);  // Wait for button release
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
    float ax = mpu.getAccX();  // Acceleration in m/s²
    float ay = mpu.getAccY();
    float az = mpu.getAccZ();
    float temp = mpu.getTemperature();
    
    // Print to Serial for MATLAB (qw,qx,qy,qz,ax,ay,az)
    Serial.print(qw, 6); Serial.print(",");
    Serial.print(qx, 6); Serial.print(",");
    Serial.print(qy, 6); Serial.print(",");
    Serial.println(qz, 6); /*Serial.print(",");
    Serial.print(ax, 6); Serial.print(",");
    Serial.print(ay, 6); Serial.print(",");
    Serial.println(az, 6);*/

    // Display on OLED (rotate quaternions and accelerations every 2 seconds)
    static unsigned long lastDisplayMillis = 0;
    static bool showQuaternions = true;
    if (currentMillis - lastDisplayMillis > update_rate_oled) {
      u8g2.clearBuffer();
       
        char buf[32];
        snprintf(buf, sizeof(buf), "Qw: %.3f", qw);
        u8g2.drawStr(0, 7, buf);
        snprintf(buf, sizeof(buf), "Qx: %.3f", qx);
        u8g2.drawStr(0, 15, buf);
        snprintf(buf, sizeof(buf), "Qy: %.3f", qy);
        u8g2.drawStr(64, 7, buf);
        snprintf(buf, sizeof(buf), "Qz: %.3f", qz);
        u8g2.drawStr(64, 15, buf);
        snprintf(buf, sizeof(buf), "Ax: %.2f", ax);
        u8g2.drawStr(0, 23, buf);
        snprintf(buf, sizeof(buf), "Ay: %.2f", ay);
        u8g2.drawStr(0, 31, buf);
        snprintf(buf, sizeof(buf), "Az: %.2f", az);
        u8g2.drawStr(64, 23, buf);
        snprintf(buf, sizeof(buf), "temp: %.2f", temp);
        u8g2.drawStr(64, 31, buf);

      u8g2.sendBuffer();
      lastDisplayMillis = currentMillis;
    }

    lastPrintMillis = currentMillis;
  }
} 

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
  // Display calibration on OLED
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
  delay(2000);  // Show calibration for 2 seconds
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
  EEPROM.commit();  // Save to EEPROM on ESP8266
}

void loadCalibration() {
  // Since setter methods are unavailable, we only read and verify calibration data
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
}