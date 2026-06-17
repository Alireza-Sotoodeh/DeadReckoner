# Issues found by AI — Remaining Action Items

> All completed issues have been removed. Only items NOT yet addressed in the codebase are listed below.

---

## 1. Logical Errors & Bugs

### Critical

- [ ] **MPU temperature read from wrong core** — `mpu.getTemperature()` is called in `loggingTask` (Core 1) while `mpu.update()` runs in `sensorTask` (Core 0). The MPU9250 library is not thread-safe. Move temperature read to `sensorTask` and pass via queue.

- [ ] **Inconsistent recovery file naming** — `attemptSDRecovery` uses `%03d%03d.BIN` while main logs use `DR_LOG_%03d.BIN`. Unify to a single format (e.g., `DR_LOG_%03d_%03d.BIN`) and adjust all scanning/counting logic.

- [ ] **Consecutive TAG button presses lost** — `tag_event_triggered` is a single flag; rapid presses within 5ms are collapsed into one. Replace with a small queue or counter.

- [ ] **`global_frame_counter` reset on new file** — Counter restarts from 0 when creating a new log file, causing duplicate sequence numbers across files. Keep it system-wide monotonic.

### Moderate

- [ ] **UI rendering depends on sensor data** — Button handling and OLED refresh are tied to `xQueueReceive`. If the queue stalls, the UI freezes. Move UI tasks outside the receive block.

- [ ] **`delay()` calls outside RTOS context** — `delay(2000)` in setup and `delay(1000)` in calibration functions are safe but should be converted to `vTaskDelay` where possible for consistency.

## 2. Redundant / Dead Code

- [ ] **Redundant `global_frame_counter = 0` in setup** — Variable is already zero-initialized at declaration; the explicit reset is unnecessary.

- [ ] **`error_handled` flag simplification** — The boolean flag and its branching logic can be simplified into a direct state check.

## 3. Stability & Reliability Risks

- [ ] **No internal watchdog configured** — No `esp_task_wdt_init` or task-level watchdog reset. A stuck task can hang the system silently.

- [ ] **CRC/checksum per frame** — Logged frames have no integrity check. Corrupted records cannot be detected during offline analysis. Add a 2-byte CRC16 to `LogFrame`.

- [ ] **File metadata header** — No header stores firmware version, sampling rate, calibration version, or session info at the start of log files.

- [ ] **Stack overflow not verified** — `loggingTask` stack (8192 bytes) with multiple buffers (`char lines[8][32]`, `snprintf`, u8g2 frames) should be checked with `uxTaskGetStackHighWaterMark()`.

- [ ] **SD card busy check** — No `sd.card()->isBusy()` verification before writes; can lead to silent write failures under heavy load.

- [ ] **Adaptive sampling rate** — No idle slowdown. Sampling continues at 100 Hz even when the device is stationary for extended periods.

- [ ] **Actual sampling rate not recorded** — MATLAB assumes a fixed 100 Hz; real timing varies with task scheduling. Measure and store actual inter-frame intervals.

## 4. Missing Features & Improvements

- [ ] **Deep sleep mode** — No low-power state after prolonged inactivity. Could save significant battery in field deployments.

- [ ] **Real-time CSV/Serial output** — No optional ASCII streaming for MATLAB live monitoring or debugging.

- [ ] **SD settings file for resume** — No persistent storage of last frame sequence number and file offset for recovery after power loss.

- [ ] **Event logging system** — Limited to gap frames only. No structured log of system events (start, stop, calibration, recovery attempts, errors).

- [ ] **Double buffering** — Single queue architecture; a double-buffer design could improve throughput and reduce write contention.

- [ ] **Task health monitoring** — No heartbeat or status reporting from individual tasks. Stalled tasks go undetected.

- [ ] **`Serial.print` cleanup in production** — Debug serial prints remain in several production paths. Guard with `#ifdef DEBUG` or remove.
