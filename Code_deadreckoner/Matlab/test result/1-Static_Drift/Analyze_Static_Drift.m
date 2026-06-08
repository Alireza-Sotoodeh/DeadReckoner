% =========================================================================
% PROJECT: DeadReckoner - IMU Sensor Fusion
% MODULE: Comprehensive Static Drift Analyzer (With Critical Points)
% AUTHOR: Alireza Sotoodeh
% =========================================================================
clc; clear; close all;

%% 1. Configuration Parameters
filename = '1- Static_Drift_Log.csv'; % Name of your data file
settling_time = 20; % Seconds to ignore at the beginning (Filter convergence)

%% 2. Load Data
disp('--> Loading data from CSV...');
if ~isfile(filename)
    error('File %s not found in the current directory!', filename);
end
data = readtable(filename);

time = data.Time_s;
qw = data.Qw; qx = data.Qx; qy = data.Qy; qz = data.Qz;
N = length(time);

%% 3. Quaternion to Euler Conversion (Roll, Pitch, Yaw)
roll  = zeros(N,1);
pitch = zeros(N,1);
yaw   = zeros(N,1);

for i = 1:N
    w = qw(i); x = qx(i); y = qy(i); z = qz(i);
    
    % Roll
    roll(i) = atan2(2*(w*x + y*z), 1 - 2*(x^2 + y^2)) * (180/pi);
    
    % Pitch
    sinp = 2*(w*y - z*x);
    if abs(sinp) >= 1
        pitch(i) = sign(sinp) * 90; 
    else
        pitch(i) = asin(sinp) * (180/pi);
    end
    
    % Yaw
    yaw(i) = atan2(2*(w*z + x*y), 1 - 2*(y^2 + z^2)) * (180/pi);
end

% Unwrap angles
roll  = unwrap(roll  * pi/180) * 180/pi;
pitch = unwrap(pitch * pi/180) * 180/pi;
yaw   = unwrap(yaw   * pi/180) * 180/pi;

%% 4. Extract Stable Data
stable_idx = time >= settling_time;
t_stable = time(stable_idx);
duration = t_stable(end) - t_stable(1);
duration_min = duration / 60;

% Relative angles (Zeroed out)
r_rel = roll(stable_idx) - roll(find(stable_idx, 1));
p_rel = pitch(stable_idx) - pitch(find(stable_idx, 1));
y_rel = yaw(stable_idx) - yaw(find(stable_idx, 1));

%% 5. Find Critical Points (Max & Min bounds)
[max_r, idx_max_r] = max(r_rel); [min_r, idx_min_r] = min(r_rel);
[max_p, idx_max_p] = max(p_rel); [min_p, idx_min_p] = min(p_rel);
[max_y, idx_max_y] = max(y_rel); [min_y, idx_min_y] = min(y_rel);

% Max Drift (Peak-to-Peak)
drift_R = max_r - min_r;
drift_P = max_p - min_p;
drift_Y = max_y - min_y;

% RMS Noise & Drift Rate
std_R = std(r_rel); std_P = std(p_rel); std_Y = std(y_rel);
rate_R = drift_R / duration_min; rate_P = drift_P / duration_min; rate_Y = drift_Y / duration_min;

%% 6. Print Report
fprintf('\n======================================================\n');
fprintf('       STATIC DRIFT TEST REPORT (ZERO-RATE OFFSET)      \n');
fprintf('======================================================\n');
fprintf('Total Stable Duration: %.2f Seconds (%.2f Min)\n', duration, duration_min);
fprintf('------------------------------------------------------\n');
fprintf('AXIS     | Max Drift (Deg) | Drift Rate (Deg/Min) | RMS Noise (Deg) \n');
fprintf('------------------------------------------------------\n');
fprintf('ROLL (X) | %14.4f | %19.4f | %14.4f \n', drift_R, rate_R, std_R);
fprintf('PITCH(Y) | %14.4f | %19.4f | %14.4f \n', drift_P, rate_P, std_P);
fprintf('YAW  (Z) | %14.4f | %19.4f | %14.4f \n', drift_Y, rate_Y, std_Y);
fprintf('======================================================\n\n');

%% 7. Visualization 1: Main Analysis Figure
figure('Name', 'Static Drift Analysis', 'Color', 'w', 'Position', [50, 100, 1000, 600]);

% Subplot 1: Absolute Raw Angles
subplot(2,1,1);
plot(time, roll, 'r', 'LineWidth', 1.5); hold on;
plot(time, pitch, 'g', 'LineWidth', 1.5);
plot(time, yaw, 'b', 'LineWidth', 1.5);
xline(settling_time, 'k-', 'Settling Time Cutoff', 'LineWidth', 1.5);
title('Absolute Orientation Over Time', 'FontSize', 12, 'FontWeight', 'bold');
xlabel('Time (s)'); ylabel('Angle (Deg)');
legend('Roll', 'Pitch', 'Yaw'); grid on;

% Subplot 2: Relative Drift with Bounds
subplot(2,1,2);
plot(t_stable, r_rel, 'r', 'LineWidth', 1.5); hold on;
plot(t_stable, p_rel, 'g', 'LineWidth', 1.5);
plot(t_stable, y_rel, 'b', 'LineWidth', 1.5);

% Plot light dashed lines for Max/Min boundaries
plot([t_stable(1) t_stable(end)], [max_r max_r], 'r--', 'Color', [1 0 0 0.3]);
plot([t_stable(1) t_stable(end)], [min_r min_r], 'r--', 'Color', [1 0 0 0.3]);
plot([t_stable(1) t_stable(end)], [max_p max_p], 'g--', 'Color', [0 1 0 0.3]);
plot([t_stable(1) t_stable(end)], [min_p min_p], 'g--', 'Color', [0 1 0 0.3]);
plot([t_stable(1) t_stable(end)], [max_y max_y], 'b--', 'Color', [0 0 1 0.3]);
plot([t_stable(1) t_stable(end)], [min_y min_y], 'b--', 'Color', [0 0 1 0.3]);

title('Relative Angle Drift with Max/Min Bounds', 'FontSize', 12, 'FontWeight', 'bold');
xlabel('Time (s)'); ylabel('Drift from Initial (Deg)');
grid on;

%% 8. Visualization 2: Critical Points Figure
figure('Name', 'Critical Drift Points', 'Color', 'w', 'Position', [150, 150, 1000, 450]);
plot(t_stable, r_rel, 'r', 'LineWidth', 1, 'Color', [1 0 0 0.6]); hold on;
plot(t_stable, p_rel, 'g', 'LineWidth', 1, 'Color', [0 1 0 0.6]);
plot(t_stable, y_rel, 'b', 'LineWidth', 1, 'Color', [0 0 1 0.6]);

% Mark Max Points (Triangles pointing UP)
plot(t_stable(idx_max_r), max_r, 'r^', 'MarkerSize', 8, 'MarkerFaceColor', 'r', 'DisplayName', 'Roll Max');
plot(t_stable(idx_max_p), max_p, 'g^', 'MarkerSize', 8, 'MarkerFaceColor', 'g', 'DisplayName', 'Pitch Max');
plot(t_stable(idx_max_y), max_y, 'b^', 'MarkerSize', 8, 'MarkerFaceColor', 'b', 'DisplayName', 'Yaw Max');

% Mark Min Points (Triangles pointing DOWN)
plot(t_stable(idx_min_r), min_r, 'rv', 'MarkerSize', 8, 'MarkerFaceColor', 'r', 'DisplayName', 'Roll Min');
plot(t_stable(idx_min_p), min_p, 'gv', 'MarkerSize', 8, 'MarkerFaceColor', 'g', 'DisplayName', 'Pitch Min');
plot(t_stable(idx_min_y), min_y, 'bv', 'MarkerSize', 8, 'MarkerFaceColor', 'b', 'DisplayName', 'Yaw Min');

title('Critical Error Points: Max/Min Deviations Captured', 'FontSize', 14, 'FontWeight', 'bold');
xlabel('Time (s)', 'FontSize', 11);
ylabel('Drift Magnitude (Deg)', 'FontSize', 11);
grid on; set(gca, 'GridAlpha', 0.4);

% Add text annotations next to the critical points for exact values
text(t_stable(idx_max_r), max_r, sprintf('  +%.2f°', max_r), 'Color', 'r', 'FontWeight', 'bold');
text(t_stable(idx_min_r), min_r, sprintf('  %.2f°', min_r), 'Color', 'r', 'FontWeight', 'bold');
text(t_stable(idx_max_p), max_p, sprintf('  +%.2f°', max_p), 'Color', 'g', 'FontWeight', 'bold');
text(t_stable(idx_min_p), min_p, sprintf('  %.2f°', min_p), 'Color', 'g', 'FontWeight', 'bold');
text(t_stable(idx_max_y), max_y, sprintf('  +%.2f°', max_y), 'Color', 'b', 'FontWeight', 'bold');
text(t_stable(idx_min_y), min_y, sprintf('  %.2f°', min_y), 'Color', 'b', 'FontWeight', 'bold');

legend('Roll Curve', 'Pitch Curve', 'Yaw Curve', 'AutoUpdate', 'off');