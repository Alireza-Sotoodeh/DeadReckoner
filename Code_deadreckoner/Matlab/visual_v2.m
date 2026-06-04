% ---------------------------
% PROJECT: DeadReckoner - Drift Test Visualizer & Logger
% ---------------------------
clc;
close all;
clear all;

port = "COM11";       % Change to match your Arduino port
baud = 115200;        % Must match Serial.begin() in Arduino

% ---------------------------
% Setup Serial Connection
% ---------------------------
try
    s = serialport(port, baud);
    configureTerminator(s, "LF");
    flush(s);
catch ME
    error('Cannot connect to %s. Ensure it is not open in another app.', port);
end

% ---------------------------
% Setup Data Logger (CSV)
% ---------------------------
logFileName = 'Static_Drift_Log.csv';
logFile = fopen(logFileName, 'w');
if logFile == -1
    error('Cannot create log file. Check folder permissions.');
end
fprintf(logFile, 'Time_s,Qw,Qx,Qy,Qz\n');
logInterval = 1.0; % Log data every 1.0 second to keep file size small
lastLogTime = 0;

% ---------------------------
% Setup 3D Figure
% ---------------------------
% Added CloseRequestFcn to handle the window 'X' button safely
fig = figure('Name', 'MPU9250 Drift Monitor', 'NumberTitle', 'off', ...
             'CloseRequestFcn', @(src, event) set(src, 'UserData', 'close'));
ax = axes('Parent', fig);
xlim(ax, [-1.2 1.2]); ylim(ax, [-1.2 1.2]); zlim(ax, [-1.2 1.2]);
xlabel(ax, 'X'); ylabel(ax, 'Y'); zlabel(ax, 'Z');
title(ax, 'MPU9250 Orientation - Drift Test');
grid(ax, 'on');
hold(ax, 'on');
view(ax, [45 30]);
set(ax, 'DataAspectRatio', [1 1 1]);
set(ax, 'CameraViewAngleMode', 'manual');
set(ax, 'CameraViewAngle', 7);
axis(ax, 'manual');

% Add a close button
uicontrol('Style', 'pushbutton', 'String', 'Stop & Save', ...
    'Position', [20 20 100 30], ...
    'Callback', @(src, event) set(fig, 'UserData', 'close'));

% ---------------------------
% Loop and Read Data
% ---------------------------
disp('Waiting for valid quaternion data...');
startTime = tic; % Start test timer

try
    while true
        % Check if user requested to close (via Button or 'X')
        if isfield(get(fig), 'UserData') && strcmp(get(fig, 'UserData'), 'close')
            disp('Test stopped by user. Executing safe shutdown...');
            break;
        end

        % Only read if data is available (prevents UI freezing)
        if s.NumBytesAvailable > 0
            line = readline(s);
            
            % Parse quaternion from string
            q_vals = sscanf(line, '%f,%f,%f,%f');
            if numel(q_vals) ~= 4
                continue; % Skip bad frames implicitly
            end
            q = quaternion(q_vals(1), q_vals(2), q_vals(3), q_vals(4));

            % --- Data Logging Logic ---
            currentTime = toc(startTime);
            if currentTime - lastLogTime >= logInterval
                fprintf(logFile, '%.2f,%.6f,%.6f,%.6f,%.6f\n', ...
                        currentTime, q_vals(1), q_vals(2), q_vals(3), q_vals(4));
                lastLogTime = currentTime;
                
                % Update terminal status without flooding it
                fprintf('Test Running... Time Elapsed: %.0f seconds\n', currentTime);
            end

            % --- Visualization Logic ---
            R = quat2rotm(q);
            x_axis = R(:,1);  % Red - X
            y_axis = R(:,2);  % Green - Y
            z_axis = R(:,3);  % Blue - Z

            cla(ax);
            quiver3(ax, 0, 0, 0, x_axis(1), x_axis(2), x_axis(3), 1, 'r', 'LineWidth', 2); hold(ax, 'on');
            quiver3(ax, 0, 0, 0, y_axis(1), y_axis(2), y_axis(3), 1, 'g', 'LineWidth', 2);
            quiver3(ax, 0, 0, 0, z_axis(1), z_axis(2), z_axis(3), 1, 'b', 'LineWidth', 2);

            xlim(ax, [-1.2 1.2]); ylim(ax, [-1.2 1.2]); zlim(ax, [-1.2 1.2]);
            xlabel(ax, 'X'); ylabel(ax, 'Y'); zlabel(ax, 'Z');
            title(ax, sprintf('MPU9250 Orientation | Time: %.0f s', currentTime));
            grid(ax, 'on');
            view(ax, [45 30]);
            set(ax, 'DataAspectRatio', [1 1 1]);
            set(ax, 'CameraViewAngle', 7);
            axis(ax, 'manual');
            drawnow;
        else
            % Give MATLAB UI time to process button clicks
            pause(0.01);
        end
    end
catch ME
    disp("Error or interruption: " + ME.message);
end

% ---------------------------
% Explicit Cleanup (Guaranteed to run)
% ---------------------------
disp('Closing resources...');

% 1. Close Serial Port safely
if exist('s', 'var') && isvalid(s)
    flush(s);  
    delete(s); 
    clear s;   
    disp('COM Port closed safely.');
end

% 2. Close Log File safely
if exist('logFile', 'var') && logFile ~= -1
    fclose(logFile);
    disp('Log file saved successfully as Static_Drift_Log.csv');
end

% 3. Close the Figure
if exist('fig', 'var') && isgraphics(fig)
    delete(fig);
end

disp('Ready for next run. (No restart needed)');