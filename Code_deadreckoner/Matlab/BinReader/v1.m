% Script to parse DR_LOG.BIN from ESP32-S3 Dead Reckoning System
clear; clc; close all;

filename = 'DR_LOG.BIN';
fid = fopen(filename, 'rb');

if fid == -1
    error('Could not open DR_LOG.BIN. Make sure it is in the same directory.');
end

% Get file size to calculate number of frames
fseek(fid, 0, 'eof');
fileSize = ftell(fid);
fseek(fid, 0, 'bof');

frameSizeBytes = 48; % 4(uint32) + 16(4*float) + 12(3*float) + 16(2*double)
numFrames = floor(fileSize / frameSizeBytes);

fprintf('Found %d frames in the log file.\n', numFrames);

% Preallocate arrays for speed
timestamp = zeros(numFrames, 1, 'uint32');
q = zeros(numFrames, 4, 'single');
accel = zeros(numFrames, 3, 'single');
gps = zeros(numFrames, 2, 'double');

% Read the binary file frame by frame
for i = 1:numFrames
    timestamp(i) = fread(fid, 1, 'uint32');
    q(i, :) = fread(fid, 4, 'single');
    accel(i, :) = fread(fid, 3, 'single');
    gps(i, :) = fread(fid, 2, 'double');
end

fclose(fid);

% --- Visualization ---
% Convert timestamp from milliseconds to seconds
time_sec = double(timestamp - timestamp(1)) / 1000;

figure('Name', 'DeadReckoner Sensor Logs', 'Position', [100, 100, 1000, 600]);

% Plot Quaternions
subplot(2,1,1);
plot(time_sec, q(:,1), 'k', 'LineWidth', 1.5); hold on;
plot(time_sec, q(:,2), 'r');
plot(time_sec, q(:,3), 'g');
plot(time_sec, q(:,4), 'b');
title('Quaternion Data over Time');
legend('Qw', 'Qx', 'Qy', 'Qz');
xlabel('Time (s)');
ylabel('Value');
grid on;

% Plot Linear Acceleration
subplot(2,1,2);
plot(time_sec, accel(:,1), 'r', 'LineWidth', 1); hold on;
plot(time_sec, accel(:,2), 'g', 'LineWidth', 1);
plot(time_sec, accel(:,3), 'b', 'LineWidth', 1);
title('Linear Acceleration (g)');
legend('Acc X', 'Acc Y', 'Acc Z');
xlabel('Time (s)');
ylabel('Acceleration');
grid on;