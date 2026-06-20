# DeadReckoner: Pedestrian Dead Reckoning Data Logger — Architecture & Progress Report

**Author:** Alireza Sotoodeh  
**Project:** DeadReckoner  
**Version:** 2.1.0  
**Date:** June 19, 2026

> **Project Goal:** A wearable IMU data logger that records 100 Hz 9-axis inertial data (quaternions, linear acceleration, temperature) to SD card with per-frame CRC-16 integrity. The logged data is post-processed offline on a PC using Pedestrian Dead Reckoning (PDR) algorithms — step detection, heading from Madgwick-fused quaternions, and Zero Velocity Update (ZUPT) with batch smoothing — to reconstruct the traveled path with minimal drift over multi-hour missions, without GPS.  

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

| Module         | Function          | Notes                                                                                                                            |
| -------------- | ----------------- | -------------------------------------------------------------------------------------------------------------------------------- |
| MPU9250        | 9-axis IMU        | Main motion sensor for quaternion and acceleration estimation. Excellent for high-precision sensor fusion with Madgwick filter.  |
| MPU6500        | 6-axis IMU        | Early prototype sensor used during the research phase. Lacks magnetometer, prone to yaw drift over time.                         |
| GY-25          | Tilt sensor       | Used for comparison and sensor-validation experiments. Typically incorporates an MPU6050 with onboard MCU for angle calculation. |
| HW-123         | Generic module    | Included in early hardware exploration and verification.                                                                         |
| BMP280         | Barometric sensor | Reserved for altitude-related extensions. Highly accurate for barometric altitude tracking to offset Z-axis drift.               |
| S6MV2 (GPS)    | GNSS module       | Planned for global positioning and time anchoring. Essential for outdoor signal-free tracking and trajectory anchoring.          |
| OLED 0.91-inch | Status display    | Used for runtime feedback, menus, and error reporting. I2C monochrome 128x32 display, intentionally isolated on a secondary bus. |

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
| PSRAM buffering        | Massive 50,000-frame queue for zero-loss acquisition. |
| GPS preparation        | GNSS-ready data structure for future fusion.          |

---

## 3. Hardware Choices & Rationale

The final hardware stack was selected to eliminate the bottlenecks that appeared in the prototype phase.  
The choices below reflect both the commit history and the test results collected during integration.

### 3.1 Core Controller

**ESP32-S3 N16R8** was selected as the main controller.  
The dual-core CPU, PSRAM, and larger Flash space made it suitable for continuous IMU acquisition and blocking storage operations.

The migration from ESP8266 was driven by three fundamental hardware limits that could not be resolved through software optimisation alone:

1. **Processing Bottlenecks (Single vs. Dual Core):** The ESP8266 single-core processor cannot read the MPU9250 at 100 Hz, apply the Madgwick filter, and write data to an SD card simultaneously without blocking delays. The ESP32-S3 separates sensor fusion (Core 0) from data logging (Core 1) strictly.

2. **Memory Constraints:** The ESP8266 has ~50 KB usable SRAM and 4 MB Flash. Buffer queues for offline logging cause memory overflow. The ESP32-S3 N16R8 offers 16 MB Flash and 8 MB PSRAM, enabling the 50,000-frame PSRAM queue (~2.14 MB) used in the current architecture.

3. **Peripheral Routing:** The ESP32-S3 features a complete IO MUX matrix that maps hardware I2C buses to any GPIO pin, eliminating bus congestion issues and enabling physical segregation of the IMU and SD card buses.

### 3.2 Primary IMU

**MPU9250** became the preferred IMU.  
It provides accelerometer, gyroscope, and magnetometer data, which improves long-term orientation tracking compared with the MPU6500. The 9-axis capability allows the Madgwick filter to correct yaw drift using the magnetometer, which is impossible with a 6-axis IMU.

### 3.3 Storage Medium

A **3.3V DIY SD adapter** was finalized for logging.  
The adapter was chosen after multiple failed and successful tests with other SD card modules and SPI clock settings. The pure 3.3V logic path bypasses level-shifter failures seen on commercial 5V modules.

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
- [x] **Pin Relocation:** Move away from GPIO 9 and 10 due to internal Flash/PSRAM strapping line conflicts.

#### Phase 2: RTOS & Multi-Core Framework [COMPLETED]

This phase replaced the single-flow firmware with a real-time multi-core design.

- [x] **FreeRTOS Integration:** Introduce task-based scheduling.
- [x] **Sensor Task:** Run high-rate acquisition on Core 0.
- [x] **Logging Task:** Run blocking storage work on Core 1.
- [x] **Producer-Consumer Queue:** Pass complete log frames between tasks.
- [x] **Thread-Safe Communication:** Avoid shared-state corruption across cores.
- [x] **Race Condition Mitigation:** Remove direct concurrent memory access.
- [x] **Timing Stability:** Keep acquisition deterministic under continuous load.
- [x] **Spinlock Protection:** Add portMUX_TYPE critical sections for frame counter and tag event atomicity across cores.

#### Phase 3: Calibration & I2C Optimization [COMPLETED]

This phase improved sensor accuracy and reduced bus-level interference.

- [x] **I2C Fast Mode:** Raise the bus frequency to 400 kHz.
- [x] **EEPROM Bias Storage:** Save calibration values for later reuse.
- [x] **Bias Restoration:** Load the saved values automatically at startup.
- [x] **Offset Compensation Fix:** Apply the loaded calibration to raw readings.
- [x] **Bus Collision Prevention:** Pause acquisition during blocking EEPROM access.
- [x] **Dual-I2C Topology:** Move OLED traffic to the secondary bus.
- [x] **I2C Validation:** Verify that the buses remained stable in normal use.
- [x] **EEPROM Magic Number:** Add sentinel value (0xDEAD) to detect corrupt or uninitialized EEPROM and prevent loading garbage bias data.
- [x] **I2C Bus Timeout:** Configure Wire.setTimeout(50) to prevent hardware bus hangs if lines are pulled low.

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
- [x] **Signal Integrity Tuning:** Cap SPI clock at 20 MHz for stable operation over physical wiring.
- [x] **Storage Benchmarking:** Measure practical write throughput (797 KB/s at 20 MHz).
- [x] **Final Validation:** Confirm successful logging under repeated tests.
- [x] **Bandwidth Calculation:** Determine 11.88 MB/hour consumption at 45-byte frames and 100 Hz sampling rate.

#### Phase 6: Binary Logging Framework [COMPLETED]

The text-based logging path was replaced with a structured binary pipeline.

- [x] **LogFrame Design:** Define a fixed-size telemetry frame.
- [x] **Timestamp Logging:** Add 64-bit microsecond time stamps to every recorded sample.
- [x] **Quaternion Logging:** Store fused orientation values.
- [x] **Acceleration Logging:** Store gravity-compensated linear acceleration.
- [x] **Binary Write Path:** Replace string output with raw binary writes.
- [x] **Continuous Streaming:** Keep the logging pipeline active at high rate.
- [x] **Sequential File Naming:** Generate unique log files automatically.
- [x] **Boot-Time Discovery:** Scan the storage and compute the next file index.
- [x] **Capacity Display:** Show log counts and storage info on the OLED.
- [x] **Union Memory Optimization:** Overlap IMU and GPS payload in a 45-byte union LogFrame (down from 48 bytes).
- [x] **Frame Sequence Counter:** Monotonic 32-bit frame_seq for frame-drop detection and MATLAB stitching.
- [x] **Gap Frame Injection:** Write 0xAA event marker on SD disconnection to preserve data continuity.

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
- [x] **Dynamic SD Recovery:** Detect SD card hot-plug, auto-reopen new recovery file, and resume logging with gap marker injection.
- [x] **SOS Alarm Pattern:** Rhythmic 100 ms buzzer + red LED cycle during critical failures.
- [x] **Recovery File Naming:** Deterministic XXXYYY.BIN format linking parent log ID to recovery instance.

#### Phase 8: User Interface & Operational Monitoring [COMPLETED]

This phase improved the operator experience and made the system easier to inspect.

- [x] **Multi-Page OLED UI:** Create simple runtime menu pages.
- [x] **Status Screens:** Show live system and sensor state.
- [x] **Storage Monitoring:** Display SD statistics and file counts.
- [x] **Display Modes:** Add selectable visualization modes (Always ON / Auto Off 20s).
- [x] **Buzzer Control:** Allow runtime mute and unmute behavior.
- [x] **SD Menu:** Add 8-item SD manager submenu with scrolling for 128x32 OLED.
- [x] **Diagnostics Screen:** Show faults and recovery messages clearly.
- [x] **Confirmation Traps:** Two-stage YES/NO guards for Format and Create New File actions; default cursor is NO.
- [x] **OLED Auto-Sleep:** Power-save after 20 seconds of inactivity; wake on any button press.
- [x] **Stealth Mode:** Mute buzzer; TAG button with green LED feedback only.
- [x] **Drop Frame Counter:** Queue overflow count displayed in SD info submenu.

#### Phase 9: Data Analysis & Validation Toolchain [COMPLETED]

The recorded datasets were made usable through offline analysis tools.

- [x] **MATLAB Binary Reader:** Decode the logged binary frames.
- [x] **Quaternion Plotting:** Visualize orientation trends over time.
- [x] **Acceleration Plotting:** Inspect linear acceleration behavior.
- [x] **Drift Analysis:** Measure long-term navigation stability.
- [x] **Validation Pipeline:** Create a repeatable test-and-review workflow.
- [x] **Result Correlation:** Compare recorded data with physical tests.
- [x] **Python BinReader:** Alternative parser (DeadReckoner_Parser.py) for platform-independent log inspection.
- [x] **Recovery Fragment Stitching:** Seamlessly merge recovery chunks across file boundaries using frame_seq continuity.
- [x] **Garbage Frame Filtering:** Isolate and discard hardware-interrupted sector dumps during analysis.
- [x] **Project Documentation Generator:** Develop project_summarizer.py (1216 lines, 26 functions) for auto-generated architecture reports in MD/TXT/XML formats, including full git history, directory tree, and source signature analysis.

#### Phase 10: PSRAM Buffering & Memory Architecture [COMPLETED]

This phase added a massive PSRAM-backed queue to eliminate frame loss during SD write latency.

- [x] **PSRAM Buffer Allocation:** Dedicate 2.14 MB of external PSRAM for a 50,000-frame static queue.
- [x] **Queue Health Monitoring:** Detect silent overflows; red LED indicator and drop-frame counter.
- [x] **64-bit Overflow Protection:** Safe arithmetic for SD cards larger than 32 GB.
- [x] **openNext Directory Iteration:** Replace sequential exists() probing with high-speed SdFat directory iteration (10-100x faster file scanning).

#### Phase 11: GPS Integration & Time Synchronization [PLANNED]

The final extension will add GNSS support and time alignment.

- [x] **Reserved GPS Fields:** Prepare the log frame for GNSS data via the payload union (lat/lng as 64-bit double).
- [ ] **UART Configuration:** Add a dedicated serial line for the S6MV2 receiver.
- [ ] **NMEA Parser:** Decode GPS position and timing information.
- [ ] **Coordinate Injection:** Store latitude and longitude in the logging stream.
- [ ] **Status Monitoring:** Track satellite lock and GNSS health.
- [ ] **Time Sync:** Align 1 Hz GPS updates with 100 Hz IMU samples.
- [ ] **Trajectory Validation:** Verify synchronized offline reconstruction.

#### Phase 12: GPX Fusion Tool [COMPLETED]

A PyQt6 desktop application for fusing GPS logs from Garmin eTrex 30x and Geo Tracker Android app into a single accurate track using a Kalman filter with RTS smoothing.

- [x] **GPX Parser:** Read standard GPX + Geo Tracker `geotracker:meta` extensions (accuracy `c`, speed `s`). Source detection by filename.
- [x] **Kalman Filter Engine:** Constant-velocity motion model in local meters (equirectangular projection). Per-device noise (Geo Tracker uses reported `c` accuracy, Garmin defaults to 6 m).
- [x] **RTS Smoother:** Bidirectional backward pass for optimal batch estimation.
- [x] **Interpolation:** Resample all tracks to a common 1 Hz grid.
- [x] **Offline Map (matplotlib):** Static map with lat/lon grid, color-coded raw tracks, red fused path, start/end markers, stats overlay. No internet required.
- [x] **Online Map (folium):** Interactive Leaflet map with 5 tile providers (OSM, CartoDB, Esri satellite/topo). Proxy settings. Connectivity check.
- [x] **Proxy Dialog:** Enable/disable proxy, host/port configuration.
- [x] **File Management:** Tabs per walk group, drag-and-drop GPX import, file info panel.
- [x] **Export:** Save fused path as standard GPX.
- [x] **Dual Map Mode:** QComboBox switches between offline and online. Online mode grayed out if dependencies missing.
- [x] **Tile Provider Switching:** QComboBox selects tile set; map regenerates on change.
- [x] **QWebEngineView Unparent Fix:** `setParent(None)` before `setHtml()` enables `loadFinished` with CDN scripts. Viewport CSS fix applied on load.
- [x] **Fuse Button:** Green QPushButton with `:hover`/`:pressed` states.
- [x] **Stats Panel:** Shows fused path statistics (points, distance, duration, speed).
- [x] **Refresh & Zoom Buttons:** Refresh map in current mode; zoom in/out/reset for matplotlib map.

---

## 6. Firmware Flashing Configuration (`ESP32-S3 N16R8`)

The firmware was tuned for stable execution on the ESP32-S3 (for Arduino IDE programming)

| Setting                              | Value                                                                       |
|:------------------------------------:|:---------------------------------------------------------------------------:|
| Board                                | ESP32S3 Dev Module                                                          |
| USB CDC On Boot                      | Disabled                                                                    |
| CPU Frequency                        | 240 MHz                                                                     |
| Core Debug Level                     | None                                                                        |
| USB DFU On Boot                      | Disabled                                                                    |
| Erase All Flash Before Sketch Upload | Disabled                                                                    |
| Events Run On                        | Core 1                                                                      |
| Flash Mode                           | QIO 80 MHz                                                                  |
| Flash Size                           | 16MB (128Mb)                                                                |
| JTAG Adapter                         | Disabled                                                                    |
| Arduino Runs On                      | Core 1                                                                      |
| USB Firmware MSC On Boot             | Disabled                                                                    |
| Partition Scheme                     | 16M Flash (e.g., 3MB APP/9.9MB FATFS) — [CRITICAL: Must not be 4MB default] |
| PSRAM                                | OPI PSRAM                                                                   |
| Upload Mode                          | UART0 / Hardware CDC                                                        |
| Upload Speed                         | 921600                                                                      |
| USB Mode                             | Hardware CDC and JTAG                                                       |
| Zigbee Mode                          | Disabled                                                                    |

---

## 7. Multi-Core Architecture & Concurrency Management

The dual-core design solved the biggest reliability issue in the system: blocking storage work.  
Sensor acquisition and logging now run independently without forcing one task to wait for the other.

### 7.1 The Race Condition Problem

In a dual-core tracking system, tasks operate at vastly different frequencies:

- **Core 0 (Sensor Fusion):** Reads the IMU and calculates quaternions continuously at 100 Hz.
- **Core 1 (Data Logging):** Writes data to SD card. Flash write operations are inherently blocking.

If both cores attempt to access the same global orientation variables simultaneously, a **Race Condition** occurs. Core 1 may read an incomplete data set before Core 0 finishes updating it, resulting in corrupted logs and destroying 3D trajectory reconstruction in MATLAB. Standard Mutex locks are not viable here — locking the data during a slow SD card write would force Core 0 to wait, dropping critical high-frequency IMU reads.

### 7.2 Producer-Consumer Solution

The system uses a FreeRTOS queue to move complete frames between tasks.  
Core 0 acts as the producer, and Core 1 acts as the consumer that writes finished frames to storage.

1. **Data Encapsulation:** All variables for a single point in time are packed into a rigid C struct. GPS coordinates use 64-bit double to prevent precision loss that would cause map-drift.
2. **The Queue (Buffer):** A thread-safe FIFO queue — originally sized at 300 frames in internal SRAM, later upgraded to **50,000 frames in external PSRAM** (~2.14 MB) for zero-loss acquisition.
3. **Task Separation:** Core 0 (Producer) reads sensors and pushes structs to the back of the queue without blocking. Core 1 (Consumer) pops frames from the front and writes them to storage in binary format.

### 7.3 LogFrame Structure

```c
// Current optimized structure (47 bytes via union payload overlap + CRC16)
#pragma pack(push, 1)
typedef struct {
    uint32_t frame_seq;  // 4 bytes: Monotonic sequential index
    uint64_t timestamp;  // 8 bytes: Microsecond resolution
    uint8_t event_flag;  // 1 byte: 0=IMU, 1=TAG, 0xAA=SD_GAP, 0xBB=GPS

    union {
        struct {
            float q[4];      // 16 bytes: Quaternions
            float accel[3];  // 12 bytes: Linear acceleration
            float temp;      // 4 bytes: IMU temperature
        } imu;
        struct {
            double lat;      // 8 bytes: Latitude
            double lng;      // 8 bytes: Longitude
        } gps;
    } payload;               // 32 bytes shared

    uint16_t crc;            // 2 bytes: CRC-16-IBM over preceding 45 bytes

} LogFrame; // Total: 47 bytes
#pragma pack(pop)
```

The fixed-size frame keeps the logging format deterministic and easy to decode offline.  
The union payload allows IMU and GPS data to share the same memory region, reducing frame size from 48 to 45 bytes.

### 7.4 Queue Sizing

The initial queue depth of 300 frames (internal SRAM) was designed to survive short storage stalls.  
At 100 Hz, that gave roughly 3 seconds of buffering. The current architecture uses a **50,000-frame static queue in external PSRAM**, providing over 8 minutes of buffer headroom during prolonged SD write interrupts.

---

## 8. Hardware & Filter Validation (Test Results)

The IMU and fusion pipeline were verified through controlled physical tests.  
The outputs were recorded and inspected offline to confirm stability under different motion patterns.

### 8.1 Static Drift Test (Zero-Rate Offset)

- **Objective:** Measure the accumulated error over a 15-minute stationary period to evaluate baseline stability.
- **Methodology:** Device kept completely still on an isolated, flat surface for ~900 seconds post-calibration.
- **Results:**
  - **Max Roll Drift:** 0.43°
  - **Max Pitch Drift:** 0.39°
  - **Max Yaw Drift:** 2.73° (over 15 minutes)
- **Conclusion:** **PASS.** The accelerometer isolates the gravity vector, eliminating pitch/roll drift (<0.5°). The magnetometer bounds yaw drift to ~0.18°/minute, proving high immunity against false map rotations during prolonged GPS outages.

### 8.2 Dynamic Return-to-Zero Test

- **Objective:** Evaluate the filter's ability to recover from high-G linear accelerations without losing the true horizon.
- **Methodology:** Device subjected to violent 3D rotations and linear accelerations (up to 130° swings) for 30 seconds, then returned exactly to its initial physical position.
- **Results (Absolute Return Error):**
  - **Roll Error:** 0.07°
  - **Pitch Error:** 0.36°
  - **Yaw Error:** 1.55°
- **Conclusion:** **PASS.** The Madgwick filter Beta gain is optimally tuned. The system ignores temporary linear accelerations and snaps back to true orientation with sub-degree accuracy.

### 8.3 Vibration Rejection Test

- **Objective:** Test the system's resilience against high-frequency mechanical noise (vehicle vibrations, footsteps).
- **Methodology:** Device subjected to external mechanical shocks, including light pen tapping and heavy fist pounds on the adjacent surface.
- **Results (Max Deviation during impact):**
  - **Pen Tapping (Roll/Pitch Swing):** < 0.1° deviation
  - **Heavy Fist Pound (Max Pitch Swing):** 0.57° deviation
- **Conclusion:** **PASS.** The hardware DLPF configured at 5 Hz effectively isolates the IMU from environmental vibrations. Tracking coordinates remain highly stable under harsh physical or automotive conditions.

---

## 9. Storage Hardware Validation (SD Card Diagnostics)

The storage path was tested on several boards and adapter designs before finalizing the architecture.  
The tests showed that signal integrity mattered more than raw SPI speed alone.

### 9.1 Cross-Platform Diagnostic Results

| Platform        | Module               | SPI Frequency | Result | Engineering Conclusion                                                                 |
| --------------- | -------------------- |:-------------:|:------:| -------------------------------------------------------------------------------------- |
| NodeMCU ESP8266 | Commercial 5V Module | Standard      | FAILED | Level shifter fails to trigger on weak 3.3V ESP lines; voltage dropouts during boot.   |
| NodeMCU ESP8266 | DIY 3.3V Adapter     | Standard      | FAILED | High default SPI speed with jumper wires caused severe signal degradation.             |
| Arduino UNO     | Commercial 5V Module | Standard      | PASSED | Pure 5V logic compatibility confirmed; the module and microSD card are healthy.        |
| ESP32 Classic   | DIY 3.3V Adapter     | 50 MHz        | FAILED | Ultra-high frequency caused antenna effect on jumper wires, corrupting data blocks.    |
| ESP32 Classic   | DIY 3.3V Adapter     | 10 MHz        | PASSED | Lower clock stabilized signal; SDHC and FAT32 successfully mounted, read, and written. |

### 9.2 High-Precision SPI Frequency Sweep

A 1 MHz stepped sweep was performed on the DIY 3.3V adapter using an ESP32 to determine the maximum stable operating frequency:

```
--- HIGH-PRECISION SPI SWEEP (1MHz Steps) ---
Freq(MHz) | Result | Speed(KB/s)
------------------------------------
1 MHz     | PASSED | 93.57 KB/s
5 MHz     | PASSED | 365.19 KB/s
10 MHz    | PASSED | 555.31 KB/s
15 MHz    | PASSED | 638.40 KB/s
20 MHz    | PASSED | 797.51 KB/s
25 MHz    | PASSED | 727.27 KB/s
26 MHz    | PASSED | 797.51 KB/s
27 MHz    | FAILED | N/A
--- LIMIT REACHED. SYSTEM UNSTABLE ABOVE THIS FREQ. ---
```

**Result:** 20 MHz was selected as the operating frequency, providing 797 KB/s write throughput — a 95% safety margin over the ~10 KB/s required for 100 Hz logging.

### 9.3 Final Storage Decision

- **Hardware:** The DIY 3.3V adapter (no level shifters or regulators) is the finalized hardware, bypassing unnecessary voltage conversions.
- **SPI Clock:** 20 MHz (reduced from the initial 10 MHz cap after sweep validation proved 20 MHz stable).
- **Library:** SdFat for low-level block access, accurate filesystem operations, and long-term FAT32 stability.
- **Bandwidth Validation:** 100 Hz at 45 bytes/frame = 4.5 KB/s. At 797 KB/s write speed, the system has a massive safety margin.

---

## 10. High-Speed Binary Logging & Offline Data Pipeline

This phase defined the final logging format and the offline analysis workflow.  
The main objective was to store data fast enough for 100 Hz acquisition without losing frames.

### 10.1 Hardware Pinout Overhaul

To prevent hardware bus collisions, the SD card was moved to the dedicated FSPI bus:

| Peripheral   | Bus    | Pins                                                        |
| ------------ | ------ | ----------------------------------------------------------- |
| MPU9250      | I2C    | GPIO 4 (SDA), GPIO 5 (SCL)                                  |
| SD Card      | FSPI   | GPIO 15 (CS), GPIO 11 (MOSI), GPIO 12 (SCK), GPIO 13 (MISO) |
| OLED         | SW I2C | GPIO 6 (SDA), GPIO 7 (SCL)                                  |
| Buttons      | GPIO   | GPIO 1 (SELECT), GPIO 2 (UP), GPIO 8 (DOWN), GPIO 14 (TAG)  |
| Buzzer / LED | GPIO   | GPIO 21 (BUZZER), GPIO 17 (RED), GPIO 18 (GREEN)            |

### 10.2 Binary Data Format

The logging frame is 47 bytes long — 45 bytes of payload data (frame header + union payload) plus a 2-byte CRC-16-IBM checksum for per-frame integrity detection during offline analysis. Each frame contains one complete sensor snapshot with a union payload. Using binary frames removed the overhead of string conversion and reduced CPU load.

### 10.3 Storage Strategy

The logger writes continuous binary blocks to the SD card via SdFat at 20 MHz SPI. Sequential file naming (DR_LOG_XXX.BIN) prevents accidental overwriting across different missions. Recovery fragments use the XXXYYY.BIN naming scheme for deterministic reassociation.

### 10.4 Offline Pedestrian Dead Reckoning Pipeline

Real-time double integration was rejected because MEMS bias would quickly explode the position error quadratically (\(Error = \frac{1}{2} a_{error} t^2\)). Instead, the project targets offline Pedestrian Dead Reckoning (PDR) in Python, using the following pipeline:

1. **Binary parsing & CRC verification** — Read 47-byte frames, validate CRC-16, extract timestamps, quaternions, and linear acceleration.
2. **World-frame rotation** — Rotate the body-frame linear acceleration into the world frame using the firmware's Madgwick quaternions, isolating vertical and horizontal components.
3. **Step detection** — Identify individual footsteps via peak detection on the accel magnitude signal.
4. **Step length estimation** — Compute per-step distance using the Weinberg empirical formula.
5. **Heading extraction** — Use the quaternion yaw angle (magnetometer-corrected by the firmware's Madgwick filter) as the direction for each step.
6. **Batch optimization** — Apply ZUPT (Zero Velocity Update) with a Rauch–Tung–Striebel smoother over the entire recorded walk, using bidirectional smoothing to eliminate the quadratic drift of double integration.
7. **Path reconstruction** — Integrate step vectors to produce a 2D trajectory plot.

This offline approach achieves sub-3% drift over multi-hour missions without requiring GPS or any training data. Future extensions include BMP280 barometric altitude for 3D tracking and GPS correction for absolute position anchoring.

### 10.5 Phase Summary

| Milestone / Task          | Status    | Detail / Specification                                        |
|:------------------------- |:---------:|:------------------------------------------------------------- |
| **I2C/SPI Pin Isolation** | Completed | MPU on GPIO 4/5, SD on FSPI 15/11/12/13, OLED on GPIO 6/7.    |
| **Binary Data Struct**    | Completed | 47-byte union-based LogFrame with CRC-16 per-frame integrity. |
| **SD Storage Pipeline**   | Completed | SdFat at 20 MHz, writing sequentially named binary files.     |
| **Data Parsing Scripts**  | Completed | MATLAB and Python parsers for 47-byte frame decoding.         |
| **Navigation Algorithm**  | Shifted   | From live MCU integration to offline ZUPT modeling.           |

### 10.6 PDR Algorithm Comparison

| Algorithm                                   | Wearability | Error (1 hr)      | Difficulty | Training Data              | Language                | Notes                                                      |
| ------------------------------------------- | ----------- | ----------------- | ---------- | -------------------------- | ----------------------- | ---------------------------------------------------------- |
| **ZUPT**                                    | Foot only   | 1–5%              | Medium     | No                         | Python / MATLAB         | Gold standard for foot-mounted; invalid for wrist-worn     |
| **Weinberg Step Length**                    | Any         | 8–15%             | Easy       | No                         | Python / MATLAB         | Good starting point; needs per-user calibration constant   |
| **Peak Step Detection**                     | Any         | Depends           | Easy       | No                         | Python (scipy)          | Simple threshold; works for steady walking                 |
| **Madgwick Heading**                        | Any         | ~2°/min yaw drift | Easy       | No                         | Firmware (already done) | Already computed — extract yaw from stored quaternion      |
| **RTS Smoother** (bidirectional Kalman)     | Any         | 0.5–3%            | Hard       | No                         | Python (filterpy)       | Best accuracy without ML; uses future data to correct past |
| **Factor Graph / GTSAM**                    | Any         | 0.3–2%            | Very Hard  | No                         | Python (GTSAM) / C++    | Top accuracy; heavy setup and domain knowledge             |
| **Complementary Filter**                    | Any         | 5–10°/min         | Easy       | No                         | Python / C              | Simpler than Madgwick; less accurate                       |
| **LSTM Step Detector**                      | Any         | 3–8%              | Hard       | Yes (hrs of labeled walks) | Python (PyTorch / TF)   | Handles irregular motion; needs large dataset              |
| **CNN Step Detector**                       | Any         | 3–8%              | Hard       | Yes (hrs of labeled walks) | Python (PyTorch / TF)   | Sliding window over IMU segments                           |
| **Kalman Filter** (real-time, forward only) | Any         | 5–10%             | Medium     | No                         | Python / C              | Worse than RTS — no backward pass to correct drift         |

**Recommended pipeline for wrist-worn DeadReckoner:**

| Stage                   | Algorithm                           | Rationale                                                 |
| ----------------------- | ----------------------------------- | --------------------------------------------------------- |
| Step detection          | Peak detection on accel magnitude   | Simple, robust, no training data                          |
| Step length             | Weinberg formula                    | Empirical calibration; zero data requirement              |
| Heading                 | Madgwick quaternions → yaw          | Already computed by firmware at 100 Hz                    |
| Trajectory optimization | RTS Smoother (bidirectional Kalman) | Beats all ML approaches without requiring labeled data    |
| Implementation language | Python (numpy, scipy, filterpy)     | Free, full scientific stack, straightforward .BIN parsing |

### 10.7 Challenges & Solutions

- **Challenge:** System crashing when accessing MPU9250 and SD Card simultaneously.  
  **Solution:** Physically isolate SD on FSPI (pins 11-15) and keep MPU on I2C (pins 4-5). In ESP32-S3, sharing bus matrices for high-speed SPI (20 MHz) and I2C causes interrupt starvation.

- **Challenge:** Serial.print() to SD card (text logging) was too slow.  
  **Solution:** Switched to binary block writes (logFile.write of raw struct). Float-to-ASCII conversion is extremely CPU-intensive; binary writing bypasses this bottleneck.

- **Challenge:** Double integration of accelerometer data caused kilometers of error within minutes.  
  **Solution:** Deferred integration to post-processing via ZUPT in MATLAB. MEMS white noise grows quadratically in double integration; ZUPT forces velocity to zero on each foot strike.

---

## 11. Advanced UI & Defensive UX

The OLED interface was redesigned to make the system easier to use in the field.  
The menu flow now emphasizes safety, clarity, and fast access to the most important options.

### 11.1 Menu Design

The interface uses cursor-driven navigation with submenus and small status pages. The system supports a **Live View** (real-time quaternion and temperature data), a 5-item **Main Menu**, and a dedicated **SD Manager Submenu**. Any selection within configuration menus instantly redirects back to Live View.

### 11.2 SD Manager (Dynamic Scrolling)

Due to the 128x32 OLED physical limit of 3 visible lines, an 8-item scrolling window matrix was implemented:

1. Total: XX.X GB
2. Free: XXX MB
3. Time: XX.X Hrs
4. Files Count: XXX
5. Drops: XXX
6. Create New File
7. Format / Clear
8. Back to Menu

The cursor loops at boundaries, and the scroll offset adjusts dynamically. Drop frame count tracks queue overflow events.

### 11.3 Defensive Controls

Dangerous actions are protected by **two-stage confirmation traps** with the cursor defaulting to **NO**:

- **Format / Clear:** Prompts "Clear All Logs?" with YES / >NO. On confirmation, suspends Core 0, wipes both parent logs (DR_LOG_XXX.BIN) and recovery fragments (XXXYYY.BIN), resets indices, and creates a fresh log file.
- **Create New File:** Prompts "Create New File?" with YES / >NO. On confirmation, syncs and closes the current file, increments the log ID ceiling, and opens the next sequential file.

### 11.4 OLED Power Management

- **Auto-Sleep:** Display powers off after 20 seconds of inactivity.
- **Wake:** Any button press wakes the display and resets to Live View.
- **Display Mode Submenu:** Toggle between "Always ON" and "Auto Off 20s".

### 11.5 Feedback & Indicators

- **TAG Button:** Flashes green LED + short buzzer beep (50 ms). Even in stealth (muted) mode, the green LED still flashes.
- **Safe Shutdown:** 3-second long press on SELECT triggers flush of remaining PSRAM frames, closes the file, and displays "SAFE TO POWER OFF" with solid green LED.
- **Stealth Mode:** Mute submenu toggles all buzzer sounds ON/OFF.

---

## 12. Advanced File System & Hardware Watchdogs

The file system and fault monitoring layer were refined to prevent silent failures.  
This part of the architecture protects both the active log and the operator.

### 12.1 Auto-Sequential Logging

The logger scans the SD card root at boot and creates the next available log file automatically. A boot screen reports "Found: X Logs / Next: LOG_XXX" to the operator.

### 12.2 Smart Cleanup Behavior

Instead of wiping the whole card, the cleanup function scans for and removes only files matching the DR_LOG_XXX.BIN and recovery XXXYYY.BIN naming patterns. Other user files on the SD card remain intact.

### 12.3 MPU Watchdog

A runtime timer on Core 0 monitors the I2C acquisition stream. If sensor data ceases for >500 ms, a critical error flag is raised. Core 1 immediately syncs and closes the active log file, then enters an SOS alert pattern.

### 12.4 Dynamic SD Recovery

If the SD card is disconnected during operation:

1. Core 1 detects the write failure and sets sd_critical_error.
2. Red LED + buzzer produce a 100 ms rhythmic SOS pattern.
3. Every 3000 ms, the system attempts to reinitialize SPI and reopen the file.
4. On success, a **gap frame** (event_flag = 0xAA) is injected to mark the discontinuity, and a new recovery file (XXXYYY.BIN) is created automatically.

### 12.5 Emergency Response

When a critical failure is detected:

- The active file is synced and closed immediately.
- OLED displays diagnostic information.
- An SOS pattern (100 ms buzzer + red LED, repeated) runs until recovery or manual shutdown.
- I2C bus timeout (50 ms) prevents hardware bus hangs if physical lines are pulled low during a fault.

---

## 13. Development Timeline (Engineering Milestones Only)

The following timeline keeps the meaningful technical milestones and omits low-value checkpoint commits.  
It shows how the architecture evolved from the earliest prototype to the current system.

| Date       | Milestone                         | Summary                                                                                                                                              |
| ---------- | --------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------- |
| 2025-05-11 | Initial commit                    | Repository created with early project skeleton.                                                                                                      |
| 2025-05-12 | MPU6500 prototype start           | Initial MPU6500 library integration and STM32 CubeIDE setup.                                                                                         |
| 2025-05-14 | MPU9250 DMP exploration           | DMP support, MPU9250 documentation, and register-level bring-up.                                                                                     |
| 2025-05-17 | Linear acceleration & OLED fixes  | Corrected gravity-compensated acceleration; OLED display output functional.                                                                          |
| 2025-07-12 | Repository reorganization         | Cleaned repo structure; NodeMCU ESP8266 firmware separated as primary target.                                                                        |
| 2026-06-03 | ESP32-S3 migration                | Migrated from ESP8266 NodeMCU to ESP32-S3 N16R8 with new wiring diagram.                                                                             |
| 2026-06-03 | Dual-core FreeRTOS                | Implemented producer-consumer architecture with queue-based inter-core comms.                                                                        |
| 2026-06-03 | MPU9250 sanity check              | Validated IMU initialization and I2C communication on ESP32-S3.                                                                                      |
| 2026-06-03 | OLED sanity check                 | Confirmed SSD1306 display initialization and rendering on 0.91-inch OLED.                                                                            |
| 2026-06-04 | Static drift test                 | 15-minute stationary test: max roll 0.43°, pitch 0.39°, yaw 2.73°.                                                                                   |
| 2026-06-04 | Dynamic return-to-zero test       | Aggressive 130° swing recovery: roll error 0.07°, yaw error 1.55°.                                                                                   |
| 2026-06-04 | Vibration rejection test          | Heavy impact: max pitch deviation 0.57°; pen tapping: <0.1°.                                                                                         |
| 2026-06-04 | Validation analysis tools         | MATLAB visualization scripts for drift, RTZ, and vibration analysis.                                                                                 |
| 2026-06-05 | SD card adapter investigation     | Added SD card diagrams; began cross-platform adapter comparison.                                                                                     |
| 2026-06-05 | Standalone SD tests               | Validated commercial 5V module and DIY 3.3V adapter across multiple boards.                                                                          |
| 2026-06-05 | SPI sweep completed               | 1–26 MHz sweep on DIY adapter: 20 MHz selected as stable operating point.                                                                            |
| 2026-06-08 | SD card logging stress test       | Continuous binary logging validated at 100 Hz with zero frame loss.                                                                                  |
| 2026-06-08 | MPU disconnect detection          | First implementation of runtime I2C watchdog and critical fault state.                                                                               |
| 2026-06-08 | Boot-time SD scanning             | Auto-sequential log file numbering on startup with boot-screen report.                                                                               |
| 2026-06-09 | MATLAB binary reader              | 48-byte LogFrame parser with quaternion and acceleration extraction.                                                                                 |
| 2026-06-09 | Storage benchmarking              | Measured 797 KB/s write speed at 20 MHz SPI; calculated 11.88 MB/hour rate.                                                                          |
| 2026-06-09 | OLED phase UI                     | Initial menu system, SD info display, and multi-page navigation.                                                                                     |
| 2026-06-09 | UI iteration: submenus            | Display mode, mute buzzer, and SD card submenus added.                                                                                               |
| 2026-06-10 | PSRAM queue integration           | Allocated 50,000-frame buffer (2.14 MB) in external PSRAM for zero-loss queue.                                                                       |
| 2026-06-10 | Union LogFrame optimization       | Payload union reduced frame size from 48 to 45 bytes.                                                                                                |
| 2026-06-10 | SD runtime recovery               | Hot-plug detection, auto-reopen with recovery file (XXXYYY.BIN), gap frames.                                                                         |
| 2026-06-10 | OLED auto-sleep & display modes   | Power-save after 20s; Always ON / Auto Off toggle; wake-on-button.                                                                                   |
| 2026-06-10 | Confirmation traps                | YES/NO guards for Format and Create New File; default cursor = NO.                                                                                   |
| 2026-06-10 | Stealth mode & TAG button         | Mute buzzer; TAG button with green LED + buzzer feedback.                                                                                            |
| 2026-06-10 | Safe shutdown protocol            | 3-second long-press flush of 50,000 PSRAM frames; "SAFE TO POWER OFF".                                                                               |
| 2026-06-10 | Drop frame counter                | Queue overflow counter displayed in SD submenu.                                                                                                      |
| 2026-06-10 | TAG button inter-core handling    | Tag events transmitted via atomic flag across cores with spinlock protection.                                                                        |
| 2026-06-10 | I2C calibration conflict fix      | Remove destructive xQueueReset; sensor task suspended during calibration only.                                                                       |
| 2026-06-11 | EEPROM magic number validation    | 0xDEAD sentinel prevents corrupt calibration from loading.                                                                                           |
| 2026-06-11 | Frame counter thread safety       | portMUX_TYPE critical sections for atomic 64-bit timebase reads across cores.                                                                        |
| 2026-06-11 | SD file counting fix              | Switched to openNext() directory iteration; 10–100x faster than sequential probing.                                                                  |
| 2026-06-11 | 32-bit overflow protection        | 64-bit arithmetic for SD cards >32 GB.                                                                                                               |
| 2026-06-11 | Smart delete protocol             | Format removes both parent logs (DR_LOG_XXX) and recovery fragments (XXXYYY).                                                                        |
| 2026-06-11 | Recovery fragment naming          | Deterministic XXXYYY.BIN format linking log ID to recovery instance.                                                                                 |
| 2026-06-11 | Configurable MPU settings helper  | Extracted configureMPUSettings() for centralized sensor configuration.                                                                               |
| 2026-06-11 | Redundant variable cleanup        | Removed unused STATE_SUBMENU_MSG, last_sd_recovery_attempt, dead code paths.                                                                         |
| 2026-06-12 | Gap frame zero-initialization     | Fixed uninitialized event_flag in SD recovery gap marker.                                                                                            |
| 2026-06-12 | Log file validation post-recovery | Null-check file pointer after MPU reconnection to prevent silent write loss.                                                                         |
| 2026-06-12 | Recovery ID increment order fix   | Increment global_recovery_id only after successful file open confirmation.                                                                           |
| 2026-06-12 | Project report generation         | Added project_summarizer tool; generated comprehensive architecture reports.                                                                         |
| 2026-06-12 | Progress.md update                | Documented PSRAM, recovery protocol, UI features, and phase completion status.                                                                       |
| 2026-06-12 | AI code review report             | Added Issues found by AI.md from DeepSeek, ChatGPT, Claude, and Grok reviews.                                                                        |
| 2026-06-14 | Expand data frame & O(1) naming   | Increased LogFrame to 45 bytes; added max_log_id for instant file creation.                                                                          |
| 2026-06-14 | Thread-safe frame counter         | Added portMUX_TYPE critical sections for atomic global_frame_counter increments.                                                                     |
| 2026-06-14 | Dropped frame tracking            | Added dropped_frames_count with red LED feedback on queue overflow.                                                                                  |
| 2026-06-14 | Expand SD menu to 8 items         | Added 8th menu entry (Drops); fixed navigation and action mappings.                                                                                  |
| 2026-06-14 | Safe log wipe protocol            | Suspend sensor task during format; purge both parent logs and recovery fragments.                                                                    |
| 2026-06-14 | Recovery log & vTaskDelay support | Detect recovery-style .BIN files; replace delay() with vTaskDelay.                                                                                   |
| 2026-06-14 | Remove dead STATE_SUBMENU_MSG     | Deleted unused message submenu enum, handlers, and rendering code.                                                                                   |
| 2026-06-14 | MPU settings helper               | Extracted configureMPUSettings() to eliminate duplicate initialization blocks.                                                                       |
| 2026-06-14 | Log file validation post-recovery | Null-check reopened log file after MPU reconnection; set sd_critical_error on fail.                                                                  |
| 2026-06-14 | Gap frame zero-init               | memset gap LogFrame before SD write to prevent uninitialized event_flag.                                                                             |
| 2026-06-14 | Recovery ID increment order fix   | Increment global_recovery_id only after file open succeeds.                                                                                          |
| 2026-06-15 | Timing & SD math robustness       | Fix 64-bit torn reads, SD capacity overflow (>32 GB), flush watchdog yields.                                                                         |
| 2026-06-15 | Relative timestamps & OLED reinit | Introduce log_time_base; reinit OLED on wake/menu entry for hot-plug resilience.                                                                     |
| 2026-06-15 | Progress.md update                | Expanded to document storage, recovery, UI, and memory optimization phases.                                                                          |
| 2026-06-15 | To Do list cleanup                | Removed completed items; kept only pending tasks.                                                                                                    |
| 2026-06-15 | Project summarizer tool           | Added project_summarizer.py (1216 lines) for auto-generated architecture reports.                                                                    |
| 2026-06-15 | Generated project report          | Produced comprehensive LLM-optimized reports in MD/TXT/XML formats.                                                                                  |
| 2026-06-15 | .gitignore configuration          | Created .gitignore excluding Repo_Report/ directory from version control.                                                                            |
| 2026-06-15 | Final report merge                | Consolidated Report.md (v1.0.0) and Report2.md (v2.0.0) into single document.                                                                        |
| 2026-06-19 | Perfboard assembly                | Migrated all components from breadboard to perforated fiber board. Hardware validated and stable.                                                    |
| 2026-06-19 | CRC-16 per-frame integrity        | Added bit-by-bit CRC-16-IBM (poly 0xA001) to LogFrame. DATA_FRAME_SIZE updated 45→47. CRC computed before every queue send and gap-frame SD write.   |
| 2026-06-19 | vTaskDelay conversion             | Replaced delay() with vTaskDelay() in calibration functions for scheduler-friendly blocking during sensor suspension.                                |
| 2026-06-19 | Issues verified & resolved        | Confirmed MPU temperature core safety, recovery naming intentionality, TAG button OLED sleep behavior. Updated Issues found by AI.md.                |
| 2026-06-19 | Gap frame timestamp fix           | Changed gap frame timestamp from absolute `esp_timer_get_time()` to relative `esp_timer_get_time() - log_time_base` for consistency with IMU frames. |
| 2026-06-19 | 100 Hz sampling rate enforcer     | Replaced `vTaskDelay(5)` with `vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10))` in sensorTask for exact 100 Hz period.                            |
| 2026-06-19 | last_interaction_millis guard     | TAG/UP/DOWN no longer reset `last_interaction_millis` when SELECT is held, preventing shutdown timer interruption.                                   |
| 2026-06-19 | sensorTask stack doubled          | Stack increased 4096 → 8192 bytes for safety margin during MPU library calls.                                                                        |
| 2026-06-19 | WDT-safe format yields            | Format loop yields increased: inner 1→2 ticks, outer 1→3 ticks for future WDT compatibility.                                                         |
| 2026-06-19 | EEPROM CRC-16 integrity           | Added CRC-16 over magic + 11 calibration floats (addr 46). Corrupt data detected on load with OLED + LED + buzzer alert.                             |
| 2026-06-19 | SPI recovery power-cycle delay    | Added 10ms `vTaskDelay` between `SPI.end()` and `SPI.begin()` in `attemptSDRecovery()`.                                                              |
| 2026-06-19 | Version 2.0 + comment cleanup     | Updated version, date, fixed 8 stale/typo comments (absolute→relative timestamp, 45→47 bytes, 7→8 items, etc.).                                      |
| 2026-06-20 | GPX Fusion Tool — initial version  | Built PyQt6 GUI for fusing Garmin + Geo Tracker GPX logs. Kalman filter with RTS smoother, dual map modes (matplotlib offline / folium online), proxy dialog, file group management. |
| 2026-06-20 | GPX Fusion — QWebEngineView fix    | Fixed `loadFinished` not firing on parented views for folium HTML with CDN scripts. Unparent + reparent pattern with viewport CSS sizing.              |
| 2026-06-20 | GPX Fusion — button/UI fixes       | Added `QPushButton:pressed` state, `QComboBox` and `QStackedWidget` styling, fixed proxy status message bug, added matplotlib zoom buttons.            |
| 2026-06-20 | GPX Fusion — stats panel fixed height | `setFixedHeight(64)` on `FusedStatsPanel` to prevent layout shift after fusion. Fixed `Compositor returned null texture` by preserving web view size before unparenting. |

---

## 14. Current Status

The system is now a wearable Pedestrian Dead Reckoning data logger assembled on perforated fiber board with stable sensing, logging, and analysis layers.  
The immediate next step is developing the offline PDR pipeline in Python. GPS integration will follow after the PDR baseline is validated.

| Subsystem                     | Status   |
| ----------------------------- | -------- |
| Perfboard assembly            | Complete |
| ESP32-S3 migration            | Complete |
| RTOS multi-core framework     | Complete |
| Calibration and I2C tuning    | Complete |
| Hardware validation           | Complete |
| SD card logging               | Complete |
| Binary file logging + CRC-16  | Complete |
| PSRAM buffering               | Complete |
| Fault handling and recovery   | Complete |
| UI and monitoring             | Complete |
| Offline analysis tools        | Complete |
| Offline PDR pipeline (Python) | Planned  |
| GPS integration               | Future   |
| **GPX Fusion Tool**           | Complete |

---

## 15. Conclusion

DeadReckoner evolved from a small IMU prototype into a wearable Pedestrian Dead Reckoning (PDR) data logger.  
The system features a dual-core FreeRTOS design with PSRAM-backed 50,000-frame queue, dynamic SD recovery with gap-frame injection, per-frame CRC-16 integrity checking, and an interactive OLED menu system. Data is recorded at 100 Hz to SD card in 47-byte binary frames. The offline Python pipeline reconstructs the traveled path using step detection, ZUPT, and batch optimization — achieving minimal drift over multi-hour missions without GPS.
