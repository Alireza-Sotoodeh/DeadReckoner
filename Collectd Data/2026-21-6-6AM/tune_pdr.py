#!/usr/bin/env python3
"""
DeadReckoner PDR Parameter Tuner
Brute-force search over Weinberg K, peak height, and peak distance
to find the parameter set that minimizes PDR error vs Geo Tracker GPS.

Usage:
    python tune_pdr.py                    # full sweep, interactive plots
    python tune_pdr.py --quick            # coarse sweep (faster)

Requires: numpy, scipy, matplotlib
"""

import os, sys, time, itertools
import numpy as np
from scipy.signal import find_peaks

# Reuse compare_paths functions
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPT_DIR)
from compare_paths import (
    parse_bin, parse_gpx, gpx_to_local,
    compute_gps_distance, compute_path_length,
    quaternion_to_yaw, find_best_heading, stitch_bin, GPX_NS, GEOT_NS
)
parse_gpx_geotracker = parse_gpx  # alias for backward compat

QUICK = '--quick' in sys.argv


def compute_pdr_fast(accel_mag, yaw, peaks, k, heading_offset_rad):
    """
    Fast PDR: given pre-computed accel_mag, yaw, and detected peaks,
    compute Weinberg step lengths and integrate.
    Returns (x_m, y_m, n_steps).
    """
    if len(peaks) < 2:
        return np.array([0.0]), np.array([0.0]), 0

    # Valley search for each peak
    step_len = np.zeros(len(peaks))
    step_yaw = np.zeros(len(peaks))

    for pk_idx, peak_i in enumerate(peaks):
        prev_boundary = (peaks[pk_idx - 1] + 5) if pk_idx > 0 else max(0, peak_i - 100)
        search_start = prev_boundary
        search_end = peak_i - 2
        if search_end <= search_start:
            valley_val = accel_mag[peak_i] * 0.5
        else:
            valley_i = search_start + np.argmin(accel_mag[search_start:search_end])
            valley_val = accel_mag[valley_i]
        peak_val = accel_mag[peak_i]
        step_len[pk_idx] = k * (peak_val - max(valley_val, 0.1)) ** 0.25
        step_yaw[pk_idx] = yaw[peak_i] + heading_offset_rad

    # Integrate
    dx = step_len * np.sin(step_yaw)
    dy = step_len * np.cos(step_yaw)
    cum_x = np.insert(np.cumsum(dx), 0, 0.0)
    cum_y = np.insert(np.cumsum(dy), 0, 0.0)

    return cum_x, cum_y, len(peaks)


def sweep_segment(bin_data, geo_lat, geo_lon, geo_x, geo_y,
                  k_values, height_values, dist_values):
    """
    Sweep all parameter combinations for one segment.
    geo_lat/geo_lon: WGS84 degrees (for Haversine path length).
    geo_x/geo_y: local meters (for heading alignment).
    Returns list of {params, error, n_steps, endpoint_error}.
    """
    # Phase A: pre-compute accel_mag and yaw (constant across all param combos)
    accel_mag = np.sqrt(np.sum(bin_data['accel'] ** 2, axis=1)) * 9.81
    q = bin_data['q']
    yaw = quaternion_to_yaw(q[:, 0], q[:, 1], q[:, 2], q[:, 3])

    # Reference GPS path length (Haversine on lat/lon)
    ref_len = compute_gps_distance(geo_lat, geo_lon)
    # GPS endpoint in local meters for heading alignment
    ref_end = np.array([geo_x[-1], geo_y[-1]])

    results = []
    total = len(k_values) * len(height_values) * len(dist_values)
    count = 0

    for k in k_values:
        for height in height_values:
            for dist in dist_values:
                # Phase B: peak detection with these thresholds
                peaks, _ = find_peaks(accel_mag, height=height, distance=dist, prominence=1.5)
                if len(peaks) < 2:
                    count += 1
                    continue

                # Find best heading alignment (independent of K, depends on peaks)
                # We need to compute paths for each K to find best heading, but
                # heading alignment depends on path shape which depends on K.
                # Do a quick heading search for each combo.
                best_heading, _ = find_best_heading_peaks(accel_mag, yaw, geo_x, geo_y,
                                                          peaks, k)

                # Compute PDR at this heading
                pdr_x, pdr_y, n_steps = compute_pdr_fast(
                    accel_mag, yaw, peaks, k, np.radians(best_heading))

                pdr_len = compute_path_length(pdr_x, pdr_y)
                pdr_end = np.array([pdr_x[-1], pdr_y[-1]])
                endpoint_error = float(np.linalg.norm(pdr_end - ref_end))
                pdr_err_pct = abs(pdr_len - ref_len) / ref_len * 100 if ref_len > 0 else 0

                results.append({
                    'k': k, 'height': height, 'dist': dist,
                    'pdr_len': pdr_len,
                    'ref_len': ref_len,
                    'error_pct': pdr_err_pct,
                    'endpoint_error': endpoint_error,
                    'n_steps': n_steps,
                    'best_heading': best_heading,
                })
                count += 1

    return results


def find_best_heading_peaks(accel_mag, yaw, ref_x, ref_y, peaks, k):
    """
    Find best heading angle given peaks and K, without computing full path for
    each angle (optimization: compute once, rotate).
    Uses brute-force 0-360 deg at 0.5 deg steps.
    """
    if len(peaks) < 2 or len(ref_x) < 2:
        return 0.0, 9999.0

    # Pre-compute step_len and direction for each peak (independent of heading)
    step_len = np.zeros(len(peaks))
    for pk_idx, peak_i in enumerate(peaks):
        prev_boundary = (peaks[pk_idx - 1] + 5) if pk_idx > 0 else max(0, peak_i - 100)
        search_start = prev_boundary
        search_end = peak_i - 2
        if search_end <= search_start:
            valley_val = accel_mag[peak_i] * 0.5
        else:
            valley_i = search_start + np.argmin(accel_mag[search_start:search_end])
            valley_val = accel_mag[valley_i]
        peak_val = accel_mag[peak_i]
        step_len[pk_idx] = k * (peak_val - max(valley_val, 0.1)) ** 0.25

    # Heading delta as function of yaw base angle (before rotation)
    base_dyaw = yaw[peaks]

    ref_end = np.array([ref_x[-1], ref_y[-1]])

    best_angle = 0.0
    best_error = 9999.0

    for angle in np.arange(0.0, 360.0, 0.5):
        rad = np.radians(angle)
        total_heading = base_dyaw + rad
        dx = step_len * np.sin(total_heading)
        dy = step_len * np.cos(total_heading)
        cum_x = float(np.sum(dx))
        cum_y = float(np.sum(dy))
        error = float(np.linalg.norm(np.array([cum_x, cum_y]) - ref_end))
        if error < best_error:
            best_error = error
            best_angle = angle

    return best_angle, best_error


def segment_opt(results, label):
    """Find best parameter set for a segment."""
    if not results:
        return None
    best = min(results, key=lambda r: r['error_pct'])
    arrow = '->'
    print(f'  [{label}] K={best["k"]:.3f} height={best["height"]:.1f} '
          f'dist={best["dist"]:.0f}  {arrow}  '
          f'error={best["error_pct"]:.1f}%  endpoint={best["endpoint_error"]:.1f}m  '
          f'n_steps={best["n_steps"]}')
    return best


def print_sensitivity(results, label):
    """Print how sensitive error is to each parameter."""
    if not results:
        return
    for param, param_label in [('k', 'K'), ('height', 'Height'), ('dist', 'Distance')]:
        unique_vals = sorted(set(r[param] for r in results))
        errors_by_val = {}
        for val in unique_vals:
            subset = [r for r in results if r[param] == val]
            if subset:
                errors_by_val[val] = {
                    'min': min(r['error_pct'] for r in subset),
                    'mean': np.mean([r['error_pct'] for r in subset]),
                    'max': max(r['error_pct'] for r in subset),
                }
        best_val = min(errors_by_val, key=lambda v: errors_by_val[v]['min'])
        worst_val = max(errors_by_val, key=lambda v: errors_by_val[v]['mean'])
        print(f'    {param_label}: best={best_val} (min err {errors_by_val[best_val]["min"]:.1f}%), '
              f'worst={worst_val} (mean err {errors_by_val[worst_val]["mean"]:.1f}%)')


def plot_tuning(seg_results, seg_ids, out_dir):
    """Generate tuning analysis plots."""
    import matplotlib
    matplotlib.use('Agg' if '--batch' in sys.argv else 'TkAgg')
    from matplotlib import pyplot as plt

    # Plot 1: Error vs K for each segment (at each segment's optimal height/distance)
    fig1, ax1 = plt.subplots(figsize=(12, 6))
    colors = ['blue', 'darkviolet', 'darkred', 'darkcyan', 'saddlebrown']
    for i, (results, sid) in enumerate(zip(seg_results, seg_ids)):
        if not results:
            continue
        # Group by K, take minimum error across height/distance
        k_vals = sorted(set(r['k'] for r in results))
        min_err_by_k = []
        for k in k_vals:
            subset = [r for r in results if r['k'] == k]
            min_err_by_k.append(min(r['error_pct'] for r in subset))
        ax1.plot(k_vals, min_err_by_k, '-o', color=colors[i], linewidth=1.5,
                 markersize=4, label=f'S{sid}', alpha=0.8)

    ax1.set_xlabel('Weinberg K')
    ax1.set_ylabel('Min Error (%)')
    ax1.set_title('Error vs Weinberg K (best height/distance per K value)')
    ax1.legend(fontsize=8)
    ax1.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.savefig(os.path.join(out_dir, 'error_vs_K.png'), dpi=200, bbox_inches='tight')
    print(f'[+] Saved error_vs_K.png')
    if not BATCH_MODE:
        plt.show()
    else:
        plt.close(fig1)

    # Plot 2: Heatmap of error over (K, height) at best distance
    for i, (results, sid) in enumerate(zip(seg_results, seg_ids)):
        if not results:
            continue
        fig2, ax2 = plt.subplots(figsize=(8, 6))
        # Take the first segment's distance for simplicity
        best_k = sorted(set(r['k'] for r in results))
        best_h = sorted(set(r['height'] for r in results))
        best_d = sorted(set(r['dist'] for r in results))
        # Pick the distance that gives lowest overall error
        dist_errs = {}
        for d in best_d:
            sub = [r for r in results if r['dist'] == d]
            dist_errs[d] = min(r['error_pct'] for r in sub) if sub else 999
        best_dist = min(dist_errs, key=dist_errs.get)
        # Build 2D grid
        Z = np.full((len(best_k), len(best_h)), np.nan)
        for ki, k in enumerate(best_k):
            for hi, h in enumerate(best_h):
                subset = [r for r in results if r['k'] == k and r['height'] == h
                          and r['dist'] == best_dist]
                if subset:
                    Z[ki, hi] = min(r['error_pct'] for r in subset)
        im = ax2.pcolormesh(best_k, best_h, Z.T, shading='auto', cmap='RdYlGn_r')
        ax2.set_xlabel('Weinberg K')
        ax2.set_ylabel('Peak Min Height')
        ax2.set_title(f'S{sid} - Error % (dist={best_dist:.0f})')
        plt.colorbar(im, ax=ax2, label='Error %')
        plt.tight_layout()
        plt.savefig(os.path.join(out_dir, f'heatmap_s{sid}.png'), dpi=200, bbox_inches='tight')
        print(f'[+] Saved heatmap_s{sid}.png')
        if not BATCH_MODE:
            plt.show()
        else:
            plt.close(fig2)


def main():
    global BATCH_MODE
    BATCH_MODE = '--batch' in sys.argv

    data_dir = SCRIPT_DIR
    out_dir = os.path.join(SCRIPT_DIR, 'analysis')
    os.makedirs(out_dir, exist_ok=True)

    # ── Parameters to sweep ──
    if QUICK:
        k_values = [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7]
        height_values = [2.0, 2.5, 3.0, 3.5, 4.0]
        dist_values = [20, 25, 30, 35]
        print('  MODE: Quick sweep')
    else:
        k_values = [round(x * 0.025, 3) for x in range(2, 33)]  # 0.050 to 0.800
        height_values = [1.5, 2.0, 2.5, 3.0, 3.5, 4.0]
        dist_values = [15, 20, 25, 30, 35]

    print(f'  Sweep: {len(k_values)} K x {len(height_values)} heights x '
          f'{len(dist_values)} distances = {len(k_values) * len(height_values) * len(dist_values)} combos')
    print()

    # ── Load all segments ──
    segments = []
    for seg_id in range(1, 6):
        print(f'  Loading segment {seg_id}...', end=' ')

        bin_data = parse_bin(os.path.join(data_dir, 'DeadReckoner', f'DR_LOG_00{seg_id}.BIN'))
        if bin_data is None:
            print('SKIP (no BIN)')
            continue

        # Stitch recovery for seg 1
        if seg_id == 1:
            rec_path = os.path.join(data_dir, 'DeadReckoner', '001001.BIN')
            if os.path.exists(rec_path):
                rec_data = parse_bin(rec_path)
                if rec_data is not None and rec_data['n_imu'] > 0:
                    stitched = stitch_bin(bin_data, rec_data)
                    if stitched['n_imu'] > bin_data['n_imu']:
                        bin_data = stitched

        geo = parse_gpx_geotracker(os.path.join(data_dir, 'GeoTracker', f'{seg_id}.gpx'))
        geo_x, geo_y = gpx_to_local(geo['lat'], geo['lon'])

        segments.append({
            'id': seg_id,
            'bin': bin_data,
            'geo': geo,
            'geo_x': geo_x,
            'geo_y': geo_y,
        })
        print(f'{bin_data["n_imu"]:,} IMU frames, Geo: {len(geo["lat"])} pts')

    print()

    # ── Sweep each segment ──
    all_results = []
    seg_best_params = []

    for s in segments:
        sid = s['id']
        print(f'  Sweeping segment {sid}...')
        t0 = time.time()
        results = sweep_segment(
            s['bin'], s['geo']['lat'], s['geo']['lon'],
            s['geo_x'], s['geo_y'],
            k_values, height_values, dist_values
        )
        elapsed = time.time() - t0
        print(f'    {len(results)} valid combos in {elapsed:.1f}s '
              f'({elapsed / max(len(results), 1):.3f}s/combo)')

        # Per-segment best
        best = segment_opt(results, f'S{sid}')
        if best:
            seg_best_params.append(best)
            print_sensitivity(results, f'S{sid}')

        all_results.append({'seg_id': sid, 'results': results, 'best': best})
        print()

    # ── Global best: average across all segments ──
    print('  Computing global optimum...')
    # For each param combo, compute the average error across segments
    param_map = {}
    for segment_results in all_results:
        for r in segment_results['results']:
            key = (r['k'], r['height'], r['dist'])
            if key not in param_map:
                param_map[key] = {'errors': [], 'endpoints': [], 'n_steps': [], 'segments': set()}
            param_map[key]['errors'].append(r['error_pct'])
            param_map[key]['endpoints'].append(r['endpoint_error'])
            param_map[key]['n_steps'].append(r['n_steps'])
            param_map[key]['segments'].add(segment_results['seg_id'])

    # Must have results for at least 4 of 5 segments
    global_results = []
    for key, val in param_map.items():
        if len(val['segments']) < 4:
            continue
        global_results.append({
            'k': key[0], 'height': key[1], 'dist': key[2],
            'avg_error': np.mean(val['errors']),
            'max_error': np.max(val['errors']),
            'avg_endpoint': np.mean(val['endpoints']),
            'n_segments': len(val['segments']),
        })

    if global_results:
        # Best by avg error
        best_global = min(global_results, key=lambda r: r['avg_error'])
        # Best by max error (minimax)
        best_minimax = min(global_results, key=lambda r: r['max_error'])

        print(f'\n  --- GLOBAL OPTIMUM ---')
        print(f'  Best by avg error: K={best_global["k"]:.3f} '
              f'height={best_global["height"]:.1f} dist={best_global["dist"]:.0f}  ->  '
              f'avg error={best_global["avg_error"]:.1f}%  '
              f'max error={best_global["max_error"]:.1f}%  '
              f'({best_global["n_segments"]} segments)')
        print(f'  Best by minimax:  K={best_minimax["k"]:.3f} '
              f'height={best_minimax["height"]:.1f} dist={best_minimax["dist"]:.0f}  ->  '
              f'avg error={best_minimax["avg_error"]:.1f}%  '
              f'max error={best_minimax["max_error"]:.1f}%  '
              f'({best_minimax["n_segments"]} segments)')

        # Also show per-segment breakdown at global best
        print(f'\n  Per-segment at global best (K={best_global["k"]:.3f}, '
              f'height={best_global["height"]:.1f}, dist={best_global["dist"]:.0f}):')
        for s in segments:
            sid = s['id']
            subset = [r for r in all_results[sid - 1]['results']
                      if r['k'] == best_global['k']
                      and r['height'] == best_global['height']
                      and r['dist'] == best_global['dist']]
            if subset:
                r = min(subset, key=lambda x: x['error_pct'])
                print(f'    S{sid}: error={r["error_pct"]:.1f}%  '
                      f'endpoint={r["endpoint_error"]:.1f}m  n_steps={r["n_steps"]}')

    # ── Sensitivity summary ──
    print(f'\n  --- PARAMETER SENSITIVITY ---')
    print('  How much does error change if a parameter is suboptimal?')
    for s in segments:
        sid = s['id']
        results = all_results[sid - 1]['results']
        if not results:
            continue
        best = seg_best_params[sid - 1]
        print(f'  S{sid}:')
        for param, label, vals in [
            ('k', 'K', sorted(set(r['k'] for r in results))),
            ('height', 'Height', sorted(set(r['height'] for r in results))),
            ('dist', 'Distance', sorted(set(r['dist'] for r in results))),
        ]:
            # Error at best value
            at_best = [r['error_pct'] for r in results if r[param] == best[param]]
            # Error +1 step away
            idx = vals.index(best[param])
            neighbor_vals = []
            if idx > 0:
                neighbor_vals.append(vals[idx - 1])
            if idx < len(vals) - 1:
                neighbor_vals.append(vals[idx + 1])
            err_at_best = np.mean(at_best) if at_best else 999
            if neighbor_vals:
                err_neighbors = []
                for nv in neighbor_vals:
                    sub = [r['error_pct'] for r in results if r[param] == nv]
                    if sub:
                        err_neighbors.append(np.mean(sub))
                if err_neighbors:
                    delta = max(err_neighbors) - err_at_best
                    pct_delta = delta / err_at_best * 100 if err_at_best > 0 else 0
                    print(f'    {label}={best[param]}: err={err_at_best:.1f}%  '
                          f'+/-1 step: +{delta:.1f}% ({pct_delta:.0f}% increase)')

    # ── Generate plots ──
    print(f'\n  Generating tuning plots...')
    seg_ids = [s['id'] for s in segments]
    seg_results_list = [all_results[sid - 1]['results'] for sid in seg_ids]
    plot_tuning(seg_results_list, seg_ids, out_dir)

    # ── Optimal path comparison plot ──
    if best_global:
        print('  Generating optimal path comparison...')
        import matplotlib
        matplotlib.use('Agg' if BATCH_MODE else 'TkAgg')
        from matplotlib import pyplot as plt
        from compare_paths import plot_segment

        fig, axes = plt.subplots(2, 3, figsize=(18, 12))
        fig.suptitle('Optimal PDR Parameters - Path Comparison',
                     fontsize=14, fontweight='bold')
        axes_flat = axes.flatten()
        pdr_colors = ['blue', 'darkviolet', 'darkred', 'darkcyan', 'saddlebrown']

        for s in segments:
            sid = s['id']
            ax = axes_flat[sid - 1]
            # Get best result for this segment
            subset = [r for r in all_results[sid - 1]['results']
                      if r['k'] == best_global['k']
                      and r['height'] == best_global['height']
                      and r['dist'] == best_global['dist']]
            if not subset:
                ax.text(0.5, 0.5, f'S{sid}: no valid combo', transform=ax.transAxes, ha='center')
                continue
            best_r = min(subset, key=lambda x: x['error_pct'])

            # Recompute full PDR path
            accel_mag = np.sqrt(np.sum(s['bin']['accel'] ** 2, axis=1)) * 9.81
            q = s['bin']['q']
            yaw = quaternion_to_yaw(q[:, 0], q[:, 1], q[:, 2], q[:, 3])
            peaks, _ = find_peaks(accel_mag,
                                  height=best_global['height'],
                                  distance=best_global['dist'],
                                  prominence=1.5)
            pdr_x, pdr_y, ns = compute_pdr_fast(
                accel_mag, yaw, peaks,
                best_global['k'], np.radians(best_r['best_heading']))

            ax.set_aspect('equal')
            ax.plot(s['geo_x'], s['geo_y'], '-', color='green', linewidth=2,
                    label='Geo Tracker', alpha=0.8)
            ax.plot(pdr_x, pdr_y, '-', color=pdr_colors[sid - 1], linewidth=2,
                    label='PDR (tuned)', alpha=0.9)
            ax.scatter(s['geo_x'][0], s['geo_y'][0], c='green', marker='o',
                       s=60, zorder=5, edgecolors='black')
            ax.scatter(pdr_x[0], pdr_y[0], c=pdr_colors[sid - 1], marker='o',
                       s=60, zorder=5, edgecolors='black')
            ax.scatter(s['geo_x'][-1], s['geo_y'][-1], c='green', marker='x',
                       s=80, zorder=5, linewidths=2)
            ax.scatter(pdr_x[-1], pdr_y[-1], c=pdr_colors[sid - 1], marker='x',
                       s=80, zorder=5, linewidths=2)

            all_x = np.concatenate([s['geo_x'], pdr_x])
            all_y = np.concatenate([s['geo_y'], pdr_y])
            mid_x = (all_x.max() + all_x.min()) / 2
            mid_y = (all_y.max() + all_y.min()) / 2
            max_extent = max(np.ptp(all_x), np.ptp(all_y)) * 0.6
            if max_extent > 1:
                ax.set_xlim(mid_x - max_extent, mid_x + max_extent)
                ax.set_ylim(mid_y - max_extent, mid_y + max_extent)

            ax.set_xlabel('East (m)')
            ax.set_ylabel('North (m)')
            ax.set_title(f'S{sid} (error {best_r["error_pct"]:.1f}%)')
            ax.grid(True, alpha=0.3)
            ax.legend(fontsize=8)

            # Text box
            txt = f'K={best_global["k"]:.3f}\nH={best_global["height"]:.1f}\nD={best_global["dist"]:.0f}'
            ax.text(0.05, 0.05, txt, transform=ax.transAxes, fontsize=7,
                    verticalalignment='bottom',
                    bbox=dict(boxstyle='round,pad=0.3', facecolor='white', alpha=0.8))

        axes_flat[5].axis('off')
        plt.tight_layout(rect=[0, 0, 1, 0.95])
        plt.savefig(os.path.join(out_dir, 'optimal_comparison.png'), dpi=200, bbox_inches='tight')
        print(f'[+] Saved optimal_comparison.png')
        if BATCH_MODE:
            plt.close(fig)

    print(f'\n  Done. Results saved to {out_dir}/')
    print()


if __name__ == '__main__':
    main()
