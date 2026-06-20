# Issues found by AI — Remaining Action Items

> `[x]` = resolved. Unchecked items remain open.

---

## 1. Logical Errors & Bugs

### Critical

- [x] **MPU temperature read from wrong core** — `mpu.getTemperature()` is called in `loggingTask` (Core 1) while `mpu.update()` runs in `sensorTask` (Core 0). The MPU9250 library is not thread-safe. Move temperature read to `sensorTask` and pass via queue.
  *[FIXED: `getTemperature()` called in `sensorTask` (Core 0) at line 330, same task as `mpu.update()` — no thread-safety issue.]*

- [x] **Inconsistent recovery file naming** — `attemptSDRecovery` uses `%03d%03d.BIN` while main logs use `DR_LOG_%03d.BIN`. Unify to a single format (e.g., `DR_LOG_%03d_%03d.BIN`) and adjust all scanning/counting logic.
  *[INTENTIONAL: `%03d%03d.BIN` is an O(1) direct-open format for fast SD recovery; scan logic at lines 762–775 handles both naming patterns correctly.]*

- [x] **Consecutive TAG button presses lost** — `tag_event_triggered` is a single flag; rapid presses within 5ms are collapsed into one. Replace with a small queue or counter.
  *[FIXED v2.2: Changed from `volatile bool` to `volatile uint8_t` counter (`tag_event_pending`). Logger increments on each press (capped at 255). Sensor decrements one per frame, so each press generates its own `event_flag=1` frame. No lost presses.]*

- ~~[ ] **`global_frame_counter` reset on new file** — Counter restarts from 0 when creating a new log file, causing duplicate sequence numbers across files. Keep it system-wide monotonic.~~ *(Each file is self-contained with its own header + epoch_ms — not a bug.)*

### Moderate

- ~~[ ] **UI rendering depends on sensor data** — Button handling and OLED refresh are tied to `xQueueReceive`. If the queue stalls, the UI freezes. Move UI tasks outside the receive block.~~ *(Already correct — UI is outside the receive block at L1109-1130. Not a bug.)*

- [x] **`delay()` calls outside RTOS context** — `delay(2000)` in setup and `delay(1000)` in calibration functions are safe but should be converted to `vTaskDelay` where possible for consistency.
  *[PARTIAL: calibration `delay()` calls (×3) replaced with `vTaskDelay()` at lines 1348, 1354, 1393. Setup delays remain as-is (pre-RTOS context).]*

## 2. Redundant / Dead Code

- ~~[ ] **Redundant `global_frame_counter = 0` in setup** — Variable is already zero-initialized at declaration; the explicit reset is unnecessary.~~ *(Harmless — one dead store. No runtime impact.)*

- ~~[ ] **`error_handled` flag simplification** — The boolean flag and its branching logic can be simplified into a direct state check.~~ *(Works correctly as-is. Cosmetic only.)*

## 3. Stability & Reliability Risks

- ~~[ ] **No internal watchdog configured** — No `esp_task_wdt_init` or task-level watchdog reset. A stuck task can hang the system silently.~~ *(All loops have explicit vTaskDelay yields — starvation unlikely. Feature request.)*

- [x] **File metadata header** — No header stores firmware version, sampling rate, calibration version, or session info at the start of log files.
  *[FIXED in v2.1: 16-byte `FileHeader` struct (magic `0xDEADC0DE`, version, frame_size, SAMPLING_RATE_HZ, epoch_ms) written at boot / new file / format. Python parser auto-detects and skips legacy files. Also fixed FRAME_SIZE 45→47 bug in parser.]*

- ~~[ ] **Stack overflow not verified** — `loggingTask` stack (8192 bytes) with multiple buffers (`char lines[8][32]`, `snprintf`, u8g2 frames) should be checked with `uxTaskGetStackHighWaterMark()`.~~ *(No evidence of current overflow. Diagnostic feature.)*

- ~~[ ] **SD card busy check** — No `sd.card()->isBusy()` verification before writes; can lead to silent write failures under heavy load.~~ *(SdFat library handles busy-wait internally. Defensive hardening only.)*

- ~~[ ] **Adaptive sampling rate** — No idle slowdown. Sampling continues at 100 Hz even when the device is stationary for extended periods.~~ *(Feature request — not a bug.)*

- ~~[ ] **Actual sampling rate not recorded** — MATLAB assumes a fixed 100 Hz; real timing varies with task scheduling. Measure and store actual inter-frame intervals.~~ *(Feature request — not a bug.)*

## 4. Missing Features & Improvements

- ~~[ ] **Deep sleep mode** — No low-power state after prolonged inactivity. Could save significant battery in field deployments.~~ *(Feature request.)*

- ~~[ ] **Real-time CSV/Serial output** — No optional ASCII streaming for MATLAB live monitoring or debugging.~~ *(Feature request.)*

- ~~[ ] **SD settings file for resume** — No persistent storage of last frame sequence number and file offset for recovery after power loss.~~ *(Feature request.)*

- ~~[ ] **Event logging system** — Limited to gap frames only. No structured log of system events (start, stop, calibration, recovery attempts, errors).~~ *(Feature request.)*

- ~~[ ] **Double buffering** — Single queue architecture; a double-buffer design could improve throughput and reduce write contention.~~ *(Architectural change — not a bug.)*

- ~~[ ] **Task health monitoring** — No heartbeat or status reporting from individual tasks. Stalled tasks go undetected.~~ *(Feature request.)*

- ~~[ ] **`Serial.print` cleanup in production** — Debug serial prints remain in several production paths. Guard with `#ifdef DEBUG` or remove.~~ *(Cleanup task — functional impact is zero.)*

---

## 5. Bug Fixes & Technical Debt (v2.1)

### Critical

- [x] **Configurable sensor task rate** — `SAMPLING_RATE_HZ` was defined but only used for bandwidth math. The task timing was hardcoded at 100 Hz regardless of the define.
  *[FIXED: `vTaskDelayUntil(..., pdMS_TO_TICKS(1000 / SAMPLING_RATE_HZ))` replaces hardcoded 10ms. Compile-time guard (1–1000) added. FIFO rate and DLPF comments updated.]*

- [x] **No gap frames written after calibration** — Calibration block suspended the sensor task, ran calibration, reset the queue, and resumed — without marking the discontinuity with a gap frame (`0xAA`).
  *[FIXED: 0xAA gap frame written after `xQueueReset` and before `vTaskResume` in calibration. Frame sequence stays continuous. `log_time_base` was already not reset (correct).]*

### Moderate

- [x] **Red error light stays on after recovery** — The red LED was not explicitly cleared in all recovery paths; it could remain HIGH after a successful SD or MPU recovery.
  *[FIXED: `digitalWrite(LED_RED_PIN, LOW)` called explicitly at L468 (SD recovery) and L522 (MPU recovery). Frame-send path (L361-362) also clears it conditionally.]*

- [x] **`sd.card()` null-check missing** — `sd.card()->errorCode()` dereferences without checking for null. If the SD card disconnects during SD Info menu, this causes a hard fault.
  *[FIXED: Added `sd.card() && sd.vol() &&` guard before dereference at L784. Falls safely to existing else block that zeros stats.]*

- [x] **EEPROM CRC overwrites magScaleZ** — The CRC-16 checksum over calibration data was stored at an address that overlapped with the `magScaleZ` float value, corrupting calibration on every save.
  *[FIXED: CRC stored at addr 50, well past all 12 calibration floats (addr 2–49). No overlap. CRC computation correctly covers bytes 0–49.]*

- [x] **DLPF = 5 Hz wrong for PDR** — The gyroscope DLPF was set to 5 Hz, which is too aggressive for pedestrian dead reckoning and introduces phase lag.
  *[FIXED: Gyro DLPF changed to `DLPF_41HZ`. Accelerometer DLPF remains at 5 Hz (intentional for noise reduction on gravity vector).]*

- [x] **WDT risk during format** — The format section deleted files in a tight loop without yielding, risking a watchdog timeout reset on some ESP32-S3 configurations.
  *[FIXED: `vTaskDelay(pdMS_TO_TICKS(2))` in inner loop and `vTaskDelay(pdMS_TO_TICKS(3))` in outer loop added to feed WDT. Further improvement noted: replace with `taskYIELD()` + directory listing.]*

### Low / Informational

- [x] **SPI pins conflict with PSRAM** — GPIO 11/12/13 (FSPI) are shared with Quad PSRAM on some ESP32-S3 modules, causing bus contention when the SD card uses the same pins.
  *[NOT AN ISSUE on N16R8: This module uses Octal PSRAM, which connects via internal SPI0/1 controller — completely separate from FSPI. No bus sharing or conflict.]*

- [x] **Recursive OLED `u8g2.begin()` calls** — `u8g2.begin()` is called on every wake-from-sleep and menu entry, causing 200-500ms blocking delays.
  *[INTENTIONAL: OLED is non-soldered (hot-plug hazard). `begin()` re-discovers the device via I2C init if physically reconnected. Comments added at L697-699 and L715-717 explaining the rationale.]*

- [x] **Uninitialized stack frame for CRC** — `LogFrame frame;` on the stack may contain stale bytes if the union payload changes to a smaller member in the future.
  *[DEFENSIVE: Changed to `LogFrame frame = {}` at L306 and `LogFrame flushFrame = {}` at L610. Current code has no bug (imu struct fills all 32 union bytes), but zero-init prevents future issues.]*

---

## 6. Bug Fixes & Technical Debt (v2.2)

### Critical

- [x] **GPS lat/lon always zero** — JavaScript sent lat/lon as quoted strings in JSON, causing `body.substring(...)` to receive string literals instead of numeric values. All GPS start/end frames stored lat=0.0, lon=0.0.
  *[FIXED: Added `parseFloat()` in JavaScript `sendGPS()` function before JSON.stringify, so JSON contains unquoted numeric values that the server correctly parses as floats.]*

- [x] **`writeGPSFrame()` ignores alt and time parameters** — The function signature accepted `float alt` and `uint32_t time` but never stored them in the LogFrame GPS union. Altitude and epoch fields were always zero in written GPS frames.
  *[FIXED: Added `gpsFrame.payload.gps.alt = alt` and `gpsFrame.payload.gps.epoch = time` before CRC computation and SD write.]*

### Moderate

- [x] **Time field UTC-only and readonly** — The HTML form used `new Date().toISOString()` which produced UTC time, and the field was marked `readonly` so users could not correct it. This made GPS anchor timestamps mismatch local time by the UTC offset.
  *[FIXED: Changed to local time display using `toLocaleString()`, removed `readonly` attribute, and updated `sendGPS()` parser to read the field value (allowing user edits).]*

- [x] **SdFat / ESP32 FS.h File class collision** — Including `<WebServer.h>` pulled in ESP32 `<FS.h>` which defines `File` as a class, conflicting with SdFat's `File` typedef. The compiler error prevented building with WiFi GPS features.
  *[FIXED: Added `#define File SdFat_File_` before WebServer include and `#undef File` after, renaming SdFat's File to avoid the collision without changing all usage sites.]*

- [x] **OLED sleep disables GPS state display** — The OLED auto-off timer could put the display to sleep during GPS WAITING state, leaving the user without visible SSID/IP information needed to connect their phone.
  *[FIXED: Auto-sleep skipped when `current_state` is STATE_GPS_WAITING, STATE_GPS_PROMPT_START, STATE_GPS_PROMPT_END, or STATE_GPS_CONFIRM_EXIT.]*

- [x] **GPS data stale after CONFIRM_EXIT or PROMPT_START NO** — If the user exited the GPS prompt without providing data, `phone_gps.has_start` / `has_end` remained true, causing stale GPS data to be written on the next file.
  *[FIXED: `phone_gps.has_start = false` set in PROMPT_START NO branch and CONFIRM_EXIT YES branch to clear stale state.]*
