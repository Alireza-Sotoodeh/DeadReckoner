import os
import glob
import struct
import pandas as pd
import matplotlib.pyplot as plt
import questionary

# =========================================================================
# DeadReckoner Advanced Binary Decoder & Analyzer
# Version: 2.0
# Architecture: 47-Byte Sequential Parsing (V2.1: FileHeader support)
# =========================================================================

class BinaryDecoder:
    FRAME_SIZE = 47
    FILE_HEADER_MAGIC = 0xDEADC0DE
    HEADER_FMT = '< I Q B'  # 4 bytes (uint32), 8 bytes (uint64), 1 byte (uint8)
    IMU_FMT = '< 4f 3f f H' # 32 bytes: 4x float (Quat), 3x float (Accel), 1x float (Temp), 2 bytes CRC
    
    FLAG_IMU = 0x00
    FLAG_TAG = 0x01
    FLAG_GAP = 0xAA
    FLAG_GPS = 0xBB

    @classmethod
    def parse_file(cls, filepath):
        data = []
        dropped_frames_total = 0
        last_seq = -1

        try:
            with open(filepath, 'rb') as f:
                # Detect and skip FileHeader (16 bytes) if present
                magic_bytes = f.read(4)
                if len(magic_bytes) < 4:
                    return data, dropped_frames_total
                magic = struct.unpack('<I', magic_bytes)[0]
                if magic == cls.FILE_HEADER_MAGIC:
                    f.read(12)  # skip rest of 16-byte header
                else:
                    f.seek(0)  # legacy file, no header

                while True:
                    chunk = f.read(cls.FRAME_SIZE)
                    if len(chunk) < cls.FRAME_SIZE:
                        break
                    
                    header_bytes = chunk[:13]
                    payload_bytes = chunk[13:]
                    
                    frame_seq, timestamp, event_flag = struct.unpack(cls.HEADER_FMT, header_bytes)
                    
                    if last_seq != -1 and frame_seq > last_seq + 1:
                        dropped_frames_total += (frame_seq - last_seq - 1)
                    last_seq = frame_seq

                    if event_flag in (cls.FLAG_IMU, cls.FLAG_TAG, cls.FLAG_GAP):
                        q0, q1, q2, q3, ax, ay, az, temp, crc = struct.unpack(cls.IMU_FMT, payload_bytes)
                        
                        data.append({
                            'Frame_Sequence': frame_seq,
                            'Timestamp_us': timestamp,
                            'Timestamp_s': timestamp / 1000000.0,
                            'Event_Flag': hex(event_flag),
                            'Q_w': q0, 'Q_x': q1, 'Q_y': q2, 'Q_z': q3,
                            'Accel_X': ax, 'Accel_Y': ay, 'Accel_Z': az,
                            'Temperature_C': temp
                        })
                    else:
                        pass
                        
        except Exception as e:
            print(f"Error reading {filepath}: {e}")
            
        return data, dropped_frames_total

class DataScanner:
    @staticmethod
    def scan_missions(directory="."):
        missions = {}
        all_bins = glob.glob(os.path.join(directory, "*.BIN"))
        
        for file in all_bins:
            filename = os.path.basename(file)
            
            if filename.startswith("DR_LOG_") and len(filename) == 14:
                mission_id = filename[7:10] 
                if mission_id not in missions:
                    missions[mission_id] = []
                missions[mission_id].append(file)
                
            elif len(filename) == 10 and filename[:6].isdigit():
                mission_id = filename[:3]
                if mission_id not in missions:
                    missions[mission_id] = []
                missions[mission_id].append(file)
                
        for m_id in missions:
            missions[m_id].sort()
            
        return missions

class DataExporter:
    @staticmethod
    def export_csv(df, output_name):
        df.to_csv(f"{output_name}.csv", index=False)
        print(f"[+] Saved: {output_name}.csv")

    @staticmethod
    def export_excel(df, output_name):
        df.to_excel(f"{output_name}.xlsx", index=False)
        print(f"[+] Saved: {output_name}.xlsx")

    @staticmethod
    def generate_report(df, dropped_count, files_merged, output_name):
        total_time = df['Timestamp_s'].iloc[-1] - df['Timestamp_s'].iloc[0] if not df.empty else 0
        tag_count = len(df[df['Event_Flag'] == '0x1'])
        gap_count = len(df[df['Event_Flag'] == '0xaa'])
        
        report = (
            f"=========================================\n"
            f"   MISSION DIAGNOSTIC REPORT\n"
            f"=========================================\n"
            f"Mission ID        : {output_name}\n"
            f"Fragments Merged  : {files_merged}\n"
            f"Total Frames      : {len(df)}\n"
            f"Mission Duration  : {total_time:.2f} Seconds\n"
            f"-----------------------------------------\n"
            f"HARDWARE INTEGRITY\n"
            f"Silent Drops      : {dropped_count} Frames\n"
            f"SD Recovery Gaps  : {gap_count} Events\n"
            f"User Tags         : {tag_count} Waypoints\n"
            f"-----------------------------------------\n"
            f"THERMAL & SENSOR DATA\n"
            f"Max Temperature   : {df['Temperature_C'].max():.2f} °C\n"
            f"Min Temperature   : {df['Temperature_C'].min():.2f} °C\n"
            f"=========================================\n"
        )
        
        with open(f"{output_name}_Report.txt", "w") as f:
            f.write(report)
        print(f"[+] Saved: {output_name}_Report.txt")

    @staticmethod
    def plot_data(df, output_name):
        fig, axs = plt.subplots(3, 1, figsize=(12, 10), sharex=True)
        fig.suptitle(f'Mission Profile: {output_name}', fontsize=16)

        axs[0].plot(df['Timestamp_s'], df['Accel_X'], label='Acc X')
        axs[0].plot(df['Timestamp_s'], df['Accel_Y'], label='Acc Y')
        axs[0].plot(df['Timestamp_s'], df['Accel_Z'], label='Acc Z')
        axs[0].set_ylabel('Acceleration (G)')
        axs[0].legend(loc='upper right')
        axs[0].grid(True)

        axs[1].plot(df['Timestamp_s'], df['Q_w'], label='Qw')
        axs[1].plot(df['Timestamp_s'], df['Q_x'], label='Qx')
        axs[1].plot(df['Timestamp_s'], df['Q_y'], label='Qy')
        axs[1].plot(df['Timestamp_s'], df['Q_z'], label='Qz')
        axs[1].set_ylabel('Quaternions')
        axs[1].legend(loc='upper right')
        axs[1].grid(True)

        axs[2].plot(df['Timestamp_s'], df['Temperature_C'], color='red')
        axs[2].set_ylabel('Temperature (°C)')
        axs[2].set_xlabel('Time (Seconds)')
        axs[2].grid(True)

        tags = df[df['Event_Flag'] == '0x1']
        for _, tag in tags.iterrows():
            for ax in axs:
                ax.axvline(x=tag['Timestamp_s'], color='green', linestyle='--', alpha=0.5)

        gaps = df[df['Event_Flag'] == '0xaa']
        for _, gap in gaps.iterrows():
            for ax in axs:
                ax.axvline(x=gap['Timestamp_s'], color='red', linestyle='-', linewidth=2, alpha=0.7)

        plt.tight_layout()
        plt.savefig(f"{output_name}_Plot.png", dpi=300)
        print(f"[+] Saved: {output_name}_Plot.png")
        plt.show()

def main():
    print("\n>>> DeadReckoner Ground Station Engine <<<\n")
    
    missions = DataScanner.scan_missions(".")
    if not missions:
        print("[-] No valid .BIN logs found in the current directory.")
        return

    mission_choices = [f"Mission {m} ({len(files)} fragments)" for m, files in missions.items()]
    
    selected_mission_str = questionary.select(
        "Select a Mission to analyze:",
        choices=mission_choices
    ).ask()
    
    mission_id = selected_mission_str.split(' ')[1]
    mission_files = missions[mission_id]
    
    export_choices = questionary.checkbox(
        "Select desired outputs (Use SPACE to tick, ENTER to confirm):",
        choices=[
            questionary.Choice("CSV Data File", checked=True),
            questionary.Choice("Excel (XLSX) Data File", checked=False),
            questionary.Choice("Diagnostic Text Report", checked=True),
            questionary.Choice("Generate & Show Plots", checked=True)
        ]
    ).ask()

    print(f"\n[*] Processing Mission {mission_id}...")
    
    all_data = []
    total_drops = 0
    
    for file in mission_files:
        print(f"    -> Parsing {os.path.basename(file)}...")
        data, drops = BinaryDecoder.parse_file(file)
        all_data.extend(data)
        total_drops += drops
        
    df = pd.DataFrame(all_data)
    
    if df.empty:
        print("[-] No valid data extracted from files.")
        return

    output_prefix = f"Mission_{mission_id}"

    print("\n[*] Exporting Data...")
    if "CSV Data File" in export_choices:
        DataExporter.export_csv(df, output_prefix)
        
    if "Excel (XLSX) Data File" in export_choices:
        DataExporter.export_excel(df, output_prefix)
        
    if "Diagnostic Text Report" in export_choices:
        DataExporter.generate_report(df, total_drops, len(mission_files), output_prefix)
        
    if "Generate & Show Plots" in export_choices:
        DataExporter.plot_data(df, output_prefix)
        
    print("\n[+] Analysis Complete!")

if __name__ == "__main__":
    main()
