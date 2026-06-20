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

## 3. Offline PDR Pipeline (Python Post-Processing)

- [ ] **Phase 1: Binary parser** — Read 47-byte frames from .BIN, verify CRC-16, extract timestamps + quaternions + linear acceleration
- [ ] **Phase 2: World-frame rotation** — Rotate body-frame linear acceleration to world frame using firmware Madgwick quaternions
- [ ] **Phase 3: Step detection** — Peak detection on acceleration magnitude
- [ ] **Phase 4: Step length** — Weinberg empirical formula
- [ ] **Phase 5: Heading** — Yaw angle from firmware quaternions (magnetometer-corrected)
- [ ] **Phase 6: ZUPT + RTS smoother** — Bidirectional batch optimization over entire walk for sub-3% drift
- [ ] **Phase 7: Path visualization** — 2D trajectory plot with matplotlib
- [ ] (Future) BMP280 barometric altitude for 3D tracking
- [ ] (Future) GPS correction for absolute position anchoring

---

## 4. Hardware & Electrical Engineering Challenges

- [x] **SD Card Inrush Current Protection:** Add $10\mu\text{F}$ or $47\mu\text{F}$ decoupling capacitors in parallel with $0.1\mu\text{F}$ right at the SD slot to prevent Brownout resets.
- [ ] **I2C Signal Integrity:** Mount rigid $4.7\text{k}\Omega$ external pull-up resistors on SDA/SCL lines to guard MPU9250 against logging noise.
- [ ] **Hardware Button Debouncing:** Solder a $0.1\mu\text{F}$ ceramic capacitor in parallel across tactile switch lines to absorb mechanical bouncing noise.

---

## 5. GPX Fusion Tool (Python/PyQt6)

- [ ] **Online map resize after fuse:** Folium map container shows wrong dimensions after clicking "Fuse Selected Walk" in online mode. Layout settling timing needs investigation.
- [ ] **Matplotlib zoom in online mode:** Zoom buttons currently only affect offline mode. Extend to folium maps (programmatic `setZoom()`).
- [ ] **GPX export elevation:** Export currently saves only lat/lon/time. Add elevation if available in fused result.

---
