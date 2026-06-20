# Session Summary

## Goal

Fix consecutive TAG button presses lost, verify old-code PDR accuracy against GPS, and implement phone GPS pairing via WiFi AP.

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

### Done

- **TAG button fixed**: `tag_event_triggered` (bool) → `tag_event_pending` (uint8_t counter), capped at 255, decremented per frame in sensorTask
- **Version bumped to 2.2.0** in Report.md, changelog entry added
- **`compare_paths.py`** created — parses old 45-byte BIN, runs PDR (Weinberg step detection + quaternion yaw heading), aligns to Garmin+Geo Tracker GPS via brute-force heading search
- **PDR accuracy verified**: tested all 5 walk segments, avg error 9.3% (max 14.3%), IMU-only PDR sufficient — GPS and BMP280 not needed
- **Saved plots**: `analysis/summary_comparison.png`, `analysis/all_segments_overlay.png`
- **WiFi GPS pairing code**: Added WiFi, WebServer, DNSServer includes; `PhoneGPSData` struct; GPS states in UI state machine (PROMPT_START, PROMPT_END, WAITING, CONFIRM_EXIT); `gps_start_needed`, `gps_end_needed`, `gps_pending_file_creation`, `gps_prompt_cursor`, `gps_data_received` globals; `startGPSAP()`, `stopGPSAP()`, `handleGPSRoot()`, `handleGPSPost()`, `handleGPSNotFound()`, `writeGPSFrame()` functions; GPS HTML page (embedded PROGMEM, manual lat/lon entry, auto-filled date/time from JS)
- **GPS POST handler**: stores start/end GPS data, defers 0xBB write when `gps_pending_file_creation` is true (for Create New File / Format scenarios), writes 0xCC immediately during shutdown
- **Boot GPS prompt**: one-shot trigger at top of `loggingTask()` loop, transitions through state machine
- **Create New File → GPS prompt**: closes old file, sets counters, defers file creation via `gps_pending_file_creation`, transitions to `STATE_GPS_PROMPT_START`
- **Format → GPS prompt**: deletes all files, resets IDs, resumes sensor task, defers file creation, transitions to `STATE_GPS_PROMPT_START`
- **Shutdown GPS end prompt**: integrated as blocking loop inside shutdown handler (after queue drain, before file close) — YES/NO menu → WiFi AP → GPS receive → file close → SAFE TO POWER OFF
- **GPS state machine handlers**: `STATE_GPS_PROMPT_START/END` (YES/NO), `STATE_GPS_WAITING` (polls HTTP/DNS, writes 0xBB + creates file if pending), `STATE_GPS_CONFIRM_EXIT` (YES: exit without data, creates file if pending)
- **GPS exit point file creation**: all three exit paths (PROMPT NO, GPS received, CONFIRM_EXIT YES) open new file, write header, and optionally write 0xBB
- **OLED UI rendering**: added draw calls for all 4 GPS states (PROMPT_START, PROMPT_END, WAITING with SSID+IP, CONFIRM_EXIT)
- **Stale GPS data cleanup**: `phone_gps.has_start = false` set in PROMPT_START NO branch and CONFIRM_EXIT YES branch

### Remaining

- Update Python parser (`compare_paths.py`) to handle 0xCC GPS-end frame (currently only handles 0xAA data frames)
- Verify compilation with ESP32 toolchain

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

## Relevant Files

- `Code_deadreckoner/ESP32_S3/ESP32_S3.ino`: main firmware, WiFi GPS pairing code
- `Collectd Data/2026-20-6/compare_paths.py`: PDR vs GPS comparison tool
- `Report/Report.md`: updated with v2.2 TAG button fix
- `Report/Issues found by AI.md`: TAG button marked [x]
