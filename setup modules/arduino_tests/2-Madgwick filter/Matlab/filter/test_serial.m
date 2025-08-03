% Open serial connection (adjust port and baud rate as needed)
s = serialport('COM10', 115200);  % Your Arduino's port
configureTerminator(s, 'LF');
flush(s);  % Clear any buffered data

% Prepare figure for 3D visualization
figure;
axis equal;
xlim([-1.5 1.5]);
ylim([-1.5 1.5]);
zlim([-1.5 1.5]);
grid on;
xlabel('X');
ylabel('Y');
zlabel('Z');
title('3D Axes Visualization (With Madgwick Filter)');
view(3);

try
    while true
        % Read a line from serial
        line = readline(s);
        disp(line);  % DEBUG: Print raw line
        
        % Split on commas and trim whitespace
        parts = strtrim(split(line, ','));
        nums = str2double(parts);
        
        if length(nums) == 4 && all(~isnan(nums))  % Check for exactly 4 valid numbers
            q0 = nums(1);
            q1 = nums(2);
            q2 = nums(3);
            q3 = nums(4);
            
            % Normalize quaternion if needed
            norm_q = sqrt(q0^2 + q1^2 + q2^2 + q3^2);
            if norm_q > 0
                q0 = q0 / norm_q;
                q1 = q1 / norm_q;
                q2 = q2 / norm_q;
                q3 = q3 / norm_q;
            end
            
            % Compute rotation matrix (v_body = R * v_earth)
            R = quatToRotMat(q0, q1, q2, q3);
            
            % Directions of body axes in earth frame
            x_dir = R' * [1; 0; 0];
            y_dir = R' * [0; 1; 0];
            z_dir = R' * [0; 0; 1];
            
            % Plot the axes
            clf;
            hold on;
            plot3([0 x_dir(1)], [0 x_dir(2)], [0 x_dir(3)], 'r', 'LineWidth', 2);  % X axis (red)
            plot3([0 y_dir(1)], [0 y_dir(2)], [0 y_dir(3)], 'g', 'LineWidth', 2);  % Y axis (green)
            plot3([0 z_dir(1)], [0 z_dir(2)], [0 z_dir(3)], 'b', 'LineWidth', 2);  % Z axis (blue)
            axis equal;
            xlim([-1.5 1.5]);
            ylim([-1.5 1.5]);
            zlim([-1.5 1.5]);
            grid on;
            xlabel('X');
            ylabel('Y');
            zlabel('Z');
            title('3D Axes Visualization (With Madgwick Filter)');
            view(3);
            drawnow;
        else
            disp('Invalid data: Not 4 numbers');  % DEBUG: If parsing fails
        end
    end
catch e
    disp(e.message);  % Show any error
    clear s;  % Close serial on exit
end

% Function to compute rotation matrix from quaternion [q0 q1 q2 q3]
function R = quatToRotMat(q0, q1, q2, q3)
    R = [1 - 2*(q2^2 + q3^2), 2*(q1*q2 - q0*q3), 2*(q1*q3 + q0*q2);
         2*(q1*q2 + q0*q3), 1 - 2*(q1^2 + q3^2), 2*(q2*q3 - q0*q1);
         2*(q1*q3 - q0*q2), 2*(q2*q3 + q0*q1), 1 - 2*(q1^2 + q2^2)];
end
