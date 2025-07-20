# 🧭 Navigation with STM32 (Without GPS)

A real-time embedded navigation system using the **STM32** and **MPU6500** to estimate orientation and position in the absence of GPS. Future modules include **WiFi**, **Bluetooth**, and **microSD logging**.

---

## 📚 Table of Contents
- [📌 Description](#-description)
- [🧩 Hardware Modules](#-hardware-modules)
- [✅ To-Do List](#-to-do-list)
- [💡 Questions to Consider](#-questions-to-consider)
- [📦 MPU6500 Overview](#-mpu6500-overview)
- [🛠️ MPU6500 Configuration & Filtering](#-mpu6500-configuration--filtering)
- [📏 Parameters to Monitor](#-parameters-to-monitor)
- [🔍 Choosing the Right Filter](#-choosing-the-right-filter)
- [🐞 Current Issues](#-current-issues)
- [📚 Libraries Used](#-libraries-used)
- [🪪 License](#-license)

---

## 📌 Description

This project aims to implement GPS-less navigation by tracking orientation and movement using an **MPU6500 IMU**. The STM32 serves as the main controller, collecting and processing motion data, with output planned over **UART**, and future logging and transmission via **SD card**, **Bluetooth**, and **WiFi**.

---

## 🧩 Hardware Modules

| Module | Function | Notes |
|--------|----------|-------|
| MPU6500 | 6-axis Accelerometer & Gyroscope | SPI & I2C support |
| HC-05 | Bluetooth Module | UART interface, ~10m range |
| ESP-01S | WiFi Module (ESP8266) | UART interface |
| microSD Adapter | Data Logging | SPI interface |

---

## ✅ To-Do List

### Hardware Setup
- [ ] Verify all hardware modules
- [ ] Initialize HC-05 Bluetooth module
- [ ] Setup ESP-01S WiFi module
- [ ] Connect and test microSD card via SPI
- [x] Integrate MPU6500 (SPI)

### Software Features (MPU6500)
- [x] Enable MPU6500 DMP for quaternion output
- [ ] Integrate **Madgwick Filter** (external fusion)
- [ ] Add Zero Velocity Update (ZUPT) for drift compensation
- [X] Increase sample rate to 200 Hz
- [x] Optimize full-scale accelerometer/gyro ranges (set to ±8g and ±1000dps for dynamic environments)
- [ ] Save persistent offsets to SD card
- [ ] Monitor FIFO overflow with `gs_handle.fifo_count`
- [ ] Optimize UART logging frequency
- [ ] Add magnetometer (e.g., AK8963) + modify the filter
- [ ] Log sensor data to SD for offline analysis

---

## 💡 Questions to Consider
- [ ] Should the initial coordinate come from GPS or mobile app?
- [ ] Why are both WiFi and Bluetooth needed?
- [ ] Is a display necessary for debugging/output?
- [ ] Should Falcon filter be used for MPU6500 calibration?
- [ ] What should be the specific role of the SD card?

---

## 📦 MPU6500 Overview

### Recommended Configuration
- **Sample Rate**: ≥200 Hz for better responsiveness
- **Accelerometer Range**: ±2g or ±4g for walking motion
- **Gyroscope Range**: ±500°/s for smooth tracking
- **Digital Low Pass Filter (DLPF)**: Balance between noise and delay
- **DMP Mode**: Use for low-load quaternion calculation
- **Interrupt-Driven Readout**: For low-latency updates

### MPU6500 Modes
| Mode | Description |
|------|-------------|
| Basic | Raw sensor data, external fusion (e.g., Kalman/Madgwick required) |
| **DMP** | Internal motion processing, outputs quaternions/Euler |
| FIFO | Buffered data collection, good for batch readouts |

---

## 🛠️ MPU6500 Configuration & Filtering

### Digital Low-Pass Filter (DLPF) Options

| Setting | Accel (Hz) | Gyro (Hz) | Notes |
|---------|------------|-----------|-------|
| DLPF_0  | 260        | 256       | Fast, noisy |
| DLPF_2  | 94         | 98        | Recommended default |
| DLPF_4  | 21         | 20        | Good for walking |
| DLPF_6  | 5          | 5         | Maximum filtering, higher latency |

*(Set in `driver_mpu6500_dmp.c`)*

---

## 📏 Parameters to Monitor (Live Expressions)

### Sensor Readings
- `accel_g[0..2]`: Linear acceleration (g)
- `gyro_dps[0..2]`: Angular velocity (°/s)
- `quat[0..3]`: Quaternion (DMP)
- `pitch`, `roll`, `yaw`: Orientation (Euler angles)

### Navigation State
- `velocity[0..2]`: Estimated velocity (m/s)
- `position[0..2]`: Estimated position (m)

### System Health
- `gs_handle.fifo_count`: FIFO size — check for overflows
- `step_count`: DMP pedometer counter

### After Madgwick Filter Integration
- `filter.q0..q3`: Fused quaternion
- `filter.beta`: Madgwick gain tuning parameter

---

## 🔍 Choosing the Right Filter

### ✅ **Madgwick Filter** – Recommended
- Efficient and accurate for STM32-class MCUs
- Complements DMP output to reduce gyro drift
- Easy to tune (`beta ≈ 0.1`)
- Scales well with magnetometer (future upgrade)

### ❌ **Kalman Filter** – Not Yet
- Too complex for your current MCU + peripherals
- Requires absolute references (GPS, mag)
- High CPU usage and tuning complexity

---

## 🐞 Current Issues

- **Dead Reckoning Drift**: Current position estimation accumulates error
- **Basic Navigation Algorithm**: Needs sensor fusion (e.g., Madgwick + ZUPT)
- **DMP Feature Tuning**: Explore which outputs are enabled
- **Error Reporting**: Add detailed error codes for each DMP function

---

## 📚 Libraries Used

- 🧠 [MPU6500 Driver (libdriver)](https://github.com/libdriver/mpu6500)
- 🎯 [Madgwick Filter (Fusion)](https://github.com/xioTechnologies/Fusion)

---

## 🪪 License

-All rights reserved.

This source code is the intellectual property of Alireza Sotoodeh. 
No part of this code may be copied, modified, distributed, or used without express written permission.

© 2025 Alireza-Sotoodeh


