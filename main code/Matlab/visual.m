% ---------------------------
% Connect to Arduino via Serial
% ---------------------------
clc;
close all;
clear all;
port = "COM11";       % Change to match your Arduino port
baud = 115200;        % Must match Serial.begin() in Arduino
s = serialport(port, baud);
configureTerminator(s, "LF");
flush(s);
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
        q_vals = sscanf(line, '%f,%f,%f,%f');
        if numel(q_vals) == 4
            valid_data = true;
            disp('Valid quaternion data received. Starting visualization...');
        else
            disp('Waiting for quaternion data...');
        end
    catch ME
        disp("Serial read error: " + ME.message);
        pause(0.1);
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
view(ax, [45 30]);
set(ax, 'DataAspectRatio', [1 1 1]);
set(ax, 'CameraViewAngleMode', 'manual');
set(ax, 'CameraViewAngle', 7);
axis(ax, 'manual');

% Add a close button
uicontrol('Style', 'pushbutton', 'String', 'Close', ...
    'Position', [20 20 60 20], ...
    'Callback', @(src, event) set(fig, 'UserData', 'close'));

% ---------------------------
% Loop and Read Data
% ---------------------------
try
    while true
        % Check for close button press
        if isfield(get(fig), 'UserData') && strcmp(get(fig, 'UserData'), 'close')
            disp('Close button pressed. Exiting...');
            cleanupSerial(s);
            close(fig);
            break;
        end

        % Read one line of text
        line = readline(s);
        disp("From Arduino: " + line);

        % Parse quaternion from string
        q_vals = sscanf(line, '%f,%f,%f,%f');
        if numel(q_vals) ~= 4
            disp("Skipping invalid data format");
            continue;
        end
        q = quaternion(q_vals(1), q_vals(2), q_vals(3), q_vals(4));

        % Convert to rotation matrix
        R = quat2rotm(q);

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
        view(ax, [45 0]);
        set(ax, 'DataAspectRatio', [1 1 1]);
        set(ax, 'CameraViewAngle', 7);
        axis(ax, 'manual');
        drawnow;
    end
catch ME
    disp("Error or interruption: " + ME.message);
    cleanupSerial(s);
    close(fig);
end

% Cleanup function to close serial port
function cleanupSerial(s)
    if ~isempty(s) && isvalid(s)
        disp('Closing serial port...');
        flush(s);  % Flush any remaining data
        delete(s); % Explicitly close the serial port
        clear s;   % Clear the variable
    end
end