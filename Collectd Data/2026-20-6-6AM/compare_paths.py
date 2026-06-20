#!/usr/bin/env python3
"""
DeadReckoner Path Comparison Tool
Compare IMU-based PDR against GPS references (Garmin eTrex 30x + Geo Tracker)
for 5 walk segments captured simultaneously on all 3 devices.

Usage:
    python compare_paths.py          # interactive (shows plots)
    python compare_paths.py --batch  # non-interactive (saves to analysis/)

Requires: numpy, scipy, matplotlib
"""

import os, sys, struct
import numpy as np
import matplotlib
from matplotlib import pyplot as plt
from matplotlib.widgets import Slider, Button, RadioButtons
import xml.etree.ElementTree as ET
from scipy.signal import find_peaks

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SEGMENTS = 5
BATCH_MODE = '--batch' in sys.argv
if BATCH_MODE:
    matplotlib.use('Agg')

# ─────────────────────────────────────────────
# 1. BIN Parser (45-byte old format, no CRC)
# ─────────────────────────────────────────────

def parse_bin(filepath):
    """Parse 45-byte old-format BIN. Returns dict with arrays."""
    with open(filepath, 'rb') as f:
        raw = f.read()
    n = len(raw) // 45
    frame = np.frombuffer(raw, dtype=np.uint8).reshape(n, 45)

    frame_seq = np.array([struct.unpack('<I', frame[i, 0:4])[0] for i in range(n)], dtype=np.uint32)
    timestamp = np.array([struct.unpack('<Q', frame[i, 4:12])[0] for i in range(n)], dtype=np.uint64)
    event_flag = frame[:, 12].copy()

    q = np.zeros((n, 4), dtype=np.float32)
    accel = np.zeros((n, 3), dtype=np.float32)
    temp = np.zeros(n, dtype=np.float32)

    for i in range(n):
        q[i] = struct.unpack('<4f', frame[i, 13:29].tobytes())
        accel[i] = struct.unpack('<3f', frame[i, 29:41].tobytes())
        temp[i] = struct.unpack('<f', frame[i, 41:45].tobytes())[0]

    return {
        'frame_seq': frame_seq,
        'timestamp': timestamp,
        'event_flag': event_flag,
        'q': q,
        'accel': accel,
        'temp': temp,
        'fs_hz': 100.0
    }


# ─────────────────────────────────────────────
# 2. PDR Pipeline
# ─────────────────────────────────────────────

def quaternion_to_yaw(qw, qx, qy, qz):
    """Yaw angle in radians (Madgwick convention)."""
    siny = 2.0 * (qw * qz + qx * qy)
    cosy = 1.0 - 2.0 * (qy * qy + qz * qz)
    return np.arctan2(siny, cosy)


def compute_pdr(data, k=0.4, heading_offset_deg=0, peak_min_height=3.0,
                peak_min_distance=30, peak_prominence=2.0):
    """
    Basic PDR: step detection via accel peaks + Weinberg step length + quaternion heading.
    Returns (x_m, y_m, step_indices, step_lengths, step_headings).
    """
    # Convert accel from G to m/s^2 for Weinberg formula (expects m/s^2)
    accel_mag = np.sqrt(np.sum(data['accel'] ** 2, axis=1)) * 9.81

    peaks, props = find_peaks(
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

    steps_x = []
    steps_y = []
    step_lengths = []
    step_headings = []
    running_x, running_y = 0.0, 0.0

    for pk_idx in range(len(peaks)):
        peak_i = peaks[pk_idx]

        # Find preceding valley: min between previous peak+5 and this peak-5
        prev_boundary = (peaks[pk_idx - 1] + 5) if pk_idx > 0 else max(0, peak_i - 100)
        search_start = prev_boundary
        search_end = peak_i - 2
        if search_end <= search_start:
            valley_val = accel_mag[peak_i] * 0.5
        else:
            valley_i = search_start + np.argmin(accel_mag[search_start:search_end])
            valley_val = accel_mag[valley_i]

        peak_val = accel_mag[peak_i]
        step_len = k * (peak_val - max(valley_val, 0.1)) ** 0.25

        step_yaw = yaw[peak_i] + heading_rad

        running_x += step_len * np.sin(step_yaw)
        running_y += step_len * np.cos(step_yaw)

        steps_x.append(running_x)
        steps_y.append(running_y)
        step_lengths.append(step_len)
        step_headings.append(step_yaw)

    return (np.array([0.0] + steps_x), np.array([0.0] + steps_y),
            peaks, np.array(step_lengths), np.array(step_headings))


# ─────────────────────────────────────────────
# 3. GPX Parsers
# ─────────────────────────────────────────────

GPX_NS = {'gpx': 'http://www.topografix.com/GPX/1/1'}


def parse_gpx(filepath):
    """Parse a GPX file. Returns dict of arrays (lat, lon, ele, time_str)."""
    tree = ET.parse(filepath)
    root = tree.getroot()
    pts = root.findall('.//gpx:trkpt', GPX_NS)

    lat = np.array([float(p.get('lat')) for p in pts])
    lon = np.array([float(p.get('lon')) for p in pts])
    ele = np.zeros(len(pts))
    time_str = []

    for i, p in enumerate(pts):
        e = p.find('gpx:ele', GPX_NS)
        ele[i] = float(e.text) if e is not None else 0.0
        t = p.find('gpx:time', GPX_NS)
        time_str.append(t.text if t is not None else '')

    return {'lat': lat, 'lon': lon, 'ele': ele, 'time': time_str}


def parse_gpx_geotracker(filepath):
    """Parse Geo Tracker GPX with accuracy/speed metadata."""
    result = parse_gpx(filepath)
    tree = ET.parse(filepath)
    pts = tree.findall('.//gpx:trkpt', GPX_NS)

    accuracy = np.full(len(pts), np.nan)
    speed = np.full(len(pts), np.nan)

    for i, p in enumerate(pts):
        meta = p.find('geotracker:meta',
                       {'geotracker': 'http://ilyabogdanovich.com/gpx/extensions/geotracker'})
        if meta is not None:
            c = meta.get('c')
            s = meta.get('s')
            if c:
                try:
                    accuracy[i] = float(c)
                except ValueError:
                    pass
            if s:
                try:
                    speed[i] = float(s)
                except ValueError:
                    pass

    result['accuracy'] = accuracy
    result['speed'] = speed
    return result


def gpx_to_local(lat, lon):
    """Convert lat/lon to local meters (equirectangular, centered at start)."""
    lat0, lon0 = np.radians(lat[0]), np.radians(lon[0])
    lat_r = np.radians(lat)
    lon_r = np.radians(lon)
    R = 6371000.0
    cos_lat0 = np.cos(lat0)
    x = R * (lon_r - lon0) * cos_lat0
    y = R * (lat_r - lat0)
    return x, y


def compute_path_length(x, y):
    """Cumulative path length. x,y in meters."""
    if len(x) < 2:
        return 0.0
    diffs = np.sqrt(np.diff(x) ** 2 + np.diff(y) ** 2)
    return np.sum(diffs)


def compute_gps_distance(lat, lon):
    """Haversine distance along a GPS track in meters."""
    if len(lat) < 2:
        return 0.0
    lat_r = np.radians(lat)
    lon_r = np.radians(lon)
    R = 6371000.0
    dlat = np.diff(lat_r)
    dlon = np.diff(lon_r)
    a = np.sin(dlat / 2) ** 2 + np.cos(lat_r[:-1]) * np.cos(lat_r[1:]) * np.sin(dlon / 2) ** 2
    c = 2 * np.arcsin(np.sqrt(np.clip(a, 0, 1)))
    return np.sum(R * c)


# ─────────────────────────────────────────────
# 4. Path Alignment
# ─────────────────────────────────────────────

def apply_heading_offset(x, y, angle_deg):
    """
    Apply heading offset to a path in navigation convention.
    (yaw=0=north, x=east, y=north).
    Adding heading h transforms: X' = X*cos(h) + Y*sin(h), Y' = Y*cos(h) - X*sin(h).
    """
    dx = x - x[0]
    dy = y - y[0]
    rad = np.radians(angle_deg)
    c, s = np.cos(rad), np.sin(rad)
    x_rot = x[0] + dx * c + dy * s
    y_rot = y[0] + dy * c - dx * s
    return x_rot, y_rot


def find_best_heading(pdr_x, pdr_y, ref_x, ref_y):
    """
    Brute-force search (0-360 deg, 0.5 deg steps) for heading offset
    that minimizes end-point distance from PDR to reference.
    Uses navigation convention (yaw=0=north, x=east, y=north).
    Returns (best_angle_deg, best_error_m).
    """
    if len(pdr_x) < 2 or len(ref_x) < 2:
        return 0.0, 9999.0

    ref_end = np.array([ref_x[-1], ref_y[-1]])
    pdr_dx = pdr_x[-1] - pdr_x[0]
    pdr_dy = pdr_y[-1] - pdr_y[0]
    pdr_len = np.sqrt(pdr_dx ** 2 + pdr_dy ** 2)

    if pdr_len < 0.1:
        return 0.0, np.linalg.norm(ref_end)

    best_angle = 0.0
    best_error = 9999.0

    for angle in np.arange(0.0, 360.0, 0.5):
        rad = np.radians(angle)
        rot_dx = pdr_dx * np.cos(rad) + pdr_dy * np.sin(rad)
        rot_dy = pdr_dy * np.cos(rad) - pdr_dx * np.sin(rad)
        rotated_end = np.array([rot_dx, rot_dy])
        error = np.linalg.norm(rotated_end - ref_end)
        if error < best_error:
            best_error = error
            best_angle = angle

    return best_angle, best_error


# ─────────────────────────────────────────────
# 5. Per-Segment Analysis
# ─────────────────────────────────────────────

class SegmentAnalysis:
    """Run analysis on one walk segment."""

    def __init__(self, seg_id):
        self.seg_id = seg_id
        self.bin_data = None
        self.garmin = None
        self.geo = None
        self.pdr_x = None
        self.pdr_y = None
        self.step_indices = None
        self.step_lengths = None
        self.step_headings = None
        self.gar_x, self.gar_y = None, None
        self.geo_x, self.geo_y = None, None
        self.metrics = {}

    def load(self, data_dir):
        # DeadReckoner
        bin_path = os.path.join(data_dir, 'deadreckoner', f'DR_LOG_00{self.seg_id}.BIN')
        self.bin_data = parse_bin(bin_path)

        # Garmin
        gar_path = os.path.join(data_dir, 'Garmin', f'{self.seg_id}.gpx')
        self.garmin = parse_gpx(gar_path)
        self.gar_x, self.gar_y = gpx_to_local(self.garmin['lat'], self.garmin['lon'])

        # GeoTracker
        geo_path = os.path.join(data_dir, 'GeoTracker', f'{self.seg_id}.gpx')
        self.geo = parse_gpx_geotracker(geo_path)
        self.geo_x, self.geo_y = gpx_to_local(self.geo['lat'], self.geo['lon'])

    def run_pdr(self, k=0.4, heading_offset_deg=0):
        result = compute_pdr(self.bin_data, k=k, heading_offset_deg=heading_offset_deg)
        self.pdr_x, self.pdr_y = result[0], result[1]
        self.step_indices = result[2]
        self.step_lengths = result[3]
        self.step_headings = result[4]
        return result

    def compute_metrics(self):
        pdr_len = compute_path_length(self.pdr_x, self.pdr_y)
        gar_len = compute_gps_distance(self.garmin['lat'], self.garmin['lon'])
        geo_len = compute_gps_distance(self.geo['lat'], self.geo['lon'])
        gps_avg = (gar_len + geo_len) / 2.0

        gps_end = np.array([
            (self.gar_x[-1] + self.geo_x[-1]) / 2.0,
            (self.gar_y[-1] + self.geo_y[-1]) / 2.0
        ])
        pdr_end = np.array([self.pdr_x[-1], self.pdr_y[-1]])

        # End-point error after auto-alignment
        # Use Garmin as heading reference (more reliable GPS source)
        best_angle, _ = find_best_heading(
            self.pdr_x, self.pdr_y,
            np.array([0] + list(self.gar_x)),
            np.array([0] + list(self.gar_y))
        )
    # Recompute with best angle
        self.run_pdr(k=self._last_k if hasattr(self, '_last_k') else 0.4,
                     heading_offset_deg=best_angle)
        pdr_rot_end = np.array([self.pdr_x[-1], self.pdr_y[-1]])
        endpoint_error = np.linalg.norm(pdr_rot_end - gps_end)

        pdr_err_pct = abs(pdr_len - gps_avg) / gps_avg * 100 if gps_avg > 0 else 0

        self.metrics = {
            'segment': self.seg_id,
            'pdr_distance_m': pdr_len,
            'garmin_distance_m': gar_len,
            'geotracker_distance_m': geo_len,
            'gps_avg_distance_m': gps_avg,
            'pdr_error_pct': pdr_err_pct,
            'endpoint_error_m': endpoint_error,
            'n_steps': len(self.step_lengths),
            'best_heading_offset': best_angle,
            'n_frames': len(self.bin_data['q']),
        }
        return self.metrics


# ─────────────────────────────────────────────
# 6. Visualization
# ─────────────────────────────────────────────

def plot_segment(ax, seg_id, pdr_x, pdr_y, gar_x, gar_y, geo_x, geo_y, metrics, color_pdr='blue'):
    ax.set_aspect('equal')
    ax.plot(gar_x, gar_y, '-', color='orange', linewidth=2, label='Garmin eTrex 30x', alpha=0.8)
    ax.plot(geo_x, geo_y, '-', color='green', linewidth=2, label='Geo Tracker', alpha=0.8)
    ax.plot(pdr_x, pdr_y, '-', color=color_pdr, linewidth=2, label='DeadReckoner PDR', alpha=0.9)

    ax.scatter(gar_x[0], gar_y[0], c='orange', marker='o', s=60, zorder=5, edgecolors='black')
    ax.scatter(geo_x[0], geo_y[0], c='green', marker='o', s=60, zorder=5, edgecolors='black')
    ax.scatter(pdr_x[0], pdr_y[0], c=color_pdr, marker='o', s=60, zorder=5, edgecolors='black')
    ax.scatter(gar_x[-1], gar_y[-1], c='orange', marker='x', s=80, zorder=5, linewidths=2)
    ax.scatter(geo_x[-1], geo_y[-1], c='green', marker='x', s=80, zorder=5, linewidths=2)
    ax.scatter(pdr_x[-1], pdr_y[-1], c=color_pdr, marker='x', s=80, zorder=5, linewidths=2)

    mid_x = (max(gar_x.max(), geo_x.max(), pdr_x.max()) + min(gar_x.min(), geo_x.min(), pdr_x.min())) / 2
    mid_y = (max(gar_y.max(), geo_y.max(), pdr_y.max()) + min(gar_y.min(), geo_y.min(), pdr_y.min())) / 2
    max_extent = max(
        abs(max(np.ptp(gar_x), np.ptp(geo_x), np.ptp(pdr_x))),
        abs(max(np.ptp(gar_y), np.ptp(geo_y), np.ptp(pdr_y)))
    ) * 0.6
    if max_extent > 1:
        ax.set_xlim(mid_x - max_extent, mid_x + max_extent)
        ax.set_ylim(mid_y - max_extent, mid_y + max_extent)

    ax.set_xlabel('East (m)')
    ax.set_ylabel('North (m)')
    ax.set_title(f'Segment {seg_id}')
    ax.grid(True, alpha=0.3)
    ax.legend(fontsize=8)

    # Text box with metrics
    if metrics:
        txt = (f'PDR: {metrics["pdr_distance_m"]:.0f}m  '
               f'GPS avg: {metrics["gps_avg_distance_m"]:.0f}m\n'
               f'Error: {metrics["pdr_error_pct"]:.1f}%  '
               f'Endpoint: {metrics["endpoint_error_m"]:.1f}m\n'
               f'Steps: {metrics["n_steps"]}  '
               f'Heading: {metrics["best_heading_offset"]:.0f}°')
        ax.text(0.05, 0.95, txt, transform=ax.transAxes, fontsize=7,
                verticalalignment='top',
                bbox=dict(boxstyle='round,pad=0.3', facecolor='white', alpha=0.8))


def plot_summary(all_metrics):
    fig, axes = plt.subplots(2, 3, figsize=(16, 10))
    fig.suptitle('DeadReckoner PDR vs GPS — Summary Across All 5 Segments', fontsize=14, fontweight='bold')

    ax_dist = axes[0, 0]
    ax_error = axes[0, 1]
    ax_endpt = axes[0, 2]
    ax_ratio = axes[1, 0]
    ax_steps = axes[1, 1]
    axes[1, 2].axis('off')

    segs = [m['segment'] for m in all_metrics]
    labels = [f'S{m}' for m in segs]

    # Distance comparison
    x = np.arange(len(segs))
    w = 0.25
    ax_dist.bar(x - w, [m['pdr_distance_m'] for m in all_metrics], w, label='PDR', color='blue', alpha=0.7)
    ax_dist.bar(x, [m['garmin_distance_m'] for m in all_metrics], w, label='Garmin', color='orange', alpha=0.7)
    ax_dist.bar(x + w, [m['geotracker_distance_m'] for m in all_metrics], w, label='Geo Tracker', color='green', alpha=0.7)
    ax_dist.set_xticks(x)
    ax_dist.set_xticklabels(labels)
    ax_dist.set_ylabel('Distance (m)')
    ax_dist.set_title('Total Path Length')
    ax_dist.legend(fontsize=8)
    ax_dist.grid(True, alpha=0.3, axis='y')

    # PDR error %
    colors = ['green' if m['pdr_error_pct'] < 10 else 'orange' if m['pdr_error_pct'] < 20 else 'red'
              for m in all_metrics]
    bars = ax_error.bar(x, [m['pdr_error_pct'] for m in all_metrics], color=colors, alpha=0.7)
    ax_error.axhline(y=10, color='green', linestyle='--', alpha=0.5, label='10% threshold')
    ax_error.axhline(y=20, color='red', linestyle='--', alpha=0.5, label='20% threshold')
    ax_error.set_xticks(x)
    ax_error.set_xticklabels(labels)
    ax_error.set_ylabel('Error (%)')
    ax_error.set_title('PDR Distance Error vs GPS Avg')
    ax_error.legend(fontsize=8)
    ax_error.grid(True, alpha=0.3, axis='y')

    for bar, val in zip(bars, [m['pdr_error_pct'] for m in all_metrics]):
        ax_error.text(bar.get_x() + bar.get_width() / 2, bar.get_height() + 0.5,
                      f'{val:.1f}%', ha='center', va='bottom', fontsize=8)

    # Endpoint error
    bars2 = ax_endpt.bar(x, [m['endpoint_error_m'] for m in all_metrics], color='purple', alpha=0.7)
    ax_endpt.set_xticks(x)
    ax_endpt.set_xticklabels(labels)
    ax_endpt.set_ylabel('Error (m)')
    ax_endpt.set_title('End-Point Displacement (Post-Alignment)')
    ax_endpt.grid(True, alpha=0.3, axis='y')

    for bar, val in zip(bars2, [m['endpoint_error_m'] for m in all_metrics]):
        ax_endpt.text(bar.get_x() + bar.get_width() / 2, bar.get_height() + 0.5,
                      f'{val:.1f}m', ha='center', va='bottom', fontsize=8)

    # Ratio plot: PDR/GPS per segment
    ratios = [m['pdr_distance_m'] / m['gps_avg_distance_m'] if m['gps_avg_distance_m'] > 0 else 0
              for m in all_metrics]
    ax_ratio.plot(x, ratios, '-o', color='blue', linewidth=2, markersize=8)
    ax_ratio.axhline(y=1.0, color='gray', linestyle='--', alpha=0.5, label='Ideal (1.0)')
    ax_ratio.set_xticks(x)
    ax_ratio.set_xticklabels(labels)
    ax_ratio.set_ylabel('PDR / GPS Ratio')
    ax_ratio.set_title('Distance Ratio (PDR ÷ GPS Avg)')
    ax_ratio.set_ylim(0, max(2.0, max(ratios) * 1.2))
    ax_ratio.legend(fontsize=8)
    ax_ratio.grid(True, alpha=0.3)

    # Step count
    ax_steps.bar(x, [m['n_steps'] for m in all_metrics], color='teal', alpha=0.7)
    ax_steps.set_xticks(x)
    ax_steps.set_xticklabels(labels)
    ax_steps.set_ylabel('Steps Detected')
    ax_steps.set_title('PDR Step Count')
    ax_steps.grid(True, alpha=0.3, axis='y')

    # Overall stats box
    errors = [m['pdr_error_pct'] for m in all_metrics]
    avg_err = np.mean(errors)
    max_err = np.max(errors)
    endpoints = [m['endpoint_error_m'] for m in all_metrics]
    avg_end = np.mean(endpoints)

    total_pdr = sum(m['pdr_distance_m'] for m in all_metrics)
    total_gps = sum(m['gps_avg_distance_m'] for m in all_metrics)
    total_err = abs(total_pdr - total_gps) / total_gps * 100 if total_gps > 0 else 0

    verdict = 'IMU-ONLY PDR IS SUFFICIENT' if (avg_err < 10 and total_err < 10) else \
              'Borderline — GPS recommended for absolute position' if (avg_err < 20) else \
              'GPS STRONGLY RECOMMENDED'

    verdict_color = 'green' if 'SUFFICIENT' in verdict else 'orange' if 'Borderline' in verdict else 'red'

    fig.text(0.35, 0.02,
             f'Overall: avg error {avg_err:.1f}%  |  max error {max_err:.1f}%  |  '
             f'avg endpoint displacement {avg_end:.1f}m  |  total error {total_err:.1f}%\n'
             f'Verdict: {verdict}',
             fontsize=11, fontweight='bold', color=verdict_color,
             bbox=dict(boxstyle='round,pad=0.5', facecolor='lightyellow', alpha=0.9))

    plt.tight_layout(rect=[0, 0.06, 1, 0.95])
    out_dir = os.path.join(SCRIPT_DIR, 'analysis')
    os.makedirs(out_dir, exist_ok=True)
    plt.savefig(os.path.join(out_dir, 'summary_comparison.png'), dpi=200, bbox_inches='tight')
    print(f'[+] Saved summary plot to analysis/summary_comparison.png')
    if not BATCH_MODE:
        plt.show()


# ─────────────────────────────────────────────
# 7. Interactive Viewer
# ─────────────────────────────────────────────

def run_interactive(analyses):
    """Create figure with per-segment plots + master slider for K and heading offset."""
    fig, axes = plt.subplots(2, 3, figsize=(18, 12))
    fig.suptitle('DeadReckoner PDR vs Garmin vs Geo Tracker — Interactive Tuning (v2.2)',
                 fontsize=13, fontweight='bold')
    axes_flat = axes.flatten()

    pdr_colors = ['blue', 'darkviolet', 'darkred', 'darkcyan', 'saddlebrown']

    # Initial PDR
    initial_k = 0.4
    for sa in analyses:
        sa.run_pdr(k=initial_k, heading_offset_deg=sa.metrics.get('best_heading_offset', 0))
        sa.compute_metrics()

    # Plot all segments
    lines_pdr = []
    for i, sa in enumerate(analyses):
        ax = axes_flat[i]
        plot_segment(ax, sa.seg_id, sa.pdr_x, sa.pdr_y,
                     sa.gar_x, sa.gar_y, sa.geo_x, sa.geo_y,
                     sa.metrics, color_pdr=pdr_colors[i])
        lines_pdr.append(ax.lines[2])  # the PDR line
    axes_flat[5].axis('off')

    plt.subplots_adjust(left=0.05, right=0.95, bottom=0.16, top=0.90, hspace=0.35, wspace=0.30)

    # Slider: Weinberg K
    ax_k = plt.axes([0.15, 0.04, 0.35, 0.025])
    slider_k = Slider(ax_k, 'Weinberg K', 0.05, 1.0, valinit=initial_k, valstep=0.005)

    # Slider preview text
    ax_info = plt.axes([0.55, 0.04, 0.40, 0.025])
    ax_info.axis('off')
    info_text = ax_info.text(0, 0.5, '', fontsize=9, verticalalignment='center')

    def update(val):
        k = slider_k.val
        total_pdr_dist = 0
        total_gps_dist = 0
        all_errs = []
        for sa in analyses:
            sa.run_pdr(k=k, heading_offset_deg=sa.metrics.get('best_heading_offset', 0))
            sa.compute_metrics()
            # Update plot
            axes_flat[sa.seg_id - 1].lines[2].set_data(sa.pdr_x, sa.pdr_y)
            axes_flat[sa.seg_id - 1].lines[2].axes.collections.clear()
            axes_flat[sa.seg_id - 1].lines[2].axes.scatter(
                sa.pdr_x[-1], sa.pdr_y[-1], c=pdr_colors[sa.seg_id - 1],
                marker='x', s=80, zorder=5, linewidths=2)
            axes_flat[sa.seg_id - 1].lines[2].axes.scatter(
                sa.pdr_x[0], sa.pdr_y[0], c=pdr_colors[sa.seg_id - 1],
                marker='o', s=60, zorder=5, edgecolors='black')
            total_pdr_dist += sa.metrics['pdr_distance_m']
            total_gps_dist += sa.metrics['gps_avg_distance_m']
            all_errs.append(sa.metrics['pdr_error_pct'])
        total_err_pct = abs(total_pdr_dist - total_gps_dist) / total_gps_dist * 100 if total_gps_dist > 0 else 0
        avg_err = np.mean(all_errs) if all_errs else 0
        info_text.set_text(
            f'K={k:.3f}  |  Avg error: {avg_err:.1f}%  |  Total error: {total_err_pct:.1f}%  |  '
            f'Total PDR: {total_pdr_dist:.0f}m  |  GPS: {total_gps_dist:.0f}m')
        fig.canvas.draw_idle()

    slider_k.on_changed(update)

    # Reset button
    ax_reset = plt.axes([0.05, 0.04, 0.06, 0.03])
    btn_reset = Button(ax_reset, 'Reset K')
    def reset(event):
        slider_k.reset()
    btn_reset.on_clicked(reset)

    # Initial info
    update(initial_k)

    out_dir = os.path.join(SCRIPT_DIR, 'analysis')
    os.makedirs(out_dir, exist_ok=True)
    plt.savefig(os.path.join(out_dir, 'segment_comparison.png'), dpi=200, bbox_inches='tight')
    print(f'[+] Saved segment plot to analysis/segment_comparison.png')
    if not BATCH_MODE:
        plt.show()


# ─────────────────────────────────────────────
# 8. Main
# ─────────────────────────────────────────────

def print_table(analyses):
    print()
    print('=' * 100)
    print(f'{"Segment":>8} {"PDR (m)":>10} {"Garmin (m)":>12} {"GeoTr (m)":>12} '
          f'{"GPS avg":>10} {"Error %":>8} {"Endpt (m)":>10} {"Steps":>7} {"Frames":>8}')
    print('-' * 100)

    total_pdr = 0
    total_gar = 0
    total_geo = 0
    all_errors = []

    for sa in analyses:
        m = sa.metrics
        total_pdr += m['pdr_distance_m']
        total_gar += m['garmin_distance_m']
        total_geo += m['geotracker_distance_m']
        all_errors.append(m['pdr_error_pct'])
        print(f'{m["segment"]:>8} {m["pdr_distance_m"]:>10.1f} {m["garmin_distance_m"]:>12.1f} '
              f'{m["geotracker_distance_m"]:>12.1f} {m["gps_avg_distance_m"]:>10.1f} '
              f'{m["pdr_error_pct"]:>7.1f}% {m["endpoint_error_m"]:>9.1f} '
              f'{m["n_steps"]:>7} {m["n_frames"]:>8}')

    print('-' * 100)
    total_gps_avg = (total_gar + total_geo) / 2
    total_err = abs(total_pdr - total_gps_avg) / total_gps_avg * 100 if total_gps_avg > 0 else 0
    avg_err = np.mean(all_errors)
    max_err = np.max(all_errors)
    print(f'{"TOTAL":>8} {total_pdr:>10.1f} {total_gar:>12.1f} {total_geo:>12.1f} '
          f'{total_gps_avg:>10.1f} {total_err:>7.1f}%')
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
    print('=' * 100)
    print()


def main():
    data_dir = SCRIPT_DIR

    print()
    print('  DEADRECKONER PATH COMPARISON TOOL v2.2')
    print('  Comparing PDR vs Garmin eTrex 30x vs Geo Tracker')
    print(f'  Data: {data_dir}')
    print()

    # Load all segments
    analyses = []
    for seg_id in range(1, SEGMENTS + 1):
        print(f'  Loading segment {seg_id}...', end=' ')
        sa = SegmentAnalysis(seg_id)
        sa.load(data_dir)
        analyses.append(sa)
        print(f'BIN: {len(sa.bin_data["q"]):,} frames, '
              f'Garmin: {len(sa.garmin["lat"])} pts, '
              f'Geo: {len(sa.geo["lat"])} pts')

    # Initial PDR with default K
    initial_k = 0.4
    print(f'\n  Running PDR (K={initial_k})...')
    for sa in analyses:
        sa.run_pdr(k=initial_k)

    # Compute metrics with auto heading alignment
    print('  Computing metrics (auto heading alignment)...')
    for sa in analyses:
        sa.compute_metrics()

    # Print results
    print_table(analyses)

    # Generate summary plot
    print('  Generating summary plot...')
    all_metrics = [sa.metrics for sa in analyses]
    plot_summary(all_metrics)

    if not BATCH_MODE:
        print('  Starting interactive viewer (adjust K slider)...')
        run_interactive(analyses)
    else:
        print('  Batch mode: saving segment plot...')
        analyses[0].run_pdr(k=initial_k)
        analyses[0].compute_metrics()
        fig2, ax2 = plt.subplots(figsize=(10, 8))
        plot_segment(ax2, '1-5 (overlay)',
                     np.concatenate([sa.pdr_x for sa in analyses]),
                     np.concatenate([sa.pdr_y for sa in analyses]),
                     np.concatenate([sa.gar_x for sa in analyses]),
                     np.concatenate([sa.gar_y for sa in analyses]),
                     np.concatenate([sa.geo_x for sa in analyses]),
                     np.concatenate([sa.geo_y for sa in analyses]),
                     None, color_pdr='blue')
        out_dir = os.path.join(SCRIPT_DIR, 'analysis')
        os.makedirs(out_dir, exist_ok=True)
        fig2.savefig(os.path.join(out_dir, 'all_segments_overlay.png'), dpi=200, bbox_inches='tight')
        plt.close(fig2)
        print(f'[+] Saved overlay to analysis/all_segments_overlay.png')

    print()
    print('  Done. Check analysis/ for saved plots.')
    print()


if __name__ == '__main__':
    main()
