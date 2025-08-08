# 🧭 Navigation with STM32 (Without GPS)

A real-time embedded navigation system using the **STM32** and **MPU-based IMUs** to estimate orientation and position in the absence of GPS. Future modules include **WiFi**, **Bluetooth**, and **microSD logging**.

---

## 📚 Table of Contents
- [📌 Description](#-description)
- [🧩 Hardware Modules](#-hardware-modules)
- [✅ To-Do List](#-to-do-list)
- [💡 Questions to Consider](#-questions-to-consider)
- [📦 MPU6500 Overview](#-mpu6500-overview)
- [📦 MPU9250 Overview](#-mpu9250-overview)
- [⚖️ IMU Module Comparison](#️-imu-module-comparison)
- [🛠️ MPU Configuration & Filtering](#-mpu-configuration--filtering)
- [🔍 Choosing the Right Filter](#-choosing-the-right-filter)
- [🐞 Current Issues](#-current-issues)
- [📚 Libraries Used](#-libraries-used)
- [🪪 License](#-license)

---

## 📌 Description

This project aims to implement GPS-less navigation by tracking orientation and movement using an **MPU IMU**. The STM32 serves as the main controller, collecting and processing motion data, with output planned over **UART**, and future logging and transmission via **SD card**, **Bluetooth**, and **WiFi**.

---

## 🧩 Hardware Modules

| Module       | Function                          | Notes                  |
|--------------|-----------------------------------|------------------------|
| MPU6500      | 6-axis Accel & Gyro               | SPI & I2C              |
| MPU9250      | 9-axis (Accel, Gyro, Magneto)     | SPI & I2C              |
| HC-05        | Bluetooth                         | UART, ~10m range       |
| ESP-01S      | WiFi (ESP8266)                    | UART                   |
| microSD      | Data Logging                      | SPI                    |
| GY-GPS6MV2   | GPS Module                        | UART, ~2.5m accuracy   |


---

## ✅ To-Do List

### Hardware Setup

- [x] Setup MPU6500
- [x] Setup MPU9250
- [x] Add 3D animation to show orientation
- [x] Compare 3-axis modules
- [ ] SD card logging
- [ ] GPS
- [ ] HC-05 Bluetooth
- [ ] BMP280

### to do for increase accuracy

- [ ] Use a Kalman Filter or Extended Kalman Filter (EKF) (e.g. TinyEKF)
- [ ] RTIMULib for MPU9250 (is it better?) 
- [ ] Incorporate BMP280 for Altitude Accuracy
- [ ] Use GY-GPS6MV2 for Initial Position and Occasional Fixes
- [ ] Dead Reckoning
- [ ] Z
---

## 💡 Questions to Consider

- [ ] Should the initial coordinate come from GPS or mobile app?
- [ ] Why are both WiFi and Bluetooth needed?
- [ ] Is a display necessary for debugging/output?
- [ ] Should Falcon filter be used for MPU calibration?
- [ ] What should be the specific role of the SD card?
- [ ] Is MPU9250 better than MPU6500?
- [x] Which filter is better: Kalman or Madgwick? → ✅ **Madgwick**

---

## 📦 MPU6500 Overview

### Recommended Configuration

- **Sample Rate**: ≥200 Hz
- **Accel Range**: ±2g to ±8g
- **Gyro Range**: ±500–1000°/s
- **DLPF**: 20–98 Hz
- **DMP Mode**: Use for quaternion output
- **Interrupts**: For responsive readouts

| Mode | Description |
|------|-------------|
| Basic | Raw accel/gyro only |
| DMP | Quaternion, low CPU load |
| FIFO | For burst data collection |

---

## 📦 MPU9250 Overview

The MPU9250 is an upgraded version of the MPU6500, integrating a **3-axis magnetometer (AK8963)** for absolute heading.

### Benefits

- 9 DoF: Accel + Gyro + Mag
- Suitable for full orientation tracking (yaw without drift)
- Still supports DMP mode (though limited for mag data)
- SPI/I2C interface

### Configuration

- **Magnetometer** sampling via AUX I2C pass-through or bypass
- Compatible with Madgwick filter (requires `beta` tuning)

---

## ⚖️ IMU Module Comparison

| Module   | Accel/Gyro | Magnetometer | Interface | Notes |
|----------|------------|--------------|-----------|-------|
| **MPU9250** | ✅ Yes     | ✅ Yes       | I2C/SPI   | Best overall, 9-axis, ideal for dead-reckoning |
| MPU6500  | ✅ Yes     | ❌ No        | I2C/SPI   | Lightweight, no heading info |
| GY521    | ✅ Yes     | ❌ No        | I2C       | Uses MPU6050, basic, lacks DMP/mag |
| GY25     | ❌ No      | ✅ Yes       | UART      | Magnetometer-only, not sufficient alone |
| HW-123   | ✅ Yes     | ❌ No        | I2C       | Generic MPU6050 board |

### ✅ **Best Choice: MPU9250**

The **MPU9250** offers full 9-axis sensing with good SPI support and works well with Madgwick filtering, making it ideal for GPS-less inertial navigation.

---

## 🛠️ MPU Configuration & Filtering

| DLPF Setting | Accel (Hz) | Gyro (Hz) | Notes         |
|--------------|------------|-----------|---------------|
| DLPF_0       | 260        | 256       | Fast, noisy   |
| DLPF_2       | 94         | 98        | Balanced      |
| DLPF_4       | 21         | 20        | Clean motion  |
| DLPF_6       | 5          | 5         | Very stable   |

---

## 🔍 Choosing the Right Filter

### ✅ Madgwick Filter
- Low CPU load
- Effective for orientation
- Works with or without magnetometer
- Tunable gain (`beta`)

### ❌ Kalman Filter
- Needs absolute reference (GPS, magnetometer)
- Complex math and tuning
- High CPU load on STM32

---

## 🐞 Current Issues

- Dead reckoning drift over time
- Need better sensor fusion + drift compensation
- DMP mode not fully explored for MPU9250
- Error codes and fault reporting need refining

---

## 📚 Libraries Used

- [MPU6500 Driver (libdriver)](https://github.com/libdriver/mpu6500)
- [Madgwick Filter](https://github.com/xioTechnologies/Fusion)

---

## 🪪 License

**All rights reserved.**

This source code is the intellectual property of **Alireza Sotoodeh**.  
No part of this code may be copied, modified, distributed, or used without express written permission.

© 2025 Alireza Sotoodeh
