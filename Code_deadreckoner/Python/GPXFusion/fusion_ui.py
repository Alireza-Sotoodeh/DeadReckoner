#!/usr/bin/env python3
"""
GPX Fusion Tool — Fuse Garmin eTrex 30x and Geo Tracker app GPX logs
into a single high-accuracy path using a Kalman filter with RTS smoothing.

Usage:
    python fusion_ui.py
"""

import sys
import os
import xml.etree.ElementTree as ET
# Add these configurations right after imports to fix the Compositor error
os.environ["QTWEBENGINE_DISABLE_GPU"] = "1"
os.environ["QT_ENABLE_HIGHDPI_SCALING"] = "1"
import urllib.request
import ssl
from datetime import datetime, timezone
from typing import List, Dict, Optional, Tuple
from collections import OrderedDict

import numpy as np
import pandas as pd
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.figure import Figure
from matplotlib.backends.backend_agg import FigureCanvasAgg
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QSplitter, QTabWidget, QListWidget, QListWidgetItem, QPushButton,
    QLabel, QFileDialog, QMessageBox, QGroupBox, QSlider, QComboBox,
    QStatusBar, QProgressBar, QTextEdit, QCheckBox, QGridLayout,
    QFrame, QScrollArea, QSpinBox, QDoubleSpinBox, QStackedWidget,
    QDialog, QLineEdit, QDialogButtonBox
)
from PyQt6.QtCore import Qt, QUrl, QTimer, QCoreApplication, pyqtSignal
from PyQt6.QtGui import QFont, QAction, QColor, QPalette, QPixmap, QImage

try:
    import gpxpy
    import gpxpy.gpx
except ImportError:
    gpxpy = None

try:
    import folium
except ImportError:
    folium = None

try:
    from PyQt6.QtWebEngineWidgets import QWebEngineView
    _HAS_WEBENGINE = True
except ImportError:
    _HAS_WEBENGINE = False


LEAFLET_URL = 'https://cdn.jsdelivr.net/npm/leaflet@1.9.3/dist/leaflet.js'


TRACK_COLORS = [
    '#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd',
    '#8c564b', '#e377c2', '#7f7f7f', '#bcbd22', '#17becf'
]

GEO_NS = {'gt': 'http://ilyabogdanovich.com/gpx/extensions/geotracker'}


def parse_gpx(filepath: str) -> pd.DataFrame:
    filename = os.path.basename(filepath)
    source = 'geotracker' if 'geotracker' in filename.lower() or 'geo' in filename.lower() else 'garmin'

    with open(filepath, 'r', encoding='utf-8') as f:
        gpx = gpxpy.parse(f)

    rows = []
    for track in gpx.tracks:
        for seg in track.segments:
            for pt in seg.points:
                accuracy = np.nan
                speed = np.nan

                if source == 'geotracker' and pt.extensions:
                    for ext in pt.extensions:
                        tag = ext.tag
                        if 'geotracker' in tag and 'meta' in tag:
                            c = ext.get('c')
                            s = ext.get('s')
                            if c:
                                accuracy = float(c)
                            if s:
                                speed = float(s)

                rows.append({
                    'time': pt.time,
                    'lat': pt.latitude,
                    'lon': pt.longitude,
                    'ele': pt.elevation if pt.elevation is not None else np.nan,
                    'accuracy': accuracy,
                    'speed': speed,
                    'source': source,
                    'file': filename
                })

    df = pd.DataFrame(rows)
    if df.empty:
        raise ValueError(f"No track points found in {filename}")

    df = df.sort_values('time').reset_index(drop=True)
    return df


def latlon_to_local(lat: np.ndarray, lon: np.ndarray, lat0: float, lon0: float) -> Tuple[np.ndarray, np.ndarray]:
    R = 6371000.0
    lat0_r = np.radians(lat0)
    x = np.radians(lon - lon0) * R * np.cos(lat0_r)
    y = np.radians(lat - lat0) * R
    return x, y


def local_to_latlon(x: np.ndarray, y: np.ndarray, lat0: float, lon0: float) -> Tuple[np.ndarray, np.ndarray]:
    R = 6371000.0
    lat0_r = np.radians(lat0)
    lat = np.degrees(y / R) + lat0
    lon = np.degrees(x / (R * np.cos(lat0_r))) + lon0
    return lat, lon


def haversine(lat1: float, lon1: float, lat2: float, lon2: float) -> float:
    R = 6371000.0
    dlat = np.radians(lat2 - lat1)
    dlon = np.radians(lon2 - lon1)
    a = np.sin(dlat / 2) ** 2 + np.cos(np.radians(lat1)) * np.cos(np.radians(lat2)) * np.sin(dlon / 2) ** 2
    return R * 2 * np.arctan2(np.sqrt(a), np.sqrt(1 - a))


def interpolate_to_grid(dfs: List[pd.DataFrame]) -> Tuple[np.ndarray, List[pd.DataFrame]]:
    all_ts = []
    for df in dfs:
        all_ts.extend(df['time'].tolist())
    t_start = min(all_ts)
    t_end = max(all_ts)
    t_start_ts = t_start.timestamp()
    t_end_ts = t_end.timestamp()
    t_grid = np.arange(t_start_ts, t_end_ts + 0.5, 1.0)
    t_grid_dt = [datetime.fromtimestamp(t, tz=timezone.utc) for t in t_grid]

    interpolated = []
    for df in dfs:
        t_sec = np.array([t.timestamp() for t in df['time']])
        if len(df) < 2:
            continue
        interp_df = pd.DataFrame({'time': t_grid_dt})
        for col in ['lat', 'lon', 'ele']:
            valid = df[col].notna()
            if valid.sum() >= 2:
                interp_df[col] = np.interp(t_grid, t_sec[valid], df[col].values[valid], left=np.nan, right=np.nan)
            else:
                interp_df[col] = np.nan
        interp_df['accuracy'] = np.nan
        interp_df['source'] = df['source'].iloc[0]
        interp_df['file'] = df['file'].iloc[0]

        for i in range(len(t_grid_dt)):
            t_dt = t_grid_dt[i]
            nearest_idx = (df['time'] - t_dt).abs().argsort().iloc[0]
            nearest = df.iloc[nearest_idx]
            time_diff = abs((nearest['time'] - t_dt).total_seconds())
            if time_diff < 1.5:
                interp_df.loc[i, 'accuracy'] = nearest['accuracy']

        interpolated.append(interp_df)

    return t_grid, interpolated


def compute_statistics(df: pd.DataFrame) -> Dict:
    if df.empty:
        return {}
    coords = df[['lat', 'lon']].values
    dists = []
    for i in range(1, len(coords)):
        d = haversine(coords[i-1][0], coords[i-1][1], coords[i][0], coords[i][1])
        dists.append(d)
    total_dist = sum(dists)
    duration = (df['time'].iloc[-1] - df['time'].iloc[0]).total_seconds()
    return {
        'points': len(df),
        'distance_m': total_dist,
        'duration_s': duration,
        'avg_speed_ms': total_dist / duration if duration > 0 else 0,
        'ele_min': df['ele'].min() if 'ele' in df and df['ele'].notna().any() else np.nan,
        'ele_max': df['ele'].max() if 'ele' in df and df['ele'].notna().any() else np.nan,
    }


def stats_text(stats: Dict, label: str) -> str:
    if not stats:
        return f"{label}: No data"
    dist_km = stats['distance_m'] / 1000
    dur_min = stats['duration_s'] / 60
    return (f"{label}: {stats['points']} pts, {dist_km:.3f} km, "
            f"{dur_min:.1f} min, {stats['avg_speed_ms']:.2f} m/s")


class KalmanFilter:
    def __init__(self, dt: float = 1.0, process_noise: float = 0.3):
        self.dt = dt
        self.x = np.zeros((4, 1))
        self.P = np.eye(4) * 100.0
        self.F = np.array([
            [1, 0, dt, 0],
            [0, 1, 0, dt],
            [0, 0, 1, 0],
            [0, 0, 0, 1]
        ], dtype=float)
        self.H = np.array([
            [1, 0, 0, 0],
            [0, 1, 0, 0]
        ], dtype=float)

        q = process_noise ** 2
        dt2 = dt * dt
        dt3 = dt2 * dt / 2
        dt4 = dt2 * dt2 / 4
        self.Q = np.array([
            [dt4 * q, 0, dt3 * q, 0],
            [0, dt4 * q, 0, dt3 * q],
            [dt3 * q, 0, dt2 * q, 0],
            [0, dt3 * q, 0, dt2 * q]
        ], dtype=float)

    def predict(self):
        self.x = self.F @ self.x
        self.P = self.F @ self.P @ self.F.T + self.Q
        return self.x.copy(), self.P.copy()

    def update(self, z: np.ndarray, R: np.ndarray):
        y = z.reshape((2, 1)) - self.H @ self.x
        S = self.H @ self.P @ self.H.T + R
        K = self.P @ self.H.T @ np.linalg.inv(S)
        self.x = self.x + K @ y
        self.P = (np.eye(4) - K @ self.H) @ self.P
        return self.x.copy(), self.P.copy()


def rts_smoother(states: List[np.ndarray], covs: List[np.ndarray],
                 F: np.ndarray, Q: np.ndarray) -> Tuple[List[np.ndarray], List[np.ndarray]]:
    n = len(states)
    s_smooth = [None] * n
    p_smooth = [None] * n
    s_smooth[-1] = states[-1].copy()
    p_smooth[-1] = covs[-1].copy()

    for t in range(n - 2, -1, -1):
        P_pred = F @ covs[t] @ F.T + Q
        G = covs[t] @ F.T @ np.linalg.inv(P_pred)
        s_smooth[t] = states[t] + G @ (s_smooth[t + 1] - F @ states[t])
        p_smooth[t] = covs[t] + G @ (p_smooth[t + 1] - P_pred) @ G.T

    return s_smooth, p_smooth


def fuse_tracks(dfs: List[pd.DataFrame], process_noise: float = 0.3,
                geo_noise: float = 4.0, garmin_noise: float = 6.0,
                use_smoother: bool = True) -> Optional[pd.DataFrame]:
    if len(dfs) == 0:
        return None

    t_grid, interp_dfs = interpolate_to_grid(dfs)
    if len(interp_dfs) == 0:
        return None

    all_lats = [df['lat'].values for df in interp_dfs]
    all_lons = [df['lon'].values for df in interp_dfs]
    any_valid = np.zeros(len(t_grid), dtype=bool)
    for arr_lat, arr_lon in zip(all_lats, all_lons):
        any_valid |= ~(np.isnan(arr_lat) | np.isnan(arr_lon))

    if not any_valid.any():
        return None

    ref_lat = np.nanmean([df['lat'].iloc[0] for df in interp_dfs])
    ref_lon = np.nanmean([df['lon'].iloc[0] for df in interp_dfs])

    kf = KalmanFilter(dt=1.0, process_noise=process_noise)

    states = []
    covs = []
    timestamps = []
    n = len(t_grid)
    initialized = False

    for i in range(n):
        if not any_valid[i]:
            if initialized:
                sp, pp = kf.predict()
                states.append(sp)
                covs.append(pp)
                timestamps.append(t_grid[i])
            continue

        positions = []
        device_info = []
        for j, df in enumerate(interp_dfs):
            lat = df['lat'].values[i]
            lon = df['lon'].values[i]
            if np.isnan(lat) or np.isnan(lon):
                continue
            x, y = latlon_to_local(np.array([lat]), np.array([lon]), ref_lat, ref_lon)
            is_geo = df['source'].iloc[0] == 'geotracker'
            acc = df['accuracy'].values[i]
            positions.append(np.array([x[0], y[0]]))
            device_info.append((is_geo, acc))

        if len(positions) == 0:
            if initialized:
                sp, pp = kf.predict()
                states.append(sp)
                covs.append(pp)
                timestamps.append(t_grid[i])
            continue

        if not initialized:
            first_pos = positions[0]
            kf.x = np.array([[first_pos[0]], [first_pos[1]], [0], [0]])
            kf.P = np.eye(4) * 100.0
            initialized = True

        sp, pp = kf.predict()

        for pos, (is_geo, acc) in zip(positions, device_info):
            if is_geo:
                noise_val = max(acc, 2.0) if not np.isnan(acc) else geo_noise
            else:
                noise_val = garmin_noise
            z = np.array([pos[0], pos[1]])
            sp, pp = kf.update(z, np.eye(2) * (noise_val ** 2))

        states.append(sp)
        covs.append(pp)
        timestamps.append(t_grid[i])

    if len(states) == 0:
        return None

    if use_smoother and len(states) > 2:
        states, covs = rts_smoother(states, covs, kf.F, kf.Q)

    result_lats = []
    result_lons = []
    for s in states:
        lat, lon = local_to_latlon(np.array([s[0, 0]]), np.array([s[1, 0]]), ref_lat, ref_lon)
        result_lats.append(lat[0])
        result_lons.append(lon[0])

    result = pd.DataFrame({
        'time': [datetime.fromtimestamp(t, tz=timezone.utc) for t in timestamps],
        'lat': result_lats,
        'lon': result_lons,
    })
    return result


def generate_map(raw_tracks: List[Tuple[str, pd.DataFrame, str]],
                 fused: Optional[pd.DataFrame] = None,
                 fused_stats: Optional[Dict] = None,
                 zoom_factor: float = 1.0) -> 'QPixmap':
    from PyQt6.QtGui import QImage, QPixmap

    fig = Figure(figsize=(8, 6), dpi=100, facecolor='white')
    ax = fig.add_subplot(111)
    ax.set_facecolor('white')
    ax.xaxis.label.set_color('black')
    ax.yaxis.label.set_color('black')
    ax.title.set_color('black')
    ax.tick_params(colors='black')

    for i, (name, df, color) in enumerate(raw_tracks):
        if len(df) < 2:
            continue
        ax.plot(df['lon'].values, df['lat'].values, color=color, linewidth=1.5, alpha=0.7, label=name)
        ax.scatter(df['lon'].values[0], df['lat'].values[0], color=color, s=40, marker='o', zorder=5)
        ax.scatter(df['lon'].values[-1], df['lat'].values[-1], color=color, s=40, marker='s', zorder=5)

    if fused is not None and len(fused) > 1:
        ax.plot(fused['lon'].values, fused['lat'].values, color='red', linewidth=3, alpha=0.9, label='Fused')
        ax.scatter(fused['lon'].values[0], fused['lat'].values[0], color='lime', s=80, marker='o', edgecolors='darkgreen', linewidths=1.5, zorder=6)
        ax.scatter(fused['lon'].values[-1], fused['lat'].values[-1], color='red', s=80, marker='s', edgecolors='darkred', linewidths=1.5, zorder=6)

    all_lats = []
    all_lons = []
    for _, df, _ in raw_tracks:
        all_lats.extend(df['lat'].values)
        all_lons.extend(df['lon'].values)
    if fused is not None and len(fused) > 1:
        all_lats.extend(fused['lat'].values)
        all_lons.extend(fused['lon'].values)

    if all_lats:
        margin = max((max(all_lons) - min(all_lons)) * 0.1, 0.0005) / max(zoom_factor, 0.01)
        ax.set_xlim(min(all_lons) - margin, max(all_lons) + margin)
        ax.set_ylim(min(all_lats) - margin, max(all_lats) + margin)

    ax.set_xlabel('Longitude')
    ax.set_ylabel('Latitude')
    ax.set_title('GPS Track Comparison')
    ax.grid(True, alpha=0.3)
    ax.legend(fontsize=9)

    if fused_stats:
        dist_km = fused_stats['distance_m'] / 1000
        dur_min = fused_stats['duration_s'] / 60
        text = f'Fused: {dist_km:.3f} km, {dur_min:.1f} min, {fused_stats["points"]} pts'
        ax.text(0.02, 0.02, text, transform=ax.transAxes, fontsize=9,
                bbox=dict(boxstyle='round,pad=0.3', facecolor='wheat', alpha=0.7))

    fig.tight_layout()
    canvas = FigureCanvasAgg(fig)
    canvas.draw()

    buf = canvas.buffer_rgba()
    qimage = QImage(buf, buf.shape[1], buf.shape[0], QImage.Format.Format_RGBA8888)
    pixmap = QPixmap.fromImage(qimage)

    plt.close(fig)
    return pixmap


TILE_PROVIDERS = [
    "OpenStreetMap",
    "CartoDB positron",
    "CartoDB dark_matter",
    "Esri WorldImagery",
    "Esri WorldTopoMap",
]


TILE_ATTR = {
    "OpenStreetMap": "OpenStreetMap",
    "CartoDB positron": "CartoDB",
    "CartoDB dark_matter": "CartoDB",
    "Esri WorldImagery": "Esri",
    "Esri WorldTopoMap": "Esri",
}


def generate_map_folium(raw_tracks: List[Tuple[str, pd.DataFrame, str]],
                        fused: Optional[pd.DataFrame] = None,
                        fused_stats: Optional[Dict] = None,
                        tiles: str = "OpenStreetMap") -> str:
    if folium is None:
        return '<html><body><h3>folium not installed</h3></body></html>'

    if not raw_tracks:
        return '<html><body><h3>No tracks loaded</h3></body></html>'

    all_lats = []
    all_lons = []
    for _, df, _ in raw_tracks:
        all_lats.extend(df['lat'].values)
        all_lons.extend(df['lon'].values)
    center_lat = float(np.mean(all_lats))
    center_lon = float(np.mean(all_lons))

    attr = TILE_ATTR.get(tiles, tiles)
    m = folium.Map(location=[center_lat, center_lon], zoom_start=17,
                   tiles=tiles, attr=attr)

    for i, (name, df, color) in enumerate(raw_tracks):
        points = df[['lat', 'lon']].values.tolist()
        if len(points) < 2:
            continue
        folium.PolyLine(points, color=color, weight=2.5, opacity=0.7,
                        popup=name, tooltip=name).add_to(m)
        folium.CircleMarker(points[0], radius=6, color=color, fill=True,
                            popup=f"{name} - Start").add_to(m)
        folium.CircleMarker(points[-1], radius=6, color=color, fill=True,
                            fill_opacity=0.5,
                            popup=f"{name} - End").add_to(m)

    if fused is not None and len(fused) > 1:
        f_points = fused[['lat', 'lon']].values.tolist()
        folium.PolyLine(f_points, color='red', weight=5, opacity=0.95,
                        popup='Fused Path', tooltip='Fused Path').add_to(m)
        folium.CircleMarker(f_points[0], radius=8, color='darkgreen', fill=True,
                            fill_color='green', popup='Fused - Start').add_to(m)
        folium.CircleMarker(f_points[-1], radius=8, color='darkred', fill=True,
                            fill_color='red', popup='Fused - End').add_to(m)
        if fused_stats:
            dist_km = fused_stats['distance_m'] / 1000
            dur_min = fused_stats['duration_s'] / 60
            info = (f'<div style="position: fixed; top: 10px; right: 10px; z-index: 1000; '
                    f'background: white; padding: 8px 14px; border-radius: 6px; '
                    f'box-shadow: 0 0 8px rgba(0,0,0,0.3); font: 13px sans-serif;">'
                    f'<b>Fused Path</b><br>'
                    f'{dist_km:.3f} km | {dur_min:.1f} min<br>'
                    f'{fused_stats["points"]} pts')
            m.get_root().html.add_child(folium.Element(info))

    folium.LayerControl().add_to(m)
    return m.get_root().render()


def check_connectivity(proxy_host: Optional[str] = None,
                       proxy_port: Optional[int] = None) -> bool:
    try:
        ctx = ssl.create_default_context()
        ctx.check_hostname = False
        ctx.verify_mode = ssl.CERT_NONE
        https_handler = urllib.request.HTTPSHandler(context=ctx)
        req = urllib.request.Request(LEAFLET_URL, method='HEAD')
        handlers = [https_handler]
        if proxy_host and proxy_port:
            proxy_url = f'http://{proxy_host}:{proxy_port}'
            handlers.insert(0, urllib.request.ProxyHandler(
                {'http': proxy_url, 'https': proxy_url}))
        opener = urllib.request.build_opener(*handlers)
        resp = opener.open(req, timeout=5)
        return resp.status == 200
    except Exception:
        return False


class ProxyDialog(QDialog):
    def __init__(self, enabled: bool, host: str, port: int, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Proxy Settings")
        self.setMinimumWidth(350)

        layout = QVBoxLayout(self)

        self.enable_cb = QCheckBox("Enable Proxy")
        self.enable_cb.setChecked(enabled)
        layout.addWidget(self.enable_cb)

        form = QGridLayout()
        form.addWidget(QLabel("Host:"), 0, 0)
        self.host_edit = QLineEdit(host)
        self.host_edit.setPlaceholderText("proxy.example.com")
        form.addWidget(self.host_edit, 0, 1)

        form.addWidget(QLabel("Port:"), 1, 0)
        self.port_spin = QSpinBox()
        self.port_spin.setRange(1, 65535)
        self.port_spin.setValue(port)
        form.addWidget(self.port_spin, 1, 1)
        layout.addLayout(form)

        buttons = QDialogButtonBox(QDialogButtonBox.StandardButton.Ok |
                                   QDialogButtonBox.StandardButton.Cancel)
        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)
        layout.addWidget(buttons)

    def get_values(self):
        return (self.enable_cb.isChecked(),
                self.host_edit.text().strip(),
                self.port_spin.value())


class GroupData:
    def __init__(self, name: str):
        self.name = name
        self.files: List[Dict] = []
        self.fused: Optional[pd.DataFrame] = None
        self.fused_stats: Optional[Dict] = None

    def add_file(self, filepath: str) -> pd.DataFrame:
        df = parse_gpx(filepath)
        self.files.append({'path': filepath, 'name': os.path.basename(filepath), 'data': df})
        return df

    def remove_file(self, index: int):
        if 0 <= index < len(self.files):
            del self.files[index]
            self.fused = None
            self.fused_stats = None

    def get_raw_tracks(self) -> List[Tuple[str, pd.DataFrame, str]]:
        tracks = []
        for i, f in enumerate(self.files):
            color = TRACK_COLORS[i % len(TRACK_COLORS)]
            tracks.append((f['name'], f['data'], color))
        return tracks

    def clear_fused(self):
        self.fused = None
        self.fused_stats = None


class FileListWidget(QListWidget):
    files_changed = pyqtSignal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setAcceptDrops(True)

    def dragEnterEvent(self, event):
        if event.mimeData().hasUrls():
            event.acceptProposedAction()

    def dragMoveEvent(self, event):
        event.acceptProposedAction()

    def dropEvent(self, event):
        for url in event.mimeData().urls():
            path = url.toLocalFile()
            if path.lower().endswith('.gpx'):
                self.parent()._import_file(path)
        event.acceptProposedAction()


class GroupTab(QWidget):
    files_changed = pyqtSignal()
    fusion_requested = pyqtSignal(str)

    def __init__(self, group_name: str, parent=None):
        super().__init__(parent)
        self.group_name = group_name
        self.group_data = GroupData(group_name)
        self._setup_ui()

    def _setup_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(4, 4, 4, 4)

        self.stats_label = QLabel("No files loaded")
        self.stats_label.setWordWrap(True)
        layout.addWidget(self.stats_label)

        self.file_list = FileListWidget(self)
        self.file_list.setAlternatingRowColors(True)
        layout.addWidget(self.file_list)

        btn_layout = QHBoxLayout()
        self.add_btn = QPushButton("Add GPX Files")
        self.add_btn.clicked.connect(self._on_add)
        self.remove_btn = QPushButton("Remove Selected")
        self.remove_btn.clicked.connect(self._on_remove)
        self.remove_btn.setEnabled(False)
        btn_layout.addWidget(self.add_btn)
        btn_layout.addWidget(self.remove_btn)
        layout.addLayout(btn_layout)

        self.file_list.itemSelectionChanged.connect(
            lambda: self.remove_btn.setEnabled(len(self.file_list.selectedItems()) > 0))

        self.info_text = QTextEdit()
        self.info_text.setReadOnly(True)
        self.info_text.setMaximumHeight(80)
        self.info_text.setPlaceholderText("File info: select a file...")
        layout.addWidget(self.info_text)

        self.file_list.currentItemChanged.connect(self._on_select_file)

    def _on_add(self):
        default_dir = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'data')
        if not os.path.isdir(default_dir):
            default_dir = ""
        files, _ = QFileDialog.getOpenFileNames(
            self, f"Add GPX files to {self.group_name}",
            default_dir, "GPX Files (*.gpx);;All Files (*)")
        for f in files:
            self._import_file(f)

    def _import_file(self, filepath: str):
        try:
            df = self.group_data.add_file(filepath)
            item = QListWidgetItem(os.path.basename(filepath))
            item.setData(Qt.ItemDataRole.UserRole, filepath)
            self.file_list.addItem(item)
            self._update_stats()
            self.files_changed.emit()
        except Exception as e:
            QMessageBox.warning(self, "Parse Error", f"Could not parse {os.path.basename(filepath)}:\n{str(e)}")

    def _on_remove(self):
        items = self.file_list.selectedItems()
        for item in items:
            path = item.data(Qt.ItemDataRole.UserRole)
            idx = next((i for i, f in enumerate(self.group_data.files) if f['path'] == path), -1)
            if idx >= 0:
                self.group_data.remove_file(idx)
            self.file_list.takeItem(self.file_list.row(item))
        self._update_stats()
        self.files_changed.emit()

    def _on_select_file(self, current, previous):
        if current is None:
            self.info_text.clear()
            return
        path = current.data(Qt.ItemDataRole.UserRole)
        match = [f for f in self.group_data.files if f['path'] == path]
        if match:
            f = match[0]
            s = compute_statistics(f['data'])
            self.info_text.setText(stats_text(s, f['name']))

    def _update_stats(self):
        n = len(self.group_data.files)
        total_pts = sum(len(f['data']) for f in self.group_data.files)
        self.stats_label.setText(f"{n} file(s), {total_pts} total track points")
        self.group_data.clear_fused()

    def get_raw_tracks(self):
        return self.group_data.get_raw_tracks()

    def clear_fused(self):
        self.group_data.clear_fused()


class FuseControlPanel(QFrame):
    fuse_clicked = pyqtSignal()
    export_clicked = pyqtSignal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self._setup_ui()

    def _setup_ui(self):
        layout = QGridLayout(self)
        layout.setContentsMargins(8, 4, 8, 4)

        row = 0
        layout.addWidget(QLabel("Process Noise:"), row, 0)
        self.process_noise = QDoubleSpinBox()
        self.process_noise.setRange(0.01, 5.0)
        self.process_noise.setSingleStep(0.05)
        self.process_noise.setValue(0.3)
        self.process_noise.setToolTip("Higher = more responsive, lower = smoother")
        layout.addWidget(self.process_noise, row, 1)

        row += 1
        layout.addWidget(QLabel("Geo Tracker Noise (m):"), row, 0)
        self.geo_noise = QDoubleSpinBox()
        self.geo_noise.setRange(1.0, 20.0)
        self.geo_noise.setSingleStep(0.5)
        self.geo_noise.setValue(4.0)
        layout.addWidget(self.geo_noise, row, 1)

        row += 1
        layout.addWidget(QLabel("Garmin Noise (m):"), row, 0)
        self.garmin_noise = QDoubleSpinBox()
        self.garmin_noise.setRange(1.0, 30.0)
        self.garmin_noise.setSingleStep(0.5)
        self.garmin_noise.setValue(6.0)
        layout.addWidget(self.garmin_noise, row, 1)

        row += 1
        self.smoother_check = QCheckBox("RTS Smoother (better, offline)")
        self.smoother_check.setChecked(True)
        layout.addWidget(self.smoother_check, row, 0, 1, 2)

        row += 1
        btn_row = QHBoxLayout()
        self.fuse_btn = QPushButton("Fuse Selected Walk")
        self.fuse_btn.setStyleSheet(
            "QPushButton{background-color:#4CAF50;color:white;padding:6px 16px;}"
            "QPushButton:hover{background-color:#45a049;}"
            "QPushButton:pressed{background-color:#3d8b40;}")
        self.fuse_btn.clicked.connect(self.fuse_clicked.emit)
        self.export_btn = QPushButton("Export Fused GPX")
        self.export_btn.setEnabled(False)
        self.export_btn.clicked.connect(self.export_clicked.emit)
        btn_row.addWidget(self.fuse_btn)
        btn_row.addWidget(self.export_btn)
        layout.addLayout(btn_row, row, 0, 1, 2)

    def get_params(self) -> dict:
        return {
            'process_noise': self.process_noise.value(),
            'geo_noise': self.geo_noise.value(),
            'garmin_noise': self.garmin_noise.value(),
            'use_smoother': self.smoother_check.isChecked(),
        }

    def enable_export(self, enabled: bool):
        self.export_btn.setEnabled(enabled)


class FusedStatsPanel(QFrame):
    def __init__(self, parent=None):
        super().__init__(parent)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(8, 4, 8, 4)
        self.label = QLabel("No fused path yet")
        self.label.setWordWrap(True)
        self.label.setStyleSheet("font-size: 12px;")
        layout.addWidget(self.label)
        self.setFixedHeight(64)

    def show_stats(self, stats: Dict):
        if not stats:
            self.label.setText("No fused path yet")
            return
        dist_km = stats['distance_m'] / 1000
        dur_min = stats['duration_s'] / 60
        self.label.setText(
            f"<b>Fused Path</b><br>"
            f"{stats['points']} points · {dist_km:.3f} km · "
            f"{dur_min:.1f} min · {stats['avg_speed_ms']:.2f} m/s"
        )

    def clear(self):
        self.label.setText("No fused path yet")


class GPXFusionWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self._check_deps()
        self.groups: Dict[str, GroupTab] = OrderedDict()
        self.fused_results: Dict[str, pd.DataFrame] = {}
        self.fused_stats: Dict[str, Dict] = {}
        self._map_mode = 0
        self._map_after_load = None
        self._zoom_level = 1.0
        self.tile_provider = "OpenStreetMap"
        self.proxy_enabled = False
        self.proxy_host = ''
        self.proxy_port = 8080
        self._setup_ui()
        self._add_initial_group()

    def _check_deps(self):
        missing = []
        if gpxpy is None:
            missing.append("gpxpy")
        if missing:
            QMessageBox.critical(None, "Missing Dependencies",
                f"Install missing packages:\n  pip install {' '.join(missing)}")
            sys.exit(1)

    def _setup_ui(self):
        self.setWindowTitle("GPX Fusion Tool — Garmin + Geo Tracker")
        self.setMinimumSize(1100, 700)
        self._apply_style()

        central = QWidget()
        self.setCentralWidget(central)
        main_layout = QVBoxLayout(central)
        main_layout.setContentsMargins(0, 0, 0, 0)

        splitter = QSplitter(Qt.Orientation.Horizontal)

        left_panel = QWidget()
        left_layout = QVBoxLayout(left_panel)
        left_layout.setContentsMargins(4, 4, 4, 4)

        group_header = QHBoxLayout()
        group_header.addWidget(QLabel("<b>Walk Groups</b>"))
        group_header.addStretch()
        self.add_group_btn = QPushButton("+ Add Walk")
        self.add_group_btn.clicked.connect(self._add_group)
        self.remove_group_btn = QPushButton("− Remove")
        self.remove_group_btn.clicked.connect(self._remove_group)
        self.remove_group_btn.setEnabled(False)
        group_header.addWidget(self.add_group_btn)
        group_header.addWidget(self.remove_group_btn)
        left_layout.addLayout(group_header)

        self.tab_widget = QTabWidget()
        self.tab_widget.setDocumentMode(True)
        self.tab_widget.currentChanged.connect(self._on_tab_changed)
        left_layout.addWidget(self.tab_widget)

        left_panel.setMinimumWidth(320)
        left_panel.setMaximumWidth(450)

        right_panel = QWidget()
        right_layout = QVBoxLayout(right_panel)
        right_layout.setContentsMargins(4, 4, 4, 4)

        self.map_stack = QStackedWidget()

        self.map_label = QLabel("No tracks loaded — add GPX files and click Fuse")
        self.map_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.map_label.setMinimumHeight(400)
        self.map_label.setStyleSheet("background: #2d2d2d; color: #cccccc; border: 1px solid #555; border-radius: 4px; padding: 10px;")
        self.map_label.setScaledContents(False)

        self.map_web_placeholder = QLabel("Online map not loaded — switch to Online mode")
        self.map_web_placeholder.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.map_web_placeholder.setMinimumHeight(400)
        self.map_web_placeholder.setStyleSheet("background: #2d2d2d; color: #888888; border: 1px solid #555; border-radius: 4px; padding: 10px;")
        self._map_web = None

        self.map_stack.addWidget(self.map_label)
        self.map_stack.addWidget(self.map_web_placeholder)
        self.map_stack.setCurrentIndex(0)
        right_layout.addWidget(self.map_stack, stretch=3)

        map_controls = QHBoxLayout()
        map_controls.addWidget(QLabel("Map:"))
        self.map_mode_combo = QComboBox()
        self.map_mode_combo.addItems(["Offline (matplotlib)", "Online (folium)"])
        if not _HAS_WEBENGINE or folium is None:
            self.map_mode_combo.model().item(1).setEnabled(False)
        self.map_mode_combo.currentIndexChanged.connect(self._on_map_mode_changed)
        map_controls.addWidget(self.map_mode_combo)

        self.tile_combo = QComboBox()
        self.tile_combo.addItems(TILE_PROVIDERS)
        self.tile_combo.setToolTip("Tile provider for online map")
        self.tile_combo.setEnabled(False)
        self.tile_combo.currentTextChanged.connect(self._on_tile_changed)
        map_controls.addWidget(QLabel("Tiles:"))
        map_controls.addWidget(self.tile_combo)

        self.refresh_btn = QPushButton("Refresh Map")
        self.refresh_btn.clicked.connect(self._refresh_map)
        map_controls.addWidget(self.refresh_btn)

        self.zoom_in_btn = QPushButton("+")
        self.zoom_in_btn.setFixedWidth(32)
        self.zoom_in_btn.setToolTip("Zoom in")
        self.zoom_in_btn.clicked.connect(self._zoom_in)
        self.zoom_out_btn = QPushButton("−")
        self.zoom_out_btn.setFixedWidth(32)
        self.zoom_out_btn.setToolTip("Zoom out")
        self.zoom_out_btn.clicked.connect(self._zoom_out)
        self.zoom_reset_btn = QPushButton("R")
        self.zoom_reset_btn.setFixedWidth(32)
        self.zoom_reset_btn.setToolTip("Reset zoom")
        self.zoom_reset_btn.clicked.connect(self._zoom_reset)
        map_controls.addWidget(self.zoom_in_btn)
        map_controls.addWidget(self.zoom_out_btn)
        map_controls.addWidget(self.zoom_reset_btn)

        self.proxy_btn = QPushButton("Proxy")
        self.proxy_btn.clicked.connect(self._show_proxy_settings)
        map_controls.addWidget(self.proxy_btn)

        map_controls.addStretch()
        right_layout.addLayout(map_controls)

        bottom_row = QHBoxLayout()
        self.control_panel = FuseControlPanel()
        self.control_panel.fuse_clicked.connect(self._on_fuse)
        self.control_panel.export_clicked.connect(self._on_export)
        bottom_row.addWidget(self.control_panel, stretch=2)

        self.stats_panel = FusedStatsPanel()
        bottom_row.addWidget(self.stats_panel, stretch=1)
        right_layout.addLayout(bottom_row)

        splitter.addWidget(left_panel)
        splitter.addWidget(right_panel)
        splitter.setStretchFactor(0, 1)
        splitter.setStretchFactor(1, 3)
        main_layout.addWidget(splitter)

        self.status_bar = QStatusBar()
        self.setStatusBar(self.status_bar)
        self.status_bar.showMessage("Ready. Add GPX files to a walk group and click Fuse.")

    def resizeEvent(self, event):
        super().resizeEvent(event)
        if self.map_stack.currentIndex() == 0:
            if hasattr(self, '_map_pixmap') and self._map_pixmap and not self._map_pixmap.isNull():
                scaled = self._map_pixmap.scaled(
                    self.map_label.size(), Qt.AspectRatioMode.KeepAspectRatio,
                    Qt.TransformationMode.SmoothTransformation)
                self.map_label.setPixmap(scaled)
        elif self.map_stack.currentIndex() == 1 and self._map_web is not None:
            self._map_web.page().runJavaScript(
                "var m=document.querySelector('.folium-map');"
                "if(typeof L!=='undefined'&&m&&m._leaflet_id){"
                "var map=window['map_'+m.id.replace('map_','')];"
                "if(map){setTimeout(function(){map.invalidateSize();},50);}}")

    def _apply_style(self):
        self.setStyleSheet("""
            QMainWindow, .QWidget, QFrame, QGroupBox, QTabWidget, QTabBar {
                background: #f5f5f5; color: #1a1a1a;
            }
            QGroupBox {
                font-weight: bold; border: 1px solid #ccc;
                border-radius: 4px; margin-top: 8px; padding-top: 12px;
                background: #f5f5f5; color: #1a1a1a;
            }
            QGroupBox::title {
                subcontrol-origin: margin; left: 10px; padding: 0 4px; color: #1a1a1a;
            }
            QListWidget {
                border: 1px solid #ddd; border-radius: 3px;
                background: #ffffff; color: #1a1a1a;
            }
            QListWidget::item {
                color: #1a1a1a; padding: 3px;
            }
            QListWidget::item:selected {
                background: #0078d4; color: #ffffff;
            }
            QTabWidget::pane {
                border: 1px solid #ccc; border-radius: 3px;
                background: #f5f5f5;
            }
            QTabBar::tab {
                background: #e0e0e0; color: #1a1a1a;
                padding: 6px 14px; border: 1px solid #ccc;
                border-bottom: none; border-top-left-radius: 4px;
                border-top-right-radius: 4px;
            }
            QTabBar::tab:selected {
                background: #f5f5f5; color: #1a1a1a;
            }
            QPushButton {
                padding: 4px 10px; border: 1px solid #aaa;
                border-radius: 3px; background: #ffffff; color: #1a1a1a;
            }
            QPushButton:hover { background: #e0e0e0; }
            QPushButton:pressed { background: #cccccc; }
            QPushButton:disabled { background: #d0d0d0; color: #888888; }
            QTextEdit {
                border: 1px solid #ddd; border-radius: 3px;
                background: #ffffff; color: #1a1a1a;
            }
            QLabel {
                font-size: 12px; color: #1a1a1a;
            }
            QStatusBar {
                font-size: 12px; background: #e8e8e8; color: #1a1a1a;
            }
            QSpinBox, QDoubleSpinBox {
                background: #ffffff; color: #1a1a1a;
                border: 1px solid #ccc; border-radius: 3px; padding: 2px;
            }
            QSpinBox:focus, QDoubleSpinBox:focus {
                border: 1px solid #0078d4;
            }
            QSpinBox::up-button, QDoubleSpinBox::up-button,
            QSpinBox::down-button, QDoubleSpinBox::down-button {
                border: 1px solid #ccc; background: #f0f0f0;
                border-radius: 2px; width: 18px;
            }
            QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover,
            QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover {
                background: #e0e0e0;
            }
            QCheckBox {
                color: #1a1a1a;
            }
            QComboBox {
                background: #ffffff; color: #1a1a1a;
                border: 1px solid #ccc; border-radius: 3px; padding: 2px 4px;
            }
            QComboBox:hover {
                border: 1px solid #999;
            }
            QComboBox::drop-down {
                border: none; width: 20px;
            }
            QComboBox::down-arrow {
                width: 10px; height: 10px;
            }
            QStackedWidget {
                background: #2d2d2d; border: 1px solid #555;
                border-radius: 4px;
            }
            QScrollBar:vertical {
                background: #e8e8e8; width: 10px;
            }
            QScrollBar::handle:vertical {
                background: #aaa; border-radius: 4px; min-height: 20px;
            }
        """)

    def _add_initial_group(self):
        self._add_group()
        self.tab_widget.setCurrentIndex(0)

    def _add_group(self):
        n = len(self.groups) + 1
        name = f"Walk {n}"
        tab = GroupTab(name)
        tab.files_changed.connect(self._update_map)
        self.groups[name] = tab
        self.tab_widget.addTab(tab, name)
        self.tab_widget.setCurrentWidget(tab)
        self.remove_group_btn.setEnabled(True)

    def _remove_group(self):
        idx = self.tab_widget.currentIndex()
        if idx < 0:
            return
        name = self.tab_widget.tabText(idx)
        if name in self.groups:
            reply = QMessageBox.question(self, "Remove Walk",
                f"Remove '{name}' and all its files?",
                QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No)
            if reply != QMessageBox.StandardButton.Yes:
                return
            del self.groups[name]
            self.tab_widget.removeTab(idx)
            if name in self.fused_results:
                del self.fused_results[name]
                del self.fused_stats[name]
        if len(self.groups) == 0:
            self.remove_group_btn.setEnabled(False)
            self.map_label.setText("No walk groups — add one with '+ Add Walk'")
            self.map_label.setPixmap(QPixmap())
            self._map_pixmap = QPixmap()
            if self._map_web is not None:
                self._map_web.setHtml("<html><body style='background:#2d2d2d; color:#888; "
                                      "display:flex; align-items:center; justify-content:center; "
                                      "height:100vh; margin:0;'><p>No walk groups</p></body></html>")
        else:
            self._update_map()

    def _on_tab_changed(self, idx: int):
        self._update_map()

    def _get_current_tab(self) -> Optional[GroupTab]:
        idx = self.tab_widget.currentIndex()
        if idx < 0:
            return None
        name = self.tab_widget.tabText(idx)
        return self.groups.get(name)

    def _update_map(self):
        tab = self._get_current_tab()
        if tab is None:
            return
        raw = tab.get_raw_tracks()
        name = tab.group_name
        fused = self.fused_results.get(name)
        fstats = self.fused_stats.get(name)

        if len(raw) == 0:
            self._update_map_empty()
            return

        fstats_actual = None
        if fused is not None:
            fstats_actual = compute_statistics(fused)

        if self.map_stack.currentIndex() == 0:
            self._update_matplotlib_map(raw, fused, fstats_actual)
        else:
            self._update_folium_map(raw, fused, fstats_actual)

    def _update_matplotlib_map(self, raw, fused, fstats):
        pixmap = generate_map(raw, fused, fstats, zoom_factor=self._zoom_level)
        if pixmap and not pixmap.isNull():
            self._map_pixmap = pixmap
            scaled = pixmap.scaled(self.map_label.size(), Qt.AspectRatioMode.KeepAspectRatio,
                                   Qt.TransformationMode.SmoothTransformation)
            self.map_label.setPixmap(scaled)
            self.map_label.setText("")
        else:
            self.map_label.setText("Map generation failed")
            self.map_label.setPixmap(QPixmap())
            self._map_pixmap = QPixmap()

    def _update_map_empty(self):
        self.map_label.setText("No tracks loaded — add GPX files and click Fuse")
        self.map_label.setPixmap(QPixmap())
        self._map_pixmap = QPixmap()
        if self._map_web is not None:
            self._map_web.setHtml("<html><body style='background:#2d2d2d; color:#888; "
                                  "display:flex; align-items:center; justify-content:center; "
                                  "height:100vh; margin:0;'><p>No tracks loaded</p></body></html>")

    def _update_folium_map(self, raw, fused, fstats):
        if self._map_web is None:
            return

        ok = check_connectivity(self.proxy_host if self.proxy_enabled else None,
                                self.proxy_port if self.proxy_enabled else None)
        if not ok:
            proxy_info = ""
            if self.proxy_enabled and self.proxy_host:
                proxy_info = f"<p>Proxy: {self.proxy_host}:{self.proxy_port}</p>"
            err = (f"<html><body style='background:#2d2d2d; color:#ccc; "
                   f"display:flex; align-items:center; justify-content:center; "
                   f"height:100vh; margin:0; font-family:sans-serif; text-align:center;'>"
                   f"<div><h2>Web page not loading — check your connection</h2>"
                   f"{proxy_info}"
                   f"<p style='font-size:12px; color:#999;'>"
                   f"Cannot reach {LEAFLET_URL}</p></div></body></html>")
            self._map_web.setHtml(err)
            self._map_after_load = self._parent_and_show
            return

        html = generate_map_folium(raw, fused, fstats, tiles=self.tile_provider)
        if not html:
            return

        # Preserve size before unparenting so compositor has valid dimensions
        size = self._map_web.size()
        self._map_web.setParent(None)
        self._map_web.resize(size)
        self._map_after_load = self._finish_folium_load
        self._map_web.setHtml(html, QUrl("https://localhost/"))

    def _parent_and_show(self):
        if self._map_web.parent() is None:
            self.map_stack.removeWidget(self.map_web_placeholder)
            self.map_stack.addWidget(self._map_web)
            self.map_web_placeholder.hide()
        self.map_stack.setCurrentIndex(1)

    def _finish_folium_load(self):
        """Parent the view, flush layout, then apply viewport fix."""
        self._parent_and_show()
        QCoreApplication.processEvents()

        js = ("document.documentElement.style.setProperty('height', '100%', 'important');"
              "document.documentElement.style.setProperty('width', '100%', 'important');"
              "document.body.style.setProperty('height', '100%', 'important');"
              "document.body.style.setProperty('width', '100%', 'important');"
              "document.body.style.setProperty('margin', '0', 'important');"
              "document.body.style.setProperty('padding', '0', 'important');"
              "var m = document.querySelector('.folium-map');"
              "if (m) {"
              "    m.style.setProperty('width', '100%', 'important');"
              "    m.style.setProperty('height', '100%', 'important');"
              "    m.style.setProperty('position', 'absolute', 'important');"
              "    m.style.setProperty('top', '0', 'important');"
              "    m.style.setProperty('left', '0', 'important');"
              "}"
              "if (m && typeof L !== 'undefined') {"
              "    var mapObj = window[m.id];"
              "    if (mapObj) {"
              "        mapObj.invalidateSize();"
              "        setTimeout(function() { mapObj.invalidateSize(); }, 50);"
              "        setTimeout(function() { mapObj.invalidateSize(); }, 200);"
              "        setTimeout(function() { mapObj.invalidateSize(); }, 600);"
              "    }"
              "}")
        self._map_web.page().runJavaScript(js)

    def _on_map_load_finished(self, ok: bool):
        if not ok:
            self._map_web.page().runJavaScript(
                "document.body.innerHTML="
                "'<h2 style=\"color:#ccc;text-align:center;margin-top:40vh;\">"
                "Map failed to load</h2>'")
            return

        if self._map_after_load is not None:
            cb = self._map_after_load
            self._map_after_load = None
            cb()

    def _on_map_mode_changed(self, idx: int):
        if idx == 0:
            self.tile_combo.setEnabled(False)
            self.map_stack.setCurrentIndex(0)
            self._update_matplotlib_map(*self._get_current_raw_fused())
        else:
            if not _HAS_WEBENGINE or folium is None:
                self.map_mode_combo.setCurrentIndex(0)
                msg = ("Install folium and PyQt6-WebEngine:\n"
                       "  pip install folium PyQt6-WebEngine")
                QMessageBox.information(self, "Missing Dependency", msg)
                return
            if self._map_web is None:
                from PyQt6.QtWebEngineWidgets import QWebEngineView
                from PyQt6.QtWebEngineCore import QWebEngineSettings
                self._map_web = QWebEngineView()
                self._map_web.setMinimumHeight(400)
                self._map_web.loadFinished.connect(self._on_map_load_finished)
                settings = self._map_web.settings()
                settings.setAttribute(
                    QWebEngineSettings.WebAttribute.LocalContentCanAccessRemoteUrls, True)
                settings.setAttribute(
                    QWebEngineSettings.WebAttribute.JavascriptEnabled, True)
                # Do NOT add to stack or show — setHtml() fails on parented views.
                # _finish_folium_load will parent and show after content loads.
            self.tile_combo.setEnabled(True)
            self._update_folium_map(*self._get_current_raw_fused())

    def _on_tile_changed(self, tile: str):
        if self.map_stack.currentIndex() == 1 and tile in TILE_PROVIDERS:
            self.tile_provider = tile
            self._update_folium_map(*self._get_current_raw_fused())

    def _refresh_map(self):
        if self.map_stack.currentIndex() == 0:
            self._update_matplotlib_map(*self._get_current_raw_fused())
        else:
            self._update_folium_map(*self._get_current_raw_fused())

    def _zoom_in(self):
        self._zoom_level = min(self._zoom_level * 1.5, 20.0)
        self._refresh_map()

    def _zoom_out(self):
        self._zoom_level = max(self._zoom_level / 1.5, 0.1)
        self._refresh_map()

    def _zoom_reset(self):
        self._zoom_level = 1.0
        self._refresh_map()

    def _get_current_raw_fused(self):
        tab = self._get_current_tab()
        if tab is None:
            return [], None, None
        raw = tab.get_raw_tracks()
        name = tab.group_name
        fused = self.fused_results.get(name)
        fstats = self.fused_stats.get(name)
        fstats_actual = None
        if fused is not None:
            fstats_actual = compute_statistics(fused)
        return raw, fused, fstats_actual

    def _show_proxy_settings(self):
        dlg = ProxyDialog(self.proxy_enabled, self.proxy_host, self.proxy_port, self)
        if dlg.exec() == QDialog.DialogCode.Accepted:
            self.proxy_enabled, self.proxy_host, self.proxy_port = dlg.get_values()
            if self.map_stack.currentIndex() == 1:
                self._update_folium_map(*self._get_current_raw_fused())
            if self.proxy_enabled:
                msg = f"Proxy enabled: {self.proxy_host}:{self.proxy_port}"
            else:
                msg = "Proxy disabled"
            self.status_bar.showMessage(msg)

    def _on_fuse(self):
        tab = self._get_current_tab()
        if tab is None:
            return
        raw = tab.get_raw_tracks()
        if len(raw) < 1:
            QMessageBox.information(self, "No Data", "Add at least one GPX file first.")
            return

        params = self.control_panel.get_params()
        self.status_bar.showMessage("Fusing tracks...")

        dfs = [df for _, df, _ in raw]
        result = fuse_tracks(
            dfs,
            process_noise=params['process_noise'],
            geo_noise=params['geo_noise'],
            garmin_noise=params['garmin_noise'],
            use_smoother=params['use_smoother'],
        )

        if result is not None and len(result) > 1:
            self.fused_results[tab.group_name] = result
            stats = compute_statistics(result)
            self.fused_stats[tab.group_name] = stats
            self.stats_panel.show_stats(stats)
            self.control_panel.enable_export(True)
            self.status_bar.showMessage(
                f"Fused: {stats['distance_m']/1000:.3f} km, {stats['duration_s']/60:.1f} min, {stats['points']} pts")
        else:
            self.status_bar.showMessage("Fusion failed — not enough overlapping data.")

        QCoreApplication.processEvents()
        self._update_map()

    def _on_export(self):
        tab = self._get_current_tab()
        if tab is None:
            return
        name = tab.group_name
        fused = self.fused_results.get(name)
        if fused is None:
            QMessageBox.information(self, "Nothing to Export", "Fuse a walk first.")
            return

        path, _ = QFileDialog.getSaveFileName(
            self, "Export Fused GPX", f"{name}_fused.gpx",
            "GPX Files (*.gpx);;All Files (*)")
        if not path:
            return

        gpx = gpxpy.gpx.GPX()
        gpx.name = f"{name} - Fused (Garmin + Geo Tracker)"
        gpx.description = "Fused with Kalman filter + RTS smoother"
        gpx.time = datetime.now(timezone.utc)

        track = gpxpy.gpx.GPXTrack()
        track.name = gpx.name
        seg = gpxpy.gpx.GPXTrackSegment()
        for _, row in fused.iterrows():
            pt = gpxpy.gpx.GPXTrackPoint(row['lat'], row['lon'], time=row['time'])
            seg.points.append(pt)
        track.segments.append(seg)
        gpx.tracks.append(track)

        with open(path, 'w', encoding='utf-8') as f:
            f.write(gpx.to_xml())

        self.status_bar.showMessage(f"Exported: {path}")

    def closeEvent(self, event):
        super().closeEvent(event)


def main():
    app = QApplication(sys.argv)
    app.setApplicationName("GPX Fusion Tool")
    window = GPXFusionWindow()
    window.show()
    sys.exit(app.exec())


if __name__ == '__main__':
    main()
