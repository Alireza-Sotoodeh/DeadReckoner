# Issues found by AI (ESP32-S3 DeadReckoner Code Review Report)

---

## deep seek

### 1. Logical Errors (Bugs)

#### Critical (Crash or complete malfunction)

- **Local variables declared inside infinite loop tasks** – Variables like `last_mpu_data_time` and `is_muted` are re-initialized each loop iteration, resetting timers and state every few ms.  
  *Fix: Move them before `for(;;)` or use `static`.*

- **Concurrent access to MPU object from two cores** – `loggingTask` calls `mpu.getTemperature()` while `sensorTask` calls `mpu.update()`; library is not thread-safe.  
  *Fix: Read temperature inside `sensorTask` and pass via queue, or add a mutex.*

- **Inconsistent file naming for recovery logs** – `attemptSDRecovery` uses `%03d%03d.BIN` but main logs use `DR_LOG_%03d.BIN`, breaking scanning and file counting.  
  *Fix: Use unified format like `DR_LOG_%03d_%03d.BIN`.*

#### Moderate (Unexpected behavior)

- **`delay()` used inside FreeRTOS task** – Blocks `loggingTask` for 1 second, missing button events and frame processing.  
  *Fix: Replace with `vTaskDelay(pdMS_TO_TICKS(1000))`.*

- **OLED auto-sleep check placed inside queue receive block** – If no frame arrives, sleep never triggers even if timeout passes.  
  *Fix: Move the check outside the `xQueueReceive` condition.*

- **Consecutive TAG button presses lost** – `tag_event_triggered` is a single flag; rapid presses within 5ms result in single tag.  
  *Fix: Use a small queue or counter for tags.*

- **`global_frame_counter` reset when creating new file** – Frame numbers restart from 0, causing duplicate sequence numbers in MATLAB analysis.  
  *Fix: Do not reset counter; keep it system-wide monotonic.*

### 2. Redundant Lines / Variables

| Item                               | Reason to remove                                                          |
| ---------------------------------- | ------------------------------------------------------------------------- |
| `lastPrintMillis`                  | Never used anywhere in code.                                              |
| `#define update_rate_oled 1500`    | Defined but never used; code uses literal `1500` instead.                 |
| `font_10_pixel`, `font_5_pixel`    | Defined but only `font_8_pixel` is set; can be removed or unified.        |
| `#define MPU9250_IMU_ADDRESS 0x68` | Macro defined but later code uses `0x68` directly; use the macro instead. |
| Duplicate `Wire.setTimeout(50)`    | One in `setup` is enough; `Wire.begin()` in recovery resets it anyway.    |

### 3. Potential Hardware/Software Stability Issues

| Issue                                                       | Description                                                                                                 |
| ----------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------- |
| **Watchdog Timer**                                          | `delay(1000)` in `loggingTask` can trigger watchdog reset if task blocks too long.                          |
| **PSRAM missing or disabled**                               | If PSRAM not available, `heap_caps_malloc` fails and system halts in infinite loop.                         |
| **FAT filesystem corruption**                               | Sudden power loss during SD write may corrupt files; a large capacitor on power rail helps.                 |
| **I2C bus noise**                                           | MPU9250 and OLED use separate I2C buses; lack of external pull-ups may cause communication errors.          |
| **SPI vs software I2C interference**                        | High-speed SD SPI (20MHz) may disturb bit-bang I2C for OLED during heavy writes.                            |
| **`last_interaction_millis` wrongly updated on long press** | During SELECT hold, `last_interaction_millis` refreshes every loop, preventing 3-second shutdown detection. |

### 4. Ideas for Improved Stability & Performance

1. Move all state variables in tasks outside the infinite loop (fixes critical bug #1).  
2. Add internal watchdog with `esp_task_wdt_init` and reset it in each task.  
3. Protect MPU access with a mutex or read temperature inside `sensorTask`.  
4. Unify log file naming format to `DR_LOG_%03d_%03d.BIN` and adjust recovery scanning.  
5. Replace all `delay()` with `vTaskDelay()` and restructure blocking UI paths.  
6. Implement deep sleep after prolonged inactivity to save battery.  
7. Check `sd.card()->isBusy()` before each write and react faster to sync failures.  
8. Add real-time CSV output over Serial for MATLAB live monitoring.  
9. Reduce MPU sampling rate to 10Hz after 30 seconds of no movement.  
10. Store last frame sequence number and file offset in a settings file on SD for resume after power loss.

### 5. Final Score (out of 10)

> **Current score: 6.5/10**  
> *Reason:* Advanced architecture (RTOS, PSRAM, recovery, UI) is excellent, but critical bugs (task-local variables, MPU race condition, file naming inconsistency) severely impact reliability.

> **After fixing critical bugs (#1 to #3): 8.5/10**

---

## Chat GPT

### Critical Issues

#### 1. MPU/I2C Race Condition

Both `sensorTask()` and `loggingTask()` access the MPU object from different CPU cores without synchronization. This may cause corrupted sensor readings, I2C lockups, or random system instability.

#### 2. Missing Real Timestamp

The system assumes a fixed 100 Hz sampling rate using `frame_seq`, but actual timing varies due to task scheduling and I2C latency. This can significantly reduce the accuracy of MATLAB-based analysis.

#### 3. Silent Queue Overflow

The return value of `xQueueSend()` is not checked. When the queue becomes full, samples may be dropped without any indication or error reporting.

#### 4. Incomplete SD Recovery Marker

A gap frame is generated during SD recovery, but the corresponding event flag is not assigned. As a result, data gaps may not be correctly detected during post-processing.

### Major Issues

#### 5. Calibration Data Not Validated

Calibration parameters are loaded directly from EEPROM without checking validity, version, or integrity. Corrupted EEPROM data may produce incorrect sensor outputs.

#### 6. UI Logic Depends on Sensor Data

Button handling and UI updates are tied to queue reception. If sensor updates stop temporarily, the user interface may become unresponsive.

#### 7. Non-Graceful Shutdown

The queue is flushed during shutdown, but the sensor task may still generate new samples. This can lead to the loss of final data frames.

#### 8. Incorrect File Counting

The SD information menu only counts files matching a specific naming pattern. Recovery fragments may be excluded from statistics.

#### 9. Excessive SD Recovery Attempts

Repeated SD failures can continuously trigger recovery routines, causing performance degradation and excessive file operations.

#### 10. Missing Data Integrity Check

Logged frames do not contain a CRC or checksum. Corrupted records cannot be detected during offline analysis.

### Minor Issues

#### 11. Unused State (`STATE_SUBMENU_MSG`)

A menu state is defined and rendered but is never entered. This increases code complexity without providing functionality.

#### 12. Unused Variables

Variables such as `lastPrintMillis` are declared but never used and can be safely removed.

#### 13. Frequent SD File Scanning

Repeated use of file existence checks may reduce UI responsiveness and increase SD card access overhead.

#### 14. Complex File Naming Scheme

The current file naming structure complicates maintenance, indexing, and recovery operations.

#### 15. No Dedicated Task Health Monitoring

The firmware lacks task-level heartbeat or watchdog monitoring, making it harder to detect stalled tasks.

#### 16. Extremely Large Queue

A queue size of 50,000 frames improves buffering but increases memory usage and delays overflow detection.

#### 17. Limited Runtime Diagnostics

No statistics are maintained for dropped frames, recovery attempts, MPU failures, or SD failures.

### Recommended Improvements

#### 18. Add File Metadata Header

Store firmware version, sampling rate, calibration version, and session information at the beginning of each log file.

#### 19. Read Temperature in Sensor Task

Acquire MPU temperature together with motion data to eliminate cross-core access to the sensor object.

#### 20. Implement Event Logging

Store important system events such as start, stop, SD recovery, and sensor errors inside the log file.

#### 21. Use Double Buffering

A double-buffer architecture can improve throughput and reduce dependency on queue performance during SD writes.

#### 22. Monitor Actual Sampling Rate

Measure and record the real sampling frequency to simplify validation and improve offline analysis accuracy.

### Overall Assessment

| Category                 | Count |
| ------------------------ | ----- |
| Critical Issues          | 4     |
| Major Issues             | 6     |
| Minor Issues             | 7     |
| Recommended Improvements | 5     |

**Overall Score:** **6.5 / 10**

The software architecture is well organized and demonstrates good use of FreeRTOS, PSRAM buffering, and fault recovery mechanisms. However, several reliability and data-integrity issues should be addressed before considering the system production-ready for long-duration data logging applications.

---

## claude

**Project:** DeadReckoner v1.9 | **Score: 7.0 / 10**

### 1. Logical Errors

#### 🔴 Critical

| #   | Location            | Issue                                                    | Description                                                                                                                                                  |
| --- | ------------------- | -------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| L1  | Lines 907–996       | **OLED & Power Management inside `xQueueReceive` block** | Phase 4 rendering and sleep timeout checks only execute when a new frame arrives. If the queue is momentarily empty, the OLED freezes and never sleeps.      |
| L2  | Lines 382–387       | **`gapFrame.event_flag` never assigned**                 | The SD gap marker frame is written with an uninitialized `event_flag`. MATLAB will receive a garbage byte instead of the expected `0xAA` sentinel.           |
| L3  | Lines 283, 807, 871 | **Race condition on `global_frame_counter`**             | `volatile` does not guarantee atomicity on dual-core ESP32-S3. The counter is incremented on Core 0 and reset on Core 1 without a mutex or critical section. |
| L4  | Line 936            | **`mpu.getTemperature()` called from wrong core**        | The `mpu` object is updated by `sensorTask` (Core 0) but `getTemperature()` is called from `loggingTask` (Core 1). The MPU9250 library is not thread-safe.   |

#### 🟡 Medium

| #   | Location            | Issue                                        | Description                                                                                                                                                 |
| --- | ------------------- | -------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------- |
| L5  | `loadCalibration()` | **EEPROM read without validation**           | On a new device, EEPROM contains random bytes that are applied directly to the MPU, potentially producing NaN/Inf offsets and crashing the Madgwick filter. |
| L6  | Lines 657–665       | **Incomplete SD file count**                 | Only `%03d001.BIN` recovery fragments are checked; fragments `002`–`999` are ignored, causing the reported file count to be wrong.                          |
| L7  | Line 449            | **`logFile` not checked after MPU recovery** | After MPU reconnection, `sd.open()` is called but the return value is never validated, so a failed re-open goes silently undetected.                        |

---

### 2. Redundant / Removable Code

| #   | Location                           | Item                                     | Reason                                                                                    |
| --- | ---------------------------------- | ---------------------------------------- | ----------------------------------------------------------------------------------------- |
| R1  | Line 260 (`sensorTask`)            | `unsigned long last_sd_recovery_attempt` | Declared but never used; SD recovery is handled entirely inside `loggingTask`.            |
| R2  | `setup()` + `attemptMPURecovery()` | Duplicate `MPU9250Setting` blocks        | Identical configuration repeated in two places; extract into a single `initMPU()` helper. |
| R3  | Line 1080 (`setup()`)              | `global_frame_counter = 0`               | Redundant — the variable is already zero-initialized at declaration (line 171).           |

---

### 3. Stability Risks

| #   | Severity  | Risk                                      | Description                                                                                                                                              |
| --- | --------- | ----------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------- |
| S1  | 🔴 High   | **SD scan UI freeze**                     | Worst case: 999 × 2 = 1,998 calls to `sd.exists()` in the SD Info submenu. Each call can take several ms, blocking the UI for multiple seconds.          |
| S2  | 🟡 Medium | **Stack overflow in `loggingTask`**       | 8,192-byte stack with heavy workload: `char lines[7][32]`, multiple `snprintf` buffers, and u8g2 frame ops. Verify with `uxTaskGetStackHighWaterMark()`. |
| S3  | 🟡 Medium | **`tag_event_triggered` flag race**       | Set in `loggingTask` and cleared in `sensorTask` across two cores using only `volatile`. Should be replaced with a binary semaphore.                     |
| S4  | 🟡 Medium | **SPI bus re-init during recovery**       | `SPI.end()` / `SPI.begin()` in `attemptSDRecovery()` will disrupt any other SPI peripheral sharing the bus.                                              |
| S5  | 🟢 Low    | **`delay(1000)` in `loggingTask` branch** | Although `delay()` maps to `vTaskDelay()` on ESP32, explicitly using `vTaskDelay(pdMS_TO_TICKS(1000))` is clearer and avoids ambiguity in future ports.  |

---

### 4. Improvement Ideas

| #   | Priority  | Idea                                                                                                                                                                          |
| --- | --------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| I1  | 🔴 High   | **Add per-frame timestamp** — store `esp_timer_get_time()` in `LogFrame`. Assuming a fixed 100 Hz in MATLAB is fragile; timestamps make analysis robust.                      |
| I2  | 🔴 High   | **EEPROM magic number** — write a fixed sentinel (e.g., `0xDEAD`) to a reserved address and check it before applying loaded calibration values.                               |
| I3  | 🟡 Medium | **Protect shared state with `portMUX_TYPE`** — replace `volatile` alone with `portENTER_CRITICAL` / `portEXIT_CRITICAL` for `global_frame_counter` and `tag_event_triggered`. |
| I4  | 🟡 Medium | **Replace SD scan loop with `openNext()`** — iterate the directory using SdFat's `openNext()` instead of probing 1,998 filenames; 10–100× faster.                             |
| I5  | 🟢 Low    | **CRC16 per frame** — add a 2-byte checksum to `LogFrame` so MATLAB can detect and discard corrupt frames rather than processing bad data silently.                           |

---

### Score Breakdown

| Category                       | Score        |
| ------------------------------ | ------------ |
| Architecture & FreeRTOS design | 9/10         |
| Code clarity & comments        | 8/10         |
| Data integrity                 | 5/10         |
| Thread safety                  | 4/10         |
| Error handling                 | 7/10         |
| **Overall**                    | **7.0 / 10** |

> Fixing **L1–L4** (critical logical errors) alone would bring this to approximately **8.5/10**.

---

## Grok

**DeadReckoner v1.9 Code Review**

### 1. Critical Logical Errors

- **Shutdown Protocol Bug**  
  `system_shutdown_requested` stops sensor task but logging task continues reading from queue and may write after `logFile.close()`. Flush logic can cause corruption or crash.

- **SD Recovery Issue**  
  `global_recovery_id++` happens before confirming successful file open. Failed recovery increments ID without creating file → broken numbering.

- **MPU Recovery Weakness**  
  Re-init is incomplete; `mpu.update()` may still fail after recovery. EEPROM load during recovery can crash if corrupted.

- **UI State Machine Gaps**  
  `STATE_SUBMENU_MSG` is dead code. File creation/format does not fully synchronize with active logging.

### 2. Redundant / Removable Code

- `STATE_SUBMENU_MSG` and `subMenuMsg` array (unused).  
- `last_sd_recovery_attempt` in sensorTask (defined but never used).  
- Excessive long comments inside functions.  
- Redundant `error_handled` flag logic (can be simplified).  
- Some `Serial.print` calls in production paths.

### 3. Potential Stability & Runtime Problems

- **Watchdog Risk**  
  Long queue flush during shutdown (up to 50k frames) may trigger task watchdog if yield is insufficient.

- **PSRAM & Memory**  
  System halts if PSRAM allocation fails. Stack size for sensorTask (4096) is tight for heavy MPU library usage.

- **Hardware Instability**  
  Runtime `Wire.end()/begin()` and hot SD card removal at 20MHz FSPI can cause I2C bus hangs or filesystem corruption.

- **Power & Timing**  
  High power draw from dual-core + software I2C OLED. Button debounce and interaction timing can overlap.

### 4. Improvement Ideas

- **Immediate Fixes**  
  Use EventGroup or notification for inter-task communication. Move UI rendering to a separate task. Add file header with version & calibration.

- **Stability Enhancements**  
  Enable ESP task watchdog with periodic reset. Add CRC to LogFrame. Implement circular buffer instead of simple queue.

- **Architecture Upgrade**  
  Split into 3 tasks: Sensor (Core 0), Logger (Core 1), UI (Core 1 or pinned). Add low-power mode when OLED sleeps.

- **Long-term**  
  Add binary log header, optional CSV export, and EEPROM checksum.

### 5. Overall Score: **7.4 / 10**

**Strengths**: Excellent recovery design, smart union + PSRAM usage, feature-rich UI on tiny OLED.  
**Weaknesses**: Overly complex single-task UI+logging, critical shutdown bugs, maintainability concerns.  

Clean the shutdown and recovery bugs first, then refactor to multi-task architecture to reach **8.5+**.
