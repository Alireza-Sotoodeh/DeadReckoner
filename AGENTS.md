# Session Summary

## Goal

Fix consecutive TAG button presses lost, verify old-code PDR accuracy against GPS, implement phone GPS pairing via WiFi AP, and **fix PDR path shape** (Madgwick magnetometer yaw lock → switch to Mahony filter).

## Constraints & Preferences

- ESP32-S3 N16R8 with 16MB Flash + 8MB Octal PSRAM
- Browser Geolocation API requires HTTPS — use manual lat/lon entry on web page instead
- WiFi AP password: "deadreckoner", SSID: "DeadReckoner-S3"
- No auto-timeout on GPS pairing — user exits via SELECT + confirmation
- GPS prompt: at boot, Create New File, Format (each time); at shutdown for end GPS
- Wait 5s after GPS confirmation then close AP regardless
- Keep GPS union in LogFrame (costs nothing, keeps option open)
- All 12 issues from v2.1 resolved

## Progress

### Done (Python tools)

- **`compare_paths.py` rewritten** — 47-byte BIN parser (magic 0xDEADC0DE, CRC-16 matching firmware), PDR pipeline (Weinberg K=0.425, peak-based yaw), 2D heading+drift optimization, path shape metrics
- **`tune_pdr.py` bug fixed** — was passing local xy to `compute_gps_distance` instead of lat/lon (causing 100% error for all K values). After fix: found optimal K=0.425, height=1.5, dist=25, avg error 4.9%
- **CRC-16 verified**: 0% failure across ~568K frames (poly 0xA001, init 0xFFFF, no final XOR)
- **`--gps-guided` mode** added — proves PDR step lengths are correct by substituting GPS heading; avg shape drops from 156m → **3m**
- **`ANALYSIS_METHODS.md`** created — documents all methods, bugs fixed, and diagnosis
- **Path shape root cause diagnosed**: Madgwick filter incorporates magnetometer into gradient descent at full `beta` strength, locking yaw to magnetic North. `zeta` is already 0.0 (unrelated). **Fix: switch to MAHONY filter** (ignores magnetometer for yaw, pure gyro integration).

### Done (Firmware)

- **TAG button fixed**: `tag_event_triggered` (bool) → `tag_event_pending` (uint8_t counter)
- **Version bumped to 2.2.0**
- **WiFi GPS pairing**: full AP mode with captive portal, manual lat/lon entry web page, GPS state machine (PROMPT_START/END/WAITING/CONFIRM_EXIT), 0xBB/0xCC frames
- **GPS prompts**: at boot, Create New File, Format (each gets anchor); at shutdown for end GPS

### Remaining

- **FIRMWARE FIX**: Change `#define MPU9250_filter_algorithm MADGWICK` → `MAHONY` in `ESP32_S3.ino:114`
- Re-flash ESP32-S3, re-collect 5 walk segments
- Re-run `compare_paths.py --batch --gps-guided` to verify shape improved
- Verify ESP32 compilation

## Key Decisions

- WiFi AP over BLE: no phone app needed, works with any browser
- Manual lat/lon entry (copy from maps app) instead of auto-fetch — browser Geolocation requires HTTPS
- Keep GPS union in LogFrame (0 bytes wasted, imu already fills 32-byte payload)
- No auto-timeout on GPS AP — user controls exit via SELECT
- Re-prompt GPS on every Create New File/Format (each file gets its own anchor)
- Frame format: 0xBB (start GPS), 0xCC (end GPS) using existing GPS union payload slot
- Shutdown GPS end prompt uses blocking while-loop (not state machine) because shutdown is sequential (drain → GPS → close → lock)
- GPS start 0xBB write is deferred to GPS_WAITING exit (not written in POST handler) to handle both boot (file exists) and Create New File/Format (file created after GPS)
- Format handler resumes sensorTask before GPS prompt (deletes are done, sensor can run during AP)
- **Switch MADGWICK → MAHONY**: Mahony ignores magnetometer for yaw (pure gyro integration). 2D heading+drift optimization handles residual linear gyro drift.

## Relevant Files

- `Code_deadreckoner/ESP32_S3/ESP32_S3.ino`: main firmware, WiFi GPS pairing, **line 114 needs MADGWICK→MAHONY change**
- `Collectd Data/2026-21-6-6AM/compare_paths.py`: PDR vs GPS comparison tool
- `Collectd Data/2026-21-6-6AM/tune_pdr.py`: brute-force K tuner (bug fixed)
- `Collectd Data/2026-21-6-6AM/ANALYSIS_METHODS.md`: full methods, fixes, and diagnosis documentation
- `Report/Report.md`: updated with v2.2 TAG button fix
- `Report/Issues found by AI.md`: TAG button marked [x]
