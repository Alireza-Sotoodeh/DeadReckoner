# Signal-Free Offline Tracking System: Progress & Architecture Report

**Author:** Alireza Sotoodeh

**Version:** 1.0.0 

**Date:** June 3, 2026

**Name of project:** DeadReckoner

---

## 1. Hardware Inventory

The following components are currently available for the development and testing of the tracking system:

### Microcontrollers

- **NodeMCU ESP8266MOD:** Current prototype board. Features a single-core processor and limited memory.
- **ESP32-S3 N16R8:** Advanced dual-core MCU with 16MB Flash and 8MB PSRAM. Built for heavy IoT and AI/data logging tasks.
- **ESP32 WIFI+BT Type-C:** Standard ESP32 dual-core module with modern USB-C interface.

### Sensors & Modules

- **MPU9250:** 9-axis IMU (3-axis Accel, 3-axis Gyro, 3-axis Mag). Excellent for high-precision sensor fusion.
- **MPU6500:** 6-axis IMU (Accel + Gyro). Lacks magnetometer, making it prone to yaw drift over time.
- **GY-25:** Tilt/serial angle sensor module (usually incorporates an MPU6050 with an onboard MCU for angle calculation).
- **HW-123:** Generic module designation (often associated with specific sensor breakouts; requires specific part verification).
- **BMP280:** Barometric pressure and temperature sensor. Highly accurate for altitude tracking.
- **S6MV2 (GPS):** GNSS module for global positioning. Essential for outdoor signal-free tracking.
- **OLED 0.91-inch:** I2C monochrome display (128x32) for live status monitoring.

---

## 2. Microcontroller Comparison

| Feature / MCU        | NodeMCU (ESP8266)    | ESP32 (Standard)    | ESP32-S3 (N16R8)                     |
| -------------------- | -------------------- | ------------------- | ------------------------------------ |
| **Cores**            | 1 (Tensilica L106)   | 2 (Xtensa LX6)      | 2 (Xtensa LX7 + Vector instructions) |
| **Clock Speed**      | 80 / 160 MHz         | 160 / 240 MHz       | 240 MHz                              |
| **SRAM**             | ~50 KB usable        | 520 KB              | 512 KB                               |
| **External RAM**     | None                 | Usually None        | **8 MB PSRAM**                       |
| **Flash Memory**     | 4 MB                 | 4 MB                | **16 MB**                            |
| **Active Power**     | ~80 mA               | ~160 mA             | ~240 mA (w/ PSRAM active)            |
| **Deep Sleep**       | ~20 µA               | ~10 µA              | **~7 µA**                            |
| **Hardware I2C**     | 1 Bus (Often shared) | 2 Independent Buses | 2 Independent Buses                  |
| **Est. Price (USD)** | 3.00 - 4.00          | 4.00 - 6.00         | 7.00 - 10.00                         |

---

## 3. Best Choices & Justifications

Based on the goal of creating an **Offline Logging System**, the recommended hardware stack is:

1. **Core Controller: `ESP32-S3 N16R8`.**
   - **Reason:** The massive 16MB Flash allows for extensive onboard LittleFS data logging without needing an immediate SD Card module. The 8MB PSRAM handles large data buffers. The dual-core architecture is mandatory to separate high-frequency IMU reading (Core 0) from low-frequency SD writing/GPS reading (Core 1), preventing sensor data loss (blocking delays).
2. **Primary IMU: `MPU9250`.** 
   - ***Reasoning:*** The 9-axis capability allows the Madgwick filter to correct yaw drift using the magnetometer, which is impossible with the 6-axis MPU6500.
3. **Positioning: S6MV2 GPS.**
   - ***Reasoning:*** Provides the absolute global coordinates needed to anchor the relative movements calculated by the IMU.

---

## 4. Codebase Analysis & Logic Breakdown

### 4.1 NodeMCU Firmware (`nodeMUC8266.ino`)

The current firmware serves as a data-streaming prototype. It initializes the IMU, applies a Madgwick filter, and streams quaternion and acceleration data via Serial.

**Key Logic Blocks:**

- **Sensor Initialization & Madgwick Filter:** The MPU9250 is configured via Hardware I2C. The Madgwick filter is used to fuse raw sensor data into quaternions (`Qw, Qx, Qy, Qz`). This is computationally heavy but mathematically superior to Euler angles (avoids gimbal lock).
- **OLED Separation:** The OLED uses Software I2C (`U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C`). *Logic:* This intentionally prevents the slow display updates from congesting the Hardware I2C bus, ensuring the IMU can be sampled rapidly.
- **EEPROM Calibration Routine (Flawed):** The code initiates a calibration sequence and saves bias data to EEPROM. However, the `loadCalibration()` function retrieves the data but lacks the setter methods to apply them back to the `mpu` object, rendering the saved calibration inert.
- **Serial Bottleneck:** Data is transmitted using `Serial.print()` with float-to-string conversion. This is highly inefficient for data logging and will consume significant CPU cycles.

### 4.2 MATLAB Visualization (`visual.m`)

A script designed to parse the incoming serial stream and render a real-time 3D orientation vector.

**Key Logic Blocks:**

- **Serial Handling:** Uses `serialport` and waits for a valid 4-value string. It employs a `try-catch` block, ensuring the script does not crash if corrupted data arrives over the serial line.
- **Math Transformation (`quat2rotm`):** Converts the incoming quaternion stream into a 3x3 Rotation Matrix. It extracts the X, Y, and Z column vectors to plot the 3D quiver arrows, successfully translating mathematical orientation into visual space.

---

## 5. Hardware Migration Rationale: ESP8266 to ESP32-S3

Before finalizing the software architecture, it is crucial to document the exact engineering reasons for migrating from the `NodeMCU (ESP8266)` to the `ESP32-S3`. The tracking system's requirements have outgrown the physical limitations of the legacy ESP8266 chip.

1. **Processing Bottlenecks (Single vs. Dual Core):** The ESP8266 is a single-core processor. It cannot read the MPU9250 at 100Hz, apply the complex Madgwick filter, and write data to an SD card simultaneously without blocking delays. The ESP32-S3 provides a dual-core architecture, allowing strict separation of sensor fusion (Core 0) and data logging (Core 1).
2. **Memory Constraints:** The ESP8266 has limited usable SRAM (~50 KB) and Flash memory (4 MB). Buffer queues for offline logging quickly cause memory overflow. The ESP32-S3 variant selected (N16R8) offers massive headroom with 16 MB of Flash and 8 MB of PSRAM, enabling robust data buffering.
3. **Peripheral Routing:** The ESP32-S3 features a complete IO MUX matrix, allowing us to map the Hardware I2C buses to any GPIO pin, eliminating the bus congestion issues seen on the ESP8266 prototype.

---

## 6. Software Architecture & Development Phases(toDo List)

These phases follow a strict dependency sequence. Each phase acts as a prerequisite for the next.

#### Phase 1: Hardware Migration & RTOS Architecture [COMPLETED]

- [x] **Pin Porting:** Update hardware pin definitions to match the `ESP32-S3 N16R8` IO MUX matrix. (my hardware was `ESP8266 NodeMCU`)
- [x] **FreeRTOS Implementation:** Decouple system logic into `sensorTask` (Core 0, high-frequency) and `loggingTask` (Core 1, low-frequency).
- [x] **Race Condition Mitigation:** Implement Producer-Consumer pattern using a 48-byte binary `LogFrame` struct and a thread-safe `xQueue` buffer.

#### Phase 2: Calibration Fixes & I2C Optimization [COMPLETED]

- [x] **I2C Acceleration:** Boost hardware I2C bus clock to 400kHz (Fast Mode) to prevent IMU read bottlenecks.
- [x] **Bus Collision Prevention:** Utilize `vTaskSuspend` and `vTaskResume` to freeze Core 0 during Core 1's blocking EEPROM routines.
- [x] **EEPROM Load Bug Fix:** Implement manual deduction of loaded EEPROM bias values from raw sensor readings.
- [x] **I2C Bus Separation:** Transition the OLED display to the secondary hardware I2C bus (`Wire1`).

#### Phase 3: Hardware Validation & Storage Selection [COMPLETED]

- [x] **MPU9250 Sanity Check**
- [x] **0.91 inch oled Sanity Check**
- [x] **Storage Medium Selection:** Finalize DIY SD Adapter directly connected to the ESP32 on the pure 3.3V logic rail. (Cap SPI frequency at 10 MHz to guarantee signal integrity over physical wires.)

#### Phase 4: Binary Logging Implementation [PENDING]

- [ ] **Binary Block Writing:** Replace string-based `Serial.print` operations with high-speed binary block writes (`file.write()`).
- [ ] **Fail-Safe Strategy:** Code a periodic `file.flush()` routine to secure data integrity against sudden power loss.

#### Phase 5: GPS Integration & Data Synchronization [PENDING]

- [ ] **Hardware UART Initialization:** Configure the secondary serial port (UART1/2) strictly for the S6MV2 GNSS module.
- [ ] **Data Injection:** Parse and inject double-precision coordinates (`gps_lat`, `gps_lng`) into the `LogFrame` queue.
- [ ] **Time Synchronization Algorithm:** Develop temporal interpolation to align 1Hz GPS data with the 100Hz IMU stream.

---

###### 7. Firmware Flashing Configuration (`ESP32-S3 N16R8`)

*Comprehensive Arduino IDE settings required to utilize the full 16MB Flash and 8MB PSRAM, ensuring stable FreeRTOS execution and maximum data logging capacity.*

- **Board:** ESP32S3 Dev Module
- **USB CDC On Boot:** Disabled
- **CPU Frequency:** 240MHz (WiFi)
- **Core Debug Level:** None
- **USB DFU On Boot:** Disabled
- **Erase All Flash Before Sketch Upload:** Disabled
- **Events Run On:** Core 1
- **Flash Mode:** QIO 80MHz
- **Flash Size:** 16MB (128Mb)
- **JTAG Adapter:** Disabled
- **Arduino Runs On:** Core 1
- **USB Firmware MSC On Boot:** Disabled
- **Partition Scheme:** 16M Flash (e.g., 3MB APP/9.9MB FATFS) *[CRITICAL: Must not be 4MB default]*
- **PSRAM:** OPI PSRAM
- **Upload Mode:** UART0 / Hardware CDC
- **Upload Speed:** 921600
- **USB Mode:** Hardware CDC and JTAG
- **Zigbee Mode:** Disabled

---

## 7. Multi-Core Architecture & Concurrency Management

Moving to a dual-core processor introduces a critical system design challenge: **Concurrency and Shared Memory Interference**.

### 7.1 The Race Condition Problem

In a dual-core tracking system, tasks operate at vastly different frequencies:

- **Core 0 (Sensor Fusion):** Reads the IMU and calculates quaternions continuously at high speeds (100 Hz).
- **Core 1 (Data Logging):** Writes data to non-volatile memory (LittleFS or SD Card). Flash write operations are slow and inherently blocking.

If both cores attempt to access the same global orientation variables simultaneously, a **Race Condition** occurs. Core 1 might read an incomplete data set before Core 0 finishes updating it, resulting in corrupted logs and destroying the 3D trajectory reconstruction in MATLAB. Standard Mutex locks are not viable here, as locking the data during a slow SD card write would force Core 0 to wait, dropping critical high-frequency IMU reads.

### 7.2 The Solution: Producer-Consumer Pattern via FreeRTOS Queues

To completely decouple the cores while ensuring 100% data integrity, the system utilizes the **Producer-Consumer architecture** using FreeRTOS Queues.

1. **Data Encapsulation:** All variables for a single point in time are packed into a rigid `C struct`. Crucially, to prevent precision loss (truncation) that causes map-drift, GPS coordinates are defined as 64-bit `double` types.
2. **The Queue (Buffer):** A thread-safe FIFO (First-In, First-Out) queue is allocated in the ESP32's RAM.
3. **Task Separation:** Core 0 (Producer) reads sensors and pushes structs to the back of the queue without blocking. Core 1 (Consumer) wakes up, pops blocks from the front of the queue, and writes them to storage in binary format.

### 7.3 Core Data Structure & Memory Calculation

```c
// Data structure for a single logging frame (Binary Logging)
typedef struct {
 uint32_t timestamp; // 4 bytes: Time since boot
 float q[4]; // 16 bytes: Quaternions (qw, qx, qy, qz)
 float accel[3]; // 12 bytes: Accelerations (ax, ay, az)
 double gps_lat; // 8 bytes: Latitude (cm-level precision)
 double gps_lng; // 8 bytes: Longitude (cm-level precision)
} LogFrame; // Total Size: 48 Bytes per frame

// FreeRTOS Queue Handle declaration
QueueHandle_t dataQueue;
```

**Buffer Sizing Analysis:** To ensure zero data loss during high-latency SD card operations, the system must buffer data.

- **Target Frequency:** 100 Hz (100 frames/sec)

- **Target Buffer Duration:** 3 seconds of maximum latency tolerance

- **Queue Depth required:** 300 items

- **RAM Footprint:** 300 items * 48 bytes = **14,400 bytes (14.06 KB)** This memory footprint is safely accommodated by the ESP32-S3's internal SRAM, leaving the 8MB PSRAM completely free for larger operational tasks.

---

## 8. Hardware & Filter Validation (Test Results)

To ensure the reliability of the Dead Reckoning system during periods of GPS signal loss, the MPU9250 IMU and the FreeRTOS-based Madgwick filter underwent three rigorous physical tests. The raw quaternion data was streamed to MATLAB (`visual.m`) for mathematical analysis.

### 8.1 Test 1: Static Drift (Zero-Rate Offset)

- **Objective:** Measure the accumulated error (drift) over a 15-minute stationary period to evaluate baseline stability.
- **Methodology:** Device kept completely still on an isolated, flat surface for ~900 seconds post-calibration.
- **Results:**
  - **Max Roll Drift:** 0.43°
  - **Max Pitch Drift:** 0.39°
  - **Max Yaw Drift:** 2.73° (over 15 minutes)
- **Conclusion:** **PASS.** The accelerometer perfectly isolates the gravity vector, effectively eliminating Pitch/Roll drift (<0.5°). The magnetometer bounds the Yaw drift to ~0.18° per minute, proving high immunity against false map rotations during prolonged GPS outages.

### 8.2 Test 2: Dynamic Return-to-Zero

- **Objective:** Evaluate the filter's ability to recover from high-G linear accelerations without losing the true horizon.
- **Methodology:** Device subjected to violent 3D rotations and linear accelerations (up to 130° swings) for 30 seconds, then returned exactly to its initial physical position.
- **Results (Absolute Return Error):**
  - **Roll Error:** 0.07°
  - **Pitch Error:** 0.36°
  - **Yaw Error:** 1.55°
- **Conclusion:** **PASS.** The Madgwick filter's `Beta` gain is optimally tuned. The system successfully ignores temporary linear accelerations (preventing them from being mistaken as gravity) and snaps back to the true orientation with sub-degree accuracy.

### 8.3 Test 3: Vibration Rejection

- **Objective:** Test the system's resilience against high-frequency mechanical noise (e.g., vehicle engine vibrations or footsteps).
- **Methodology:** Device subjected to external mechanical shocks, including light pen tapping and heavy fist pounds on the adjacent surface.
- **Results (Max Deviation during impact):**
  - **Pen Tapping (Roll/Pitch Swing):** < 0.1° deviation
  - **Heavy Fist Pound (Max Pitch Swing):** 0.57° deviation
- **Conclusion:** **PASS.** The hardware Digital Low Pass Filter (DLPF) configured at `5Hz` effectively isolates the IMU from environmental vibrations. The tracking coordinates will remain highly stable even under harsh physical or automotive conditions.

---

## 9. Storage Hardware Validation (SD Card Diagnostics)

**Objective:** Validate the physical stability of SD Card modules on 3.3V logic for 100Hz continuous logging and eliminate SPI communication bottlenecks prior to RTOS implementation.

### 9.1 Cross-Platform Diagnostic Results

| Test Platform         | Hardware Tested      | SPI Frequency | Result | Engineering Conclusion & Root Cause                                                                                                                  |
|:--------------------- |:-------------------- |:------------- |:------ |:---------------------------------------------------------------------------------------------------------------------------------------------------- |
| **NodeMCU (ESP8266)** | Commercial 5V Module | Standard      | FAILED | **Level Shifter Trap:** The 5V logic level shifter fails to trigger on weak 3.3V ESP lines. Also susceptible to voltage dropouts during boot.        |
| **NodeMCU (ESP8266)** | DIY 3.3V Adapter     | Standard      | FAILED | **Signal Distortion:** The high default SPI speed of the ESP8266 combined with jumper wires caused severe signal degradation.                        |
| **Arduino UNO**       | Commercial 5V Module | Standard      | PASSED | **5V Logic Match:** Pure 5V logic compatibility confirmed. This test proved the commercial module and the MicroSD card are 100% healthy.             |
| **ESP32 Classic**     | DIY 3.3V Adapter     | 50 MHz        | FAILED | **Signal Integrity Loss:** The ultra-high frequency caused an antenna effect on the jumper wires, corrupting the data blocks during read operations. |
| **ESP32 Classic**     | DIY 3.3V Adapter     | 10 MHz        | PASSED | **Optimal Stability:** Lowering the clock stabilized the signal. SDHC card and FAT32 volume successfully mounted, read, and written.                 |

### 9.2 Key Architectural Decisions: Storage & Bandwidth

* **Hardware Choice:** The DIY Adapter (pure 3.3V logic without onboard regulators or level shifters) is the finalized hardware for the ESP32 architecture, bypassing unnecessary voltage conversions.
* **Clock Limitation:** The SPI clock frequency is strictly capped at **10 MHz** to guarantee signal integrity over physical wires.
* **Bandwidth Validation:** 100Hz data logging requires approximately **10 KB/s** of bandwidth. A 10 MHz SPI clock provides practical write speeds of over **200 KB/s**, leaving a massive 95% safety margin to prevent RTOS queue bottlenecks.
* **Library Selection:** The `SdFat` library (version 1.1.4 for API compatibility) will be utilized for low-level memory block access, accurate file system operations, and long-term FAT32 stability.

the result of testind DIY adaptor is like this:

```
Jul 29 2019 12:21:46

rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
configsip: 0, SPIWP:0xee
clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
mode:DIO, clock div:1
load:0x3fff0030,len:4980
load:0x40078000,len:16612
load:0x40080400,len:3480
entry 0x400805b4

--- HIGH-PRECISION SPI SWEEP (1MHz Steps) ---
Freq(MHz) | Result | Speed(KB/s)
------------------------------------
1 MHz    | PASSED | 93.57 KB/s
2 MHz    | PASSED | 175.34 KB/s
3 MHz    | PASSED | 226.35 KB/s
4 MHz    | PASSED | 278.56 KB/s
5 MHz    | PASSED | 365.19 KB/s
6 MHz    | PASSED | 399.38 KB/s
7 MHz    | PASSED | 440.62 KB/s
8 MHz    | PASSED | 465.03 KB/s
9 MHz    | PASSED | 532.22 KB/s
10 MHz    | PASSED | 555.31 KB/s
11 MHz    | PASSED | 554.71 KB/s
12 MHz    | PASSED | 568.26 KB/s
13 MHz    | PASSED | 608.08 KB/s
14 MHz    | PASSED | 638.40 KB/s
15 MHz    | PASSED | 638.40 KB/s
16 MHz    | PASSED | 709.14 KB/s
17 MHz    | PASSED | 709.14 KB/s
18 MHz    | PASSED | 709.14 KB/s
19 MHz    | PASSED | 709.14 KB/s
20 MHz    | PASSED | 797.51 KB/s
21 MHz    | PASSED | 791.34 KB/s
22 MHz    | PASSED | 797.51 KB/s
23 MHz    | PASSED | 797.51 KB/s
24 MHz    | PASSED | 797.51 KB/s
25 MHz    | PASSED | 727.27 KB/s
26 MHz    | PASSED | 797.51 KB/s
27 MHz    | FAILED | N/A
--- LIMIT REACHED. SYSTEM UNSTABLE ABOVE THIS FREQ. ---
--- SWEEP FINISHED ---


```

---

## 10. High-Speed Binary Logging & Offline Data Pipeline

### 4.1 Hardware Pinout Overhaul (Collision Avoidance)

To prevent hardware bus collisions between the high-speed SD card and the MPU9250 sensor, the SPI pins were migrated to the dedicated hardware FSPI bus of the ESP32-S3. 

- **MPU9250 (I2C):** GPIO 4 (SDA), GPIO 5 (SCL)
- **SD Card (FSPI):** GPIO 10 (CS), GPIO 11 (MOSI), GPIO 12 (SCK), GPIO 13 (MISO)
- **OLED (Software I2C):** GPIO 6 (SDA), GPIO 7 (SCL)

### 4.2 Binary Data Structure (`LogFrame`)

To maximize write speeds and prevent string conversion overhead, data is structured into a precise 48-byte C-struct:

- `uint32_t timestamp` (4 bytes)
- `float q[4]` (16 bytes) - Quaternions
- `float accel[3]` (12 bytes) - Linear Acceleration
- `double gps_lat`, `gps_lng` (16 bytes) - Auxiliary GPS data

### 4.3 Storage Strategy & File Generation

The system utilizes the `SdFat` library running on Core 1 at a locked SPI frequency of 20 MHz. The system generates three distinct binary files during operations:

1. `SWEEP.BIN`: Used for finding the maximum stable SPI frequency (failed at 27MHz, stabilized at 20MHz).
2. `STRESS.BIN`: Used for burst-writing 16KB buffers to test bandwidth capabilities.
3. `DR_LOG.BIN`: The primary vault containing the 100Hz continuous 48-byte `LogFrame` data.

### 4.4 Navigation Strategy Shift: Offline ZUPT

A critical architectural decision was made to **abandon real-time double integration on the ESP32**. Due to MEMS bias instability, live double integration leads to exponential quadratic drift. Instead, the project shifted to an **Offline Pedestrian Dead Reckoning (PDR)** approach using the **Zero Velocity Update (ZUPT)** technique via MATLAB/Python scripts.

### Phase 4 Summary Table

| Milestone / Task          | Status    | Detail / Specification                                           |
|:------------------------- |:---------:|:---------------------------------------------------------------- |
| **I2C/SPI Pin Isolation** | Completed | MPU on GPIO 4/5, SD on FSPI 10-13, OLED on GPIO 6/7.             |
| **Binary Data Struct**    | Completed | 48-byte `LogFrame` structure optimized for RAM alignment.        |
| **SD Storage Pipeline**   | Completed | `SdFat` library integration, writing `DR_LOG.BIN` continuously.  |
| **Data Parsing Script**   | Completed | MATLAB/NumPy script developed to instantly parse 48-byte frames. |
| **Navigation Algorithm**  | Shifted   | Moved from live MCU integration to Offline ZUPT modeling.        |

---

### Challenges & Solutions (Phase 4)

- **Challenge:** System crashing/freezing when accessing the MPU9250 and SD Card simultaneously.
  
  - **Solution:** Moved SD card to pins 10-13 (FSPI) and kept MPU on pins 4-5. 
  - **Educational Depth:** In ESP32-S3, sharing pins or internal bus matrices for high-speed SPI (20MHz) and delicate I2C logic causes interrupt starvation. Physical bus isolation is mandatory for stability in RTOS environments.

- **Challenge:** `Serial.print()` to the SD card (saving as plain text) was too slow and caused data loss at 100Hz.
  
  - **Solution:** Switched to binary block writing (`logFile.write`) of a raw 48-byte structure.
  - **Educational Depth:** Converting floating-point numbers to ASCII text is extremely CPU-intensive. Binary writing pushes raw zeroes and ones directly from RAM to the physical memory sectors, bypassing CPU calculation bottlenecks.

- **Challenge:** Double integration of accelerometer data for displacement caused kilometers of error within minutes.
  
  - **Solution:** Delayed integration to the post-processing phase using the ZUPT algorithm in MATLAB.
  - **Educational Depth:** MEMS sensors have inherent white noise. In double integration ($Error = \frac{1}{2} a_{error} t^2$), the error grows quadratically. ZUPT relies on the physical constraint of a foot hitting the ground to force the velocity back to absolute zero, thus killing the accumulated drift.
