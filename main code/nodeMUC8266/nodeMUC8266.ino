/*////////////////////////////includes////////////////////////////*/
#include "MPU9250.h" //for setting up MPU9250
#include <EEPROM.h>  // ESP8266 EEPROM library
/*////////////////////////////defines////////////////////////////*/
#define bud_rate 9600
//pins
#define BUTTON_PIN 14             											// Push button on D5 (GPIO 14) for NodeMCU
//MpU9205
#define MPU9250_IMU_ADDRESS 0x68 												// Specifies the I2C slave address of the MPU9250:0x68 GND / 0x69 High
#define MAGNETIC_DECLINATION 3.4	 											// angle between magnetic north and true north
#define INTERVAL_MS_PRINT 50 														// Sets the minimum time interval between printing sensor data in the loop to avoid flooding the serial output. higher measn slower 
MPU9250 mpu; 																						// handler: allowing access to all library methods
unsigned long lastPrintMillis = 0; 											// Tracks the timestamp (from millis()) of the last time sensor data was printed to Serial

//MPU9250 setting
#define MPU9250_Accelerometer_Rang  A16G 								//select: A2G, A4G, A8G, A16G
#define MPU9250_Gyroscope_Rang  G1000DPS 								//select: G250DPS, G500DPS, G1000DPS, G2000DPS
#define MPU9250_Magnetometer_resolution  M16BITS 				//select: M14BITS, M16BITS
#define MPU9250_fifo_sample_rate  SMPL_250HZ 						//select: SMPL_1000HZ, SMPL_500HZ, SMPL_333HZ, SMPL_250HZ, SMPL_200HZ, SMPL_167HZ, SMPL_143HZ, SMPL_125HZ
#define MPU9250_Gyroscope_filter_choice  0x03						//select: 0x00: Enables DLPF with 8kHz sample rate. 0x01: Enables DLPF with 1kHz sample rate. 0x02 or 0x03: Bypasses DLPF
#define MPU9250_Gyroscope_DLPF_cutoff  DLPF_20HZ 				//select: DLPF_250HZ, DLPF_184HZ, DLPF_92HZ, DLPF_41HZ, DLPF_20HZ, DLPF_10HZ, DLPF_5HZ, DLPF_3600HZ
#define MPU9250_Accelerometer_filter_choice  0x01				//select: 0x01 Enable, 0x00 bypass
#define MPU9250_Accelerometer_DLPF_cutoff  DLPF_45HZ 		//select: DLPF_218HZ_0, DLPF_218HZ_1, DLPF_99HZ, DLPF_45HZ, DLPF_21HZ, DLPF_10HZ, DLPF_5HZ, DLPF_420HZ
#define MPU9250_filter_algorithm	MADGWICK 							//select: MADGWICK, MAHONY, NONE
#define MPU9250_filter_iterations	15										//select: 1-50 higher better but may slow down

void setup() 
{ 
	Serial.begin(bud_rate); 
	Wire.begin(); 
	pinMode(BUTTON_PIN, INPUT_PULLUP);	 // Button with internal pull-up (active-low)

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
	if (!mpu.setup(MPU9250_IMU_ADDRESS, setting)) {
    while (1) {
      Serial.println("MPU connection failed. Please check your connection.");
      delay(5000);
    }
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
  loadCalibration();
  print_calibration();
  Serial.println("Press the button to start calibration...");
} 

void loop() 
{ 
	// Check for button press (active-low)
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50);  // Debounce delay
    if (digitalRead(BUTTON_PIN) == LOW) {  // Confirm button press
      Serial.println("Button pressed. Starting calibration...");
      performCalibration();
      saveCalibration();
      print_calibration();
      Serial.println("Calibration saved to EEPROM. Press button to recalibrate.");
      while (digitalRead(BUTTON_PIN) == LOW) {
        delay(10);  // Wait for button release
      }
    }
  }
	// Main sensor data loop
	unsigned long currentMillis = millis(); 
	if (mpu.update() && currentMillis - lastPrintMillis > INTERVAL_MS_PRINT) { 
		Serial.print(mpu.getQuaternionW(), 6); Serial.print(",");
		Serial.print(mpu.getQuaternionX(), 6); Serial.print(",");
		Serial.print(mpu.getQuaternionY(), 6); Serial.print(",");
		Serial.println(mpu.getQuaternionZ(), 6);
	  lastPrintMillis = currentMillis; 
	} 
} 

void performCalibration() {
  Serial.println("Accel Gyro calibration will start in 5sec.");
  Serial.println("Please leave the device still on a flat plane.");
  mpu.verbose(true);
  delay(5000);
  mpu.calibrateAccelGyro();

  Serial.println("Mag calibration will start in 5sec.");
  Serial.println("Please wave device in a figure eight until done.");
  delay(5000);
  mpu.calibrateMag();

  mpu.verbose(false);
  Serial.println("Calibration complete.");
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