#include "MPU9250.h" 
#define MPU9250_IMU_ADDRESS 0x68 
#define MAGNETIC_DECLINATION 1.63 // To be defined by user 
#define INTERVAL_MS_PRINT 1000 
MPU9250 mpu; 
unsigned long lastPrintMillis = 0; 
void setup() 
{ 
	 Serial.begin(9600); 
	 Wire.begin(); 
	 Serial.println("Starting..."); 
	 MPU9250Setting setting; 
	 // Sample rate must be at least 2x DLPF rate 
	 setting.accel_fs_sel = ACCEL_FS_SEL::A16G; 
	 setting.gyro_fs_sel = GYRO_FS_SEL::G1000DPS; 
	 setting.mag_output_bits = MAG_OUTPUT_BITS::M16BITS; 
	 setting.fifo_sample_rate = FIFO_SAMPLE_RATE::SMPL_250HZ; 
	 setting.gyro_fchoice = 0x03; 
	 setting.gyro_dlpf_cfg = GYRO_DLPF_CFG::DLPF_20HZ; 
	 setting.accel_fchoice = 0x01; 
	 setting.accel_dlpf_cfg = ACCEL_DLPF_CFG::DLPF_45HZ; 
	 mpu.setup(MPU9250_IMU_ADDRESS, setting); 
	 mpu.setMagneticDeclination(MAGNETIC_DECLINATION); 
	 mpu.selectFilter(QuatFilterSel::MADGWICK); 
	 mpu.setFilterIterations(15); 
	 Serial.println("Calibration will start in 5sec."); 
	 Serial.println("Please leave the device still on the flat plane."); 
	 delay(5000); 
	 Serial.println("Calibrating..."); 
	 mpu.calibrateAccelGyro(); 
	 Serial.println("Magnetometer calibration will start in 5sec."); 
	 Serial.println("Please Wave device in a figure eight until done."); 
	 delay(5000); 
	 Serial.println("Calibrating..."); 
	 mpu.calibrateMag(); 
	 Serial.println("Ready!"); 
} 
void loop() 
{ 
	 unsigned long currentMillis = millis(); 
	 if (mpu.update() && currentMillis - lastPrintMillis > INTERVAL_MS_PRINT) { 
		/*
	   Serial.print("TEMP:"); 
	   Serial.print(mpu.getTemperature(), 2); 
	   Serial.print("C"); 
	   Serial.println(); 
	   Serial.print("Pitch:"); 
	   Serial.print(mpu.getPitch()); 
	   Serial.println(); 
	   Serial.print("Roll:"); 
	   Serial.print(mpu.getRoll()); 
	   Serial.println(); 
	   Serial.print("Yaw:"); 
	   Serial.print(mpu.getYaw());
	   Serial.println(); */
		Serial.print(mpu.getQuaternionW(), 6); Serial.print(",");
		Serial.print(mpu.getQuaternionX(), 6); Serial.print(",");
		Serial.print(mpu.getQuaternionY(), 6); Serial.print(",");
		Serial.println(mpu.getQuaternionZ(), 6);

	   lastPrintMillis = currentMillis; 
	 } 
} 
