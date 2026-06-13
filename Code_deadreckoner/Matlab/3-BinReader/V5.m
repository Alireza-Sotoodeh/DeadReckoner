% Last Edit: 2026-06-13 15:10:00  
% Reason for Last Edit: Fixed the missing END syntax error by expanding inline loops. 
% Resolved rawData typo to sessionData and added missing drop_instances definition.

clc; clear; close all;

% --- HARDWARE ARCHITECTURE CONFIGURATION ---
FRAME_SIZE = 33; % Exact packed 33-byte union struct size from ESP32

fprintf('\n==================================================\n');
fprintf('   DEADRECKONER DETERMINISTIC STITCHING PIPELINE   \n');
fprintf('==================================================\n');

% 1. Extract Unique Session IDs (XXX) from directory
allFiles = dir('*.BIN');
if isempty(allFiles)
    fprintf('CRITICAL ERROR: No .BIN log files discovered in directory.\n');
    return;
end

% Extract all 3-digit main log IDs present in the folder
sessionIDs = [];
for f = 1:length(allFiles)
    name = allFiles(f).name;
    % Match DR_LOG_XXX.BIN
    tokens = regexp(name, 'DR_LOG_(\d{3})\.BIN', 'tokens');
    if ~isempty(tokens)
        sessionIDs = [sessionIDs, str2double(tokens{1}{1})];
    else
        % Match XXXXXX.BIN (Recovery Files)
        tokens = regexp(name, '^(\d{3})(\d{3})\.BIN', 'tokens');
        if ~isempty(tokens)
            sessionIDs = [sessionIDs, str2double(tokens{1}{1})];
        end
    end
end
uniqueSessions = unique(sessionIDs);

if isempty(uniqueSessions)
    fprintf('CRITICAL ERROR: No structured logs found matching naming protocols.\n');
    return;
end

% 2. Session-by-Session Chaining and Decoding Loop
for s = 1:length(uniqueSessions)
    currSessionID = uniqueSessions(s);
    sessionData = uint8([]);
    fileChain = {};
    
    % Step A: Search and load the parent log file if it exists
    parentName = sprintf('DR_LOG_%03d.BIN', currSessionID);
    if exist(parentName, 'file')
        fid = fopen(parentName, 'rb');
        sessionData = [sessionData; fread(fid, inf, '*uint8')];
        fclose(fid);
        fileChain{end+1} = parentName;
    end
    
    % Step B: Search and load child recovery logs in strict linear order (YYY)
    recoveryID = 1;
    while true
        childName = sprintf('%03d%03d.BIN', currSessionID, recoveryID);
        if exist(childName, 'file')
            fid = fopen(childName, 'rb');
            sessionData = [sessionData; fread(fid, inf, '*uint8')];
            fclose(fid);
            fileChain{end+1} = childName;
            recoveryID = recoveryID + 1;
        else
            break; % Chain broken, no more recovery files for this session
        end
    end
    
    % Validate session size
    numFrames = floor(length(sessionData) / FRAME_SIZE);
    if numFrames < 2, continue; end
    
    % 3. Preallocate memory for high-speed extraction
    frame_seq = zeros(numFrames, 1, 'uint32');
    event_flag = zeros(numFrames, 1, 'uint8');
    q = nan(numFrames, 4, 'single');        
    acc = nan(numFrames, 3, 'single');      
    
    % 4. Decode Packed Bytes
    for i = 1:numFrames
        offset = (i - 1) * FRAME_SIZE;
        
        frame_seq(i) = typecast(sessionData(offset+1 : offset+4), 'uint32');
        event_flag(i) = sessionData(offset+5);
        
        if event_flag(i) == 0 || event_flag(i) == 1
            q(i, 1) = typecast(sessionData(offset+6  : offset+9),  'single'); % Qw
            q(i, 2) = typecast(sessionData(offset+10 : offset+13), 'single'); % Qx
            q(i, 3) = typecast(sessionData(offset+14 : offset+17), 'single'); % Qy
            q(i, 4) = typecast(sessionData(offset+18 : offset+21), 'single'); % Qz
            
            acc(i, 1) = typecast(sessionData(offset+22 : offset+25), 'single'); % Acc X
            acc(i, 2) = typecast(sessionData(offset+26 : offset+29), 'single'); % Acc Y
            acc(i, 3) = typecast(sessionData(offset+30 : offset+33), 'single'); % Acc Z
        end
    end
    
    % --- MATHEMATICAL TRANSFORMATIONS ---
    % Convert Quaternions to standard Euler Angles (Roll, Pitch, Yaw) in Degrees
    roll = nan(numFrames, 1); pitch = nan(numFrames, 1); yaw = nan(numFrames, 1);
    valid_q_idx = find(event_flag == 0 | event_flag == 1);
    
    for idx = 1:length(valid_q_idx)
        ii = valid_q_idx(idx);
        w = q(ii,1); x = q(ii,2); y = q(ii,3); z = q(ii,4);
        
        roll(ii)  = atan2(2*(w*x + y*z), 1 - 2*(x^2 + y^2)) * (180/pi);
        pitch(ii) = asin(clamp(2*(w*y - z*x), -1, 1)) * (180/pi);
        yaw(ii)   = atan2(2*(w*z + x*y), 1 - 2*(y^2 + z^2)) * (180/pi);
    end
    
    % --- ENGINEERING STATS & HEALTH DIAGNOSTICS ---
    gap_indices = find(event_flag == 170);
    tag_indices = find(event_flag == 1);
    
    valid_seq = double(frame_seq(valid_q_idx));
    jumps = diff(valid_seq);
    drop_instances = find(jumps > 1);
    total_dropped_frames = sum(jumps(drop_instances) - 1);
    
    % Statistical Signal Metrics
    mean_acc = mean(acc(valid_q_idx, :));
    rms_acc = rms(acc(valid_q_idx, :));
    
    % Print Engineering Logs to Screen
    chainString = strjoin(fileChain, ' -> ');
    fprintf('\n[✓] SESSION %d REGISTERED MAP:\n', currSessionID);
    fprintf('    [*] Chained Nodes: %s\n', chainString);
    fprintf('    [*] Combined Payload: %d frames (~%.2f seconds)\n', numFrames, numFrames/100);
    fprintf('    [+] User Waypoint Markers Pressed: %d times\n', length(tag_indices));
    fprintf('    [!] Intercepted SD Crashes: %d instances\n', length(gap_indices));
    fprintf('    [!] Memory Drops (Queue Drops): %d frames\n', total_dropped_frames);
    fprintf('    [*] Dynamic Acceleration RMS: [X:%.2f, Y:%.2f, Z:%.2f] g\n', rms_acc(1), rms_acc(2), rms_acc(3));

    % --- ADVANCED PLOTTING ENGINE ---
    figure('Name', sprintf('Session %d Analysis Matrix', currSessionID), 'Position', [100+(s*30), 50+(s*30), 1200, 800]);
    
    % Subplot 1: Linear Acceleration Profile
    subplot(3, 1, 1); hold on; grid on;
    plot(acc(:,1), 'r', 'LineWidth', 1.0, 'DisplayName', 'Acc X'); 
    plot(acc(:,2), 'g', 'LineWidth', 1.0, 'DisplayName', 'Acc Y'); 
    plot(acc(:,3), 'b', 'LineWidth', 1.0, 'DisplayName', 'Acc Z');
    if ~isempty(tag_indices)
        plot(tag_indices, acc(tag_indices, 3), 'k^', 'MarkerFaceColor', 'y', 'MarkerSize', 8, 'DisplayName', 'User Tag');
    end
    if ~isempty(gap_indices)
        for g = 1:length(gap_indices)
            xline(gap_indices(g), 'm--', 'LineWidth', 1.5, 'HandleVisibility', 'off'); 
        end
        plot(NaN, NaN, 'm--', 'LineWidth', 1.5, 'DisplayName', 'SD Crash Link Event');
    end
    title(sprintf('Session %d - 3-Axis Acceleration Profile', currSessionID), 'FontSize', 11);
    ylabel('Acceleration (g)'); legend('Location', 'best');
    
    % Subplot 2: Physical Attitude Navigation Profile (Euler Angles)
    subplot(3, 1, 2); hold on; grid on;
    plot(roll, 'm', 'LineWidth', 1.1, 'DisplayName', 'Roll (Φ)'); 
    plot(pitch, 'c', 'LineWidth', 1.1, 'DisplayName', 'Pitch (θ)'); 
    plot(yaw, 'k', 'LineWidth', 1.3, 'DisplayName', 'Yaw (Ψ)');
    title('Real-World Attitude Extrapolated Data (Euler Degrees)', 'FontSize', 11);
    ylabel('Degrees (°)'); legend('Location', 'best');
    
    % Subplot 3: Stream Quality & Time-Continuity Monitor
    subplot(3, 1, 3); hold on; grid on;
    plot(valid_q_idx, valid_seq, 'b-', 'LineWidth', 1.2, 'DisplayName', 'Sequence Stream');
    if ~isempty(drop_instances)
        plot(valid_q_idx(drop_instances), valid_seq(drop_instances), 'ro', 'MarkerFaceColor', 'r', 'DisplayName', 'Buffer Overflow');
    end
    title(sprintf('Timeline Stream Continuity Quality (Total Drops: %d frames)', total_dropped_frames), 'FontSize', 11);
    xlabel('Global Merged Row Index (Samples @ 100Hz)'); ylabel('MCU Counter Value');
    legend('Location', 'northwest');
end

% Helper clipping function to safe-guard asin calculation from floating errors
function out = clamp(in, minVal, maxVal)
    out = min(max(in, minVal), maxVal);
end