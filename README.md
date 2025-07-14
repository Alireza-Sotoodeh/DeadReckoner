# Navigation-STM32


## 🚩 Table of Contents

more will be added ...
- [Description](#-Description)
- [To Do list](#-To-Do-list)
- [Questioned](#-Questioned)
- [log](#-log)
- [MPU6500](#-MPU6500)
- [License](#-license)


## ⚡ Description
 trying to navigate by usage of STM32 as main processor while there is no signal.
	### features
	...
## To-Do-list

Hardware tasks:
- [ ] verify all below hardware before getting started
- [ ] setup Bluetooth
	HC-05 Bluetooth module_ distance: 10m _ UART protocol
- [ ] setup WiFi
	ESP-01S WIFI module_ main chip: ESP8266
- [ ] setup SD card
	micro-SD card adapter module _ SPI protocol
- [ ] setup Gyroscope
	GY_521 module/(3-axis Acceleration & Gyroscope)/main chip:MPU-6050/I2C protocol
	GY_25 module/(3-axis Acceleration & Gyroscope)/main chip: MPU-6050/I2C & UART protocol
	HW123 module/(3-axis Acceleration & Gyroscope)/main chip: MPU-6050/
	MPU6500 module_ (6-axis Acceleration & Gyroscope)_main chip: MPU-6500_I2C & UART protocol

## Questioned
- [ ] dose the navigation start coordinates is based on separate module (GPS6mv2) or user phone data?
- [ ] why both WiFi and Bluetooth? 
- [ ] need a display?
- [ ] doe the MPU6500(gyroscope) need Falcon filter for calibration.

## log

### 1404/04/22
- time: 10 A.M => working on an MPU6500 library and studying it(you can fined it in setup modules)
- time: 1  P.M => setting up I2C and UART for MPU6500
- time: 8  P.M => the libraries setup has been done! now its time to work to know the functions better

### 1404/04/23
- time: 10 working on MPU6500 modes (which mode I should do)


## MPU6500

To achieve high accuracy and fast performance:
- High Sample Rate: Configure the MPU6500 for a high sample rate (e.g., 1 kHz for the accelerometer and gyroscope) to capture rapid changes in motion.
-  Low Noise Settings: Use low-pass filters (DLPF) to reduce noise while maintaining responsiveness.
- Full-Scale Range: Select appropriate full-scale ranges for the accelerometer (±2g or ±4g) and gyroscope (±500°/s or ±1000°/s) to balance sensitivity and range.
- DMP Usage: Utilize the DMP for processed orientation data (quaternions) to reduce computational load on the STM32 and improve accuracy for navigation.
- Interrupt-Driven Operation: Use the MPU6500’s data-ready interrupt to ensure timely data acquisition without polling.
- Calibration: Implement offset calibration to minimize bias errors in accelerometer and gyroscope readings.
- MPU6500 has 3 mode: 1_basic , 2_DMP(data memory processing), 3-FIFO
_ MPU6500 in basic mode: The basic mode is simpler but requires you to implement sensor fusion (e.g., using a Kalman or Madgwick filter) on the microcontroller, which can be computationally intensive and less accurate if not optimized.
_ MPU6500 in DMP mode: DMP mode is the best choice because it=> 1-Provides processed orientation data (quaternions, Euler angles) directly. 2-Includes features like gyro calibration and tap detection. 3-Reduces the microcontroller’s processing load, allowing faster and more reliable data handling.
MPU6500 in FIFO mode: The FIFO mode stores raw sensor data for batch processing, which is useful for high-speed data collection but doesn’t inherently improve accuracy without additional processing.

## 📜 License

-All rights reserved.

This source code is the intellectual property of Alireza Sotoodeh. 
No part of this code may be copied, modified, distributed, or used without express written permission.

© 2025 Alireza-Sotoodeh


