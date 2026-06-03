# Signal-Free Offline Tracking System: Progress & Architecture Report

**Author:** Alireza Sotoodeh
**Version:** 1.0.0
**Date:** June 3, 2026

**Name of project:** DeadReckoner

---

## 1. Hardware Inventory

The following components are currently available for the development and testing of the tracking system:

### Microcontrollers

* **NodeMCU ESP8266MOD:** Current prototype board. Features a single-core processor and limited memory.
* **ESP32-S3 N16R8:** Advanced dual-core MCU with 16MB Flash and 8MB PSRAM. Built for heavy IoT and AI/data logging tasks.
* **ESP32 WIFI+BT Type-C:** Standard ESP32 dual-core module with modern USB-C interface.
* **ESP32 ROHS:** Standard compliant ESP32 variant.

### Sensors & Modules

* **MPU9250:** 9-axis IMU (3-axis Accel, 3-axis Gyro, 3-axis Mag). Excellent for high-precision sensor fusion.
* **MPU6500:** 6-axis IMU (Accel + Gyro). Lacks magnetometer, making it prone to yaw drift over time.
* **GY-25:** Tilt/serial angle sensor module (usually incorporates an MPU6050 with an onboard MCU for angle calculation).
* **HW-123:** Generic module designation (often associated with specific sensor breakouts; requires specific part verification).
* **BMP280:** Barometric pressure and temperature sensor. Highly accurate for altitude tracking.
* **S6MV2 (GPS):** GNSS module for global positioning. Essential for outdoor signal-free tracking.
* **OLED 0.91-inch:** I2C monochrome display (128x32) for live status monitoring.

---

## 2. Microcontroller Comparison

| Feature / MCU        | NodeMCU (ESP8266)    | ESP32 (Standard)    | ESP32-S3 (N16R8)                     |
|:-------------------- |:-------------------- |:------------------- |:------------------------------------ |
| **Cores**            | 1 (Tensilica L106)   | 2 (Xtensa LX6)      | 2 (Xtensa LX7 + Vector instructions) |
| **Clock Speed**      | 80 / 160 MHz         | 160 / 240 MHz       | 240 MHz                              |
| **SRAM**             | ~50 KB usable        | 520 KB              | 512 KB                               |
| **External RAM**     | None                 | Usually None        | **8 MB PSRAM**                       |
| **Flash Memory**     | 4 MB                 | 4 MB                | **16 MB**                            |
| **Active Power**     | ~80 mA               | ~160 mA             | ~240 mA (w/ PSRAM active)            |
| **Deep Sleep**       | ~20 µA               | ~10 µA              | **~7 µA**                            |
| **Hardware I2C**     | 1 Bus (Often shared) | 2 Independent Buses | 2 Independent Buses                  |
| **Est. Price (USD)** | $3.00 - $4.00        | $4.00 - $6.00       | $7.00 - $10.00                       |

---

## 3. Best Choices & Justifications

Based on the goal of creating an **Offline Logging System**, the recommended hardware stack is:

1. **Core Controller: ESP32-S3 N16R8.** * *Reasoning:* The massive 16MB Flash allows for extensive onboard LittleFS data logging without needing an immediate SD Card module. The 8MB PSRAM handles large data buffers. The dual-core architecture is mandatory to separate high-frequency IMU reading (Core 0) from low-frequency SD writing/GPS reading (Core 1), preventing sensor data loss (blocking delays).
2. **Primary IMU: MPU9250.** * *Reasoning:* The 9-axis capability allows the Madgwick filter to correct yaw drift using the magnetometer, which is impossible with the 6-axis MPU6500.
3. **Positioning: S6MV2 GPS.**
   * *Reasoning:* Provides the absolute global coordinates needed to anchor the relative movements calculated by the IMU.

---

## 4. Codebase Analysis & Logic Breakdown

### 4.1 NodeMCU Firmware (`nodeMUC8266.ino`)

The current firmware serves as a data-streaming prototype. It initializes the IMU, applies a Madgwick filter, and streams quaternion and acceleration data via Serial.

**Key Logic Blocks:**

* **Sensor Initialization & Madgwick Filter:** The MPU9250 is configured via Hardware I2C. The Madgwick filter is used to fuse raw sensor data into quaternions (`Qw, Qx, Qy, Qz`). This is computationally heavy but mathematically superior to Euler angles (avoids gimbal lock).
* **OLED Separation:** The OLED uses Software I2C (`U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C`). *Logic:* This intentionally prevents the slow display updates from congesting the Hardware I2C bus, ensuring the IMU can be sampled rapidly.
* **EEPROM Calibration Routine (Flawed):** The code initiates a calibration sequence and saves bias data to EEPROM. However, the `loadCalibration()` function retrieves the data but lacks the setter methods to apply them back to the `mpu` object, rendering the saved calibration inert.
* **Serial Bottleneck:** Data is transmitted using `Serial.print()` with float-to-string conversion. This is highly inefficient for data logging and will consume significant CPU cycles.

### 4.2 MATLAB Visualization (`visual.m`)

A script designed to parse the incoming serial stream and render a real-time 3D orientation vector.

**Key Logic Blocks:**

* **Serial Handling:** Uses `serialport` and waits for a valid 4-value string. It employs a `try-catch` block, ensuring the script does not crash if corrupted data arrives over the serial line.
* **Math Transformation (`quat2rotm`):** Converts the incoming quaternion stream into a 3x3 Rotation Matrix. It extracts the X, Y, and Z column vectors to plot the 3D quiver arrows, successfully translating mathematical orientation into visual space.

---

## 5. Next Steps Roadmap (To-Do List)

To transform this prototype into an industrial-grade offline tracker, we will tackle the following phases sequentially:

- [ ] **Phase 1: Hardware Migration & Architecture Update**
  - Port the existing codebase from ESP8266 to ESP32-S3.
  - Implement FreeRTOS tasks (Task 1: IMU reading on Core 0, Task 2: Data formatting/Logging on Core 1).
- [ ] **Phase 2: Fix Calibration & I2C Optimization**
  - Resolve the EEPROM load issue so calibration applies correctly on boot.
  - Move OLED and MPU9250 to separate *Hardware* I2C buses using the ESP32's `Wire` and `Wire1` interfaces.
- [ ] **Phase 3: Binary Logging Implementation**
  - Define a strict `C struct` for the data packet (Timestamp, Quaternions, Acceleration, GPS coords).
  - Implement LittleFS / SD Card write operations using block binary writes (`file.write((uint8_t*)&data, sizeof(data))`) instead of string conversion.
- [ ] **Phase 4: GPS Integration & Data Synchronization**
  - Integrate S6MV2 reading via hardware UART.
  - Develop an interpolation/sync algorithm to match 1Hz GPS data with 100Hz IMU data.
