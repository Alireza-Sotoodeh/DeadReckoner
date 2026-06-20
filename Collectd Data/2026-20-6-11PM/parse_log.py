import struct
import os
from datetime import datetime, timezone

LOG_PATH = r"C:\Users\Alireza\Desktop\DeadReckoner\DeadReckoner\Collectd Data\2026-20-6-11PM\DR_LOG_001.BIN"

# FileHeader: magic(4) version(1) frame_size(1) sample_rate(2) epoch_ms(8) = 16 bytes
# LogFrame: frame_seq(4) timestamp(8) event_flag(1) payload(32) crc(2) = 47 bytes

FILE_HEADER_FMT = "< I B B H Q"  # little-endian
LOG_FRAME_FMT = "< I Q B 32s H"
HEADER_SIZE = struct.calcsize(FILE_HEADER_FMT)
FRAME_SIZE = struct.calcsize(LOG_FRAME_FMT)

size = os.path.getsize(LOG_PATH)
print(f"File size: {size:,} bytes")
print(f"Header: {HEADER_SIZE} bytes, Frame: {FRAME_SIZE} bytes")
print()

with open(LOG_PATH, "rb") as f:
    raw = f.read(HEADER_SIZE)
    magic, version, frame_size, sample_rate, epoch_ms = struct.unpack(FILE_HEADER_FMT, raw)

    print("=== FILE HEADER ===")
    print(f"Magic:        0x{magic:08X} ({'OK' if magic == 0xDEADC0DE else 'BAD'})")
    print(f"Version:      {version}")
    print(f"Frame size:   {frame_size} bytes ({'matches' if frame_size == FRAME_SIZE else f'MISMATCH (expected {FRAME_SIZE})'})")
    print(f"Sample rate:  {sample_rate} Hz")
    ts = datetime.fromtimestamp(epoch_ms / 1000, tz=timezone.utc)
    print(f"Epoch:        {epoch_ms} ms ({ts})")
    print()

    # Read all frames
    data_len = size - HEADER_SIZE
    frame_count = data_len // frame_size
    remainder = data_len % frame_size
    print(f"Data bytes:   {data_len:,}")
    print(f"Frame count:  {frame_count:,}")
    print(f"Remainder:    {remainder} bytes {'(OK)' if remainder == 0 else 'WARNING: partial frame!'}")
    print()

    if frame_count == 0:
        print("No data frames.")
        exit()

    # Gather stats
    events = {}
    first_ts = None
    last_ts = None
    gps_frames = []  # frames with event_flag == 0xBB or 0xCC

    for i in range(frame_count):
        offset = HEADER_SIZE + i * frame_size
        f.seek(offset)
        raw_frame = f.read(frame_size)
        seq, ts, evt, payload_raw, crc = struct.unpack(LOG_FRAME_FMT, raw_frame)

        if first_ts is None:
            first_ts = ts
        last_ts = ts

        events[evt] = events.get(evt, 0) + 1

        # Decode GPS frames
        if evt in (0xBB, 0xCC):
            gps_lat, gps_lng, gps_alt, gps_epoch = struct.unpack("< d d f I", payload_raw[:24])
            print(f"\nRAW GPS frame [{seq}]: event=0x{evt:02X} offset={offset}")
            print(f"  Payload hex: {payload_raw.hex()}")
            print(f"  lat={gps_lat} lon={gps_lng} alt={gps_alt} epoch={gps_epoch}")
            gps_frames.append((seq, ts, evt, gps_lat, gps_lng, gps_alt, gps_epoch))

    print("=== EVENT BREAKDOWN ===")
    evt_names = {0: "IMU", 1: "TAG", 0xAA: "SD_GAP", 0xBB: "GPS_START", 0xCC: "GPS_END"}
    for evt, count in sorted(events.items()):
        name = evt_names.get(evt, f"UNKNOWN(0x{evt:02X})")
        print(f"  0x{evt:02X} ({name}): {count:,} frames")

    duration_us = last_ts - first_ts if first_ts is not None else 0
    duration_s = duration_us / 1_000_000
    imu_count = events.get(0, 0)
    actual_rate = imu_count / duration_s if duration_s > 0 else 0
    print(f"\nDuration: {duration_s:.1f}s ({duration_s/60:.1f} min)")
    print(f"IMU frames: {imu_count:,}")
    print(f"Actual rate: {actual_rate:.1f} Hz (expected {sample_rate} Hz)")

    if gps_frames:
        print(f"\n=== GPS FRAMES ({len(gps_frames)}) ===")
        for seq, ts, evt, lat, lng, alt, epoch in gps_frames:
            name = "START" if evt == 0xBB else "END"
            rel_s = ts / 1_000_000
            from datetime import datetime, timezone
            dt = datetime.fromtimestamp(epoch, tz=timezone.utc) if epoch else "N/A"
            print(f"  [{seq:5d}] +{rel_s:.1f}s  {name}:  lat={lat:.6f}, lon={lng:.6f}  alt={alt:.0f}m  epoch={epoch} ({dt})")

    # Show first/last few IMU frames
    print(f"\n=== FIRST 3 IMU FRAMES ===")
    count = 0
    for i in range(frame_count):
        offset = HEADER_SIZE + i * frame_size
        f.seek(offset)
        raw_frame = f.read(frame_size)
        seq, ts, evt, payload_raw, crc = struct.unpack(LOG_FRAME_FMT, raw_frame)
        if evt == 0:  # IMU
            q = struct.unpack("< 4f", payload_raw[:16])
            accel = struct.unpack("< 3f", payload_raw[16:28])
            temp = struct.unpack("< f", payload_raw[28:32])[0]
            rel_s = ts / 1_000_000
            print(f"  [{seq:5d}] +{rel_s:.1f}s  Q({q[0]:.4f},{q[1]:.4f},{q[2]:.4f},{q[3]:.4f})  "
                  f"A({accel[0]:.2f},{accel[1]:.2f},{accel[2]:.2f})  T={temp:.1f}°C")
            count += 1
            if count >= 3:
                break

    print(f"\n=== LAST 3 IMU FRAMES ===")
    count = 0
    for i in range(frame_count - 1, -1, -1):
        offset = HEADER_SIZE + i * frame_size
        f.seek(offset)
        raw_frame = f.read(frame_size)
        seq, ts, evt, payload_raw, crc = struct.unpack(LOG_FRAME_FMT, raw_frame)
        if evt == 0:  # IMU
            q = struct.unpack("< 4f", payload_raw[:16])
            accel = struct.unpack("< 3f", payload_raw[16:28])
            temp = struct.unpack("< f", payload_raw[28:32])[0]
            rel_s = ts / 1_000_000
            print(f"  [{seq:5d}] +{rel_s:.1f}s  Q({q[0]:.4f},{q[1]:.4f},{q[2]:.4f},{q[3]:.4f})  "
                  f"A({accel[0]:.2f},{accel[1]:.2f},{accel[2]:.2f})  T={temp:.1f}°C")
            count += 1
            if count >= 3:
                break

    # Check for dropped frames
    seqs = []
    for i in range(frame_count):
        offset = HEADER_SIZE + i * frame_size
        f.seek(offset)
        raw_frame = f.read(frame_size)
        seq = struct.unpack("< I", raw_frame[:4])[0]
        seqs.append(seq)

    dropped = sum(1 for i in range(1, len(seqs)) if seqs[i] != seqs[i-1] + 1)
    print(f"\n=== INTEGRITY ===")
    print(f"Sequence gaps: {dropped}")
    if dropped > 0:
        for i in range(1, min(10, len(seqs))):
            if seqs[i] != seqs[i-1] + 1:
                print(f"  Gap at index {i}: {seqs[i-1]} -> {seqs[i]} (skip={seqs[i] - seqs[i-1] - 1})")
    print(f"Total frames: {frame_count:,}")
