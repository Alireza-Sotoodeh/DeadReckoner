# To Do List - DeadReckoner

---

## 1. Core Data Logging & Architecture (Firmware)

- [ ] Implement MPU disconnect marker inside filename/metadata for easier post-analysis filtering
- [ ] Investigate `mpu_critical_error` loop behaviors when hardware connection cuts during I2C calibration
- [ ] Evaluate the necessity of an active internal hardware/software Watchdog for core loop safety
- [ ] Integrate **GPS Payload Decoding** (Limit writing rate to 1Hz inside `LogFrame` Union to preserve bandwidth)
- [ ] Evaluate adding a **BMP280 Barometer** to offset accumulated drift in Z-Axis calculations

---

## 2. User Interface & Experience (UI/UX - Core 1)

- [ ] Refactor and optimize Boot Menu layout for smoother peripheral device checking
- [ ] Implement a non-blocking battery charge state icon indicator on the line header
- [ ] Fix OLED power-up state latching (cold-booting without display requires board hard-reset)

---

## 3. Software Engineering & Post-Processing (MATLAB)

- [ ] Research and implement the primary Dead Reckoning localization filter:
  - [ ] Zero Velocity Update (ZUPT)
  - [ ] Step Hunter / Step Length Estimation (SHS)
  - [ ] LSTM / Deep Learning Sequence Modeling

---

## 4. Hardware & Electrical Engineering Challenges

- [x] **SD Card Inrush Current Protection:** Add $10\mu\text{F}$ or $47\mu\text{F}$ decoupling capacitors in parallel with $0.1\mu\text{F}$ right at the SD slot to prevent Brownout resets.
- [ ] **I2C Signal Integrity:** Mount rigid $4.7\text{k}\Omega$ external pull-up resistors on SDA/SCL lines to guard MPU9250 against logging noise.
- [ ] **Hardware Button Debouncing:** Solder a $0.1\mu\text{F}$ ceramic capacitor in parallel across tactile switch lines to absorb mechanical bouncing noise.

---

## 
