/*////////////////////////////includes////////////////////////////*/
#include "MPU9250.h" //for setting up MPU9250

/*////////////////////////////defines////////////////////////////*/
#define bud_rate 9600
//MpU9205
#define MPU9250_IMU_ADDRESS 0x68 												// Specifies the I2C slave address of the MPU9250:0x68 GND / 0x69 High
#define MAGNETIC_DECLINATION 3.4	 											// angle between magnetic north and true north
#define INTERVAL_MS_PRINT 100 													// Sets the minimum time interval between printing sensor data in the loop to avoid flooding the serial output. higher measn slower 
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
	mpu.setup(MPU9250_IMU_ADDRESS, setting); 									//start to setup the MPU based on setting and address
	mpu.setMagneticDeclination(MAGNETIC_DECLINATION); 
	mpu.selectFilter(QuatFilterSel::MPU9250_filter_algorithm); 
	mpu.setFilterIterations(MPU9250_filter_iterations);

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
