% =========================================================================
% PROJECT: DeadReckoner - IMU Sensor Fusion
% MODULE: Dynamic Return-to-Zero Analyzer (Anti-Aliasing & True Math)
% AUTHOR: Alireza Sotoodeh
% =========================================================================
clc; clear; close all;

%% 1. Configuration Parameters
filename = '2_Dynamic_Return_to_Zero.csv'; 
startup_ignore_sec = 20; % Strictly ignore first 20s of startup noise
motion_threshold = 3.0; % Threshold for velocity in Deg/sec

%% 2. Load Data
disp('--> Loading data from CSV...');
if ~isfile(filename), error('File %s not found!', filename); end
data = readtable(filename);
time = data.Time_s;
qw = data.Qw; qx = data.Qx; qy = data.Qy; qz = data.Qz;
N = length(time);

%% 3. Quaternion to Euler (Strictly [-180, 180])
roll = zeros(N,1); pitch = zeros(N,1); yaw = zeros(N,1);
for i = 1:N
    w = qw(i); x = qx(i); y = qy(i); z = qz(i);
    roll(i) = atan2(2*(w*x + y*z), 1 - 2*(x^2 + y^2)) * (180/pi);
    sinp = 2*(w*y - z*x);
    if abs(sinp) >= 1, pitch(i) = sign(sinp) * 90; else, pitch(i) = asin(sinp) * (180/pi); end
    yaw(i) = atan2(2*(w*z + x*y), 1 - 2*(y^2 + z^2)) * (180/pi);
end

%% Helper: Circular Difference (Prevents 360 jump errors)
calc_diff = @(a, b) mod((b - a) + 180, 360) - 180;

%% 4. True Angular Velocity & Phase Detection
vel = zeros(N,1);
for i = 2:N
    dt = time(i) - time(i-1);
    if dt == 0, dt = 0.01; end % safety
    vR = calc_diff(roll(i-1), roll(i)) / dt;
    vP = calc_diff(pitch(i-1), pitch(i)) / dt;
    vY = calc_diff(yaw(i-1), yaw(i)) / dt;
    vel(i) = abs(vR) + abs(vP) + abs(vY);
end

% Ignore startup noise
ignore_idx = find(time < startup_ignore_sec, 1, 'last');
if isempty(ignore_idx), ignore_idx = 1; end

is_moving = vel > motion_threshold;
is_moving(1:ignore_idx) = 0; % Mute startup

first_move = find(is_moving, 1, 'first');
last_move = find(is_moving, 1, 'last');

if isempty(first_move)
    first_move = floor(N/3); last_move = floor(2*N/3);
end

% Define precise phase indices
idx_init  = (first_move - 5) : (first_move - 1); % 5 samples right before movement
if idx_init(1) < 1, idx_init = 1:(first_move-1); end
idx_final = (N - 5) : N; % Last 5 samples of the file
idx_dyn   = first_move : last_move;

%% 5. Calculate TRUE Circular Metrics
% Standard Mean for Pitch/Roll
init_R = mean(roll(idx_init)); init_P = mean(pitch(idx_init));
final_R = mean(roll(idx_final)); final_P = mean(pitch(idx_final));

% Circular Mean for Yaw (Crucial for values near +/- 180)
init_Y_rad = mean(exp(1i * yaw(idx_init) * pi/180)); init_Y = angle(init_Y_rad) * 180/pi;
final_Y_rad = mean(exp(1i * yaw(idx_final) * pi/180)); final_Y = angle(final_Y_rad) * 180/pi;

% Return-to-Zero Errors
err_R = abs(calc_diff(init_R, final_R));
err_P = abs(calc_diff(init_P, final_P));
err_Y = abs(calc_diff(init_Y, final_Y));

pct_R = (err_R / 360) * 100; pct_P = (err_P / 360) * 100; pct_Y = (err_Y / 360) * 100;

% Max Deviation (Stress) from Initial Position
dev_R = zeros(length(idx_dyn),1); dev_P = zeros(length(idx_dyn),1); dev_Y = zeros(length(idx_dyn),1);
for k = 1:length(idx_dyn)
    i = idx_dyn(k);
    dev_R(k) = abs(calc_diff(init_R, roll(i)));
    dev_P(k) = abs(calc_diff(init_P, pitch(i)));
    dev_Y(k) = abs(calc_diff(init_Y, yaw(i)));
end
max_dev_R = max(dev_R); max_dev_P = max(dev_P); max_dev_Y = max(dev_Y);

%% 6. Print Report
fprintf('\n===================================================================\n');
fprintf('       TRUE DYNAMIC RETURN-TO-ZERO TEST REPORT (ANTI-ALIAS)          \n');
fprintf('===================================================================\n');
fprintf('Phase 1 (Initial Stable) : %.1f sec to %.1f sec\n', time(idx_init(1)), time(idx_init(end)));
fprintf('Phase 2 (Dynamic Motion) : %.1f sec to %.1f sec\n', time(first_move), time(last_move));
fprintf('Phase 3 (Final Stable)   : %.1f sec to %.1f sec\n', time(idx_final(1)), time(idx_final(end)));
fprintf('-------------------------------------------------------------------\n');
fprintf('AXIS     | Max Deviation(Deg)| Return Error (Deg) | Error (%% FS) \n');
fprintf('-------------------------------------------------------------------\n');
fprintf('ROLL (X) | %17.2f | %18.4f | %10.4f %% \n', max_dev_R, err_R, pct_R);
fprintf('PITCH(Y) | %17.2f | %18.4f | %10.4f %% \n', max_dev_P, err_P, pct_P);
fprintf('YAW  (Z) | %17.2f | %18.4f | %10.4f %% \n', max_dev_Y, err_Y, pct_Y);
fprintf('===================================================================\n\n');

%% 7. Visualization: Timeline without vertical wrap-around lines
figure('Name', 'True Dynamic RTZ Timeline', 'Color', 'w', 'Position', [100, 100, 1000, 500]);
hold on;

% Function to insert NaNs where jumps > 180 occur (hides ugly vertical lines)
break_wrap = @(x,y) insert_nans(x,y);
[t_R, r_plot] = break_wrap(time, roll);
[t_P, p_plot] = break_wrap(time, pitch);
[t_Y, y_plot] = break_wrap(time, yaw);

% Patches
ylim([-190, 190]);
patch([time(idx_init(1)) time(idx_init(end)) time(idx_init(end)) time(idx_init(1))], [-200 -200 200 200], [0.8 0.9 0.8], 'EdgeColor', 'none', 'FaceAlpha', 0.5, 'DisplayName', 'Init Phase');
patch([time(first_move) time(last_move) time(last_move) time(first_move)], [-200 -200 200 200], [0.9 0.8 0.8], 'EdgeColor', 'none', 'FaceAlpha', 0.5, 'DisplayName', 'Dynamic Phase');
patch([time(idx_final(1)) time(idx_final(end)) time(idx_final(end)) time(idx_final(1))], [-200 -200 200 200], [0.8 0.9 0.8], 'EdgeColor', 'none', 'FaceAlpha', 0.5, 'DisplayName', 'Final Phase');

% Plot Broken Lines
plot(t_R, r_plot, 'r', 'LineWidth', 1.5, 'DisplayName', 'Roll (Raw)');
plot(t_P, p_plot, 'g', 'LineWidth', 1.5, 'DisplayName', 'Pitch (Raw)');
plot(t_Y, y_plot, 'b', 'LineWidth', 1.5, 'DisplayName', 'Yaw (Raw)');

title('True Dynamic Motion Timeline (Wrap-Around Corrected)', 'FontSize', 14, 'FontWeight', 'bold');
xlabel('Time (Seconds)', 'FontSize', 11); ylabel('Raw Angle (Degrees)', 'FontSize', 11);
legend('Location', 'best'); grid on; set(gca, 'GridAlpha', 0.3);

%% 8. Bar Chart
figure('Name', 'Return Error', 'Color', 'w', 'Position', [150, 150, 800, 400]);
errors = [err_R, err_P, err_Y]; labels = {'Roll (X)', 'Pitch (Y)', 'Yaw (Z)'};
b = bar(errors, 'FaceColor', 'flat');
b.CData(1,:) = [0.8500 0.3250 0.0980]; b.CData(2,:) = [0.4660 0.6740 0.1880]; b.CData(3,:) = [0.0000 0.4470 0.7410]; 
set(gca, 'xticklabel', labels, 'FontSize', 11, 'FontWeight', 'bold');
ylabel('Absolute Error (Degrees)', 'FontSize', 12); title('True Filter Recovery Error', 'FontSize', 14); grid on;
for i = 1:length(errors), text(i, errors(i), sprintf(' %.3f°', errors(i)), 'HorizontalAlignment', 'center', 'VerticalAlignment', 'bottom', 'FontSize', 12, 'FontWeight', 'bold'); end

%% Helper Function
function [x_out, y_out] = insert_nans(x, y)
    x_out = x; y_out = y;
    jumps = find(abs(diff(y)) > 100);
    for i = length(jumps):-1:1
        idx = jumps(i);
        x_out = [x_out(1:idx); NaN; x_out(idx+1:end)];
        y_out = [y_out(1:idx); NaN; y_out(idx+1:end)];
    end
end