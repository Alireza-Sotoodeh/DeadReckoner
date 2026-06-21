# DeadReckoner PDR Analysis — Methods & Fixes

## Overview

Two Python tools analyze PDR (Pedestrian Dead Reckoning) accuracy against Geo Tracker GPS reference across 5 walk segments collected with an ESP32-S3 IMU logger.

---

## Tool 1: `compare_paths.py`

### BIN File Parser

- Parses 47-byte fixed-length frames
- Validates FileHeader magic (`0xDEADC0DE`), version, frame size, sample rate, epoch timestamp
- CRC-16 validation per frame (poly `0xA001`, init `0xFFFF`, **no final XOR** — matching firmware `calcCRC16`)
- Detects frame types by `event_flag`:
  - `0x00` = IMU frame (accel + gyro + quaternion in 32-byte union payload)
  - `0xBB` = GPS start anchor (lat/lon/alt/date/time in GPS union payload)
  - `0xCC` = GPS end anchor
- Auto-stitches recovery fragment files (`001001.BIN` → appended to `DR_LOG_001`)
- Converts quaternion `(w, x, y, z)` to Euler yaw via:
  `yaw = atan2(2*(w*z + x*y), 1 - 2*(y² + z²))`

### PDR Pipeline

1. **Step Detection**: `scipy.signal.find_peaks` on acceleration magnitude
   
   - `height=1.5` (min accel peak amplitude)
   - `distance=25` (min samples between peaks ≈ 250ms at 100Hz)
   - `prominence=1.0`
   - Uses **peak-based yaw** (mid-swing phase) for step heading

2. **Step Length**: Weinberg model
   
   - `L = K · (accel_max - accel_min) / (accel_max + accel_min)`
   - `K = 0.425` (optimal, found by `tune_pdr.py`)

3. **Heading**:
   
   - Extracts yaw at each step peak index from quaternion
   - Applies 2D optimization: `heading_offset` + `heading_drift_rate` (deg/step)
   - Drift rate transforms: `yaw_corrected = yaw + offset + drift · step_index`

### GPS-Guided PDR (`--gps-guided`)

Proves PDR step lengths are correct by substituting GPS heading:

- Interpolates GPS heading at PDR step cumulative-distance positions
- Builds path using PDR step lengths + GPS heading
- Result: avg shape error drops from **156m → 3m** vs GPS

### Path Similarity Metrics

- **Avg symmetric distance**: KDTree nearest-neighbor distance (PDR→GPS, sampled every 5th step)
- **Hausdorff distance**: maximum nearest-neighbor distance (worst deviation)
- **Endpoint error**: Euclidean distance between PDR and GPS final positions (after 2D heading+drift optimization)

---

## Tool 2: `tune_pdr.py`

### Purpose

Brute-force search for optimal Weinberg K parameter across 5 segments.

### Search Space

- `K` = 0.05 to 1.0 (step 0.025)
- `peak_min_height` = 1.5 to 4.0 (step 0.5)
- `peak_min_distance` = 15 to 35 (step 5)

### Global Optimum Found

| Parameter         | Value      |
| ----------------- | ---------- |
| Weinberg K        | 0.425      |
| Peak min height   | 1.5 m/s²   |
| Peak min distance | 25 samples |
| Avg error         | 4.9%       |
| Total error       | 3.3%       |

### Bug Fixed

- `ref_len = compute_gps_distance(geo_x, geo_y)` was receiving **local xy coordinates** (meters) instead of **lat/lon** (degrees)
- `compute_gps_distance` uses `haversine(geography)` which requires lat/lon in degrees
- Local xy values (~84–1984) were interpreted as degrees at equator, producing ~9000–220,000 km distances
- Result: every K value showed ~100% error, making tuning impossible
- **Fix**: changed to `ref_len = compute_gps_distance(geo['lat'], geo['lon'])`

---

## Fixed Issues

### 1. CRC-16 Mismatch (Original compare_paths.py)

- Original script used `binascii.crc_hqx` with final XOR `0x0000`
- Firmware `calcCRC16` has **no final XOR**
- After matching firmware's pure `0xA001` poly, `init=0xFFFF`, no XOR: **0% CRC failure** across all ~568K frames

### 2. BIN Parser Wrong Frame Layout

- Original assumed 46-byte frames with 4-byte headers per sub-frame
- Actual format: 16-byte FileHeader + 47-byte LogFrame
- LogFrame: `frame_seq[0:4]`, `timestamp[4:12]`, `event_flag[12]`, `payload[13:45]`, `crc[45:47]`
- Quaternion stored in IMU union at offset 16 within payload (total payload offset from frame start: 29)

### 3. tune_pdr.py 100% Error Bug

- `compute_gps_distance` was called with local xy instead of lat/lon
- Fixed by passing `geo['lat'], geo['lon']` arrays

### 4. Duplicate FileHeader in compare_paths.py

- FileHeader (16 bytes at offset 0) was being parsed, then frame reading started at offset 0 again
- Fixed: start frame reading at offset 16 (after FileHeader)

---

## Root Cause: Shape Mismatch

### Verified Finding

PDR **step length accuracy is <5%** (Weinberg K=0.425 well-calibrated). The path shape divergence is **entirely a heading problem**:

| Metric             | Standard PDR       | GPS-Guided PDR |
| ------------------ | ------------------ | -------------- |
| Avg shape distance | 156m               | **3m**         |
| Endpoint error     | ~1m (after 2D opt) | ~20m           |
| Hausdorff distance | 463m               | **12m**        |

### Diagnosis

- The MPU9250 library's Madgwick filter **incorporates magnetometer data into its gradient descent step** at full `beta` strength
- `beta = sqrt(3/4) * 40°/s ≈ 0.605` — this controls ALL correction (accel + mag)
- `zeta = 0.0` (GyroMeasDrift = 0°/s/s) — **zeta is unrelated to the issue**
- The Madgwick magnetometer correction pulls the quaternion yaw to align with Earth's magnetic field direction
- Yaw drift rate: **0.04–0.10°/min** (unrealistically low; confirms magnetic locking)
- GPS heading changes by ~180° (loops), but IMU yaw changes only 44–114°
- GPS-heading direct test: using PDR step lengths + GPS heading → avg shape distance **3m** vs GPS

### Firmware Fix

**Switch from `MADGWICK` to `MAHONY` filter** (one-line change in `ESP32_S3.ino`):

```c
// Before (line 114):
#define MPU9250_filter_algorithm    MADGWICK

// After:
#define MPU9250_filter_algorithm    MAHONY
```

**Why Mahony works**: The Mahony filter in this library (`QuaternionFilter.h`) **ignores magnetometer data entirely**. It only uses accelerometer to correct pitch/roll, while yaw comes from pure gyroscope integration. This means:

- Every body turn is tracked correctly (no magnetic locking)
- Pitch/roll stay drift-free (corrected by gravity reference)
- Yaw drifts linearly from gyro bias (~1-5°/min), but the 2D heading+drift optimization in `compare_paths.py` is **already designed to handle exactly this** (reduces endpoint error to ~0m)

**Fallback**: If Mahony's Kp=30 proves too aggressive for foot-strike accelerations, switch to `NONE` (pure gyro integration for everything). Step detection uses raw accelerometer peaks (Weinberg), not quaternion-corrected values, so pitch/roll noise doesn't affect PDR.

---

## Final Solution Summary

| #   | Problem                               | Fix                                                                       | Status      |
| --- | ------------------------------------- | ------------------------------------------------------------------------- | ----------- |
| 1   | `compare_paths.py` wrong frame parser | Rewritten with 47-byte Layout, correct magic, proper CRC                  | ✅ Done      |
| 2   | CRC-16 mismatch                       | Switched to firmware's `0xA001` poly, no final XOR                        | ✅ Done      |
| 3   | `tune_pdr.py` 100% error              | Pass lat/lon to `compute_gps_distance` instead of xy                      | ✅ Done      |
| 4   | Optimal K unknown                     | Brute-force search found K=0.425, height=1.5, dist=25                     | ✅ Done      |
| 5   | Shape mismatch diagnosed              | Madgwick uses magnetometer in gradient descent at full `beta` — locks yaw | ✅ Diagnosed |
| 6   | No way to verify step lengths         | Added `--gps-guided` mode proving PDR steps are correct                   | ✅ Done      |
| 7   | Path shape still diverges             | **Switch `MADGWICK` → `MAHONY`** in `ESP32_S3.ino:114`                    | 🔧 Pending  |

**Bottom line**: The PDR distance model is verified accurate (avg 4.9% error). The path shape mismatch is caused by the Madgwick filter incorporating magnetometer data into yaw correction, locking heading to magnetic North. **Fix**: switch to Mahony filter (ignores magnetometer for yaw, uses pure gyro integration). The 2D heading+drift optimization in `compare_paths.py` already handles the residual linear yaw drift from gyro bias.
