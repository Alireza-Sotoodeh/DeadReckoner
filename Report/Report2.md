# Signal-Free Offline Tracking System: Progress & Architecture Report

**Author:** Alireza Sotoodeh  
**Project:** DeadReckoner  
**Version:** 2.0.0  
**Date:** June 12, 2026  

---

## 1. Hardware Inventory

The system was developed around a set of IMU, storage, display, and positioning modules.  
The final architecture prioritizes real-time sensing, reliable logging, and offline analysis.

### Microcontrollers

| Board                | Role               | Notes                                                              |
| -------------------- | ------------------ | ------------------------------------------------------------------ |
| NodeMCU ESP8266MOD   | Legacy prototype   | Single-core baseline used for the early IMU/display experiments.   |
| ESP32-S3 N16R8       | Final controller   | Dual-core MCU with 16MB Flash and 8MB PSRAM for real-time logging. |
| ESP32 WIFI+BT Type-C | Auxiliary platform | Used as a reference ESP32-class board during development.          |

### Microcontroller Comparison

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

### Sensors & Modules

| Module         | Function          | Notes                                                          |
| -------------- | ----------------- | -------------------------------------------------------------- |
| MPU9250        | 9-axis IMU        | Main motion sensor for quaternion and acceleration estimation. |
| MPU6500        | 6-axis IMU        | Early prototype sensor used during the research phase.         |
| GY-25          | Tilt sensor       | Used for comparison and sensor-validation experiments.         |
| HW-123         | Generic module    | Included in early hardware exploration and verification.       |
| BMP280         | Barometric sensor | Reserved for altitude-related extensions.                      |
| S6MV2 (GPS)    | GNSS module       | Planned for global positioning and time anchoring.             |
| OLED 0.91-inch | Status display    | Used for runtime feedback, menus, and error reporting.         |

---

## 2. Project Evolution Summary

The project started as a small IMU-based prototype and gradually evolved into a multi-core offline tracking system.  
Each major stage introduced one architectural layer: sensing, fusion, storage, validation, fault handling, and analysis.

| Milestone              | Meaning                                               |
| ---------------------- | ----------------------------------------------------- |
| Legacy IMU prototype   | Initial sensor research and driver bring-up.          |
| ESP32-S3 migration     | Hardware upgrade for concurrency and memory headroom. |
| RTOS architecture      | Separation of sensing and logging workloads.          |
| Persistent calibration | EEPROM-backed bias storage and restore flow.          |
| SD logging             | Reliable binary storage on external media.            |
| Fault handling         | Safe shutdown and recovery on sensor failure.         |
| Validation tools       | MATLAB-based analysis and result inspection.          |
| GPS preparation        | GNSS-ready data structure for future fusion.          |

---

## 3. Hardware Choices & Rationale

The final hardware stack was selected to eliminate the bottlenecks that appeared in the prototype phase.  
The choices below reflect both the commit history and the test results collected during integration.

### 3.1 Core Controller

**ESP32-S3 N16R8** was selected as the main controller.  
The dual-core CPU, PSRAM, and larger Flash space made it suitable for continuous IMU acquisition and blocking storage operations.

### 3.2 Primary IMU

**MPU9250** became the preferred IMU.  
It provides accelerometer, gyroscope, and magnetometer data, which improves long-term orientation tracking compared with the MPU6500.

### 3.3 Storage Medium

A **3.3V DIY SD adapter** was finalized for logging.  
The adapter was chosen after multiple failed and successful tests with other SD card modules and SPI clock settings.

### 3.4 Display Layer

The **0.91-inch OLED** was used for runtime diagnostics and menu-driven interaction.  
It was separated from the sensor bus to avoid blocking the high-frequency acquisition loop.

---

## 4. System Challenges & Engineering Decisions

This section summarizes the main engineering problems that shaped the final architecture.  
Each problem led to a concrete design decision rather than a temporary workaround.

### Memory and Throughput Limits

The ESP8266 prototype could not safely handle continuous sensing, display updates, and logging at the same time.  
The project moved to ESP32-S3 to gain dual-core execution, PSRAM, and more flexible peripheral routing.

### I2C Bus Contention

The IMU, EEPROM, and OLED initially shared a constrained communication path.  
The OLED was moved to a separate I2C bus so display updates would not interfere with sensor timing.

### Blocking Storage Operations

Text-based logging and slow file writes created data loss risk at high sample rates.  
The logging layer was redesigned around binary blocks and a queue-based producer-consumer model.

### Reliability Under Fault Conditions

Unexpected IMU disconnection could leave the system half-active and corrupt the active log file.  
A dedicated fault layer was added to close files safely, signal the operator, and stop unsafe execution.

---

## 5. Software Architecture Evolution & Development Phases

The development path is organized as a staged roadmap rather than a simple task list.  
Each phase introduced a dependency that was required by the next one.

#### Phase 0: Legacy Prototype & Sensor Research [COMPLETED]

The first phase established the baseline firmware and tested the early IMU stack.

- [x] **Project Bootstrap:** Create the initial repository and firmware skeleton.
- [x] **MPU6500 Evaluation:** Bring up the early IMU driver and validate communication.
- [x] **DMP Exploration:** Test Digital Motion Processor support for orientation output.
- [x] **Calibration Research:** Study raw motion data and sensor bias behavior.
- [x] **Display Prototyping:** Add an OLED feedback layer for quick debugging.
- [x] **Prototype Validation:** Confirm basic orientation and acceleration reading.

#### Phase 1: ESP32-S3 Migration & System Architecture [COMPLETED]

The second phase moved the project to a stronger controller and reorganized the hardware.

- [x] **Platform Migration:** Move from ESP8266 NodeMCU to ESP32-S3 N16R8.
- [x] **Firmware Rebuild:** Reinitialize the project around the new board layout.
- [x] **Pin Porting:** Remap the sensor and display pins to the ESP32-S3 IO matrix.
- [x] **Wiring Redesign:** Update the hardware connections for the new controller.
- [x] **Flash Configuration:** Prepare the board settings for large-memory operation.
- [x] **Bring-Up Verification:** Validate the migrated platform before adding complexity.

#### Phase 2: RTOS & Multi-Core Framework [COMPLETED]

This phase replaced the single-flow firmware with a real-time multi-core design.

- [x] **FreeRTOS Integration:** Introduce task-based scheduling.
- [x] **Sensor Task:** Run high-rate acquisition on Core 0.
- [x] **Logging Task:** Run blocking storage work on Core 1.
- [x] **Producer-Consumer Queue:** Pass complete log frames between tasks.
- [x] **Thread-Safe Communication:** Avoid shared-state corruption across cores.
- [x] **Race Condition Mitigation:** Remove direct concurrent memory access.
- [x] **Timing Stability:** Keep acquisition deterministic under continuous load.

#### Phase 3: Calibration & I2C Optimization [COMPLETED]

This phase improved sensor accuracy and reduced bus-level interference.

- [x] **I2C Fast Mode:** Raise the bus frequency to 400 kHz.
- [x] **EEPROM Bias Storage:** Save calibration values for later reuse.
- [x] **Bias Restoration:** Load the saved values automatically at startup.
- [x] **Offset Compensation Fix:** Apply the loaded calibration to raw readings.
- [x] **Bus Collision Prevention:** Pause acquisition during blocking EEPROM access.
- [x] **Dual-I2C Topology:** Move OLED traffic to the secondary bus.
- [x] **I2C Validation:** Verify that the buses remained stable in normal use.

#### Phase 4: Hardware Validation & Performance Testing [COMPLETED]

This phase proved that the sensor stack was stable before adding storage complexity.

- [x] **MPU9250 Sanity Check:** Validate the IMU and its register communication.
- [x] **OLED Sanity Check:** Confirm display initialization and output.
- [x] **Static Drift Test:** Measure long-term orientation drift while stationary.
- [x] **Dynamic Return-to-Zero:** Check recovery after large motion.
- [x] **Vibration Rejection Test:** Verify robustness against physical noise.
- [x] **Filter Validation:** Confirm that the fusion output stayed consistent.
- [x] **Analysis Tools:** Create visualization scripts for offline review.

#### Phase 5: SD Card Storage Architecture [COMPLETED]

The storage subsystem was selected and tested as a reliable logging target.

- [x] **SD Interface Study:** Compare several SD card module options.
- [x] **SPI Storage Path:** Adopt SPI as the physical logging interface.
- [x] **Standalone SD Tests:** Validate the module without the full firmware stack.
- [x] **Adapter Selection:** Finalize the 3.3V DIY SD adapter.
- [x] **Signal Integrity Tuning:** Reduce the SPI clock to a stable operating point.
- [x] **Storage Benchmarking:** Measure practical write throughput.
- [x] **Final Validation:** Confirm successful logging under repeated tests.

#### Phase 6: Binary Logging Framework [COMPLETED]

The text-based logging path was replaced with a structured binary pipeline.

- [x] **LogFrame Design:** Define a fixed-size telemetry frame.
- [x] **Timestamp Logging:** Add time stamps to every recorded sample.
- [x] **Quaternion Logging:** Store fused orientation values.
- [x] **Acceleration Logging:** Store gravity-compensated linear acceleration.
- [x] **Binary Write Path:** Replace string output with raw binary writes.
- [x] **Continuous Streaming:** Keep the logging pipeline active at high rate.
- [x] **Sequential File Naming:** Generate unique log files automatically.
- [x] **Boot-Time Discovery:** Scan the storage and compute the next file index.
- [x] **Capacity Display:** Show log counts and storage info on the OLED.

#### Phase 7: Fault Detection, Recovery & Mission Safety [COMPLETED]

A dedicated safety layer was added to handle IMU failure and protect recorded data.

- [x] **IMU Disconnect Detection:** Detect sensor communication loss.
- [x] **Timeout Monitoring:** Track the health of the I2C acquisition loop.
- [x] **Critical Fault State:** Stop unsafe execution after a fatal error.
- [x] **Safe Log Shutdown:** Close the file cleanly before stopping.
- [x] **Automatic Sync:** Flush buffered data before shutdown.
- [x] **OLED Fault Reporting:** Show readable diagnostic messages.
- [x] **Audible Alerts:** Use buzzer feedback for critical faults.
- [x] **Visual Alerts:** Use LED signaling for emergency status.
- [x] **Recovery Flow:** Add a reconnection-oriented recovery protocol.

#### Phase 8: User Interface & Operational Monitoring [COMPLETED]

This phase improved the operator experience and made the system easier to inspect.

- [x] **Multi-Page OLED UI:** Create simple runtime menu pages.
- [x] **Status Screens:** Show live system and sensor state.
- [x] **Storage Monitoring:** Display SD statistics and file counts.
- [x] **Display Modes:** Add selectable visualization modes.
- [x] **Buzzer Control:** Allow runtime mute and unmute behavior.
- [x] **SD Menu:** Add storage-related menu items.
- [x] **Diagnostics Screen:** Show faults and recovery messages clearly.

#### Phase 9: Data Analysis & Validation Toolchain [COMPLETED]

The recorded datasets were made usable through offline analysis tools.

- [x] **MATLAB Binary Reader:** Decode the logged binary frames.
- [x] **Quaternion Plotting:** Visualize orientation trends over time.
- [x] **Acceleration Plotting:** Inspect linear acceleration behavior.
- [x] **Drift Analysis:** Measure long-term navigation stability.
- [x] **Validation Pipeline:** Create a repeatable test-and-review workflow.
- [x] **Result Correlation:** Compare recorded data with physical tests.

#### Phase 10: GPS Integration & Time Synchronization [PLANNED]

The final extension will add GNSS support and time alignment.

- [x] **Reserved GPS Fields:** Prepare the log frame for GNSS data.
- [ ] **UART Configuration:** Add a dedicated serial line for the S6MV2 receiver.
- [ ] **NMEA Parser:** Decode GPS position and timing information.
- [ ] **Coordinate Injection:** Store latitude and longitude in the logging stream.
- [ ] **Status Monitoring:** Track satellite lock and GNSS health.
- [ ] **Time Sync:** Align 1 Hz GPS updates with 100 Hz IMU samples.
- [ ] **Trajectory Validation:** Verify synchronized offline reconstruction.

---

## 6. Firmware Flashing Configuration (`ESP32-S3 N16R8`)

The firmware was tuned for stable execution on the ESP32-S3 (for Arduino IDE programming)

| Setting                              | Value                                                                      |
|:------------------------------------:|:--------------------------------------------------------------------------:|
| Board                                | ESP32S3 Dev Module                                                         |
| USB CDC On Boot                      | Disabled                                                                   |
| CPU Frequency                        | 240 MHz                                                                    |
| Core Debug Level                     | None                                                                       |
| USB DFU On Boot                      | Disabled                                                                   |
| Erase All Flash Before Sketch Upload | Disabled                                                                   |
| Events Run On                        | Core 1                                                                     |
| Flash Mode                           | QIO 80 MHz                                                                 |
| Flash Size                           | 16MB (128Mb)                                                               |
| JTAG Adapter                         | Disabled                                                                   |
| Arduino Runs On                      | Core 1                                                                     |
| USB Firmware MSC On Boot             | Disabled                                                                   |
| Partition Scheme                     | 16M Flash (e.g., 3MB APP/9.9MB FATFS) -[CRITICAL: Must not be 4MB default] |
| PSRAM                                | OPI PSRAM                                                                  |
| Partition Scheme                     | Large flash / FATFS-friendly layout                                        |
| Upload Mode                          | UART0 / Hardware CDC                                                       |
| Upload Speed                         | 921600                                                                     |
| USB Mode                             | Hardware CDC and JTAG                                                      |
| Zigbee Mode                          | Disabled                                                                   |

---

## 7. Multi-Core Architecture & Concurrency Management

The dual-core design solved the biggest reliability issue in the system: blocking storage work.  
Sensor acquisition and logging now run independently without forcing one task to wait for the other.

### 7.1 Race Condition Problem

Core 0 collects data at high speed, while Core 1 performs slower storage operations.  
If both tasks touch the same data directly, partially updated values can reach storage and corrupt the log.

### 7.2 Producer-Consumer Solution

The system uses a FreeRTOS queue to move complete frames between tasks.  
Core 0 acts as the producer, and Core 1 acts as the consumer that writes finished frames to storage.

### 7.3 LogFrame Structure

```c
typedef struct {
  uint32_t timestamp;
  float q[4];
  float accel[3];
  double gps_lat;
  double gps_lng;
} LogFrame;
```

The fixed-size frame keeps the logging format deterministic and easy to decode offline.  
The structure also makes binary writing much faster than converting numbers to text.

### 7.4 Queue Sizing

A queue depth of 300 frames was selected to survive short storage stalls.  
At 100 Hz, that gives roughly 3 seconds of buffering and a safe margin against temporary blocking.

---

## 8. Hardware & Filter Validation (Test Results)

The IMU and fusion pipeline were verified through controlled physical tests.  
The outputs were recorded and inspected offline to confirm stability under different motion patterns.

### 8.1 Static Drift Test

The device was kept stationary for a long period after calibration.  
The test checked whether the estimated orientation stayed stable without external motion.

**Result:** The pitch and roll stayed close to zero, and yaw drift remained bounded.  
**Conclusion:** The filter was stable enough for long offline tracking sessions.

### 8.2 Dynamic Return-to-Zero Test

The device was moved aggressively and then returned to its original pose.  
This test checked whether the system could recover after large disturbances.

**Result:** The recovered orientation remained close to the initial one.  
**Conclusion:** The filter handled motion spikes without losing the baseline orientation.

### 8.3 Vibration Rejection Test

The device was exposed to tapping and vibration to simulate noisy physical conditions.  
The goal was to check whether the filter would overreact to short disturbances.

**Result:** The output remained within a small deviation window.  
**Conclusion:** The sensor setup was sufficiently resistant to common vibration noise.

---

## 9. Storage Hardware Validation (SD Card Diagnostics)

The storage path was tested on several boards and adapter designs before finalizing the architecture.  
The tests showed that signal integrity mattered more than raw SPI speed alone.

### 9.1 Summary Table

| Platform        | Module                    | Result | Meaning                                           |
| --------------- | ------------------------- | ------ | ------------------------------------------------- |
| NodeMCU ESP8266 | 5V commercial SD module   | Failed | Logic-level mismatch and boot instability.        |
| NodeMCU ESP8266 | DIY 3.3V adapter          | Failed | Signal degradation over wiring and timing limits. |
| Arduino UNO     | 5V commercial SD module   | Passed | Confirmed the module itself was healthy.          |
| ESP32 Classic   | DIY 3.3V adapter @ 50 MHz | Failed | Too fast for stable wiring-based transfer.        |
| ESP32 Classic   | DIY 3.3V adapter @ 10 MHz | Passed | Stable and reliable transfer was achieved.        |

### 9.2 Final Storage Decision

The final storage design uses the DIY 3.3V adapter.  
The SPI clock is capped at a stable operating point so the system can log continuously without corruption.

### 9.3 Image References

![SD card adapter comparison 1](https://github.com/Alireza-Sotoodeh/DeadReckoner/blob/71b12039574a37fd38f1bd668eab1e9d6a19c88f/Diagram_deadreckoner/SD%20card%20adaptors-4.png)

![SD card adapter comparison 2](https://github.com/Alireza-Sotoodeh/DeadReckoner/blob/71b12039574a37fd38f1bd668eab1e9d6a19c88f/Diagram_deadreckoner/SD%20card%20adaptors-3.jpg)

---

## 10. High-Speed Binary Logging & Offline Data Pipeline

This phase defined the final logging format and the offline analysis workflow.  
The main objective was to store data fast enough for 100 Hz acquisition without losing frames.

### 10.1 Binary Data Format

The logging frame is 48 bytes long and contains one complete sensor snapshot.  
Using binary frames removed the overhead of string conversion and reduced CPU load.

### 10.2 Storage Strategy

The logger writes continuous binary blocks to the SD card using a stable SPI configuration.  
Sequential file naming prevents accidental overwriting across different missions.

### 10.3 Offline Analysis Shift

Real-time double integration was rejected because IMU bias would quickly explode the position error.  
Instead, the project moved toward offline processing, where the logs can be filtered and reconstructed more safely.

### 10.4 Image References

![Li-Ion battery discharge curve](https://github.com/Alireza-Sotoodeh/DeadReckoner/blob/06d4af02f534a441f3935a825941592ffc650554/Diagram_deadreckoner/Li-ion-battery-discharge-voltage-curve.png)

![LiPo vs Li-Ion comparison](https://github.com/Alireza-Sotoodeh/DeadReckoner/blob/0b6d062ec90fd5a83eaf73b524ed320667db5fe3/Diagram_deadreckoner/Lipo_VS_LIIon.png)

---

## 11. Advanced UI & Defensive UX

The OLED interface was redesigned to make the system easier to use in the field.  
The menu flow now emphasizes safety, clarity, and fast access to the most important options.

### 11.1 Menu Design

The interface uses cursor-based navigation and small status pages.  
The user can move between live view, storage status, and configuration screens without stopping the system.

### 11.2 Defensive Controls

Dangerous actions were protected with confirmation steps.  
This reduces the chance of accidental log deletion or configuration loss during field operation.

### 11.3 Storage Awareness

The display shows free space, log counts, and mission-related warnings.  
That makes the system easier to operate without connecting to a PC.

---

## 12. Advanced File System & Hardware Watchdogs

The file system and fault monitoring layer were refined to prevent silent failures.  
This part of the architecture protects both the active log and the operator.

### 12.1 Auto-Sequential Logging

The logger scans the SD card at boot and creates the next available log file automatically.  
This avoids overwriting older missions and makes repeated field runs safer.

### 12.2 Smart Cleanup Behavior

Instead of wiping the whole card, the cleanup function targets only mission logs.  
That keeps unrelated user files intact and reduces the risk of accidental data loss.

### 12.3 MPU Watchdog

A runtime watchdog monitors whether the IMU stream stops unexpectedly.  
If the sensor disconnects, the system raises a fault and shuts down logging safely.

### 12.4 Emergency Response

When a critical failure is detected, the active file is closed first and then the alert system starts.  
The OLED, buzzer, and LED provide immediate feedback so the fault is obvious in the field.

---

## 13. Development Timeline (Engineering Milestones Only)

The following timeline keeps the meaningful technical milestones and omits low-value checkpoint commits.  
It is meant to show how the architecture evolved from the earliest prototype to the current system.

| Date       | Milestone           | Summary                                                                        |
| ---------- | ------------------- | ------------------------------------------------------------------------------ |
| 2025-07-12 | Project bootstrap   | Initial repository and prototype files were created.                           |
| 2025-07-13 | MPU6500 bring-up    | Early IMU library, test code, and CubeIDE setup were added.                    |
| 2025-07-25 | MPU9250 branch work | DMP support, MPU9250 documentation, and workspace updates were introduced.     |
| 2025-08-08 | Output fixes        | Linear acceleration and OLED output behavior were corrected.                   |
| 2026-06-03 | ESP32-S3 migration  | The project moved to the ESP32-S3 firmware architecture.                       |
| 2026-06-04 | Validation phase    | MPU and OLED sanity checks, drift tools, and vibration tests were added.       |
| 2026-06-05 | SD architecture     | SD card adapter diagrams and standalone SD tests were introduced.              |
| 2026-06-08 | SD test workflow    | The SD logging path was debugged and validated successfully.                   |
| 2026-06-09 | Binary pipeline     | The MATLAB BIN reader and storage benchmarking were added.                     |
| 2026-06-10 | UI iteration        | OLED phase screens and architecture updates were refined.                      |
| 2026-06-11 | Safety layer        | MPU disconnect handling, recovery, and boot-time SD scanning were implemented. |

---

## 14. Current Status

The system is now a structured offline tracking platform with stable sensing, logging, and analysis layers.  
The remaining major item is GNSS integration and the timing alignment between GPS and IMU data.

| Subsystem                   | Status      |
| --------------------------- | ----------- |
| ESP32-S3 migration          | Complete    |
| RTOS multi-core framework   | Complete    |
| Calibration and I2C tuning  | Complete    |
| Hardware validation         | Complete    |
| SD card logging             | Complete    |
| Binary file logging         | Complete    |
| Fault handling and recovery | Complete    |
| UI and monitoring           | Complete    |
| Offline analysis tools      | Complete    |
| GPS integration             | In progress |

---

## 15. Conclusion

DeadReckoner evolved from a small IMU prototype into a multi-core offline tracking system.  
The final architecture prioritizes real-time acquisition, safe logging, and reproducible offline analysis.
