Based on the results, here's my analysis of what's causing the higher error (21.8% vs 9.3% last time) and what can be done:

## Root Cause Analysis

### 1. Different walking session characteristics

The previous test had walks averaging ~500m each. This session has longer walks (1170–1984m each). The Weinberg K=0.4 was tuned for the shorter walks — the step length formula may need recalibration per session.

### 2. The biggest clue: segment 1 is better (12%) than 2-5 (23-25%)

Segment 1 has a recovery fragment stitched in — but it also shows **accel mean=13.5 m/s²** vs segments 2-5 at **~4.5 m/s²**. This is suspicious — segments 2-5 have unusually low linear acceleration magnitude for walking. This could mean:

- **IMU calibration changed** between segments (e.g., sensor recalibrated mid-walk)
- **Gravity compensation** in the Madgwick filter was inconsistent across segments
- The signal-to-noise ratio is worse on segments 2-5, making step detection harder

### 3. Corrupted GPS anchors in BIN

Segments 3 and 4 have **lat=22.47** (should be ~29.47 for your city). This is a manual phone-entry error. These are the BIN anchors (0xBB/0xCC frames), not the GPX reference — they don't affect PDR accuracy comparison but indicate phone GPS entry mistakes.

### 4. Missing altitude (alt=0) in all anchors

The web form alt field was left blank.

## Recommendations

| #   | Fix                                                                                                                                                                                                    | Effort    | Impact |
| --- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | --------- | ------ |
| 1   | **Run the interactive mode** (`python compare_paths.py` without `--batch`) — adjust the Weinberg K slider and see if a different K value brings error down to ~10%                                     | Immediate | High   |
| 2   | **Add per-user step calibration** — measure your actual step length and compute the optimal K using the formula: `K = actual_step_length / (peak_accel - valley_accel)^0.25`                           | Easy      | High   |
| 3   | **Investigate accel magnitude discrepancy** — plot accel data for all segments side-by-side to see if segments 2-5 have systematically lower signal (could indicate a calibration issue between walks) | Medium    | High   |
| 4   | **Tune peak detection per segment** — the tool's `analyze_step_detection()` already tests 24 threshold combos. Use the optimal params per segment rather than one-size-fits-all                        | Medium    | Medium |
| 5   | **Add GPS anchor validation** — firmware should reject implausible lat/lon (e.g., outside 20-40°N, 50-65°E for your city) and ask the user to re-enter                                                 | Easy      | Low    |
| 6   | **Implement ZUPT + RTS smoother** — the bidirectional Kalman smoother (the one uncompleted item in Phase 10) would correct drift mid-walk rather than just at endpoints                                | Hard      | High   |
| 7   | **Enter altitude in web form** — gives better 3D constraint for fusion                                                                                                                                 | Trivial   | Low    |

**Quickest win:** Run the interactive mode and tune K. If that brings error to ~10%, the firmware is fine — it's just a parameter calibration issue, not a firmware bug. If not, let's investigate the accel signal segments 2-5 more deeply.


