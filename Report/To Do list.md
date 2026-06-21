# To Do List - DeadReckoner

---

## 1. Core Data Logging & Architecture (Firmware)

- [ ] Implement MPU disconnect marker inside filename/metadata for easier post-analysis filtering
- [ ] Investigate `mpu_critical_error` loop behaviors when hardware connection cuts during I2C calibration
- [ ] Evaluate the necessity of an active internal hardware/software Watchdog for core loop safety
- [x] **WiFi GPS Pairing** — Phone GPS start/end anchoring via WiFi AP, DNS captive portal, browser form → 0xBB/0xCC frames
- [x] **GPS Bug Fixes** — JSON string-vs-number, writeGPSFrame alt/epoch, time field UTC/readonly, SdFat/FS.h conflict
- [x] Evaluate adding a **BMP280 Barometer** to offset accumulated drift in Z-Axis calculations (Sensor validated)

---

## 2. User Interface & Experience (UI/UX - Core 1)

- [ ] Refactor and optimize Boot Menu layout for smoother peripheral device checking
- [ ] Implement a non-blocking battery charge state icon indicator on the line header
- [ ] Fix OLED power-up state latching (cold-booting without display requires board hard-reset)

---

## 3. Offline PDR Pipeline (Python Post-Processing)

- [x] **Phase 1: Binary parser** — Read 47-byte frames from .BIN, verify CRC-16, extract timestamps + quaternions + linear acceleration [FileHeader support added in v2.1]
- [x] **Phase 2: World-frame rotation** — Rotate body-frame linear acceleration to world frame using firmware quaternions
- [x] **Phase 3: Step detection** — Peak detection on acceleration magnitude (Weinberg)
- [x] **Phase 4: Step length** — Weinberg empirical formula (K=0.425 optimized via tune_pdr.py)
- [x] **Phase 5: Heading** — Yaw angle from firmware quaternions (now Mahony — pure gyro integration)
- [ ] **Phase 6: ZUPT + RTS smoother** — Bidirectional batch optimization over entire walk for sub-3% drift
- [x] **Phase 7: Path visualization** — 2D trajectory plot with matplotlib
- [x] **2D heading+drift optimization** — Replaces brute-force heading search; finds optimal offset + linear drift for each segment
- [x] **PDR accuracy verified** — Avg 4.9% error, total 2.8% over 5 walk segments (post-tune_pdr bug fix)
- [x] **Path shape diagnosis** — Madgwick magnetometer yaw lock identified as root cause
- [x] **`--gps-guided` mode** — Proves PDR step lengths are correct (shape avg 3m vs GPS)
- [x] **`tune_pdr.py` bug fixed** — Was passing local xy to haversine instead of lat/lon
- [x] **CRC-16 verified** — 0% failure across ~568K frames
- [x] (Future) BMP280 barometric altitude for 3D tracking (Sensor validated — ready for integration)
- [ ] (Future) GPS correction for absolute position anchoring

---

## 4. Hardware & Electrical Engineering Challenges

- [x] **SD Card Inrush Current Protection:** Add decoupling capacitors at the SD slot to prevent Brownout resets.
- [ ] **I2C Signal Integrity:** Mount rigid $4.7\text{k}\Omega$ external pull-up resistors on SDA/SCL lines to guard MPU9250 against logging noise.
- [ ] **Hardware Button Debouncing:** Solder a $0.1\mu\text{F}$ ceramic capacitor in parallel across tactile switch lines to absorb mechanical bouncing noise.

---

## 5. GPX Fusion Tool (Python/PyQt6)

- [ ] **Online map resize after fuse:** Folium map container shows wrong dimensions after clicking "Fuse Selected Walk" in online mode. Layout settling timing needs investigation.
- [ ] **Matplotlib zoom in online mode:** Zoom buttons currently only affect offline mode. Extend to folium maps (programmatic `setZoom()`).
- [ ] **GPX export elevation:** Export currently saves only lat/lon/time. Add elevation if available in fused result.

---

## 6. WiFi GPS Pairing (v2.2) — Completed Items

- [x] WiFi AP + DNS captive portal (SSID: "DeadReckoner-S3", password: "deadreckoner")
- [x] HTML form: lat, lon, alt (optional), editable local date/time
- [x] 0xBB (start GPS) / 0xCC (end GPS) frame writing with alt+epoch
- [x] GPS state machine: PROMPT_START → WAITING → CONFIRM_EXIT / PROMPT_END
- [x] Boot, Create New File, Format triggers for GPS start prompt
- [x] Shutdown GPS end prompt (blocking loop)
- [x] Deferred file creation for GPS-first frames
- [x] OLED sleep prevention during GPS states
- [x] GPS data recording verified with `parse_log.py`

## 7. Remaining / Future

- [ ] **Re-collect walks with Mahony filter:** Flash ESP32-S3 (MADGWICK→MAHONY), re-run 5 walk segments, verify shape improvement via `python compare_paths.py --batch --gps-guided`
- [ ] **UART NEO-6M GPS Module:** Direct GPS module integration (no phone needed) — NMEA parser, 1 Hz coordinate injection
- [x] **0xCC parser support:** `compare_paths.py` already handles 0xCC GPS-end frames
- [x] **Verify ESP32 compilation:** Build ESP32_S3.ino with Arduino IDE after GPS pairing + Mahony changes
- [ ] **ZUPT + RTS smoother:** Implement bidirectional batch optimization in Python for sub-3% drift
- [ ] **Per-user step calibration:** Tune Weinberg constant per user height for improved PDR accuracy
- [ ] **BMP280 altitude integration:** Add barometric altitude to LogFrame for 3D tracking
- [ ] **OLED battery icon:** Non-blocking voltage read + charge state on line header
- [ ] **Button hardware debouncing:** Solder capacitors across tactile switch lines

---
