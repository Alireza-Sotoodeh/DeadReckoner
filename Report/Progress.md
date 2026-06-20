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

- [x] LogFrame Memory Optimization: Redesign the telemetry structure using a compact union-based memory layout.

- [x] PSRAM Logging Buffer: Implement high-capacity external PSRAM buffering for sustained logging operations.

- [x] Deterministic File Naming Scheme: Develop O(1) mission and recovery file generation logic.

- [x] Gap Frame Injection: Record SD-card disconnection events using dedicated marker frames.

- [x] Frame Counter Synchronization: Add thread-safe frame tracking across acquisition and logging tasks.

- [x] **LogFrame Memory Optimization:** Redesign the logging structure using packed memory layout and union-based payload sharing.

- [x] **64-bit Timestamp Support:** Replace overflow-prone timestamps with microsecond-resolution 64-bit hardware timestamps.

- [x] **Frame Sequence Tracking:** Add monotonic frame numbering for drop detection and recovery analysis.

- [x] **CRC-16 Per-Frame Integrity:** Add bit-by-bit CRC-16-IBM checksum to every `LogFrame` for silent corruption detection during offline analysis.

- [x] **Dropped Frame Accounting:** Monitor queue overflows and track lost samples during runtime.

- [x] **PSRAM Queue Architecture:** Introduce a large PSRAM-backed logging queue to decouple acquisition and storage workloads.

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

- [x] Runtime SD Card Recovery: Detect and recover from SD-card removal and reinsertion events during operation.

- [x] Recovery Fragment Logging: Create recovery log segments automatically after storage restoration.

- [x] Queue Integrity Protection: Eliminate stale queue data and cross-session frame contamination.

- [x] FAT Filesystem Performance Optimization: Prevent UI freezes caused by large directory traversal operations.

- [x] Storage Capacity Overflow Protection: Correct calculations for high-capacity SD cards (>32 GB).

- [x] Sensor Reconnection Stability Improvements: Remove queue-reset race conditions during MPU9250 recovery.

- [x] Safe Shutdown Protocol: Gracefully stop logging and synchronize pending data before power-off.

- [x] Watchdog-Safe Buffer Flushing: Prevent watchdog resets during large shutdown write operations.

- [x] **Mission Safety Mechanisms:** Prevent operation under unsafe sensor conditions.

- [x] **Runtime SD Card Recovery:** Automatically attempt SD-card reinitialization after storage failures.

- [x] **Recovery Log Fragment System:** Create recovery files automatically after successful SD reconnection.

- [x] **Logging Gap Markers:** Insert dedicated gap records into the binary stream after recovery events.

- [x] **MPU9250 Automatic Reinitialization:** Reconfigure the sensor and reload calibration after reconnection.

- [x] **Write Verification Layer:** Validate every SD write operation and detect silent storage failures.

- [x] **Recovery State Isolation:** Prevent corrupted file handles from resuming normal logging.

- [x] **Safe Power-Off Procedure:** Flush queued telemetry and synchronize storage before shutdown.

- [x] **Watchdog-Safe Queue Draining:** Prevent resets while flushing large buffered datasets.

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

- [x] Recording Time Visualization: Replace raw frame counters with mission elapsed time display.

- [x] OLED Power Management: Implement automatic display sleep and wake-up functionality.

- [x] Runtime Recovery Feedback: Present SD-card recovery and fault status information to the user.

- [x] **Hierarchical Menu System:** Implement multi-level OLED menu navigation.

- [x] **SD Card Information Dashboard:** Display storage capacity, free space, remaining recording time, file count, and frame-drop statistics.

- [x] **Runtime Log Management:** Allow creation of new mission log files directly from the device UI.

- [x] **Log Cleanup Utility:** Add on-device deletion and formatting workflow for recorded logs.

- [x] **OLED Power Management:** Implement automatic display sleep and wake-up behavior.

- [x] **Display Mode Configuration:** Support always-on and auto-off display modes.

- [x] **Runtime Audio Configuration:** Allow enabling and disabling audible notifications.

- [x] **Waypoint Tagging Interface:** Add dedicated user event markers with visual and audible feedback.

- [x] **Recovery Status Visualization:** Display real-time recovery progress and fault diagnostics.

---

#### Phase 8.5: Memory Optimization & Scalability [COMPLETED]

This phase focused on improving memory efficiency, scalability, and long-duration mission support.

- [x] Union-Based Telemetry Architecture: Share memory between future sensor payload types.
- [x] Packed Binary Structures: Reduce memory footprint and storage bandwidth requirements.
- [x] PSRAM Buffer Integration: Utilize external PSRAM for large-scale telemetry buffering.
- [x] Concurrent Data Protection: Introduce synchronization mechanisms for shared counters and event tagging.
- [x] Scalable Logging Foundation: Prepare the logging framework for future GPS integration.

---

#### Phase 9: Data Analysis & Validation Toolchain [COMPLETED]

Tools were developed to analyze recorded datasets and verify system performance.

- [x] **MATLAB Binary Reader:** Develop an offline decoder for recorded log files.

- [x] **Quaternion Visualization:** Plot recorded orientation data.

- [x] **Acceleration Visualization:** Analyze recorded acceleration signals.

- [x] **Drift Analysis Workflow:** Evaluate long-term navigation stability.

- [x] **Performance Validation Pipeline:** Create a repeatable testing methodology.

- [x] **Experimental Result Verification:** Correlate recorded data with physical tests.

- [x] Binary Fragment Reconstruction: Automatically stitch fragmented recovery logs into continuous datasets.

- [x] Garbage Frame Filtering: Remove corrupted trailing frames generated by interrupted writes.

- [x] Quaternion-to-Euler Conversion: Add orientation visualization in roll, pitch, and yaw form.

- [x] Statistical Signal Analysis: Calculate RMS, mean, peak, and noise metrics from recorded data.

---

#### Phase 10: Offline PDR Pipeline & Post-Processing [PLANNED]

This phase will develop the Python-based Pedestrian Dead Reckoning pipeline to reconstruct the traveled path from logged IMU data.

- [x] **GPS Fields Reserved in LogFrame:** Prepare the logging structure for GNSS data.

- [x] **GPS Payload Reservation:** Reserve binary payload space for future GNSS records through a shared LogFrame union architecture.

- [x] **Future-Proof Log Structure:** Design the logging format to support mixed IMU and GPS event packets without breaking compatibility.

- [ ] Binary Parser & CRC Verification: Read 47-byte frames, validate CRC-16, extract timestamps + quaternions + acceleration.

- [ ] World-Frame Acceleration Rotation: Transform body-frame acceleration using stored quaternions.

- [ ] Step Detection: Peak-finding on acceleration magnitude for footstep identification.

- [ ] Step Length Estimation: Weinberg empirical formula.

- [ ] Heading from Quaternions: Extract magnetometer-stabilized yaw for each step.

- [ ] ZUPT + RTS Smoother: Bidirectional batch optimization over entire walk for minimal drift.

- [ ] Trajectory Visualization: 2D path plot and distance metrics with matplotlib.

- [ ] **Dedicated UART Configuration:** Configure a hardware UART for the S6MV2 GNSS receiver.

- [ ] **NMEA Parsing Engine:** Decode GNSS position and timing messages.

- [ ] **Coordinate Injection & GPS-IMU Fusion:** Align 1 Hz GPS with 100 Hz IMU for absolute position anchoring.

---

#### Phase 12: GPX Fusion Tool (Python/PyQt6) [COMPLETED]

A PyQt6 desktop application at `Code_deadreckoner/Python/GPXFusion/fusion_ui.py` that fuses GPS logs from Garmin eTrex 30x and Geo Tracker Android app into a single accurate path using a Kalman filter with RTS smoothing.

- [x] **GPX Parser:** Parse standard GPX + Geo Tracker `geotracker:meta` extensions (accuracy `c`, speed `s`). Source detection by filename substring.
- [x] **Kalman Filter Engine:** Constant-velocity motion model in local meters (equirectangular projection). Per-device noise: Geo Tracker uses reported `c` field, Garmin defaults to 6 m.
- [x] **RTS Backward Smoother:** Bidirectional Rauch–Tung–Striebel pass over entire fused sequence.
- [x] **Time Grid Interpolation:** Resample all device tracks to a common 1 Hz grid.
- [x] **Offline Map (matplotlib):** Static map with lat/lon grid, color-coded raw tracks, bold red fused path, start/end markers, stats overlay box. No internet required.
- [x] **Online Map (folium):** Interactive Leaflet map with 5 tile providers (OpenStreetMap, CartoDB positron, CartoDB dark_matter, Esri WorldImagery, Esri WorldTopoMap).
- [x] **Map Mode Switching:** QComboBox toggles between offline and online. Online grayed out if folium/PyQt6-WebEngine missing.
- [x] **Tile Provider Selection:** QComboBox triggers map regeneration on change.
- [x] **Proxy Settings:** ProxyDialog (QDialog) with enable checkbox, host QLineEdit, port QSpinBox. Applied to connectivity check and folium tile fetching.
- [x] **Connectivity Check:** `urllib` HEAD request to jsdelivr CDN before loading folium map.
- [x] **Walk Group Management:** Tabs for organizing walks. "+ Add Walk" / "− Remove" buttons.
- [x] **GPX File Import:** QFileDialog + drag-and-drop support. File info panel on selection.
- [x] **Fuse Button:** Green-styled QPushButton. Runs Kalman fusion on all files in the current tab.
- [x] **Export Fused GPX:** Save fused result as standard GPX via QFileDialog.
- [x] **Stats Panel:** Shows fused path: points, distance (km), duration (min), avg speed (m/s).
- [x] **Control Panel:** Process Noise, Geo Tracker Noise, Garmin Noise spinboxes + RTS Smoother checkbox.
- [x] **Refresh Map Button:** Regenerates current map mode.
- [x] **Matplotlib Zoom Buttons:** Zoom In (+), Zoom Out (−), Reset (R) — adjusts plot margin factor.
- [x] **Resize Handling:** `resizeEvent` re-scales pixmap in offline mode; calls `map.invalidateSize()` for online folium map.
- [x] **QWebEngineView Unparent Fix:** `setParent(None)` before `setHtml()` makes `loadFinished` fire for CDN-script pages. Reparent with CSS viewport fix on load.
- [x] **Compositor null texture fix:** Preserve web view size via `resize(size)` before unparenting.
- [x] **Stylesheet Fixes:** `.QWidget` selector to avoid subclass interference. QPushButton `:pressed` state. QComboBox styling. QSpinBox up/down button sub-controls. QStackedWidget background.
- [x] **Stats panel fixed height:** `setFixedHeight(64)` prevents layout shift after fusion.

---
