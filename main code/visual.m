% ---------------------------
% Connect to Arduino via Serial
% ---------------------------
clc;
close all;
port = "COM11";       % Change to match your Arduino port
baud = 9600;          % Must match Serial.begin() in Arduino
s = serialport(port, baud);
configureTerminator(s, "LF");
flush(s);  % Clear any buffered serial data

% ---------------------------
% Setup 3D Figure
% ---------------------------
figure;
axis equal;
xlim([-1.2 1.2]);
ylim([-1.2 1.2]);
zlim([-1.2 1.2]);
xlabel('X'); ylabel('Y'); zlabel('Z');
title('MPU9250 Orientation - 3D Axes');
grid on;
hold on;

% ---------------------------
% Loop and Read Data
% ---------------------------
while true
    try
        % Read one line of text (quaternion)
        line = readline(s);
        disp("From Arduino: " + line);

        % Parse quaternion from string
        q_vals = sscanf(line, '%f,%f,%f,%f');
        if numel(q_vals) ~= 4
            warning("Invalid data format");
            continue;
        end
        q = quaternion(q_vals(1), q_vals(2), q_vals(3), q_vals(4));  % MATLAB quaternion

        % Convert to rotation matrix
        R = quat2rotm(q);  % 3x3 rotation matrix

        % Extract rotated axes
        x_axis = R(:,1);  % Red - X
        y_axis = R(:,2);  % Green - Y
        z_axis = R(:,3);  % Blue - Z

        % Clear plot and draw new orientation
        cla;
        quiver3(0,0,0, x_axis(1), x_axis(2), x_axis(3), 1, 'r', 'LineWidth', 2); hold on;
        quiver3(0,0,0, y_axis(1), y_axis(2), y_axis(3), 1, 'g', 'LineWidth', 2);
        quiver3(0,0,0, z_axis(1), z_axis(2), z_axis(3), 1, 'b', 'LineWidth', 2);

        % Plot settings
        xlim([-1.2 1.2]); ylim([-1.2 1.2]); zlim([-1.2 1.2]);
        xlabel('X'); ylabel('Y'); zlabel('Z');
        title('MPU9250 Orientation - 3D Axes');
        grid on;
        axis equal;
        drawnow;

    catch ME
        warning("Error: %s", ME.message);
    end
end
