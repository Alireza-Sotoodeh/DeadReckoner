% =========================================================================
% PROJECT: DeadReckoner - Data Extraction & Diagnostic Pipeline
% DESCRIPTION: Parses binary .BIN logs from ESP32, decodes the 49-byte 
% packed struct, identifies dropped frames, SD failures, and User Tags.
% =========================================================================

clc; clear; close all;

% --- CONFIGURATION ---
% Explicitly defined 49 bytes to match __attribute__((packed)) in ESP32
FRAME_SIZE = 49; 

% Find all .BIN files in the current MATLAB directory
filePattern = fullfile(pwd, '*.BIN');
binFiles = dir(filePattern);

if isempty(binFiles)
    fprintf('CRITICAL: No .BIN files found in %s\n', pwd);
    return;
end

for k = 1:length(binFiles)
    baseFileName = binFiles(k).name;
    fprintf('\n=======================================\n');
    fprintf('Processing Log File: %s\n', baseFileName);
    
    % 1. Open and Read Raw Binary Data
    fid = fopen(baseFileName, 'rb');
    if fid == -1
        warning('Could not open %s', baseFileName);
        continue;
    end
    rawData = fread(fid, inf, '*uint8');
    fclose(fid);
    
    % 2. Validate Data Integrity
    numFrames = floor(length(rawData) / FRAME_SIZE);
    if numFrames == 0
        warning('File %s is empty or corrupted.', baseFileName);
        continue;
    end
    
    % 3. Preallocate Memory for Speed
    frame_seq = zeros(numFrames, 1, 'uint32');
    q = zeros(numFrames, 4, 'single');
    acc = zeros(numFrames, 3, 'single');
    gps = zeros(numFrames, 2, 'double');
    event_flag = zeros(numFrames, 1, 'uint8');
    
    % 4. Byte-Level Decoding (Endian-safe)
    for i = 1:numFrames
        offset = (i - 1) * FRAME_SIZE;
        
        frame_seq(i) = typecast(rawData(offset+1 : offset+4), 'uint32');
        q(i, 1) = typecast(rawData(offset+5 : offset+8), 'single');    % Qw
        q(i, 2) = typecast(rawData(offset+9 : offset+12), 'single');   % Qx
        q(i, 3) = typecast(rawData(offset+13 : offset+16), 'single');  % Qy
        q(i, 4) = typecast(rawData(offset+17 : offset+20), 'single');  % Qz
        
        acc(i, 1) = typecast(rawData(offset+21 : offset+24), 'single'); % Acc X
        acc(i, 2) = typecast(rawData(offset+25 : offset+28), 'single'); % Acc Y
        acc(i, 3) = typecast(rawData(offset+29 : offset+32), 'single'); % Acc Z
        
        gps(i, 1) = typecast(rawData(offset+33 : offset+40), 'double'); % Lat
        gps(i, 2) = typecast(rawData(offset+41 : offset+48), 'double'); % Lng
        
        event_flag(i) = rawData(offset+49);
    end
    
    % --- DIAGNOSTICS & ANALYSIS ---
    
    % A. Find SD Card Failures (Gap Frames marked with 0xAA / 170)
    gap_indices = find(event_flag == 170);
    
    % B. Find User Waypoint Tags (Marked with 1)
    tag_indices = find(event_flag == 1);
    
    % C. Detect Queue Overflows (Dropped Frames)
    % Exclude the 0xFFFFFFFF gap frames to calculate pure sensor jumps
    valid_seq_idx = find(event_flag ~= 170);
    valid_seq = double(frame_seq(valid_seq_idx));
    
    seq_jumps = diff(valid_seq);
    drop_instances = find(seq_jumps > 1);
    total_dropped_frames = sum(seq_jumps(drop_instances) - 1);
    
    % Print Analysis to Console
    fprintf('-> Total Recorded Frames: %d\n', numFrames);
    fprintf('-> Number of SD Card Disconnects (Gaps): %d\n', length(gap_indices));
    fprintf('-> User Waypoint Tags Pressed: %d\n', length(tag_indices));
    fprintf('-> Dropped Sensor Frames (Queue Overflows): %d\n', total_dropped_frames);
    if (total_dropped_frames > 0)
        fprintf('   [!] WARNING: You have missing IMU data. Check SPI speed or Queue Size.\n');
    end

    % --- ADVANCED VISUALIZATION ---
    fig = figure('Name', ['Data Analysis: ', baseFileName], 'Position', [100, 100, 1200, 800]);
    
    % Subplot 1: Linear Acceleration & Events
    subplot(3, 1, 1);
    hold on; grid on;
    plot(acc(:,1), 'r', 'DisplayName', 'Acc X');
    plot(acc(:,2), 'g', 'DisplayName', 'Acc Y');
    plot(acc(:,3), 'b', 'DisplayName', 'Acc Z');
    
    % Plot User Tags (Yellow Triangles)
    if ~isempty(tag_indices)
        plot(tag_indices, acc(tag_indices, 3), 'k^', 'MarkerFaceColor', 'y', 'MarkerSize', 8, 'DisplayName', 'User Tag (Event)');
    end
    
    % Plot SD Card Gaps (Vertical Red Lines)
    if ~isempty(gap_indices)
        for g = 1:length(gap_indices)
            xline(gap_indices(g), 'm--', 'LineWidth', 2, 'HandleVisibility', 'off');
        end
        plot(NaN, NaN, 'm--', 'DisplayName', 'SD Recovery Gap (0xAA)'); % Dummy plot for legend
    end
    title('3D Linear Acceleration (g) with Hardware Events');
    ylabel('Acceleration (g)');
    legend('Location', 'best');
    
    % Subplot 2: Madgwick Quaternions
    subplot(3, 1, 2);
    hold on; grid on;
    plot(q(:,1), 'k', 'LineWidth', 1.5, 'DisplayName', 'Qw');
    plot(q(:,2), 'r', 'DisplayName', 'Qx');
    plot(q(:,3), 'g', 'DisplayName', 'Qy');
    plot(q(:,4), 'b', 'DisplayName', 'Qz');
    title('Orientation (Madgwick Quaternions)');
    ylabel('Value');
    legend('Location', 'best');
    
    % Subplot 3: Data Integrity (Sequence Jumps)
    subplot(3, 1, 3);
    hold on; grid on;
    plot(valid_seq_idx, valid_seq, 'b.', 'DisplayName', 'Received Sequences');
    if ~isempty(drop_instances)
        % Mark the exact points where a frame jump occurred
        plot(valid_seq_idx(drop_instances), valid_seq(drop_instances), 'ro', 'MarkerSize', 6, 'MarkerFaceColor', 'r', 'DisplayName', 'Queue Overflow Point');
    end
    title(sprintf('Data Integrity Monitor (Total Dropped Frames: %d)', total_dropped_frames));
    xlabel('Time (Frame Index @ 100Hz)'); ylabel('Frame Sequence Number');
    legend('Location', 'northwest');
end