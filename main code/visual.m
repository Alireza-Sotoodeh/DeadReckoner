% ---------------------------
% Connect to Arduino via Serial
% ---------------------------
clc;
close all;
clear all;  % Clear workspace to remove any lingering serialport objects
port = "COM11";       % Change to match your Arduino port
baud = 9600;          % Must match Serial.begin() in Arduino
s = serialport(port, baud);
configureTerminator(s, "LF");
flush(s);  % Clear any buffered serial data
% Create cleanup object to close serial port on script termination
cleanupObj = onCleanup(@() cleanupSerial(s));

% ---------------------------
% Wait for Valid Quaternion Data
% ---------------------------
disp('Waiting for valid quaternion data...');
valid_data = false;
while ~valid_data
    try
        line = readline(s);
        disp("From Arduino: " + line);
        
        % Check if the line contains valid quaternion data (four comma-separated numbers)
        q_vals = sscanf(line, '%f,%f,%f,%f');
        if numel(q_vals) == 4
            valid_data = true;  % Valid quaternion data received
            disp('Valid quaternion data received. Starting visualization...');
        else
            disp('Waiting for quaternion data...');
        end
    catch ME
        disp("Serial read error: " + ME.message);
        pause(0.1);  % Brief pause to avoid flooding
    end
end

% ---------------------------
% Setup 3D Figure
% ---------------------------
fig = figure;
ax = axes('Parent', fig);
xlim(ax, [-1.2 1.2]);
ylim(ax, [-1.2 1.2]);
zlim(ax, [-1.2 1.2]);
xlabel(ax, 'X'); ylabel(ax, 'Y'); zlabel(ax, 'Z');
title(ax, 'MPU9250 Orientation - 3D Axes');
grid(ax, 'on');
hold(ax, 'on');
view(ax, [45 30]);  % Set fixed viewpoint (azimuth=45, elevation=30)
set(ax, 'DataAspectRatio', [1 1 1]);  % Ensure equal scaling for all axes
set(ax, 'CameraViewAngleMode', 'manual');  % Prevent zooming effect
set(ax, 'CameraViewAngle', 7);  % Set fixed zoom level
axis(ax, 'manual');  % Prevent automatic axis adjustments

% ---------------------------
% Loop and Read Data
% ---------------------------
while true
    try
        % Read one line of text
        line = readline(s);
        disp("From Arduino: " + line);

        % Parse quaternion from string
        q_vals = sscanf(line, '%f,%f,%f,%f');
        if numel(q_vals) ~= 4
            disp("Skipping invalid data format");
            continue;
        end
        q = quaternion(q_vals(1), q_vals(2), q_vals(3), q_vals(4));  % MATLAB quaternion

        % Convert to rotation matrix
        R = quat2rotm(q);  % 3x3 rotation matrix

        % Extract and normalize rotated axes
        x_axis = R(:,1);  % Red - X
        y_axis = R(:,2);  % Green - Y
        z_axis = R(:,3);  % Blue - Z

        % Clear previous plot and draw new orientation
        cla(ax);
        quiver3(ax, 0, 0, 0, x_axis(1), x_axis(2), x_axis(3), 1, 'r', 'LineWidth', 2); hold(ax, 'on');
        quiver3(ax, 0, 0, 0, y_axis(1), y_axis(2), y_axis(3), 1, 'g', 'LineWidth', 2);
        quiver3(ax, 0, 0, 0, z_axis(1), z_axis(2), z_axis(3), 1, 'b', 'LineWidth', 2);

        % Maintain fixed plot settings
        xlim(ax, [-1.2 1.2]); ylim(ax, [-1.2 1.2]); zlim(ax, [-1.2 1.2]);
        xlabel(ax, 'X'); ylabel(ax, 'Y'); zlabel(ax, 'Z');
        title(ax, 'MPU9250 Orientation - 3D Axes');
        grid(ax, 'on');
        view(ax, [45 30]);  % Reinforce fixed viewpoint
        set(ax, 'DataAspectRatio', [1 1 1]);  % Reinforce equal scaling
        set(ax, 'CameraViewAngle', 7);  % Reinforce fixed zoom
        axis(ax, 'manual');  % Prevent automatic axis adjustments
        drawnow;

    catch ME
        disp("Error: " + ME.message);
        pause(0.1);  % Brief pause to avoid flooding
    end
end

% Cleanup function to close serial port
function cleanupSerial(s)
    if ~isempty(s)
        disp('Closing serial port...');
        clear s;  % Close and delete the serialport object
    end
end