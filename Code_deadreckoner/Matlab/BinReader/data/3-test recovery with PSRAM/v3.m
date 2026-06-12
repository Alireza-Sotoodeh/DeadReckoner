% Last Edit: 2026-06-12 19:30:00  
% Reason for Last Edit: Implemented Relational Sequence Chaining (Greedy Stitcher) 
% to automatically merge crashed SD sessions without needing an RTC.

clc; clear; close all;

% --- HARDWARE ARCHITECTURE CONFIGURATION ---
FRAME_SIZE = 33; % Exact packed union struct size from ESP32

% 1. Scan for all Binary Logs
filePattern = fullfile(pwd, '*.BIN');
binFiles = dir(filePattern);

if isempty(binFiles)
    fprintf('CRITICAL ERROR: No .BIN log files discovered in directory: %s\n', pwd);
    return;
end

fprintf('\n==================================================\n');
fprintf('PHASE 1: METADATA EXTRACTION & RELATIONAL CHAINING\n');
fprintf('==================================================\n');

% 2. Extract Metadata from all files
fileMeta = struct('name', {}, 'data', {}, 'start_seq', {}, 'end_seq', {}, 'processed', {});

validFileCount = 0;
for k = 1:length(binFiles)
    baseFileName = binFiles(k).name;
    fid = fopen(baseFileName, 'rb');
    if fid == -1, continue; end
    
    rawData = fread(fid, inf, '*uint8');
    fclose(fid);
    
    numFrames = floor(length(rawData) / FRAME_SIZE);
    if numFrames < 2 % Ignore completely empty or corrupt fragments
        fprintf('  [!] Ignoring %s: Too small to contain valid sequence.\n', baseFileName);
        continue; 
    end
    
    validFileCount = validFileCount + 1;
    fileMeta(validFileCount).name = baseFileName;
    fileMeta(validFileCount).data = rawData;
    fileMeta(validFileCount).processed = false;
    
    % Read very first frame sequence (Bytes 1-4)
    fileMeta(validFileCount).start_seq = typecast(rawData(1:4), 'uint32');
    % Read very last frame sequence (Last 33 bytes, Bytes 1-4)
    lastFrameOffset = (numFrames - 1) * FRAME_SIZE;
    fileMeta(validFileCount).end_seq = typecast(rawData(lastFrameOffset+1 : lastFrameOffset+4), 'uint32');
end

% 3. Greedy Stitching Algorithm (The "RTC Emulator")
sessions = {};
sessionCount = 0;

while any(~[fileMeta.processed])
    sessionCount = sessionCount + 1;
    
    % Find the unprocessed file with the absolute minimum start_seq (Usually a boot-up)
    unproc_idx = find(~[fileMeta.processed]);
    [~, min_idx] = min([fileMeta(unproc_idx).start_seq]);
    curr_idx = unproc_idx(min_idx);
    
    current_data = fileMeta(curr_idx).data;
    current_end = fileMeta(curr_idx).end_seq;
    fileMeta(curr_idx).processed = true;
    
    session_chain = {fileMeta(curr_idx).name};
    
    % Search for crashed fragments that belong to this session
    while true
        unproc = find(~[fileMeta.processed]);
        if isempty(unproc), break; end
        
        % Candidates MUST start after the current end sequence
        candidates = unproc([fileMeta(unproc).start_seq] > current_end);
        if isempty(candidates), break; end
        
        % Pick the candidate that starts closest to our end point
        [~, c_min_idx] = min([fileMeta(candidates).start_seq]);
        next_idx = candidates(c_min_idx);
        
        % Safety Check: If the gap is impossibly huge (e.g. > 10 mins of dropped frames = 60,000)
        % It probably belongs to a completely different test on a different day!
        if fileMeta(next_idx).start_seq - current_end > 100000 
            break; 
        end
        
        % Stitch it!
        current_data = [current_data; fileMeta(next_idx).data];
        current_end = fileMeta(next_idx).end_seq;
        fileMeta(next_idx).processed = true;
        session_chain{end+1} = fileMeta(next_idx).name;
    end
    
    sessions{sessionCount}.data = current_data;
    sessions{sessionCount}.chain = session_chain;
    
    fprintf(' -> Successfully Built Session %d: ', sessionCount);
    fprintf('%s ', session_chain{:});
    fprintf('\n');
end

fprintf('\n==================================================\n');
fprintf('PHASE 2: PAYLOAD DECODING & DIAGNOSTICS\n');
fprintf('==================================================\n');

% 4. Decode and Plot each merged Session
for s = 1:length(sessions)
    rawData = sessions{s}.data;
    chainNames = strjoin(sessions{s}.chain, ' + ');
    
    numFrames = floor(length(rawData) / FRAME_SIZE);
    
    frame_seq = zeros(numFrames, 1, 'uint32');
    event_flag = zeros(numFrames, 1, 'uint8');
    q = nan(numFrames, 4, 'single');        
    acc = nan(numFrames, 3, 'single');      
    
    for i = 1:numFrames
        offset = (i - 1) * FRAME_SIZE;
        
        frame_seq(i) = typecast(rawData(offset+1 : offset+4), 'uint32');
        event_flag(i) = rawData(offset+5);
        
        if event_flag(i) == 0 || event_flag(i) == 1
            q(i, 1) = typecast(rawData(offset+6  : offset+9),  'single'); 
            q(i, 2) = typecast(rawData(offset+10 : offset+13), 'single'); 
            q(i, 3) = typecast(rawData(offset+14 : offset+17), 'single'); 
            q(i, 4) = typecast(rawData(offset+18 : offset+21), 'single'); 
            
            acc(i, 1) = typecast(rawData(offset+22 : offset+25), 'single'); 
            acc(i, 2) = typecast(rawData(offset+26 : offset+29), 'single'); 
            acc(i, 3) = typecast(rawData(offset+30 : offset+33), 'single'); 
        end
    end
    
    gap_indices = find(event_flag == 170);
    tag_indices = find(event_flag == 1);
    
    valid_imu_idx = find(event_flag == 0 | event_flag == 1);
    valid_seq = double(frame_seq(valid_imu_idx));
    
    seq_jumps = diff(valid_seq);
    drop_instances = find(seq_jumps > 1);
    total_dropped_frames = sum(seq_jumps(drop_instances) - 1);
    
    fprintf('\n[SESSION %d STATS] (%s)\n', s, chainNames);
    fprintf('  - Total Merged Frames: %d\n', numFrames);
    fprintf('  - SD Card Recovery Gaps: %d\n', length(gap_indices));
    fprintf('  - Missing Frames (Queue Drops): %d\n', total_dropped_frames);
    
    % --- PLOTTING ---
    figure('Name', sprintf('Session %d Analysis [%s]', s, chainNames), 'Position', [150+(s*20), 100+(s*20), 1100, 750]);
    
    subplot(3, 1, 1); hold on; grid on;
    plot(acc(:,1), 'r', 'DisplayName', 'Acc X');
    plot(acc(:,2), 'g', 'DisplayName', 'Acc Y');
    plot(acc(:,3), 'b', 'DisplayName', 'Acc Z');
    if ~isempty(tag_indices)
        plot(tag_indices, acc(tag_indices, 3), 'k^', 'MarkerFaceColor', 'y', 'MarkerSize', 7, 'DisplayName', 'Waypoint');
    end
    if ~isempty(gap_indices)
        for g = 1:length(gap_indices), xline(gap_indices(g), 'm--', 'LineWidth', 1.8, 'HandleVisibility', 'off'); end
        plot(NaN, NaN, 'm--', 'LineWidth', 1.8, 'DisplayName', 'Recovery Gap');
    end
    title(sprintf('SESSION %d: 3-Axis Acceleration', s)); ylabel('g'); legend('Location', 'best');
    
    subplot(3, 1, 2); hold on; grid on;
    plot(q(:,1), 'k', 'DisplayName', 'Qw'); plot(q(:,2), 'r', 'DisplayName', 'Qx');
    plot(q(:,3), 'g', 'DisplayName', 'Qy'); plot(q(:,4), 'b', 'DisplayName', 'Qz');
    title('Orientation (Madgwick)'); ylabel('Value'); legend('Location', 'best');
    
    subplot(3, 1, 3); hold on; grid on;
    plot(valid_imu_idx, valid_seq, 'b-', 'DisplayName', 'Stream');
    if ~isempty(drop_instances)
        plot(valid_imu_idx(drop_instances), valid_seq(drop_instances), 'ro', 'MarkerFaceColor', 'r', 'DisplayName', 'Data Drop');
    end
    title(sprintf('Frame Continuity (Lost: %d)', total_dropped_frames)); xlabel('Merged File Row Index'); ylabel('Frame Seq');
end