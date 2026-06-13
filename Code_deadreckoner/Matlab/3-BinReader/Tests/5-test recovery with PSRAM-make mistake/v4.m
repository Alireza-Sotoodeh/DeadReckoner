% Last Edit: 2026-06-12 21:00:00  
% Reason for Last Edit: Replaced strict sequence matching with Absolute Distance Minimizer. 
% Added Garbage-Frame filter to automatically discard corrupted tail bytes from hot-plugged SD cards.

clc; clear; close all;

% --- HARDWARE ARCHITECTURE CONFIGURATION ---
FRAME_SIZE = 33; 

filePattern = fullfile(pwd, '*.BIN');
binFiles = dir(filePattern);

if isempty(binFiles)
    fprintf('CRITICAL ERROR: No .BIN log files discovered in directory: %s\n', pwd);
    return;
end

fprintf('\n==================================================\n');
fprintf('PHASE 1: METADATA EXTRACTION & GARBAGE FILTERING\n');
fprintf('==================================================\n');

fileMeta = struct('name', {}, 'data', {}, 'start_seq', {}, 'end_seq', {}, 'processed', {});
validFileCount = 0;

for k = 1:length(binFiles)
    baseFileName = binFiles(k).name;
    fid = fopen(baseFileName, 'rb');
    if fid == -1, continue; end
    
    rawData = fread(fid, inf, '*uint8');
    fclose(fid);
    
    numFrames = floor(length(rawData) / FRAME_SIZE);
    if numFrames < 2, continue; end
    
    % Extract all sequences to find the TRUE start and end, avoiding corrupted tails
    f_seq = zeros(numFrames, 1, 'double');
    e_flag = zeros(numFrames, 1, 'uint8');
    for f = 1:numFrames
        offset = (f - 1) * FRAME_SIZE;
        f_seq(f) = double(typecast(rawData(offset+1:offset+4), 'uint32'));
        e_flag(f) = rawData(offset+5);
    end
    
    valid_idx = find(e_flag == 0 | e_flag == 1);
    if isempty(valid_idx), continue; end
    
    valid_seqs = f_seq(valid_idx);
    
    % GARBAGE FILTER: Detect impossible sequence jumps caused by abrupt power loss on the SD card
    jumps = diff(valid_seqs);
    bad_jump_idx = find(jumps > 500 | jumps < 0, 1, 'first'); 
    if ~isempty(bad_jump_idx)
        valid_seqs = valid_seqs(1:bad_jump_idx); % Slice off the corrupted tail
    end
    
    validFileCount = validFileCount + 1;
    fileMeta(validFileCount).name = baseFileName;
    fileMeta(validFileCount).data = rawData;
    fileMeta(validFileCount).processed = false;
    fileMeta(validFileCount).start_seq = valid_seqs(1);
    fileMeta(validFileCount).end_seq = valid_seqs(end);
end

% 3. Smart Stitching Algorithm (Absolute Distance Minimizer)
sessions = {};
sessionCount = 0;

while any(~[fileMeta.processed])
    sessionCount = sessionCount + 1;
    
    unproc_idx = find(~[fileMeta.processed]);
    [~, min_idx] = min([fileMeta(unproc_idx).start_seq]);
    curr_idx = unproc_idx(min_idx);
    
    current_data = fileMeta(curr_idx).data;
    current_end = fileMeta(curr_idx).end_seq;
    fileMeta(curr_idx).processed = true;
    
    session_chain = {fileMeta(curr_idx).name};
    
    while true
        unproc = find(~[fileMeta.processed]);
        if isempty(unproc), break; end
        
        % Calculate DIRECTIONAL difference (Time must move forward, or slightly backward for PSRAM overlaps)
        diffs = [fileMeta(unproc).start_seq] - current_end;
        
        % Rule: The next file can overlap by max -5000 frames, or jump forward by max +50000 frames
        valid_mask = diffs > -5000 & diffs < 50000;
        
        if ~any(valid_mask)
            break; % No candidates fit this session. The session is closed!
        end
        
        % Among valid candidates, pick the one with the smallest absolute gap
        candidates = unproc(valid_mask);
        cand_diffs = abs(diffs(valid_mask));
        [~, best_idx] = min(cand_diffs);
        next_idx = candidates(best_idx);
        
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
    
    % Clean drops visualization by applying the same garbage filter
    jumps = diff(valid_seq);
    clean_jumps = jumps(jumps < 50000 & jumps > 0);
    drop_instances = find(clean_jumps > 1);
    total_dropped_frames = sum(clean_jumps(drop_instances) - 1);
    
    fprintf('\n[SESSION %d STATS] (%s)\n', s, chainNames);
    fprintf('  - Total Merged Frames: %d\n', numFrames);
    fprintf('  - SD Card Recovery Gaps: %d\n', length(gap_indices));
    fprintf('  - Missing Frames (Queue Drops): %d\n', total_dropped_frames);
    
    figure('Name', sprintf('Session %d Analysis [%s]', s, chainNames), 'Position', [150+(s*20), 100+(s*20), 1100, 750]);
    
    subplot(3, 1, 1); hold on; grid on;
    plot(acc(:,1), 'r', 'DisplayName', 'Acc X'); plot(acc(:,2), 'g', 'DisplayName', 'Acc Y'); plot(acc(:,3), 'b', 'DisplayName', 'Acc Z');
    if ~isempty(tag_indices), plot(tag_indices, acc(tag_indices, 3), 'k^', 'MarkerFaceColor', 'y', 'MarkerSize', 7, 'DisplayName', 'Waypoint'); end
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
    if ~isempty(drop_instances), plot(valid_imu_idx(drop_instances), valid_seq(drop_instances), 'ro', 'MarkerFaceColor', 'r', 'DisplayName', 'Data Drop'); end
    title(sprintf('Frame Continuity (Lost: %d)', total_dropped_frames)); xlabel('Merged File Row Index'); ylabel('Frame Seq');
end