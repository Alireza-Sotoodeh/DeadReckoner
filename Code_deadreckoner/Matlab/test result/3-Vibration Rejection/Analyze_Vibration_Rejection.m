% =========================================================================
% PROJECT: DeadReckoner - IMU Sensor Fusion
% MODULE: Comprehensive Vibration Rejection Analyzer
% AUTHOR: Alireza Sotoodeh
% =========================================================================
clc; clear; close all;

%% 1. Configuration Parameters
filename = '3_Vibration_Rejection.csv'; 
startup_ignore_sec = 15; % Ignore initial filter convergence noise
noise_window_sec = 3.0;  % Moving window size (in seconds) to calculate RMS Noise

%% 2. Load Data
disp('--> Loading data from CSV...');
if ~isfile(filename), error('File %s not found!', filename); end
data = readtable(filename);
time = data.Time_s;
qw = data.Qw; qx = data.Qx; qy = data.Qy; qz = data.Qz;
N = length(time);

%% 3. Quaternion to Euler (Raw [-180, 180])
roll = zeros(N,1); pitch = zeros(N,1); yaw = zeros(N,1);
for i = 1:N
    w = qw(i); x = qx(i); y = qy(i); z = qz(i);
    roll(i) = atan2(2*(w*x + y*z), 1 - 2*(x^2 + y^2)) * (180/pi);
    sinp = 2*(w*y - z*x);
    if abs(sinp) >= 1, pitch(i) = sign(sinp) * 90; else, pitch(i) = asin(sinp) * (180/pi); end
    yaw(i) = atan2(2*(w*z + x*y), 1 - 2*(y^2 + z^2)) * (180/pi);
end

%% Helper: Circular Difference
calc_diff = @(a, b) mod((b - a) + 180, 360) - 180;

%% 4. Baseline Calculation & Auto-Zeroing
% Find the baseline (quiet period) just after startup to "zero out" the angles
base_idx = (time >= startup_ignore_sec) & (time <= startup_ignore_sec + 10);
if sum(base_idx) == 0, base_idx = time >= time(1); end % Fallback

base_R = mean(roll(base_idx));
base_P = mean(pitch(base_idx));
% Circular mean for Yaw to prevent wrap-around bugs near +/-180
base_Y_rad = mean(exp(1i * yaw(base_idx) * pi/180)); 
base_Y = angle(base_Y_rad) * 180/pi;

% Calculate Pure Relative Deviations (Centered at 0)
dev_R = zeros(N,1); dev_P = zeros(N,1); dev_Y = zeros(N,1);
for i = 1:N
    dev_R(i) = calc_diff(base_R, roll(i));
    dev_P(i) = calc_diff(base_P, pitch(i));
    dev_Y(i) = calc_diff(base_Y, yaw(i));
end

%% 5. Rolling Noise (Seismograph Logic)
% Calculate sampling rate roughly
dt = mean(diff(time));
if dt <= 0 || isnan(dt), dt = 0.01; end
window_size = max(3, round(noise_window_sec / dt));

noise_R = movstd(dev_R, window_size);
noise_P = movstd(dev_P, window_size);
noise_Y = movstd(dev_Y, window_size);

%% 6. Calculate Metrics (Ignoring Startup)
valid_idx = time >= startup_ignore_sec;

% Max Positive Spikes
max_pos_R = max(dev_R(valid_idx));
max_pos_P = max(dev_P(valid_idx));
max_pos_Y = max(dev_Y(valid_idx));

% Max Negative Spikes
max_neg_R = min(dev_R(valid_idx));
max_neg_P = min(dev_P(valid_idx));
max_neg_Y = min(dev_Y(valid_idx));

% Peak-to-Peak Swing (Total Vibration Amplitude)
p2p_R = max_pos_R - max_neg_R;
p2p_P = max_pos_P - max_neg_P;
p2p_Y = max_pos_Y - max_neg_Y;

% Overall RMS Noise
rms_R = std(dev_R(valid_idx));
rms_P = std(dev_P(valid_idx));
rms_Y = std(dev_Y(valid_idx));

%% 7. Print Report
fprintf('\n=======================================================================\n');
fprintf('               VIBRATION REJECTION TEST REPORT                         \n');
fprintf('=======================================================================\n');
fprintf('Test Duration: %.1f sec (First %d sec ignored for startup)\n', time(end), startup_ignore_sec);
fprintf('-----------------------------------------------------------------------\n');
fprintf('AXIS     | Max Positive | Max Negative | Peak-to-Peak | Overall RMS \n');
fprintf('         | Spike (Deg)  | Spike (Deg)  | Swing (Deg)  | Noise (Deg) \n');
fprintf('-----------------------------------------------------------------------\n');
fprintf('ROLL (X) | %12.3f | %12.3f | %12.3f | %11.3f \n', max_pos_R, max_neg_R, p2p_R, rms_R);
fprintf('PITCH(Y) | %12.3f | %12.3f | %12.3f | %11.3f \n', max_pos_P, max_neg_P, p2p_P, rms_P);
fprintf('YAW  (Z) | %12.3f | %12.3f | %12.3f | %11.3f \n', max_pos_Y, max_neg_Y, p2p_Y, rms_Y);
fprintf('=======================================================================\n\n');

%% 8. Visualization 1: Relative Deviation & Seismograph
figure('Name', 'Vibration Analysis', 'Color', 'w', 'Position', [50, 50, 1000, 700]);

% Subplot 1: Pure Angular Deviation
subplot(2,1,1);
plot(time, dev_R, 'r', 'LineWidth', 1.5, 'DisplayName', 'Roll Deviation'); hold on;
plot(time, dev_P, 'g', 'LineWidth', 1.5, 'DisplayName', 'Pitch Deviation');
plot(time, dev_Y, 'b', 'LineWidth', 1.5, 'DisplayName', 'Yaw Deviation');
xline(startup_ignore_sec, 'k--', 'Filter Ready', 'LineWidth', 1.5);
yline(0, 'k-', 'HandleVisibility', 'off'); % Center line

% Set dynamic Y-limits to zoom in on the vibrations
max_dev = max([p2p_R, p2p_P, p2p_Y]) / 2;
ylim([-(max_dev + 1), (max_dev + 1)]);

title('Angular Deviation During Mechanical Shocks (Zero-Centered)', 'FontSize', 12, 'FontWeight', 'bold');
xlabel('Time (s)'); ylabel('Deviation from Baseline (Deg)');
legend('Location', 'best'); grid on; set(gca, 'GridAlpha', 0.4);

% Subplot 2: Rolling RMS Noise (Seismograph)
subplot(2,1,2);
plot(time, noise_R, 'r', 'LineWidth', 1.2, 'DisplayName', 'Roll Noise'); hold on;
plot(time, noise_P, 'g', 'LineWidth', 1.2, 'DisplayName', 'Pitch Noise');
plot(time, noise_Y, 'b', 'LineWidth', 1.2, 'DisplayName', 'Yaw Noise');
xline(startup_ignore_sec, 'k--', 'LineWidth', 1.5, 'HandleVisibility', 'off');

title('Vibration Intensity over Time (Rolling RMS Noise)', 'FontSize', 12, 'FontWeight', 'bold');
xlabel('Time (s)'); ylabel('Noise Level (Deg)');
legend('Location', 'best'); grid on; set(gca, 'GridAlpha', 0.4);

%% 9. Visualization 2: Max Vibration Swing (Bar Chart)
figure('Name', 'Vibration Swing Analysis', 'Color', 'w', 'Position', [150, 150, 700, 400]);

swings = [p2p_R, p2p_P, p2p_Y];
labels = {'Roll (X)', 'Pitch (Y)', 'Yaw (Z)'};

b = bar(swings, 'FaceColor', 'flat');
b.CData(1,:) = [0.8500 0.3250 0.0980]; 
b.CData(2,:) = [0.4660 0.6740 0.1880]; 
b.CData(3,:) = [0.0000 0.4470 0.7410]; 

set(gca, 'xticklabel', labels, 'FontSize', 11, 'FontWeight', 'bold');
ylabel('Peak-to-Peak Swing (Degrees)', 'FontSize', 12);
title('Maximum Angular Fluctuation During Physical Impacts', 'FontSize', 14);
grid on; set(gca, 'GridAlpha', 0.3);

for i = 1:length(swings)
    text(i, swings(i), sprintf(' %.3f°', swings(i)), ...
        'HorizontalAlignment', 'center', 'VerticalAlignment', 'bottom', ...
        'FontSize', 12, 'FontWeight', 'bold');
end