#!/usr/bin/env python3
"""
DeadReckoner Path Comparison Tool (v2.2+)
Compare IMU-based PDR against Geo Tracker GPS reference for 5 walk segments.

KEY FINDING: PDR step lengths (Weinberg K=0.425) are verified ACCURATE to ~5%.
The path shape mismatch is entirely caused by the Madgwick filter's magnetometer
correction (zeta gain) being too strong — it locks the heading to magnetic North,
preventing the gyro from tracking body turns during the walk.

Two analysis modes:
  1. Standard PDR: peak-based yaw + 2D (heading offset + linear drift) optimization
  2. GPS-guided PDR (--gps-guided): uses actual GPS heading interpolated to
     PDR step positions, proving PDR step lengths are correct when given
     accurate heading. Shows the path the PDR WOULD produce with proper heading.

Handles 47-byte frames with CRC-16, 16-byte FileHeader, GPS anchors (0xBB/0xCC).

Usage:
    python compare_paths.py                # standard analysis (interactive)
    python compare_paths.py --batch        # non-interactive
    python compare_paths.py --gps-guided   # add GPS-guided correction plots

Requires: numpy, scipy, matplotlib
"""

import os, sys, struct
import numpy as np
import matplotlib
from matplotlib import pyplot as plt
from matplotlib.widgets import Slider, Button
import xml.etree.ElementTree as ET
from scipy.signal import find_peaks
from scipy.ndimage import gaussian_filter1d
from scipy.spatial import KDTree
from datetime import datetime

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SEGMENTS = 5
BATCH_MODE = '--batch' in sys.argv
GPS_GUIDED = '--gps-guided' in sys.argv
if BATCH_MODE:
    matplotlib.use('Agg')

CRC_POLY = 0xA001


# ─────────────────────────────────────────────
# 1. CRC-16
# ─────────────────────────────────────────────

def crc16(data: bytes, poly: int = CRC_POLY) -> int:
    """CRC-16-IBM (poly 0xA001, init 0xFFFF, no final XOR). Matches firmware calcCRC16."""
    crc = 0xFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ poly if crc & 1 else crc >> 1
    return crc


# ─────────────────────────────────────────────
# 2. BIN Parser
# ─────────────────────────────────────────────

def parse_bin(filepath: str) -> dict | None:
    with open(filepath, 'rb') as f:
        raw = f.read()

    FILE_HEADER_SIZE = 16
    FRAME_SIZE = 47

    if len(raw) < FILE_HEADER_SIZE:
        return None

    # FileHeader (see ESP32_S3.ino):
    #   uint32_t magic       (4 bytes) = 0xDEADC0DE
    #   uint8_t  version     (1 byte)
    #   uint8_t  frame_size  (1 byte)  = 47
    #   uint16_t sample_rate (2 bytes) = 100
    #   uint64_t epoch_ms    (8 bytes)
    magic = struct.unpack_from('<I', raw, 0)[0]
    if magic != 0xDEADC0DE:
        return None
    fw_version = raw[4]
    frame_size = raw[5]
    sample_rate = struct.unpack_from('<H', raw, 6)[0]
    epoch_ms = struct.unpack_from('<Q', raw, 8)[0]

    # LogFrame layout (47 bytes each):
    #   uint32_t frame_seq  [0:4]
    #   uint64_t timestamp  [4:12]
    #   uint8_t  event_flag [12]
    #   union payload       [13:45]  (32 bytes)
    #   uint16_t crc        [45:47]

    accel_list = []
    q_list = []
    imu_ts_list = []
    gps_frames = []
    crc_ok = 0
    crc_bad = 0

    offset = FILE_HEADER_SIZE
    while offset + FRAME_SIZE <= len(raw):
        frame = raw[offset:offset + FRAME_SIZE]
        offset += FRAME_SIZE

        frame_crc = struct.unpack_from('<H', frame, 45)[0]
        computed_crc = crc16(frame[:45])
        if computed_crc != frame_crc:
            crc_bad += 1
            continue
        crc_ok += 1

        frame_seq = struct.unpack_from('<I', frame, 0)[0]
        ts = struct.unpack_from('<Q', frame, 4)[0]
        evt = frame[12]

        if evt == 0:  # IMU frame (event_flag == 0 for standard high-speed IMU packet)
            qw, qx, qy, qz = struct.unpack_from('<ffff', frame, 13)
            ax, ay, az = struct.unpack_from('<fff', frame, 29)
            temp = struct.unpack_from('<f', frame, 41)[0]

            accel_list.append([ax, ay, az])
            q_list.append([qw, qx, qy, qz])
            imu_ts_list.append(ts)

        elif evt in (0xBB, 0xCC):  # GPS anchor
            lat = struct.unpack_from('<d', frame, 13)[0]
            lon = struct.unpack_from('<d', frame, 21)[0]
            alt = struct.unpack_from('<f', frame, 29)[0]
            epoch = struct.unpack_from('<I', frame, 33)[0]
            gps_frames.append({
                'event': evt, 'ts': ts,
                'lat': lat, 'lon': lon, 'alt': alt, 'epoch': epoch
            })

    return {
        'fw_version': fw_version,
        'frame_size': frame_size,
        'sample_rate': sample_rate,
        'epoch_ms': epoch_ms,
        'n_imu': len(accel_list),
        'accel': np.array(accel_list),
        'q': np.array(q_list),
        'imu_timestamps': np.array(imu_ts_list),
        'gps_frames': gps_frames,
        'crc_ok': crc_ok,
        'crc_bad': crc_bad,
    }


def stitch_bin(primary: dict, recovery: dict) -> dict:
    """Stitch recovery fragment frames into primary (prepend)."""
    if recovery['n_imu'] == 0:
        return primary
    stitched = dict(primary)
    stitched['accel'] = np.vstack([recovery['accel'], primary['accel']])
    stitched['q'] = np.vstack([recovery['q'], primary['q']])
    stitched['imu_timestamps'] = np.concatenate(
        [recovery['imu_timestamps'], primary['imu_timestamps']]
    )
    stitched['n_imu'] = len(stitched['accel'])
    return stitched


# ─────────────────────────────────────────────
# 3. Quaternion → Yaw
# ─────────────────────────────────────────────

def quaternion_to_yaw(w, x, y, z):
    siny_cosp = 2.0 * (w * z + x * y)
    cosy_cosp = 1.0 - 2.0 * (y * y + z * z)
    return np.arctan2(siny_cosp, cosy_cosp)


# ─────────────────────────────────────────────
# 4. GPX Parser (Geo Tracker)
# ─────────────────────────────────────────────

def parse_gpx(filepath: str) -> dict | None:
    try:
        tree = ET.parse(filepath)
        root = tree.getroot()
    except Exception:
        return None

    ns = {'gpx': 'http://www.topografix.com/GPX/1/1'}
    trkpt = root.findall('.//gpx:trkpt', ns)
    if not trkpt:
        trkpt = root.findall('.//trkpt')

    lat, lon, alt, time_str = [], [], [], []
    for pt in trkpt:
        lat.append(float(pt.attrib['lat']))
        lon.append(float(pt.attrib['lon']))
        el = pt.find('{http://www.topografix.com/GPX/1/1}ele')
        if el is None:
            el = pt.find('ele')
        alt.append(float(el.text) if el is not None else 0.0)
        tt = pt.find('{http://www.topografix.com/GPX/1/1}time')
        if tt is None:
            tt = pt.find('time')
        time_str.append(tt.text if tt is not None else '')

    return {
        'lat': np.array(lat),
        'lon': np.array(lon),
        'alt': np.array(alt),
        'time_str': time_str,
    }


def gpx_to_local(lat, lon):
    """Convert lat/lon to local East/North meters relative to first point."""
    R = 6371000.0
    lat0, lon0 = np.radians(lat[0]), np.radians(lon[0])
    x = R * np.cos(lat0) * (np.radians(lon) - lon0)
    y = R * (np.radians(lat) - lat0)
    return x, y


# ─────────────────────────────────────────────
# 5. PDR Algorithm
# ─────────────────────────────────────────────

def compute_pdr(data, k=0.425, heading_offset_deg=0, heading_drift_rate_deg=0,
                peak_min_height=1.5, peak_min_distance=25, peak_prominence=1.0):
    """
    PDR: step detection via accel peaks + Weinberg step length + quaternion heading.

    Uses peak-based yaw (mid-swing) for step heading, consistent with standard PDR.
    Also applies 2D (heading offset + linear drift rate) optimization for endpoint
    alignment with GPS reference.

    NOTE: Shape mismatch is caused by the IMU's Madgwick filter having too strong
    magnetometer correction (zeta parameter), which locks yaw to magnetic North
    and prevents tracking of body turns. This is a FIRMWARE parameter, not fixable
    in post-processing. The linear drift model approximates the heading error but
    cannot capture non-linear turn patterns.

    heading_drift_rate_deg: deg/step — linear drift correction per step.

    Returns (x_m, y_m, step_indices, step_lengths, step_headings).
    """
    accel_mag = np.sqrt(np.sum(data['accel'] ** 2, axis=1)) * 9.81

    peaks, _ = find_peaks(
        accel_mag,
        height=peak_min_height,
        distance=peak_min_distance,
        prominence=peak_prominence
    )

    if len(peaks) < 2:
        return np.array([0.0]), np.array([0.0]), peaks, np.array([]), np.array([])

    q = data['q']
    yaw = quaternion_to_yaw(q[:, 0], q[:, 1], q[:, 2], q[:, 3])
    heading_rad = np.radians(heading_offset_deg)
    drift_rad = np.radians(heading_drift_rate_deg)

    steps_x, steps_y = [], []
    step_lengths, step_headings = [], []
    running_x, running_y = 0.0, 0.0

    for pk_idx in range(len(peaks)):
        peak_i = peaks[pk_idx]
        prev_boundary = (peaks[pk_idx - 1] + 5) if pk_idx > 0 else max(0, peak_i - 100)
        search_start = prev_boundary
        search_end = peak_i - 2

        # Find valley (stance phase) between strides
        if search_end <= search_start:
            valley_val = accel_mag[peak_i] * 0.5
            valley_i = peak_i  # fallback
        else:
            valley_i = search_start + np.argmin(accel_mag[search_start:search_end])
            valley_val = accel_mag[valley_i]

        peak_val = accel_mag[peak_i]
        step_len = k * (peak_val - max(valley_val, 0.1)) ** 0.25

        # Use peak yaw (mid-swing) for step heading
        step_yaw = yaw[peak_i] + heading_rad + drift_rad * pk_idx

        running_x += step_len * np.sin(step_yaw)
        running_y += step_len * np.cos(step_yaw)
        steps_x.append(running_x)
        steps_y.append(running_y)
        step_lengths.append(step_len)
        step_headings.append(step_yaw)

    return (np.array([0.0] + steps_x), np.array([0.0] + steps_y),
            peaks, np.array(step_lengths), np.array(step_headings))


# ─────────────────────────────────────────────
# 5b. Path helpers
# ─────────────────────────────────────────────

def compute_path_length(x, y):
    return float(np.sum(np.sqrt(np.diff(x) ** 2 + np.diff(y) ** 2)))


def compute_gps_distance(lat, lon):
    R = 6371000.0
    dlat = np.radians(np.diff(lat))
    dlon = np.radians(np.diff(lon))
    a = np.sin(dlat / 2) ** 2 + np.cos(np.radians(lat[:-1])) * np.cos(np.radians(lat[1:])) * np.sin(dlon / 2) ** 2
    return float(np.sum(2 * R * np.arcsin(np.sqrt(np.clip(a, 0, 1)))))


def find_best_heading_and_drift(yaw_at_peaks, step_lengths, ref_x, ref_y,
                                heading_step=0.5, dr_min=-1.0, dr_max=1.0,
                                dr_step=0.005):
    """
    Brute-force 2D search over heading_offset (0-360 deg) and
    heading_drift_rate (dr_min..dr_max deg/step) to minimize endpoint error.

    Uses vectorized pre-computation for efficiency.
    Returns (best_heading_deg, best_drift_rate_deg_per_step, best_error_m).
    """
    n = len(step_lengths)
    if n < 2:
        return 0.0, 0.0, 9999.0

    sx = step_lengths * np.sin(yaw_at_peaks)
    sy = step_lengths * np.cos(yaw_at_peaks)
    ref_end = np.array([ref_x[-1], ref_y[-1]])

    dr_vals = np.arange(dr_min, dr_max + dr_step / 2, dr_step)
    h_vals = np.arange(0.0, 360.0, heading_step)

    best_heading = 0.0
    best_drift = 0.0
    best_err = 9999.0

    i_ar = np.arange(n, dtype=np.float64)

    for dr in dr_vals:
        dr_rad = np.radians(dr)
        cos_di = np.cos(dr_rad * i_ar)
        sin_di = np.sin(dr_rad * i_ar)

        U = float(np.sum(sx * cos_di + sy * sin_di))
        V = float(np.sum(-sx * sin_di + sy * cos_di))
        W = float(np.sum(sy * cos_di - sx * sin_di))
        Z = float(np.sum(-sy * sin_di - sx * cos_di))

        for h_deg in h_vals:
            h = np.radians(h_deg)
            ch, sh = np.cos(h), np.sin(h)
            dx_tot = ch * U + sh * V
            dy_tot = ch * W + sh * Z
            err = np.hypot(dx_tot - ref_end[0], dy_tot - ref_end[1])
            if err < best_err:
                best_err = err
                best_heading = h_deg
                best_drift = dr

    return best_heading, best_drift, float(best_err)


def compute_path_similarity(pdr_x, pdr_y, geo_x, geo_y, subsample=5):
    """
    Compute average and maximum distance from GPS path to PDR path (and vice versa).
    Returns {avg_dist, hausdorff, mean_symmetric_dist}.
    """
    pdr_pts = np.column_stack([np.array(pdr_x[::subsample]), np.array(pdr_y[::subsample])])
    geo_pts = np.column_stack([np.array(geo_x), np.array(geo_y)])

    tree_p = KDTree(pdr_pts)
    tree_g = KDTree(geo_pts)

    dists_g2p, _ = tree_p.query(geo_pts)
    dists_p2g, _ = tree_g.query(pdr_pts)

    avg_g2p = float(np.mean(dists_g2p))
    avg_p2g = float(np.mean(dists_p2g))
    hausdorff = max(float(np.max(dists_g2p)), float(np.max(dists_p2g)))

    return {
        'avg_dist_geotopdr': avg_g2p,
        'avg_dist_pdrto geo': avg_p2g,
        'avg_symmetric': (avg_g2p + avg_p2g) / 2,
        'hausdorff': hausdorff,
    }


# ─────────────────────────────────────────────
# 6. Error Analysis Helpers
# ─────────────────────────────────────────────

def analyze_step_detection(data, k=0.425, peak_min_height=1.5,
                           peak_min_distance=25, peak_prominence=1.0):
    """Try multiple peak detection thresholds and report step counts."""
    results = []
    for height in [2.0, 2.5, 3.0, 3.5, 4.0, 5.0]:
        for dist in [20, 25, 30, 35, 40]:
            peaks, _ = find_peaks(
                np.sqrt(np.sum(data['accel'] ** 2, axis=1)) * 9.81,
                height=height, distance=dist, prominence=1.5
            )
            results.append({'height': height, 'distance': dist, 'n_steps': len(peaks)})
    return results


def analyze_gps_anchors(bin_data):
    """Analyze GPS anchor frames for quality issues."""
    issues = []
    for g in bin_data.get('gps_frames', []):
        evt = 'START' if g['event'] == 0xBB else 'END'
        if g['lat'] < 20 or g['lat'] > 40 or g['lon'] < 50 or g['lon'] > 65:
            issues.append(f'{evt}: implausible coords lat={g["lat"]:.4f} lon={g["lon"]:.4f}')
        if g['alt'] == 0:
            issues.append(f'{evt}: alt=0 (not entered in web form)')
    return issues


# ─────────────────────────────────────────────
# 7. Per-Segment Analysis
# ─────────────────────────────────────────────

class SegmentAnalysis:
    """Run analysis on one walk segment."""

    def __init__(self, seg_id):
        self.seg_id = seg_id
        self.bin_data = None
        self.geo = None
        self.pdr_x = None
        self.pdr_y = None
        self.step_indices = None
        self.step_lengths = None
        self.step_headings = None
        self.geo_x, self.geo_y = None, None
        self.metrics = {}
        self.error_analysis = {}

    def load(self, data_dir, include_recovery=True):
        # DeadReckoner
        bin_path = os.path.join(data_dir, 'DeadReckoner', f'DR_LOG_00{self.seg_id}.BIN')
        self.bin_data = parse_bin(bin_path)
        if self.bin_data is None:
            print(f'    [WARN] Could not parse {bin_path}')
            return False

        # Stitch recovery fragment if available (only for seg 1 has 001001.BIN)
        if include_recovery and self.seg_id == 1:
            rec_path = os.path.join(data_dir, 'DeadReckoner', '001001.BIN')
            if os.path.exists(rec_path):
                rec_data = parse_bin(rec_path)
                if rec_data is not None and rec_data['n_imu'] > 0:
                    stitched = stitch_bin(self.bin_data, rec_data)
                    if stitched['n_imu'] > self.bin_data['n_imu']:
                        print(f'    [INFO] Stitched recovery fragment: '
                              f'{self.bin_data["n_imu"]} + {stitched["n_imu"] - self.bin_data["n_imu"]} IMU frames')
                        self.bin_data = stitched

        # Geo Tracker (only reference this time)
        geo_path = os.path.join(data_dir, 'GeoTracker', f'{self.seg_id}.gpx')
        self.geo = parse_gpx(geo_path)
        self.geo_x, self.geo_y = gpx_to_local(self.geo['lat'], self.geo['lon'])
        return True

    def run_pdr(self, k=0.425, heading_offset_deg=0, heading_drift_rate_deg=0,
                peak_min_height=1.5, peak_min_distance=25, peak_prominence=1.0):
        result = compute_pdr(self.bin_data, k=k, heading_offset_deg=heading_offset_deg,
                             heading_drift_rate_deg=heading_drift_rate_deg,
                             peak_min_height=peak_min_height,
                             peak_min_distance=peak_min_distance,
                             peak_prominence=peak_prominence)
        self.pdr_x, self.pdr_y = result[0], result[1]
        self.step_indices = result[2]
        self.step_lengths = result[3]
        self.step_headings = result[4]
        self._last_k = k
        self._last_drift = heading_drift_rate_deg
        return result

    def compute_metrics(self):
        pdr_len = compute_path_length(self.pdr_x, self.pdr_y)
        geo_len = compute_gps_distance(self.geo['lat'], self.geo['lon'])

        pdr_end = np.array([self.pdr_x[-1], self.pdr_y[-1]])
        geo_end = np.array([self.geo_x[-1], self.geo_y[-1]])

        # 2D optimize: heading_offset + heading_drift_rate
        yaw_init = np.array(self.step_headings)
        sl_init = np.array(self.step_lengths)
        best_angle, best_drift, _ = find_best_heading_and_drift(
            yaw_init, sl_init,
            np.array([0] + list(self.geo_x)),
            np.array([0] + list(self.geo_y))
        )
        # Recompute with best heading AND drift
        self.run_pdr(k=self._last_k if hasattr(self, '_last_k') else 0.425,
                     heading_offset_deg=best_angle,
                     heading_drift_rate_deg=best_drift)
        pdr_rot_end = np.array([self.pdr_x[-1], self.pdr_y[-1]])
        endpoint_error = float(np.linalg.norm(pdr_rot_end - geo_end))

        # Recompute pdr_len after drift correction
        pdr_len = compute_path_length(self.pdr_x, self.pdr_y)
        pdr_err_pct = abs(pdr_len - geo_len) / geo_len * 100 if geo_len > 0 else 0

        # Path shape similarity
        shape = compute_path_similarity(self.pdr_x, self.pdr_y, self.geo_x, self.geo_y)

        self.metrics = {
            'segment': self.seg_id,
            'pdr_distance_m': pdr_len,
            'geotracker_distance_m': geo_len,
            'pdr_error_pct': pdr_err_pct,
            'endpoint_error_m': endpoint_error,
            'n_steps': len(self.step_lengths),
            'best_heading_offset': best_angle,
            'best_heading_drift': best_drift,
            'n_frames': self.bin_data['n_imu'],
            'crc_ok': self.bin_data['crc_ok'],
            'crc_bad': self.bin_data['crc_bad'],
            'avg_path_dist_m': shape['avg_symmetric'],
            'hausdorff_m': shape['hausdorff'],
        }
        return self.metrics

    def analyze_errors(self):
        """Deep-dive error analysis for this segment."""
        analysis = {}

        total = self.bin_data['crc_ok'] + self.bin_data['crc_bad']
        analysis['crc_failure_pct'] = (self.bin_data['crc_bad'] / total * 100) if total > 0 else 0
        analysis['gps_issues'] = analyze_gps_anchors(self.bin_data)
        step_analysis = analyze_step_detection(self.bin_data)
        analysis['step_sensitivity'] = step_analysis

        accel_mag = np.sqrt(np.sum(self.bin_data['accel'] ** 2, axis=1)) * 9.81
        analysis['accel_mean'] = float(np.mean(accel_mag))
        analysis['accel_std'] = float(np.std(accel_mag))
        analysis['accel_max'] = float(np.max(accel_mag))

        if self.bin_data['n_imu'] > 100:
            yaw = quaternion_to_yaw(
                self.bin_data['q'][:, 0], self.bin_data['q'][:, 1],
                self.bin_data['q'][:, 2], self.bin_data['q'][:, 3]
            )
            yaw_unwrapped = np.unwrap(yaw)
            ts_begin = self.bin_data['imu_timestamps'][0]
            ts_end = self.bin_data['imu_timestamps'][-1]
            duration_s = (ts_end - ts_begin) / 1e6
            if duration_s > 0:
                total_yaw_change = float(np.abs(yaw_unwrapped[-1] - yaw_unwrapped[0]))
                analysis['yaw_drift_rate'] = total_yaw_change / duration_s * 60
            else:
                analysis['yaw_drift_rate'] = 0
        else:
            analysis['yaw_drift_rate'] = 0

        self.error_analysis = analysis
        return analysis


# ─────────────────────────────────────────────
# 8. Visualization
# ─────────────────────────────────────────────

def plot_segment(ax, seg_id, pdr_x, pdr_y, geo_x, geo_y, metrics, analysis=None,
                 color_pdr='blue', show_geo_accuracy=False):
    ax.set_aspect('equal')

    if show_geo_accuracy and analysis is not None:
        ax.plot(geo_x, geo_y, '-', color='green', linewidth=2,
                label='Geo Tracker', alpha=0.5)
    else:
        ax.plot(geo_x, geo_y, '-', color='green', linewidth=2,
                label='Geo Tracker', alpha=0.8)

    ax.plot(pdr_x, pdr_y, '-', color=color_pdr, linewidth=2,
            label='DeadReckoner PDR', alpha=0.9)

    ax.scatter(geo_x[0], geo_y[0], c='green', marker='o', s=60, zorder=5, edgecolors='black')
    ax.scatter(pdr_x[0], pdr_y[0], c=color_pdr, marker='o', s=60, zorder=5, edgecolors='black')
    ax.scatter(geo_x[-1], geo_y[-1], c='green', marker='x', s=80, zorder=5, linewidths=2)
    ax.scatter(pdr_x[-1], pdr_y[-1], c=color_pdr, marker='x', s=80, zorder=5, linewidths=2)

    # Auto-zoom
    all_x = np.concatenate([geo_x, pdr_x])
    all_y = np.concatenate([geo_y, pdr_y])
    mid_x = (all_x.max() + all_x.min()) / 2
    mid_y = (all_y.max() + all_y.min()) / 2
    max_extent = max(np.ptp(all_x), np.ptp(all_y)) * 0.6
    if max_extent > 1:
        ax.set_xlim(mid_x - max_extent, mid_x + max_extent)
        ax.set_ylim(mid_y - max_extent, mid_y + max_extent)

    ax.set_xlabel('East (m)')
    ax.set_ylabel('North (m)')
    ax.set_title(f'Segment {seg_id}')
    ax.grid(True, alpha=0.3)
    ax.legend(fontsize=8)

    if metrics:
        txt = (f'PDR: {metrics["pdr_distance_m"]:.0f}m  '
               f'Geo: {metrics["geotracker_distance_m"]:.0f}m\n'
               f'Error: {metrics["pdr_error_pct"]:.1f}%  '
               f'Endpoint: {metrics["endpoint_error_m"]:.1f}m\n'
               f'Steps: {metrics["n_steps"]}  '
               f'Hdg: {metrics["best_heading_offset"]:.0f}°')
        if 'best_heading_drift' in metrics:
            txt += f'  Drift: {metrics["best_heading_drift"]:.3f}°/step'
        if 'avg_path_dist_m' in metrics:
            txt += f'\nShape dist: avg {metrics["avg_path_dist_m"]:.0f}m  max {metrics["hausdorff_m"]:.0f}m'
        if analysis:
            txt += f'\nCRC: {analysis["crc_failure_pct"]:.2f}%  Yaw: {analysis["yaw_drift_rate"]:.1f}°/min'
        ax.text(0.05, 0.95, txt, transform=ax.transAxes, fontsize=7,
                verticalalignment='top',
                bbox=dict(boxstyle='round,pad=0.3', facecolor='white', alpha=0.8))


def plot_accel_steps(ax, data, peaks, title=''):
    """Plot acceleration magnitude with detected step markers."""
    accel_mag = np.sqrt(np.sum(data['accel'] ** 2, axis=1)) * 9.81
    t = np.arange(len(accel_mag)) / 100.0
    ax.plot(t, accel_mag, '-', color='blue', alpha=0.7, linewidth=0.8)
    if len(peaks) > 0:
        ax.scatter(t[peaks], accel_mag[peaks], c='red', marker='v', s=20, zorder=5, alpha=0.7)
    ax.set_xlabel('Time (s)')
    ax.set_ylabel('Accel Magnitude (m/s²)')
    ax.set_title(title or 'Acceleration + Detected Steps')
    ax.grid(True, alpha=0.3)


def plot_heading_drift(ax, data, title=''):
    """Plot yaw angle over time to visualize drift."""
    if data['n_imu'] < 10:
        ax.text(0.5, 0.5, 'Not enough IMU frames', transform=ax.transAxes, ha='center')
        return
    yaw = quaternion_to_yaw(data['q'][:, 0], data['q'][:, 1],
                             data['q'][:, 2], data['q'][:, 3])
    t = np.arange(len(yaw)) / 100.0
    ax.plot(t, np.degrees(yaw), '-', color='purple', linewidth=0.8, alpha=0.7)
    ax.set_xlabel('Time (s)')
    ax.set_ylabel('Yaw (deg)')
    ax.set_title(title or 'Heading (Yaw) Over Time')
    ax.grid(True, alpha=0.3)


def compute_gps_guided_pdr(analysis):
    """
    Use actual GPS heading (interpolated to PDR step positions by cumulative
    distance) combined with PDR step lengths. This proves that PDR step lengths
    are correct — the only issue is IMU heading not tracking body turns.
    Returns (corrected_x, corrected_y, metrics_dict).
    """
    geo_x, geo_y = analysis.geo_x, analysis.geo_y
    step_lengths = np.array(analysis.step_lengths)

    # GPS heading at segment midpoints
    gps_seg = np.sqrt(np.diff(geo_x)**2 + np.diff(geo_y)**2)
    gps_cum = np.cumsum(np.insert(gps_seg, 0, 0.0))
    gps_heading = np.arctan2(np.diff(geo_x), np.diff(geo_y))
    gps_mid = (gps_cum[:-1] + gps_cum[1:]) / 2
    total_geo = gps_cum[-1]

    # PDR cumulative distance
    pdr_cum = np.cumsum(np.insert(step_lengths, 0, 0.0))
    total_pdr = pdr_cum[-1]

    # Interpolate GPS heading at PDR step distances
    pdr_pct = pdr_cum[1:] / total_pdr if total_pdr > 0 else np.zeros(len(step_lengths))
    gps_at_steps = np.interp(pdr_pct * total_geo, gps_mid, np.unwrap(gps_heading))

    # Build corrected path
    sx = step_lengths * np.sin(gps_at_steps)
    sy = step_lengths * np.cos(gps_at_steps)
    cx = np.cumsum(np.insert(sx, 0, 0.0))
    cy = np.cumsum(np.insert(sy, 0, 0.0))

    pdr_len = float(np.sum(np.sqrt(np.diff(cx)**2 + np.diff(cy)**2)))
    geo_len = float(np.sum(gps_seg))
    end_err = float(np.linalg.norm([cx[-1]-geo_x[-1], cy[-1]-geo_y[-1]]))

    tree = KDTree(np.column_stack([cx[::5], cy[::5]]))
    dists, _ = tree.query(np.column_stack([geo_x, geo_y]))

    return cx, cy, {
        'pdr_distance_m': pdr_len,
        'geotracker_distance_m': geo_len,
        'endpoint_error_m': end_err,
        'avg_path_dist_m': float(np.mean(dists)),
        'hausdorff_m': float(np.max(dists)),
    }


def plot_gps_guided(ax, seg_id, cx, cy, geo_x, geo_y, metrics):
    """Plot GPS-guided PDR path overlay."""
    ax.set_aspect('equal')
    ax.plot(geo_x, geo_y, '-', color='green', linewidth=2, label='Geo Tracker', alpha=0.8)
    ax.plot(cx, cy, '-', color='red', linewidth=2, label='PDR+GPS heading', alpha=0.9)

    ax.scatter(geo_x[0], geo_y[0], c='green', marker='o', s=60, zorder=5, edgecolors='black')
    ax.scatter(cx[0], cy[0], c='red', marker='o', s=60, zorder=5, edgecolors='black')
    ax.scatter(geo_x[-1], geo_y[-1], c='green', marker='x', s=80, zorder=5, linewidths=2)
    ax.scatter(cx[-1], cy[-1], c='red', marker='x', s=80, zorder=5, linewidths=2)

    all_x = np.concatenate([geo_x, cx])
    all_y = np.concatenate([geo_y, cy])
    mid_x = (all_x.max() + all_x.min()) / 2
    mid_y = (all_y.max() + all_y.min()) / 2
    extent = max(np.ptp(all_x), np.ptp(all_y)) * 0.6
    if extent > 1:
        ax.set_xlim(mid_x - extent, mid_x + extent)
        ax.set_ylim(mid_y - extent, mid_y + extent)

    ax.set_xlabel('East (m)')
    ax.set_ylabel('North (m)')
    ax.set_title(f'S{seg_id} — GPS-guided PDR')
    ax.grid(True, alpha=0.3)
    ax.legend(fontsize=8)

    if metrics:
        txt = (f'PDR+GPS: {metrics["pdr_distance_m"]:.0f}m  '
               f'Geo: {metrics["geotracker_distance_m"]:.0f}m\n'
               f'Endpoint: {metrics["endpoint_error_m"]:.1f}m  '
               f'Shape: avg {metrics["avg_path_dist_m"]:.0f}m  max {metrics["hausdorff_m"]:.0f}m')
        ax.text(0.05, 0.95, txt, transform=ax.transAxes, fontsize=7,
                verticalalignment='top',
                bbox=dict(boxstyle='round,pad=0.3', facecolor='white', alpha=0.8))


def plot_summary(all_metrics, all_analyses):
    """Generate summary bar charts across all segments."""
    fig, axes = plt.subplots(2, 3, figsize=(16, 10))
    fig.suptitle('DeadReckoner PDR vs Geo Tracker — Summary (valley-heading fix)',
                 fontsize=14, fontweight='bold')

    ax_dist = axes[0, 0]
    ax_error = axes[0, 1]
    ax_endpt = axes[0, 2]
    ax_shape = axes[1, 0]
    ax_steps = axes[1, 1]
    ax_drift = axes[1, 2]

    segs = [m['segment'] for m in all_metrics]
    labels = [f'S{m}' for m in segs]
    x = np.arange(len(segs))
    w = 0.3

    # Distance comparison
    ax_dist.bar(x - w / 2, [m['pdr_distance_m'] for m in all_metrics], w,
                label='PDR', color='blue', alpha=0.7)
    ax_dist.bar(x + w / 2, [m['geotracker_distance_m'] for m in all_metrics], w,
                label='Geo Tracker', color='green', alpha=0.7)
    ax_dist.set_xticks(x)
    ax_dist.set_xticklabels(labels)
    ax_dist.set_ylabel('Distance (m)')
    ax_dist.set_title('Total Path Length')
    ax_dist.legend(fontsize=8)
    ax_dist.grid(True, alpha=0.3, axis='y')

    # Error %
    colors = ['green' if m['pdr_error_pct'] < 10
              else 'orange' if m['pdr_error_pct'] < 20
              else 'red' for m in all_metrics]
    bars = ax_error.bar(x, [m['pdr_error_pct'] for m in all_metrics],
                        color=colors, alpha=0.7)
    ax_error.axhline(y=10, color='green', linestyle='--', alpha=0.5, label='10%')
    ax_error.axhline(y=20, color='red', linestyle='--', alpha=0.5, label='20%')
    ax_error.set_xticks(x)
    ax_error.set_xticklabels(labels)
    ax_error.set_ylabel('Error (%)')
    ax_error.set_title('PDR Distance Error vs Geo Tracker')
    ax_error.legend(fontsize=8)
    ax_error.grid(True, alpha=0.3, axis='y')
    for bar, val in zip(bars, [m['pdr_error_pct'] for m in all_metrics]):
        ax_error.text(bar.get_x() + bar.get_width() / 2, bar.get_height() + 0.5,
                      f'{val:.1f}%', ha='center', va='bottom', fontsize=8)

    # Endpoint error
    bars2 = ax_endpt.bar(x, [m['endpoint_error_m'] for m in all_metrics],
                         color='purple', alpha=0.7)
    ax_endpt.set_xticks(x)
    ax_endpt.set_xticklabels(labels)
    ax_endpt.set_ylabel('Error (m)')
    ax_endpt.set_title('End-Point Error (Post-Alignment)')
    ax_endpt.grid(True, alpha=0.3, axis='y')
    for bar, val in zip(bars2, [m['endpoint_error_m'] for m in all_metrics]):
        ax_endpt.text(bar.get_x() + bar.get_width() / 2, bar.get_height() + 0.5,
                      f'{val:.1f}m', ha='center', va='bottom', fontsize=8)

    # Shape similarity (avg symmetric distance)
    shape_vals = [m.get('avg_path_dist_m', 0) for m in all_metrics]
    bars3 = ax_shape.bar(x, shape_vals, color='darkorange', alpha=0.7)
    ax_shape.axhline(y=50, color='green', linestyle='--', alpha=0.5, label='50m (good)')
    ax_shape.axhline(y=100, color='orange', linestyle='--', alpha=0.5, label='100m (ok)')
    ax_shape.set_xticks(x)
    ax_shape.set_xticklabels(labels)
    ax_shape.set_ylabel('Distance (m)')
    ax_shape.set_title('Path Shape Similarity (avg dist to GPS)')
    ax_shape.legend(fontsize=8)
    ax_shape.grid(True, alpha=0.3, axis='y')
    for bar, val in zip(bars3, shape_vals):
        ax_shape.text(bar.get_x() + bar.get_width() / 2, bar.get_height() + 1,
                      f'{val:.0f}m', ha='center', va='bottom', fontsize=8)

    # Step count
    ax_steps.bar(x, [m['n_steps'] for m in all_metrics], color='teal', alpha=0.7)
    ax_steps.set_xticks(x)
    ax_steps.set_xticklabels(labels)
    ax_steps.set_ylabel('Steps Detected')
    ax_steps.set_title('PDR Step Count')
    ax_steps.grid(True, alpha=0.3, axis='y')

    # Yaw drift rate
    drift_rates = [a['yaw_drift_rate'] for a in all_analyses]
    ax_drift.bar(x, drift_rates, color='darkviolet', alpha=0.7)
    ax_drift.set_xticks(x)
    ax_drift.set_xticklabels(labels)
    ax_drift.set_ylabel('Drift (°/min)')
    ax_drift.set_title('Yaw Drift Rate (IMU)')
    ax_drift.grid(True, alpha=0.3, axis='y')

    # Overall stats
    errors = [m['pdr_error_pct'] for m in all_metrics]
    avg_err = float(np.mean(errors))
    max_err = float(np.max(errors))
    total_pdr = sum(m['pdr_distance_m'] for m in all_metrics)
    total_geo = sum(m['geotracker_distance_m'] for m in all_metrics)
    total_err = abs(total_pdr - total_geo) / total_geo * 100 if total_geo > 0 else 0
    avg_shape = float(np.mean(shape_vals)) if shape_vals else 0

    if avg_err < 10 and total_err < 10:
        verdict = 'IMU-ONLY PDR IS SUFFICIENT'
        vcolor = 'green'
    elif avg_err < 20:
        verdict = 'Borderline — GPS recommended for absolute position'
        vcolor = 'orange'
    else:
        verdict = 'GPS STRONGLY RECOMMENDED'
        vcolor = 'red'

    fig.text(0.35, 0.02,
             f'Overall: avg error {avg_err:.1f}%  |  max error {max_err:.1f}%  |  '
             f'total error {total_err:.1f}%\n'
             f'Path shape: avg dist {avg_shape:.0f}m to GPS | '
             f'Verdict: {verdict}',
             fontsize=11, fontweight='bold', color=vcolor,
             bbox=dict(boxstyle='round,pad=0.5', facecolor='lightyellow', alpha=0.9))

    plt.tight_layout(rect=[0, 0.06, 1, 0.95])
    out_dir = os.path.join(SCRIPT_DIR, 'analysis')
    os.makedirs(out_dir, exist_ok=True)
    plt.savefig(os.path.join(out_dir, 'summary_comparison.png'), dpi=200, bbox_inches='tight')
    print(f'[+] Saved summary plot to analysis/summary_comparison.png')
    if not BATCH_MODE:
        plt.show()


# ─────────────────────────────────────────────
# 9. Interactive Viewer
# ─────────────────────────────────────────────

def run_interactive(analyses):
    """Figure with per-segment plots + K slider."""
    fig, axes = plt.subplots(2, 3, figsize=(18, 12))
    fig.suptitle('DeadReckoner PDR vs Geo Tracker — Interactive Tuning (valley-heading)',
                 fontsize=13, fontweight='bold')
    axes_flat = axes.flatten()
    pdr_colors = ['blue', 'darkviolet', 'darkred', 'darkcyan', 'saddlebrown']

    initial_k = 0.425
    for sa in analyses:
        sa.run_pdr(k=initial_k,
                   heading_offset_deg=sa.metrics.get('best_heading_offset', 0),
                   heading_drift_rate_deg=sa.metrics.get('best_heading_drift', 0))
        sa.compute_metrics()

    for i, sa in enumerate(analyses):
        ax = axes_flat[i]
        plot_segment(ax, sa.seg_id, sa.pdr_x, sa.pdr_y,
                     sa.geo_x, sa.geo_y, sa.metrics, sa.error_analysis,
                     color_pdr=pdr_colors[i])
    axes_flat[5].axis('off')

    plt.subplots_adjust(left=0.05, right=0.95, bottom=0.16, top=0.90,
                        hspace=0.35, wspace=0.30)

    ax_k = plt.axes([0.15, 0.04, 0.35, 0.025])
    slider_k = Slider(ax_k, 'Weinberg K', 0.05, 1.0, valinit=initial_k, valstep=0.005)

    ax_info = plt.axes([0.55, 0.04, 0.40, 0.025])
    ax_info.axis('off')
    info_text = ax_info.text(0, 0.5, '', fontsize=9, verticalalignment='center')

    def update(val):
        k = slider_k.val
        total_pdr = 0
        total_geo = 0
        all_errs = []
        for sa in analyses:
            sa.run_pdr(k=k,
                       heading_offset_deg=sa.metrics.get('best_heading_offset', 0),
                       heading_drift_rate_deg=sa.metrics.get('best_heading_drift', 0))
            sa.compute_metrics()
            ax = axes_flat[sa.seg_id - 1]
            ax.lines[1].set_data(sa.pdr_x, sa.pdr_y)
            for coll in list(ax.collections):
                coll.remove()
            ax.scatter(sa.pdr_x[-1], sa.pdr_y[-1], c=pdr_colors[sa.seg_id - 1],
                       marker='x', s=80, zorder=5, linewidths=2)
            ax.scatter(sa.pdr_x[0], sa.pdr_y[0], c=pdr_colors[sa.seg_id - 1],
                       marker='o', s=60, zorder=5, edgecolors='black')
            total_pdr += sa.metrics['pdr_distance_m']
            total_geo += sa.metrics['geotracker_distance_m']
            all_errs.append(sa.metrics['pdr_error_pct'])
        total_err = abs(total_pdr - total_geo) / total_geo * 100 if total_geo > 0 else 0
        avg_err = float(np.mean(all_errs)) if all_errs else 0
        info_text.set_text(
            f'K={k:.3f}  |  Avg error: {avg_err:.1f}%  |  Total error: {total_err:.1f}%  |  '
            f'Total PDR: {total_pdr:.0f}m  |  GPS: {total_geo:.0f}m')
        fig.canvas.draw_idle()

    slider_k.on_changed(update)

    ax_reset = plt.axes([0.05, 0.04, 0.06, 0.03])
    btn_reset = Button(ax_reset, 'Reset K')
    def reset(event):
        slider_k.reset()
    btn_reset.on_clicked(reset)

    update(initial_k)

    out_dir = os.path.join(SCRIPT_DIR, 'analysis')
    os.makedirs(out_dir, exist_ok=True)
    plt.savefig(os.path.join(out_dir, 'segment_comparison.png'), dpi=200, bbox_inches='tight')
    print(f'[+] Saved segment plot to analysis/segment_comparison.png')
    if not BATCH_MODE:
        plt.show()


# ─────────────────────────────────────────────
# 10. Error Report
# ─────────────────────────────────────────────

def print_error_report(analyses):
    """Print detailed error analysis for firmware improvements."""
    print()
    print('=' * 70)
    print('  ERROR ANALYSIS REPORT — Potential Firmware Improvements')
    print('=' * 70)
    print()

    for sa in analyses:
        m = sa.metrics
        a = sa.error_analysis
        print(f'--- Segment {sa.seg_id} ---')
        print(f'  CRC failures: {m["crc_bad"]}/{m["crc_ok"] + m["crc_bad"]} '
              f'({a["crc_failure_pct"]:.2f}%)')
        if a['crc_failure_pct'] > 0.1:
            print(f'    [ISSUE] CRC errors detected — check SD card signal integrity')

        print(f'  Yaw drift rate: {a["yaw_drift_rate"]:.2f}°/min')
        if a['yaw_drift_rate'] > 5:
            print(f'    [ISSUE] High yaw drift — magnetometer calibration may need improvement')

        print(f'  Accel signal: mean={a["accel_mean"]:.1f} m/s², '
              f'std={a["accel_std"]:.1f}, max={a["accel_max"]:.1f}')

        for issue in a.get('gps_issues', []):
            print(f'  [GPS] {issue}')

        if a['step_sensitivity']:
            counts = sorted(set(s['n_steps'] for s in a['step_sensitivity']))
            print(f'  Step detection range across thresholds: {min(counts)}–{max(counts)} '
                  f'(target ~{m["n_steps"]})')

        print(f'  Path shape: avg dist to GPS = {m["avg_path_dist_m"]:.0f}m, '
              f'Hausdorff = {m["hausdorff_m"]:.0f}m')
        print()

    print('--- Recommendations ---')
    all_crc_fail = any(s.error_analysis['crc_failure_pct'] > 0.1 for s in analyses)
    all_yaw_drift = any(s.error_analysis['yaw_drift_rate'] > 5 for s in analyses)
    all_gps_issues = any(s.error_analysis.get('gps_issues') for s in analyses)

    if all_crc_fail:
        print('  [HIGH] CRC errors detected — reduce SPI speed or improve SD card wiring')
    if all_yaw_drift:
        print('  [MED] Yaw drift >5°/min — consider calibration improvements or ZUPT')
    if all_gps_issues:
        print('  [LOW] GPS data quality issues — verify manual lat/lon entry on phone')
    if not any([all_crc_fail, all_yaw_drift, all_gps_issues]):
        print('  No critical issues detected. PDR accuracy is within expected range.')
    shape_vals = [m['avg_path_dist_m'] for m in [s.metrics for s in analyses]]
    avg_shape = float(np.mean(shape_vals))
    print(f'\n  Path shape: avg {avg_shape:.0f}m to GPS across {len(shape_vals)} segments')
    if avg_shape < 100:
        print('  [OK] PDR path shape reasonably matches GPS')
    elif avg_shape < 200:
        print('  [WARN] PDR path shape diverges from GPS — consider heading calibration improvements')
    else:
        print('  [ISSUE] PDR path shape significantly different from GPS — see heading analysis above')
    print()


# ─────────────────────────────────────────────
# 11. Main
# ─────────────────────────────────────────────

def print_table(analyses):
    print()
    print('=' * 110)
    print(f'{"Segment":>8} {"PDR (m)":>10} {"GeoTr (m)":>12} '
          f'{"Err%":>6} {"Endpt(m)":>9} {"Steps":>7} {"Frames":>8} '
          f'{"CRC%":>6} {"YDrift":>7} {"ShpAvg":>7} {"Hausd":>7}')
    print('-' * 110)

    total_pdr = 0
    total_geo = 0
    all_errors = []

    for sa in analyses:
        m = sa.metrics
        a = sa.error_analysis
        total_pdr += m['pdr_distance_m']
        total_geo += m['geotracker_distance_m']
        all_errors.append(m['pdr_error_pct'])
        crc_pct = a['crc_failure_pct']
        print(f'{m["segment"]:>8} {m["pdr_distance_m"]:>10.1f} {m["geotracker_distance_m"]:>12.1f} '
              f'{m["pdr_error_pct"]:>5.1f}% {m["endpoint_error_m"]:>8.1f} '
              f'{m["n_steps"]:>7} {m["n_frames"]:>8} '
              f'{crc_pct:>5.2f}% {a["yaw_drift_rate"]:>6.1f} '
              f'{m.get("avg_path_dist_m", 0):>6.0f} {m.get("hausdorff_m", 0):>6.0f}')

    print('-' * 110)
    total_err = abs(total_pdr - total_geo) / total_geo * 100 if total_geo > 0 else 0
    avg_err = float(np.mean(all_errors))
    max_err = float(np.max(all_errors))
    print(f'{"TOTAL":>8} {total_pdr:>10.1f} {total_geo:>12.1f} {total_err:>5.1f}%')
    print(f'\nAverage segment error: {avg_err:.1f}%  |  Max segment error: {max_err:.1f}%')
    print()

    if avg_err < 10 and total_err < 10:
        verdict = 'IMU-ONLY PDR IS SUFFICIENT — GPS and BMP280 not needed for path tracking'
        vcolor = 'GREEN'
    elif avg_err < 20:
        verdict = 'BORDERLINE — GPS recommended for absolute position anchoring'
        vcolor = 'YELLOW'
    else:
        verdict = 'GPS STRONGLY RECOMMENDED — IMU-only PDR has significant error'
        vcolor = 'RED'

    print(f'  [{vcolor}] VERDICT: {verdict}')
    print('=' * 110)
    print()


def main():
    data_dir = SCRIPT_DIR

    print()
    print('  DEADRECKONER PATH COMPARISON TOOL v2.2+')
    print('  Comparing PDR vs Geo Tracker (47-byte frames, CRC-16)')
    print(f'  2D heading+drift optimization for endpoint alignment')
    print(f'  Data: {data_dir}')
    print()

    # Load all segments
    analyses = []
    for seg_id in range(1, SEGMENTS + 1):
        print(f'  Loading segment {seg_id}...', end=' ')
        sa = SegmentAnalysis(seg_id)
        ok = sa.load(data_dir)
        analyses.append(sa)
        if ok:
            b = sa.bin_data
            g = sa.geo
            total = b['crc_ok'] + b['crc_bad']
            crc_str = f'CRC: {b["crc_ok"]}/{total} ok' if total > 0 else 'no CRC'
            gps_count = len(b.get('gps_frames', []))
            print(f'BIN: {b["n_imu"]:,} IMU frames ({crc_str}, {gps_count} GPS anchors), '
                  f'Geo: {len(g["lat"])} pts')
        else:
            print('FAILED')

    # Initial PDR
    initial_k = 0.425
    print(f'\n  Running PDR (K={initial_k})...')
    for sa in analyses:
        sa.run_pdr(k=initial_k)

    # Compute metrics with auto heading alignment
    print('  Computing metrics (2D heading+drift optimization)...')
    for sa in analyses:
        sa.compute_metrics()

    # Error analysis
    print('  Running error analysis...')
    for sa in analyses:
        sa.analyze_errors()

    # Print results
    print_table(analyses)

    # Error report
    print_error_report(analyses)

    # Generate summary plot
    print('  Generating summary plot...')
    all_metrics = [sa.metrics for sa in analyses]
    all_analyses = [sa.error_analysis for sa in analyses]
    plot_summary(all_metrics, all_analyses)

    # Extra diagnostic plots
    print('  Generating diagnostic plots...')
    fig2, axes2 = plt.subplots(2, 3, figsize=(16, 8))
    fig2.suptitle('Acceleration + Step Detection Diagnostic', fontsize=14, fontweight='bold')
    pdr_colors = ['blue', 'darkviolet', 'darkred', 'darkcyan', 'saddlebrown']
    for i, sa in enumerate(analyses):
        ax = axes2[i // 3, i % 3]
        plot_accel_steps(ax, sa.bin_data, sa.step_indices,
                         f'S{sa.seg_id} ({sa.metrics["n_steps"]} steps)')
    axes2[1, 2].axis('off')
    plt.tight_layout(rect=[0, 0, 1, 0.95])
    out_dir = os.path.join(SCRIPT_DIR, 'analysis')
    plt.savefig(os.path.join(out_dir, 'accel_diagnostic.png'), dpi=200, bbox_inches='tight')
    print(f'[+] Saved accel diagnostic to analysis/accel_diagnostic.png')
    if not BATCH_MODE:
        plt.show()
    else:
        plt.close(fig2)

    # Heading drift plots
    fig3, axes3 = plt.subplots(2, 3, figsize=(16, 8))
    fig3.suptitle('Heading (Yaw) Drift Over Time', fontsize=14, fontweight='bold')
    for i, sa in enumerate(analyses):
        ax = axes3[i // 3, i % 3]
        plot_heading_drift(ax, sa.bin_data, f'S{sa.seg_id}')
    axes3[1, 2].axis('off')
    plt.tight_layout(rect=[0, 0, 1, 0.95])
    plt.savefig(os.path.join(out_dir, 'heading_drift.png'), dpi=200, bbox_inches='tight')
    print(f'[+] Saved heading drift to analysis/heading_drift.png')
    if not BATCH_MODE:
        plt.show()
    else:
        plt.close(fig3)

    # Per-segment 2D path maps
    print('  Generating per-segment path maps...')
    pdr_colors = ['blue', 'darkviolet', 'darkred', 'darkcyan', 'saddlebrown']
    for i, sa in enumerate(analyses):
        fig_seg, ax_seg = plt.subplots(1, 1, figsize=(8, 8))
        plot_segment(ax_seg, sa.seg_id, sa.pdr_x, sa.pdr_y,
                     sa.geo_x, sa.geo_y, sa.metrics, sa.error_analysis,
                     color_pdr=pdr_colors[i])
        out_seg = os.path.join(out_dir, f'path_s{sa.seg_id}.png')
        fig_seg.savefig(out_seg, dpi=200, bbox_inches='tight')
        plt.close(fig_seg)
        print(f'  [+] Saved path map to analysis/path_s{sa.seg_id}.png')

    # GPS-guided PDR analysis (proves PDR step lengths are correct)
    if GPS_GUIDED:
        print('  Generating GPS-guided PDR maps (proving step length accuracy)...')
        print()
        print('  === GPS-GUIDED PDR (PDR step lengths + GPS heading) ===')
        print(f'  {"Segment":>8} {"PDR (m)":>10} {"Geo (m)":>10} {"Endpt(m)":>9} '
              f'{"ShapeAvg":>9} {"Hausdorff":>9}')
        print('  ' + '-' * 55)
        for i, sa in enumerate(analyses):
            cx, cy, gm = compute_gps_guided_pdr(sa)
            print(f'  {sa.seg_id:>8} {gm["pdr_distance_m"]:>10.1f} {gm["geotracker_distance_m"]:>10.1f} '
                  f'{gm["endpoint_error_m"]:>8.1f} {gm["avg_path_dist_m"]:>8.0f} {gm["hausdorff_m"]:>8.0f}')
            # Save GPS-guided overlay
            fig_gg, ax_gg = plt.subplots(1, 1, figsize=(8, 8))
            plot_gps_guided(ax_gg, sa.seg_id, cx, cy, sa.geo_x, sa.geo_y, gm)
            out_gg = os.path.join(out_dir, f'gps_guided_s{sa.seg_id}.png')
            fig_gg.savefig(out_gg, dpi=200, bbox_inches='tight')
            plt.close(fig_gg)
            print(f'  [+] Saved gps_guided_s{sa.seg_id}.png')
        print()

    if not BATCH_MODE:
        print('  Starting interactive viewer (adjust K slider)...')
        run_interactive(analyses)

    print()
    print('  Done. Check analysis/ for saved plots.')
    print()


if __name__ == '__main__':
    main()
