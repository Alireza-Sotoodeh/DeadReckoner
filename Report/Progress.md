## Software Architecture Evolution & Development Phases

The development of DeadReckoner followed a staged architecture-driven roadmap. Each phase established the technical foundation required for subsequent phases. The progression reflects the actual implementation history of the project, from early sensor evaluation to the current multi-core data-logging platform.

---

#### Phase 0: Legacy Prototype & Sensor Research [COMPLETED]

This phase focused on evaluating inertial navigation hardware and establishing the initial software baseline.

- [x] **Project Bootstrap:** Create the initial firmware framework and repository structure.
- [x] **MPU6500 Evaluation:** Integrate and validate the MPU6500 sensor library.
- [x] **DMP Integration:** Add and evaluate Digital Motion Processor (DMP) support.
- [x] **STM32/CubeIDE Development Environment:** Establish the initial embedded development workflow.
- [x] **Sensor Communication Validation:** Verify low-level IMU communication and register access.
- [x] **Linear Acceleration Processing:** Correct gravity-compensated acceleration calculations.
- [x] **OLED Interface Experiments:** Develop and validate the first display subsystem.
- [x] **Prototype Validation:** Confirm basic IMU acquisition and display functionality.

---

#### Phase 1: ESP32-S3 Migration & System Architecture [COMPLETED]

The project architecture was redesigned around the ESP32-S3 platform to provide additional processing power, memory resources, and real-time scheduling capabilities.

- [x] **Platform Migration:** Migrate the firmware from the ESP8266 NodeMCU platform to the ESP32-S3 N16R8.
- [x] **Firmware Reinitialization:** Rebuild the project structure around the new hardware platform.
- [x] **Pin Porting:** Remap all peripherals to the ESP32-S3 IO matrix.
- [x] **Hardware Redesign:** Update wiring architecture and peripheral connections.
- [x] **Firmware Flash Configuration:** Configure flashing and deployment parameters for ESP32-S3.
- [x] **System Bring-Up Verification:** Validate successful operation of all onboard peripherals.

---

#### Phase 2: RTOS & Multi-Core Framework [COMPLETED]

This phase transformed the firmware from a monolithic design into a real-time multi-core architecture.

- [x] **FreeRTOS Integration:** Introduce task-based scheduling.
- [x] **Dual-Core Architecture:** Separate acquisition and storage workloads across CPU cores.
- [x] **Sensor Task Implementation:** Execute high-frequency sensor acquisition on Core 0.
- [x] **Logging Task Implementation:** Execute storage operations on Core 1.
- [x] **Producer–Consumer Model:** Establish queue-based communication between tasks.
- [x] **Thread-Safe Data Exchange:** Replace shared-state communication with message passing.
- [x] **Race Condition Mitigation:** Eliminate direct inter-task memory contention.
- [x] **Real-Time Stability Validation:** Verify deterministic operation under sustained load.

---

#### Phase 3: Calibration & I2C Optimization [COMPLETED]

This phase focused on improving sensor accuracy, calibration persistence, and I2C bus reliability.

- [x] **I2C Fast Mode:** Increase bus speed to 400 kHz.
- [x] **EEPROM Calibration Storage:** Implement persistent storage for calibration data.
- [x] **Calibration Restoration:** Automatically restore bias values during startup.
- [x] **Calibration Compensation Fix:** Correct post-load sensor offset behavior.
- [x] **Bus Collision Prevention:** Suspend acquisition during EEPROM write operations.
- [x] **Dual-I2C Topology:** Move OLED traffic to the secondary I2C controller (`Wire1`).
- [x] **I2C Stability Validation:** Verify reliable operation of all I2C peripherals.

---

#### Phase 4: Hardware Validation & Performance Testing [COMPLETED]

This phase established confidence in hardware reliability and sensor performance through structured testing.

- [x] **MPU9250 Sanity Check:** Validate IMU initialization and communication.
- [x] **OLED Sanity Check:** Validate display communication and rendering.
- [x] **Hardware Integration Testing:** Verify complete system functionality.
- [x] **Static Drift Evaluation:** Measure long-term orientation stability.
- [x] **Dynamic Return-to-Zero Testing:** Evaluate accumulated drift after motion.
- [x] **Vibration Rejection Testing:** Assess filter robustness under vibration.
- [x] **Filter Validation:** Verify orientation estimation consistency.
- [x] **Performance Visualization Tools:** Develop drift-analysis visualization utilities.

---

#### Phase 5: SD Card Storage Architecture [COMPLETED]

This phase focused on selecting and validating a reliable storage subsystem for mission logging.

- [x] **SD Card Interface Investigation:** Evaluate multiple SD card connection strategies.
- [x] **SPI Storage Architecture:** Adopt SPI-based storage communication.
- [x] **Standalone SD Validation:** Create isolated SD card validation firmware.
- [x] **Hardware Adapter Evaluation:** Compare SD adapter implementations.
- [x] **Signal Integrity Optimization:** Operate entirely on the 3.3 V logic domain.
- [x] **SPI Frequency Optimization:** Limit SPI frequency to 10 MHz for stable operation.
- [x] **Storage Benchmarking:** Evaluate practical write performance.
- [x] **Final Storage Validation:** Successfully validate long-duration SD card operation.

---

#### Phase 6: Binary Logging Framework [COMPLETED]

The storage subsystem was expanded into a structured binary data-logging architecture.

- [x] **LogFrame Definition:** Design a fixed-size binary telemetry structure.
- [x] **Timestamp Integration:** Store acquisition timestamps for every sample.
- [x] **Quaternion Logging:** Record fused orientation estimates.
- [x] **Linear Acceleration Logging:** Record gravity-compensated acceleration vectors.
- [x] **Binary Block Writing:** Replace text-based logging with binary storage.
- [x] **Continuous SD Streaming:** Implement sustained high-rate data recording.
- [x] **Sequential Log File Naming:** Generate unique mission log files automatically.
- [x] **Boot-Time Log Discovery:** Scan storage and determine the next available log index.
- [x] **Mission File Management:** Track active and future log identifiers.
- [x] **Storage Capacity Monitoring:** Display available storage information to the user.

---

#### Phase 7: Fault Detection, Recovery & Mission Safety [COMPLETED]

A dedicated fault-management layer was added to increase system robustness during field operation.

- [x] **MPU9250 Disconnect Detection:** Detect unexpected sensor communication failures.
- [x] **Sensor Timeout Monitoring:** Monitor acquisition timing and communication health.
- [x] **Critical Fault State Machine:** Introduce centralized fault handling.
- [x] **Safe Logging Shutdown:** Protect recorded data during failures.
- [x] **Automatic File Synchronization:** Flush pending data before shutdown.
- [x] **Boot-Time SD Validation:** Verify storage availability during startup.
- [x] **OLED Fault Reporting:** Display diagnostic information to the operator.
- [x] **Audible Fault Alerts:** Add buzzer-based alarm notifications.
- [x] **Visual Fault Alerts:** Add LED-based warning indicators.
- [x] **Dynamic Recovery Protocol:** Attempt automatic recovery after sensor reconnection.
- [x] **Mission Safety Mechanisms:** Prevent operation under unsafe sensor conditions.

---

#### Phase 8: User Interface & Operational Monitoring [COMPLETED]

This phase improved usability and runtime observability.

- [x] **Multi-Page OLED Interface:** Develop operational display screens.
- [x] **Runtime Status Visualization:** Present sensor and system status information.
- [x] **Storage Monitoring Interface:** Display SD card statistics and capacity information.
- [x] **Display Mode Selection:** Implement selectable display modes.
- [x] **Buzzer Control Interface:** Add runtime buzzer enable/disable functionality.
- [x] **SD Card Management Menu:** Add dedicated storage-management screens.
- [x] **System Diagnostics Display:** Present fault and recovery information.

---

#### Phase 9: Data Analysis & Validation Toolchain [COMPLETED]

Tools were developed to analyze recorded datasets and verify system performance.

- [x] **MATLAB Binary Reader:** Develop an offline decoder for recorded log files.
- [x] **Quaternion Visualization:** Plot recorded orientation data.
- [x] **Acceleration Visualization:** Analyze recorded acceleration signals.
- [x] **Drift Analysis Workflow:** Evaluate long-term navigation stability.
- [x] **Performance Validation Pipeline:** Create a repeatable testing methodology.
- [x] **Experimental Result Verification:** Correlate recorded data with physical tests.

---

#### Phase 10: GPS Integration & Time Synchronization [PLANNED]

This phase will extend the inertial logger into a complete GNSS-assisted navigation platform.

- [x] **GPS Fields Reserved in LogFrame:** Prepare the logging structure for GNSS data.
- [ ] **Dedicated UART Configuration:** Configure a hardware UART for the S6MV2 GNSS receiver.
- [ ] **NMEA Parsing Engine:** Decode GNSS position and timing messages.
- [ ] **Coordinate Injection:** Insert GPS coordinates into the logging pipeline.
- [ ] **GNSS Status Monitoring:** Track satellite lock and navigation status.
- [ ] **Time Synchronization Algorithm:** Align 1 Hz GNSS updates with the 100 Hz IMU stream.
- [ ] **Trajectory Reconstruction Validation:** Verify synchronized IMU/GPS datasets.
- [ ] **Dead-Reckoning Fusion Layer:** Integrate GNSS corrections into the navigation framework.
