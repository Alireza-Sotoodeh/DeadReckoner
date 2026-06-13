# To Do List - DeadReckoner

---

## 🛠 1. Core Data Logging & Architecture (Firmware)

### 💾 SD Card Dynamic Recovery Pipeline

- [x] Handle **Runtime SD Failure** (Hot-plugging/Disconnect post-boot)
- [x] Implement memory-optimized **Union Structure** (Reduced frame from 49B to strictly 33B)
- [x] Formulate high-capacity static buffering in **external PSRAM** (Allocated massive 50,000 frames)
- [x] Invent deterministic relational name-generation logic (`[XXX][YYY].BIN`) for $O(1)$ fast-recovery
- [x] Inject **Gap Frames** (`0xFFFFFFFF`) to log physical card disconnection events
- [x] Fix **Queue Poisoning / Ghost Frames** leaking into fresh sessions during creation/formatting
- [x] Resolve **FAT Table Freeze** on UI by optimizing long directory search tasks
- [x] Mitigate 32-bit Integer Overflow calculation for high-capacity SD cards (>32GB)
- [x] Calibrate SD storage bandwidth estimation formula from 20.16 to **11.88 MB/Hour**
- [x] Upgrade formatting option to completely wipe parent files alongside nested recovery fragments

### 🩺 MPU9250 Runtime Health & Tracking

- [x] Remove destructive `xQueueReset` in `loggingTask` to wipe out Race Conditions on sensor reconnect
- [x] Transition `global_frame_counter` to global volatile state to clear MATLAB stitching offsets
- [ ] Implement MPU disconnect marker inside filename/metadata for easier post-analysis filtering
- [ ] Investigate `mpu_critical_error` loop behaviors when hardware connection cuts during I2C calibration

### ⚡ Power Management & Reliability

- [x] Formulate **Safe Shutdown Protocol** via 3-second long press on SELECT button
- [x] Fix Task Watchdog Panic during massive 1.6MB data flushing upon shutdown
- [ ] Evaluate the necessity of an active internal hardware/software Watchdog for core loop safety

### 🛰 Secondary Sensor & Pin Assignments

- [x] Relocate pins from GPIO 9 & 10 due to Internal Flash/PSRAM strapping line conflicts
- [ ] Integrate **GPS Payload Decoding** (Limit writing rate to 1Hz inside `LogFrame` Union to preserve bandwidth)
- [ ] Evaluate adding a **BMP280 Barometer** to offset accumulated drift in Z-Axis calculations

---

## 📺 2. User Interface & Experience (UI/UX - Core 1)

- [x] Transform Live View frame counter into a fixed-width chronological timestamp (`MM:SS`)
- [ ] Refactor and optimize Boot Menu layout for smoother peripheral device checking
- [ ] Implement a non-blocking battery charge state icon indicator on the line header
- [ ] Fix OLED power-up state latching (Fixing bug where cold-booting without display requires board hard-reset)

---

## 📊 3. Software Engineering & Post-Processing (MATLAB)

- [x] Create a robust MATLAB script to parse, align, and stitch binary fragments seamlessly
- [x] Build Garbage-Frame tail filters to isolate hardware-interrupted sector dumps
- [x] Integrate mathematical Quaternion-to-Euler conversion matrix (Roll, Pitch, Yaw) for physical insight
- [x] Incorporate signal analysis parameters (Mean, Peak Acceleration, RMS noise floor tracking)
- [ ] Research and implement the primary Dead Reckoning localization filter:
  - [ ] Zero Velocity Update (ZUPT)
  - [ ] Step Hunter / Step Length Estimation (SHS)
  - [ ] LSTM / Deep Learning Sequence Modeling

---

## 🔌 4. Hardware & Electrical Engineering Challenges

- [ ] **SD Card Inrush Current Protection:** Add $10\mu\text{F}$ or $47\mu\text{F}$ decoupling capacitors in parallel with $0.1\mu\text{F}$ right at the SD slot to prevent Brownout resets.
- [ ] **I2C Signal Integrity:** Mount rigid $4.7\text{k}\Omega$ external pull-up resistors on SDA/SCL lines to guard MPU9250 against logging noise.
- [ ] **Hardware Button Debouncing:** Solder a $0.1\mu\text{F}$ ceramic capacitor in parallel across tactile switch lines to absorb mechanical bouncing noise.
