# Project Report: DeadReckoner

> Generated: 2026-06-15 17:22:14  |  Format: LLM-optimized

## 1. Project Identity

| Property | Value |
|----------|-------|
| **Name** | DeadReckoner |
| **Language** | MATLAB |
| **Total Files** | 90 |
| **Total Lines** | 112583 |
| **Dependencies** | 0 |
| **Git Commits** | 162 |

### Architecture Overview

Project 'DeadReckoner' is a MATLAB project. Contains 2 source file(s) with 27 function(s) and 3 class(es). 2 file(s) have imports, referencing 14 external package(s). Repository has 90 total file(s) and ~112583 line(s) of code.

### Description

# DeadReckoner

A real-time, offline dead-reckoning and data-logging system built around the **ESP32-S3 N16R8**, the **MPU9250 IMU**, and a **binary SD-card logging pipeline**.

The project started as a small IMU prototype and evolved into a multi-core embedded platform for high-rate sensing, safe storage, fault handling, and offline analysis.

---

## 2. File Reference

| File | Lines | Functions | Classes | Imports |
|------|-------|-----------|---------|---------|
| `Code_deadreckoner/Matlab/3-BinReader/DeadReckoner_Parser.py` | 236 | `main` | `BinaryDecoder`, `DataScanner`, `DataExporter` | 6 |
| `project_summarizer.py` | 1216 | `check_git_repo`, `get_git_commits`, `build_tree_dict`, `render_tree_text`, `get_file_stats`, `parse_readme`, `parse_requirements`, `get_project_summary`, `extract_source_signatures`, `extract_import_graph`, `collect_config_files`, `generate_architecture_summary`, `get_file_contents`, `estimate_tokens`, `generate_text_report`, `generate_llm_markdown_report`, `_xc`, `generate_xml_report`, `show_welcome`, `get_config`, `run_with_spinner`, `show_data_overview`, `show_preview`, `_resolve_path`, `feedback_loop`, `main` | — | 10 |

## 3. Import Dependency Graph

- **`Code_deadreckoner/Matlab/3-BinReader/DeadReckoner_Parser.py`**
  - imports `glob` (external)
  - imports `matplotlib` (external)
  - imports `os` (external)
  - imports `pandas` (external)
  - imports `questionary` (external)
  - imports `struct` (external)
- **`project_summarizer.py`**
  - imports `ast` (external)
  - imports `collections` (external)
  - imports `datetime` (external)
  - imports `os` (external)
  - imports `pathlib` (external)
  - imports `re` (external)
  - imports `subprocess` (external)
  - imports `sys` (external)
  - imports `xml` (external)

## 4. Function & Class Signatures

### `Code_deadreckoner/Matlab/3-BinReader/DeadReckoner_Parser.py` (236 lines)

**Functions:**

- `def main`

**Classes:**

- `class BinaryDecoder`
  - `def parse_file (@classmethod)`
- `class DataScanner`
  - `def scan_missions (@staticmethod)`
- `class DataExporter`
  - `def export_csv (@staticmethod)`
  - `def export_excel (@staticmethod)`
  - `def generate_report (@staticmethod)`
  - `def plot_data (@staticmethod)`

### `project_summarizer.py` (1216 lines)

**Functions:**

- `def check_git_repo`
- `def get_git_commits`
- `def build_tree_dict`
- `def render_tree_text`
- `def get_file_stats`
- `def parse_readme`
- `def parse_requirements`
- `def get_project_summary`
- `def extract_source_signatures`
- `def extract_import_graph`
- `def collect_config_files`
- `def generate_architecture_summary`
- `def get_file_contents`
- `def estimate_tokens`
- `def generate_text_report`
- `def generate_llm_markdown_report`
- `def _xc` — XML-escape a string for use in attributes/text content.
- `def generate_xml_report`
- `def show_welcome`
- `def get_config`
- `def run_with_spinner`
- `def show_data_overview`
- `def show_preview`
- `def _resolve_path` — If file exists, ask overwrite or auto-number. Return final Path.
- `def feedback_loop`
- `def main`

## 5. Directory Structure

```
DeadReckoner/
├── Code_deadreckoner/
│   ├── ESP32_S3/
│   │   └── ESP32_S3.ino
│   ├── Matlab/
│   │   ├── 1-Visual/
│   │   │   ├── DR_LOG_001.BIN
│   │   │   ├── Visual.m
│   │   │   └── visual_v2.m
│   │   ├── 2-test result/
│   │   │   ├── 1-Static_Drift/
│   │   │   │   ├── 1- Static_Drift_Log.csv
│   │   │   │   ├── 1- Static_Drift_Log.png
│   │   │   │   └── Analyze_Static_Drift.m
│   │   │   ├── 2-Dynamic Return-to-Zero/
│   │   │   │   ├── 2_Dynamic_Return_to_Zero.csv
│   │   │   │   ├── Analyze_Dynamic_RTZ.m
│   │   │   │   ├── Dynamic_Return_to_Zero-1.png
│   │   │   │   └── Dynamic_Return_to_Zero-2.png
│   │   │   └── 3-Vibration Rejection/
│   │   │       ├── 3_Vibration_Rejection.csv
│   │   │       ├── Analyze_Vibration_Rejection.m
│   │   │       ├── Vibration_Rejection_1.png
│   │   │       └── Vibration_Rejection_2.png
│   │   └── 3-BinReader/
│   │       ├── Tests/
│   │       │   ├── 1-first log/
│   │       │   │   ├── DR_LOG.BIN
│   │       │   │   └── v1.m
│   │       │   ├── 2-test the recovery without PSRAM/
│   │       │   │   ├── 1-test the recovery.png
│   │       │   │   ├── DR_LOG_001.BIN
│   │       │   │   └── v2.m
│   │       │   ├── 3-test recovery with PSRAM- cant glue back/
│   │       │   │   ├── 1.png
│   │       │   │   ├── 2.png
│   │       │   │   ├── 3.png
│   │       │   │   ├── DR_LOG_001.BIN
│   │       │   │   ├── REC_001.BIN
│   │       │   │   ├── REC_002.BIN
│   │       │   │   ├── ResutCommandWindow.txt
│   │       │   │   └── v3.m
│   │       │   ├── 4-test recovery with PSRAM/
│   │       │   │   ├── 1.png
│   │       │   │   └── 2.png
│   │       │   └── 5-test recovery with PSRAM-make mistake/
│   │       │       ├── 1.png
│   │       │       ├── 2.png
│   │       │       ├── 3.png
│   │       │       ├── DR_LOG_001.BIN
│   │       │       ├── DR_LOG_002.BIN
│   │       │       ├── DR_LOG_003.BIN
│   │       │       ├── REC_001.BIN
│   │       │       ├── REC_002.BIN
│   │       │       ├── REC_003.BIN
│   │       │       ├── REC_004.BIN
│   │       │       ├── REC_005.BIN
│   │       │       ├── REC_006.BIN
│   │       │       ├── REC_007.BIN
│   │       │       ├── REC_008.BIN
│   │       │       ├── REC_009.BIN
│   │       │       ├── REC_010.BIN
│   │       │       ├── REC_011.BIN
│   │       │       └── v4.m
│   │       ├── 001001.BIN
│   │       ├── 002001.BIN
│   │       ├── 002002.BIN
│   │       ├── 002003.BIN
│   │       ├── DR_LOG_001.BIN
│   │       ├── DR_LOG_002.BIN
│   │       ├── DeadReckoner_Parser.py
│   │       ├── Mission_001.csv
│   │       ├── Mission_001_Plot.png
│   │       ├── Mission_001_Report.txt
│   │       └── V5.m
│   └── nodeMUC8266/
│       └── nodeMUC8266.ino
├── Diagram_deadreckoner/
│   ├── Li-ion-battery-discharge-voltage-curve.png
│   ├── Lipo_VS_LIIon.png
│   ├── SD card adaptors-1.png
│   ├── SD card adaptors-2.png
│   ├── SD card adaptors-3.jpg
│   ├── SD card adaptors-4.png
│   └── pinsNodeMUC8266.jpg
├── Repo_Report/
├── Report/
│   ├── Issues found by AI.md
│   ├── Progress.md
│   ├── Report.md
│   ├── Report2.md
│   └── To Do list.md
├── Test- Sanity Check/
│   ├── Chip_Identification_ESP/
│   │   └── Chip_Identification_ESP.ino
│   ├── MPU9250/
│   │   └── MPU9250.ino
│   ├── OLED/
│   │   └── OLED.ino
│   ├── Result Test/
│   │   └── Result Test_MPU9250.txt
│   └── SD card/
│       ├── Final test/
│       │   └── SDCard_ESP32S_DIY_addaptor/
│       │       ├── Result.txt
│       │       └── SDCard_ESP32S_DIY_addaptor.ino
│       ├── SDCardModule3.3V_nanoVersion/
│       │   └── SDCardModule3.3V_nanoVersion.ino
│       ├── SpeedTestForDiyAdaptor/
│       │   ├── Result speed test.txt
│       │   └── SpeedTestForDiyAdaptor.ino
│       ├── failed Tests/
│       │   ├── DIY2_SDCardAdaptor/
│       │   │   └── DIY2_SDCardAdaptor.ino
│       │   ├── DIY_SDCardAdaptor/
│       │   │   └── DIY_SDCardAdaptor.ino
│       │   ├── SDCardModule3.3V/
│       │   │   └── SDCardModule3.3V.ino
│       │   ├── SDCardModule3.3V_ES32Version/
│       │   │   └── SDCardModule3.3V_ES32Version.ino
│       │   └── microSDCardModule5V_NodeMCUVersion/
│       │       └── microSDCardModule5V_NodeMCUVersion.ino
│       └── microSDCardModule5V_nanoVersion/
│           └── microSDCardModule5V_nanoVersion.ino
├── .gitattributes
├── README.md
└── project_summarizer.py
```

## 6. Git History

- **Total Commits:** 162

| Hash | Date | Author | Message |
|------|------|--------|---------|
| `bc779b4` | 2026-06-15 15:13:39 | Alireza Sotoodeh | Clean up To Do list.md: remove completed items, keep pending only |
| `d6fc8e6` | 2026-06-15 15:09:01 | Alireza Sotoodeh | Add project_summarizer interactive report tool |
| `b50fcc2` | 2026-06-15 13:32:42 | Alireza Sotoodeh | Update Progress.md with storage, recovery, UI |
| `71b59a6` | 2026-06-15 10:57:32 | Alireza Sotoodeh | Fix timing, SD math, and logging robustness |
| `1d7378d` | 2026-06-15 10:45:39 | Alireza Sotoodeh | Relative timestamps, OLED reinit, buzzer beeps |
| `0a27909` | 2026-06-14 18:04:57 | Alireza Sotoodeh | Improve SD recovery and new log file creation |
| `19e696d` | 2026-06-14 17:25:58 | Alireza Sotoodeh | Expand SD menu to 8 items and fix actions |
| `f734a62` | 2026-06-14 17:07:36 | Alireza Sotoodeh | Track dropped frames and fix SD menu/UI |
| `c809e2f` | 2026-06-14 16:58:24 | Alireza Sotoodeh | Make frame sequence counter thread-safe |
| `73cdf91` | 2026-06-14 15:35:43 | Alireza Sotoodeh | Expand data frame & add O(1) log naming |
| `0a1eadd` | 2026-06-14 15:17:04 | Alireza Sotoodeh | Safely wipe logs: suspend sensor, purge SD |
| `23ce3b7` | 2026-06-14 15:13:07 | Alireza Sotoodeh | Remove unused STATE_SUBMENU_MSG and handlers |
| `7385d3b` | 2026-06-14 14:59:50 | Alireza Sotoodeh | Support recovery logs and use vTaskDelay |
| `b528985` | 2026-06-14 14:56:35 | Alireza Sotoodeh | Add configureMPUSettings helper |
| `9cafc3e` | 2026-06-14 14:53:17 | Alireza Sotoodeh | Remove unused SD recovery timer variable |
| `4cb4971` | 2026-06-14 14:51:20 | Alireza Sotoodeh | Validate log file after MPU recovery |
| `baf3ece` | 2026-06-14 14:46:45 | Alireza Sotoodeh | Zero-initialize gap LogFrame before SD write |
| `b5937cf` | 2026-06-14 14:44:54 | Alireza Sotoodeh | Fix runtime display and EEPROM offset |
| `1795018` | 2026-06-14 14:32:29 | Alireza Sotoodeh | Increment recovery ID after SD file open |
| `23b0d02` | 2026-06-14 14:30:28 | Alireza Sotoodeh | Improve SD space calc and file counting |
| `50661c2` | 2026-06-14 14:26:36 | Alireza Sotoodeh | Add timestamp to LogFrame and update frame size |
| `3dba014` | 2026-06-14 14:20:13 | Alireza Sotoodeh | Add EEPROM sentinel and robust calibration load/save |
| `986aa4c` | 2026-06-14 12:02:47 | Alireza Sotoodeh | Update Issues found by AI.md |
| `96593bd` | 2026-06-14 12:02:43 | Alireza Sotoodeh | Initialize SD gap LogFrame before write |
| `82eaf99` | 2026-06-14 12:00:35 | Alireza Sotoodeh | Add spinlocks for frame counter and tag events |
| `40f38a9` | 2026-06-14 11:54:38 | Alireza Sotoodeh | Add calibration prototypes and reorder globals |
| `e184656` | 2026-06-14 11:36:52 | Alireza Sotoodeh | Move OLED auto-off and trigger force UI update |
| `e9e168c` | 2026-06-14 11:34:07 | Alireza Sotoodeh | Add bandwidth metrics, PSRAM sizing & IMU temp |
| `39aaedf` | 2026-06-14 11:06:37 | Alireza Sotoodeh | Add AI-generated code review report |
| `aaf1c56` | 2026-06-13 23:41:37 | Alireza Sotoodeh | Rewrite To Do list with structured sections |
| `5d6f761` | 2026-06-13 23:29:43 | Alireza Sotoodeh | Purge dataQueue on log start and after wipe |
| `d2947c3` | 2026-06-13 23:24:30 | Alireza Sotoodeh | Yield to watchdog during SD flush; clarify SD calc |
| `b898876` | 2026-06-13 22:51:50 | Alireza Sotoodeh | Adjust SD remaining-hours calculation |
| `d18b758` | 2026-06-13 22:47:05 | Alireza Sotoodeh | Remove xQueueReset to avoid race condition |
| `756fa36` | 2026-06-13 17:53:44 | Alireza Sotoodeh | Fix SD capacity overflow on large cards |
| `18df5bb` | 2026-06-13 17:51:07 | Alireza Sotoodeh | Remove parent logs and nested recovery fragments |
| `7c4c10b` | 2026-06-13 17:48:29 | Alireza Sotoodeh | Use global frame counter and reset on log events |
| `c2042e5` | 2026-06-13 17:33:15 | Alireza Sotoodeh | Add safe long-press shutdown and SD flush |
| `aae3057` | 2026-06-13 17:23:08 | Alireza Sotoodeh | Display elapsed time instead of frame seq |
| `276a5f4` | 2026-06-13 17:12:42 | Alireza Sotoodeh | Improve SD runtime recovery and UI feedback |
| `a2c7a71` | 2026-06-13 16:55:30 | Alireza Sotoodeh | Move BinReader tests, add V5, tighten jump filter |
| `49fc4a0` | 2026-06-13 16:54:36 | Alireza Sotoodeh | Add global log/recovery IDs and improved recovery naming |
| `a9abe2b` | 2026-06-13 16:53:58 | Alireza Sotoodeh | Update To Do list.md |
| `12969b5` | 2026-06-12 21:40:34 | Alireza Sotoodeh | Mark Queue Overflow and 100Hz write fix done |
| `72ea622` | 2026-06-12 20:45:04 | Alireza Sotoodeh | Report: update To Do list tasks and notes |
| `2c07c5a` | 2026-06-12 20:44:48 | Alireza Sotoodeh | Optimize LogFrame, add PSRAM queue & SD recovery |
| `f8cf9f5` | 2026-06-12 20:44:24 | Alireza Sotoodeh | Move BinReader data to Tests and add v4 |
| `fba5d59` | 2026-06-12 20:40:06 | Alireza Sotoodeh | Add PSRAM recovery test data and v3 MATLAB reader |
| `d664e11` | 2026-06-12 19:39:13 | Alireza Sotoodeh | Reorganize MATLAB files and add BIN parser |
| `f401a2b` | 2026-06-12 18:38:40 | Alireza Sotoodeh | SD runtime recovery and pin remap |
| `5e9c419` | 2026-06-12 15:21:13 | Alireza Sotoodeh | Update To Do list with finding the final issues |
| `175ddba` | 2026-06-12 14:44:16 | Alireza Sotoodeh | Update README.md |
| `53ee269` | 2026-06-12 14:44:06 | Alireza Sotoodeh | Update To Do list.md |
| `ae8b760` | 2026-06-12 14:30:50 | Alireza Sotoodeh | Add comprehensive project reports and TODO |
| `109b15d` | 2026-06-11 22:18:56 | Alireza Sotoodeh | DYNAMIC RECOVERY PROTOCOL for MPU reconnection |
| `fff0137` | 2026-06-11 22:01:44 | Alireza Sotoodeh | Update Progress & Architecture Report.md |
| `61546f3` | 2026-06-11 21:09:23 | Alireza Sotoodeh | implement MPU9250 critical disconnect trap and boot-time SD scanning |
| `1e77d9b` | 2026-06-11 20:34:19 | Alireza Sotoodeh | fix (Total: XX.X GB) in submenu SD card |
| `cc09b6b` | 2026-06-11 20:26:26 | Alireza Sotoodeh | update menu - SD card sub menu |
| `2441549` | 2026-06-11 17:26:05 | Alireza Sotoodeh | update menu - mute buzzer menu |
| `82630a8` | 2026-06-11 17:16:46 | Alireza Sotoodeh | update menu - display mode |
| `34b6077` | 2026-06-11 16:49:15 | Alireza Sotoodeh | added buzzer and LED for Tag and SD card failure |
| `a19f32f` | 2026-06-10 22:22:11 | Alireza Sotoodeh | try to update oled-phase 4 |
| `74f1cf1` | 2026-06-10 18:05:30 | Alireza Sotoodeh | try to update oled- phase 3 |
| `a6f7e2a` | 2026-06-10 16:25:03 | Alireza Sotoodeh | try to update oled phase 2 |
| `e0a9af6` | 2026-06-10 16:14:20 | Alireza Sotoodeh | try to update oled- phase 1 |
| `20b4be7` | 2026-06-10 14:59:53 | Alireza Sotoodeh | Update Progress & Architecture Report.md |
| `0b6d062` | 2026-06-09 15:46:21 | Alireza Sotoodeh | Create Lipo_VS_LIIon.png |
| `06d4af0` | 2026-06-09 15:46:19 | Alireza Sotoodeh | Create Li-ion-battery-discharge-voltage-curve.png |
| `3a02459` | 2026-06-09 13:36:22 | Alireza Sotoodeh | first version of a bin file reader (MATLAB code) |
| `62a3aa7` | 2026-06-09 12:33:17 | Alireza Sotoodeh | update for SD card logger |
| `2d598a3` | 2026-06-09 11:40:52 | Alireza Sotoodeh | speed test Sd card |
| `63c2d1f` | 2026-06-08 17:22:48 | Alireza Sotoodeh | added test analyzer for no SD card mode |
| `b92c2d9` | 2026-06-08 16:57:49 | Alireza Sotoodeh | Update Progress & Architecture Report.md |
| `ff103ec` | 2026-06-08 16:32:36 | Alireza Sotoodeh | Final SD card Test (success!) |
| `f4be198` | 2026-06-08 16:32:09 | Alireza Sotoodeh | SD card Test |
| `56e2f99` | 2026-06-08 13:04:57 | Alireza Sotoodeh | Create Chip_Identification_ESP.ino |
| `571117f` | 2026-06-08 12:31:36 | Alireza Sotoodeh | clean up and added some test for SD card |
| `c64fab2` | 2026-06-08 11:04:07 | Alireza Sotoodeh | added SD card test (all methods has been failed) |
| `71b1203` | 2026-06-05 21:10:39 | Alireza Sotoodeh | added SD card Diagram |
| `38a5df4` | 2026-06-05 20:15:23 | Alireza Sotoodeh | diagram SD card adaptors |
| `d61bf42` | 2026-06-05 20:14:00 | Alireza Sotoodeh | Standalone Sanity Check for SPI SD Card Module |
| `95b4553` | 2026-06-05 03:57:14 | Alireza Sotoodeh | Sanity Check for SPI SD Card Module |
| `4770619` | 2026-06-04 22:18:46 | Alireza Sotoodeh | Hardware & Filter Validation (Test Results) |
| `49a098e` | 2026-06-04 22:18:32 | Alireza Sotoodeh | Vibration Rejection test result |
| `490ca10` | 2026-06-04 21:09:55 | Alireza Sotoodeh | Clean up and add a second test (Dynamic Return-to-Zero) |
| `3ff7a07` | 2026-06-04 16:28:32 | Alireza Sotoodeh | Drift Test Visualizer |
| `4fe7d7c` | 2026-06-04 16:28:10 | Alireza Sotoodeh | Update Progress & Architecture Report.md |
| `6db47c8` | 2026-06-04 15:41:16 | Alireza Sotoodeh | fix OLED |
| `e5a2f29` | 2026-06-04 15:33:59 | Alireza Sotoodeh | ix issues before test |
| `5fe3bf7` | 2026-06-04 15:31:49 | Alireza Sotoodeh | Firmware Flashing Configuration (ESP32-S3 N16R8) |
| `6b28855` | 2026-06-04 15:03:10 | Alireza Sotoodeh | Update Progress & Architecture Report.md |
| `66e7771` | 2026-06-04 14:59:02 | Alireza Sotoodeh | Sanity Check for OLED 0.91 |
| `8a65805` | 2026-06-04 14:46:46 | Alireza Sotoodeh | Basic I2C sanity check MPU9250 |
| `5b4c454` | 2026-06-04 13:05:46 | Alireza Sotoodeh | Updated wiring diagram |
| `b099dad` | 2026-06-04 13:05:08 | Alireza Sotoodeh | Update Progress & Architecture Report.md |
| `08459b3` | 2026-06-03 19:32:06 | Alireza Sotoodeh | dual core with Free Atreus |
| `0ee83a3` | 2026-06-03 19:09:52 | Alireza Sotoodeh | Update Progress & Architecture Report.md |
| `ef89423` | 2026-06-03 18:43:14 | Alireza Sotoodeh | Create ESP32_S3.ino |
| `d77ccbc` | 2026-06-03 18:42:35 | Alireza Sotoodeh | Create Progress & Architecture Report.md |
| `6ed1539` | 2026-06-03 17:00:09 | Alireza Sotoodeh | added wiring diagram for ESP |
| `92d4cb3` | 2026-06-03 16:34:13 | Alireza Sotoodeh | init_project(again) |
| `d7407d4` | 2026-06-03 15:53:09 | Alireza Sotoodeh | Delete setup modules directory |
| `863ff07` | 2026-06-03 15:52:57 | Alireza Sotoodeh | Delete main code directory |
| `196cec4` | 2026-06-03 15:52:38 | Alireza Sotoodeh | Delete .metadata directory |
| `3b96d1d` | 2025-08-09 11:17:13 | Alireza Sotoodeh | Merge pull request #8 from Alireza-Sotoodeh/NodeMUC8266 |
| `3359107` | 2025-08-09 11:16:43 | Alireza Sotoodeh | cleaning repo-05/18-11:16PM |
| `2bc2774` | 2025-08-08 16:57:22 | Alireza Sotoodeh | fixed the acceleration (linear acceleration)-05/17-4:57 |
| `b0166f0` | 2025-08-08 11:19:45 | Alireza Sotoodeh | oled display fixed-05/17-11:19PM |
| `ff11272` | 2025-08-07 12:26:53 | Alireza Sotoodeh | 05/16-12:26PM |
| `04cdf7e` | 2025-08-06 14:43:03 | Alireza Sotoodeh | 05/15-2:42PM |
| `9cbcf16` | 2025-08-06 14:17:22 | Alireza Sotoodeh | 05/15-2:17PM |
| `d69f889` | 2025-08-05 20:33:51 | Alireza Sotoodeh | 05/14-8:33Pm |
| `bd57081` | 2025-08-05 19:59:43 | Alireza Sotoodeh | 05/14-7:59PM |
| `a18b783` | 2025-08-05 19:45:52 | Alireza Sotoodeh | Merge pull request #7 from Alireza-Sotoodeh/NodeMUC8266 |
| `f623caf` | 2025-08-05 19:44:23 | Alireza Sotoodeh | 05/14-7:43PM |
| `dac7043` | 2025-08-05 19:34:07 | Alireza Sotoodeh | 05/14_7:33PM |
| `3e039ce` | 2025-08-05 17:19:11 | Alireza Sotoodeh | 05/14-5:19PM |
| `8a9c9e9` | 2025-08-05 15:57:33 | Alireza Sotoodeh | 05/14 |
| `1b1623d` | 2025-08-04 10:20:12 | Alireza Sotoodeh | 05/13-10:20Pm |
| `5e9d19f` | 2025-08-04 09:02:54 | Alireza Sotoodeh | start |
| `31c9ae8` | 2025-08-04 09:01:25 | Alireza Sotoodeh | save before start |
| `9dc8987` | 2025-08-03 15:42:08 | Alireza Sotoodeh | removed the project |
| `bba6109` | 2025-08-03 15:35:11 | Alireza Sotoodeh | 05/12-3:34PM |
| `d44cee1` | 2025-08-02 14:05:01 | Alireza Sotoodeh | 05/11-2:04PM |
| `b578859` | 2025-08-02 14:02:40 | Alireza Sotoodeh | 05/11-2:02PM |
| `da3fe11` | 2025-07-25 16:48:20 | Alireza Sotoodeh | Update workbench.xmi |
| `0e84527` | 2025-07-25 16:34:39 | Alireza Sotoodeh | Add DMP.c for MPU_9250 and update workspace state |
| `3cb9f77` | 2025-07-25 16:31:47 | Alireza Sotoodeh | Revert "05/03-3:53PM" |
| `35e3c88` | 2025-07-25 15:53:44 | Alireza Sotoodeh | 05/03-3:53PM |
| `527e4d0` | 2025-07-25 15:04:37 | Alireza Sotoodeh | Merge branch 'MPU9250' of https://github.com/Alireza-Sotoodeh/Navigation-STM32 into MPU9250 |
| `8df6c0a` | 2025-07-25 15:04:30 | Alireza Sotoodeh | Update MPU_9250.md |
| `fccbc95` | 2025-07-25 15:02:46 | Alireza Sotoodeh | 05/03-3:02PM |
| `e1c129e` | 2025-07-25 12:17:52 | Alireza Sotoodeh | 05/03-12:17PM |
| `ac92d29` | 2025-07-25 12:17:27 | Alireza Sotoodeh | 05/03-12:17PM |
| `cfa9612` | 2025-07-25 12:13:08 | Alireza Sotoodeh | 05/03-12:12PM |
| `d7b0656` | 2025-07-25 12:11:14 | Alireza Sotoodeh | 05/03-12:10PM |
| `27d3701` | 2025-07-25 11:23:10 | Alireza Sotoodeh | 05/03-11:22AM (save before start) |
| `b869740` | 2025-07-24 19:39:06 | Alireza Sotoodeh | 05/02-7:38PM |
| `0a7b903` | 2025-07-24 19:31:16 | Alireza Sotoodeh | 05/02-7:30PM |
| `04bcf97` | 2025-07-23 20:54:00 | Alireza Sotoodeh | 05/01-8:53PM |
| `3f6cd2f` | 2025-07-23 20:50:15 | Alireza Sotoodeh | 05/01-8:49 |
| `5b426fa` | 2025-07-23 11:05:54 | Alireza Sotoodeh | Update README.md |
| `0455e92` | 2025-07-20 21:44:39 | Alireza Sotoodeh | 04/29-9:44PM |
| `3b1b5ac` | 2025-07-18 20:16:29 | Alireza Sotoodeh | 04/27- 8:15 PM |
| `4305256` | 2025-07-18 18:04:53 | Alireza Sotoodeh | 04/27-6:03 PM |
| `701ae4b` | 2025-07-18 10:48:51 | Alireza Sotoodeh | 04/27-10:48 AM |
| `62df236` | 2025-07-17 22:30:28 | Alireza Sotoodeh | 04/26 |
| `768b21c` | 2025-07-16 08:01:08 | Alireza Sotoodeh | now code has no error |
| `ec842c7` | 2025-07-14 11:33:26 | Alireza Sotoodeh | 04/23 saving before going for a break! |
| `2327c58` | 2025-07-14 10:47:22 | Alireza Sotoodeh | before testing the MPU6500 commands |
| `e532bc4` | 2025-07-13 20:11:22 | Alireza Sotoodeh | added all necceasry files |
| `8ac5a28` | 2025-07-13 15:01:44 | Alireza Sotoodeh | save beffore going to my new brand branch |
| `9b63d4f` | 2025-07-13 15:00:15 | Alireza Sotoodeh | adding scr MPU6500 |
| `d95b49d` | 2025-07-13 14:56:47 | Alireza Sotoodeh | setup CubeIDE |
| `4239272` | 2025-07-13 14:52:23 | Alireza Sotoodeh | added MPU6500 library |
| `b31009d` | 2025-07-13 14:50:43 | Alireza Sotoodeh | removing all file(from the top) |
| `2f0ded6` | 2025-07-13 14:26:16 | Alireza Sotoodeh | fixing readme file |
| `adfc075` | 2025-07-13 14:24:16 | Alireza Sotoodeh | initial MPU6500 and trying to test it |
| `05d017f` | 2025-07-12 20:39:48 | Alireza Sotoodeh | initial PJ |
| `bed8a35` | 2025-07-12 19:01:00 | Alireza Sotoodeh | modified readme file |
| `d72b9ad` | 2025-07-12 18:41:59 | Alireza Sotoodeh | Initial commit |

<details>
<summary><b>Commit 1:</b> <code>bc779b4</code> — Clean up To Do list.md: remove completed items, keep pending only</summary>

```
Hash:   bc779b4
Date:   2026-06-15 15:13:39 +0330
Author: Alireza Sotoodeh

Clean up To Do list.md: remove completed items, keep pending only
```
</details>

<details>
<summary><b>Commit 2:</b> <code>d6fc8e6</code> — Add project_summarizer interactive report tool</summary>

```
Hash:   d6fc8e6
Date:   2026-06-15 15:09:01 +0330
Author: Alireza Sotoodeh

Add project_summarizer interactive report tool

Introduce project_summarizer.py â€” an interactive CLI tool to generate project documentation and machine-friendly context. It extracts git commit history, builds a directory tree, computes file stats and project summary (README, requirements), and performs code analysis (Python function/class signatures, import dependency graph, config file collection, optional file contents). Outputs include a human-readable text report, an LLM-optimized Markdown report, and a prompt-optimized XML export; optional token estimation via tiktoken. The script uses rich for UI and questionary for prompts, saves reports to an output folder, and includes spinner/progress UX and overwrite handling.
```
</details>

<details>
<summary><b>Commit 3:</b> <code>b50fcc2</code> — Update Progress.md with storage, recovery, UI</summary>

```
Hash:   b50fcc2
Date:   2026-06-15 13:32:42 +0330
Author: Alireza Sotoodeh

Update Progress.md with storage, recovery, UI

Expand the project progress report to document extensive storage, logging, recovery, UI, and analysis improvements. Adds detailed binary LogFrame design (packed/union payloads), 64-bit timestamps, PSRAM-backed buffering and queue architecture, deterministic file naming, gap-frame markers, frame sequencing and dropped-frame accounting, SD recovery/fragment logging and write verification, watchdog-safe shutdowns, and FAT/performance fixes. Also documents UI/monitoring enhancements (OLED menus, storage dashboard, runtime recovery feedback, log management), a new Phase 8.5 (memory optimization & scalability), data-analysis toolchain updates (fragment reconstruction, garbage-frame filtering, quaternion/euler visualization), and reserved GPS payloads for future GNSS integration.
```
</details>

<details>
<summary><b>Commit 4:</b> <code>71b59a6</code> — Fix timing, SD math, and logging robustness</summary>

```
Hash:   71b59a6
Date:   2026-06-15 10:57:32 +0330
Author: Alireza Sotoodeh

Fix timing, SD math, and logging robustness

Address multiple stability and correctness issues:

- Remove unused subMenuMsg variable.
- Prevent 64-bit torn reads across cores by atomically copying shared log_time_base into a local uint64_t inside a critical section and use that for timestamp calculation. This ensures high-precision esp_timer timestamps are computed relative to a consistent time base.
- Zero-initialize receivedFrame to avoid rendering garbage stack bytes before the first frame arrives.
- Use 64-bit casts when calculating free/total MB from cluster counts to avoid overflow on large SD cards (>32GB) and update related comments.
- Update bandwidth accounting comment to reflect 45-Byte packets and recalc remaining hours accordingly.
- Add periodic vTaskDelay yields during large PSRAM flushes (every 500 frames) to prevent RTOS watchdog resets; updated comment to reflect buffer size change.
- After wiping memory and reopening the log file, defensively check logFile and set sd_critical_error if allocation/opening fails to catch file errors immediately.
- Misc: minor comment/clarity refinements.

These changes improve multi-core timestamp correctness, SD capacity math reliability, and runtime robustness during heavy I/O.
```
</details>

<details>
<summary><b>Commit 5:</b> <code>1d7378d</code> — Relative timestamps, OLED reinit, buzzer beeps</summary>

```
Hash:   1d7378d
Date:   2026-06-15 10:45:39 +0330
Author: Alireza Sotoodeh

Relative timestamps, OLED reinit, buzzer beeps

Introduce log_time_base (uint64_t) and record frame.timestamp as esp_timer_get_time() - log_time_base so timestamps are relative to the current log. Initialize log_time_base at initial boot and whenever the frame/ log counters are reset (inside critical sections). Add a two-beep audible pattern when entering the safety/green-LED state. Improve OLED resilience by calling u8g2.begin(), setFont() and setPowerSave(0) on wake and when entering the menu, and force a UI update to avoid stale displays. These changes improve timestamp consistency across log files and recoverability of the display/hardware feedback after hot-plug or power events.
```
</details>

<details>
<summary><b>Commit 6:</b> <code>0a27909</code> — Improve SD recovery and new log file creation</summary>

```
Hash:   0a27909
Date:   2026-06-14 18:04:57 +0330
Author: Alireza Sotoodeh

Improve SD recovery and new log file creation

Disable hardware alarms (LED/buzzer) before calling blocking SD recovery to prevent continuous buzzer output. Ensure thread-safe capture and monotonic increment of the frame sequence counter and capture a 64-bit microsecond timestamp. After successful recovery, explicitly clear/reset hardware signals and provide user feedback. When creating a new log file, sync and close the previous file, safely bump the max_log_id to guarantee a new filename, reset recovery and frame counters within a critical section, and null-check the opened file to set a critical error if creation fails. Added clarifying comments and minor cleanup.
```
</details>

<details>
<summary><b>Commit 7:</b> <code>19e696d</code> — Expand SD menu to 8 items and fix actions</summary>

```
Hash:   19e696d
Date:   2026-06-14 17:25:58 +0330
Author: Alireza Sotoodeh

Expand SD menu to 8 items and fix actions

Add an 8th item to the SD submenu and correct navigation and action mappings to avoid off-by-one/select bugs. Adjust sdMenuCursor bounds (0..7) and remap SELECT behavior so the new "Drops" entry jumps to Live View; shift Create/Format/Back handling to the correct indices and default confirmation focus to NO for safety. Add a null-check after dynamic log file creation to set sd_critical_error on failure. Improve robustness during full card wipe: suspend the sensor task while wiping, insert small vTaskDelay yields to avoid WDT timeouts, reset max_log_id after format, reset the data queue, and reopen the initial log file. Minor whitespace/cleanup changes and UI update triggers retained.
```
</details>

<details>
<summary><b>Commit 8:</b> <code>f734a62</code> — Track dropped frames and fix SD menu/UI</summary>

```
Hash:   f734a62
Date:   2026-06-14 17:07:36 +0330
Author: Alireza Sotoodeh

Track dropped frames and fix SD menu/UI

Add dropped_frames_count to global state and increment it when xQueueSend fails, providing LED_RED feedback on silent queue overflows to help diagnose missed frames. Zero-initialize gapFrame with memset before populating and perform global_frame_counter updates inside the frameCounterMux critical section to avoid uninitialized data and race conditions. Update SD submenu UI: expand the lines buffer, add a "Drops" entry, shift menu indices, and handle the new SD menu cursor case to return to the main menu. Misc: minor comment cleanup.
```
</details>

<details>
<summary><b>Commit 9:</b> <code>c809e2f</code> — Make frame sequence counter thread-safe</summary>

```
Hash:   c809e2f
Date:   2026-06-14 16:58:24 +0330
Author: Alireza Sotoodeh

Make frame sequence counter thread-safe

Clear the LogFrame structure and protect capture/increment of global_frame_counter with a critical section (portENTER_CRITICAL/portEXIT_CRITICAL). This ensures the frame_seq is assigned atomically and monotonically to avoid race conditions and uninitialized/garbage bytes in the frame.
```
</details>

<details>
<summary><b>Commit 10:</b> <code>73cdf91</code> — Expand data frame & add O(1) log naming</summary>

```
Hash:   73cdf91
Date:   2026-06-14 15:35:43 +0330
Author: Alireza Sotoodeh

Expand data frame & add O(1) log naming

Increase DATA_FRAME_SIZE from 41 to 45 and change frame timestamp from uint32_t to uint64_t to prevent the ~71-minute esp_timer overflow; corresponding timestamp assignments removed 32-bit casts. Add max_log_id to track the highest DR_LOG index and replace the sd.exists scanning loop with an O(1) instantaneous file creation path (increments max_log_id, capped at 999). Ensure new-file creation resets global_frame_counter under mutex and purges the dataQueue so new logs start from a sterile buffer. Initialize max_log_id in setup and apply small formatting/comment clarifications.
```
</details>

<details>
<summary><b>Commit 11:</b> <code>0a1eadd</code> — Safely wipe logs: suspend sensor, purge SD</summary>

```
Hash:   0a1eadd
Date:   2026-06-14 15:17:04 +0330
Author: Alireza Sotoodeh

Safely wipe logs: suspend sensor, purge SD

During SD wipe confirmation the sensor task is now suspended/resumed to avoid concurrent access. The delete loop was enhanced to remove parent logs and orphaned recovery fragments, and a short vTaskDelay is added each iteration to feed the watchdog. After wiping the code resets current_log_filename, global_log_id, global_recovery_id and zeroes global_frame_counter inside a critical section, resets the data queue, reopens a fresh log file, and updates the UI. These changes prevent race conditions, watchdog resets, and stale frames leaking into the new session.
```
</details>

<details>
<summary><b>Commit 12:</b> <code>23ce3b7</code> — Remove unused STATE_SUBMENU_MSG and handlers</summary>

```
Hash:   23ce3b7
Date:   2026-06-14 15:13:07 +0330
Author: Alireza Sotoodeh

Remove unused STATE_SUBMENU_MSG and handlers

Delete the deprecated STATE_SUBMENU_MSG enum value and remove its navigation and rendering code in Code_deadreckoner/ESP32_S3/ESP32_S3.ino. This cleans up the UI state machine by eliminating an unused message submenu, removing the select/back handling and the drawStr rendering block, and simplifies conditional branches with no intended functional changes.
```
</details>

<details>
<summary><b>Commit 13:</b> <code>7385d3b</code> — Support recovery logs and use vTaskDelay</summary>

```
Hash:   7385d3b
Date:   2026-06-14 14:59:50 +0330
Author: Alireza Sotoodeh

Support recovery logs and use vTaskDelay

Detect recovery-style .BIN files (exactly 10 chars with first 6 digits) in addition to existing DR_LOG_ parent sessions and include them in totalFilesCount. Replace blocking delay() calls with FreeRTOS vTaskDelay(pdMS_TO_TICKS(...)) for the buzzer and the "All Logs Cleared!" pause to avoid blocking the logging task and potential watchdog starvation.
```
</details>

<details>
<summary><b>Commit 14:</b> <code>b528985</code> — Add configureMPUSettings helper</summary>

```
Hash:   b528985
Date:   2026-06-14 14:56:35 +0330
Author: Alireza Sotoodeh

Add configureMPUSettings helper

Introduce configureMPUSettings(MPU9250Setting&) to centralize MPU9250 setting assignments (accel/gyro/mag/fifo/filter). Replace duplicated manual initialization in setup() and attemptMPURecovery() with the new helper to reduce code duplication and improve maintainability; no functional behavior changes.
```
</details>

<details>
<summary><b>Commit 15:</b> <code>9cafc3e</code> — Remove unused SD recovery timer variable</summary>

```
Hash:   9cafc3e
Date:   2026-06-14 14:53:17 +0330
Author: Alireza Sotoodeh

Remove unused SD recovery timer variable

Delete the unused `last_sd_recovery_attempt` variable from sensorTask in ESP32_S3.ino. This cleans up an unnecessary local variable, avoiding potential compiler warnings and slightly reducing memory usage while keeping existing MPU recovery logic intact.
```
</details>

<details>
<summary><b>Commit 16:</b> <code>4cb4971</code> — Validate log file after MPU recovery</summary>

```
Hash:   4cb4971
Date:   2026-06-14 14:51:20 +0330
Author: Alireza Sotoodeh

Validate log file after MPU recovery

After recovering the MPU, validate the reopened log file and set sd_critical_error if the file pointer is invalid to avoid silent write failures (addresses FIX ISSUE 5). Also make minor loggingTask cleanup: ensure periodic sync sets sd_critical_error on failure and adjust closing braces/formatting for the loggingTask function.
```
</details>

<details>
<summary><b>Commit 17:</b> <code>baf3ece</code> — Zero-initialize gap LogFrame before SD write</summary>

```
Hash:   baf3ece
Date:   2026-06-14 14:46:45 +0330
Author: Alireza Sotoodeh

Zero-initialize gap LogFrame before SD write

When recovering the SD card, fully clear the LogFrame used as a gap marker to eliminate any uninitialized/garbage bytes. Replace selective payload memsets with a single memset(&gapFrame, 0, sizeof(LogFrame)) and then assign frame_seq, timestamp and event_flag before writing the sterile block to the log file. This ensures consistent, deterministic gap records during SD recovery.
```
</details>

<details>
<summary><b>Commit 18:</b> <code>b5937cf</code> — Fix runtime display and EEPROM offset</summary>

```
Hash:   b5937cf
Date:   2026-06-14 14:44:54 +0330
Author: Alireza Sotoodeh

Fix runtime display and EEPROM offset

Compute displayed runtime from receivedFrame.timestamp (microseconds) instead of deriving seconds from frame_seq, ensuring the on-screen T: value reflects real elapsed time. Also fix EEPROM calibration storage by starting writes after the 16-bit magic (addr = sizeof(uint16_t)) and consolidating addr increments so each float is stored at the correct offset, preventing overlap with the magic value.
```
</details>

<details>
<summary><b>Commit 19:</b> <code>1795018</code> — Increment recovery ID after SD file open</summary>

```
Hash:   1795018
Date:   2026-06-14 14:32:29 +0330
Author: Alireza Sotoodeh

Increment recovery ID after SD file open

Fixes Issue #9: generate the recovery filename from the current validated indices and only increment global_recovery_id after the SD file is successfully opened. Previously the index was incremented before verifying the file open, which could lead to skipped or mismatched filenames on failure. Also updates comments and cleans up minor whitespace.
```
</details>

<details>
<summary><b>Commit 20:</b> <code>23b0d02</code> — Improve SD space calc and file counting</summary>

```
Hash:   23b0d02
Date:   2026-06-14 14:30:28 +0330
Author: Alireza Sotoodeh

Improve SD space calc and file counting

Prevent arithmetic overflow for large SD cards by casting cluster calculations to uint64_t and compute sd_total_gb. Replace slow brute-force sd.exists() loop with a high-speed directory iteration (openNext) that filters for ".BIN" files and counts only files starting with "DR_LOG_". Add safer buffer sizes, explicit file/root closes, and ensure SD-related globals are zeroed in the fallback branch. These changes improve performance and correctness when scanning large cards and many log files.
```
</details>

<details>
<summary><b>Commit 21:</b> <code>50661c2</code> — Add timestamp to LogFrame and update frame size</summary>

```
Hash:   50661c2
Date:   2026-06-14 14:26:36 +0330
Author: Alireza Sotoodeh

Add timestamp to LogFrame and update frame size

Increase DATA_FRAME_SIZE to 41 and add a uint32_t timestamp to the packed LogFrame to capture a high-precision microsecond hardware timestamp. Populate frame.timestamp in sensorTask (using esp_timer_get_time()) and for SD gap frames in loggingTask, and ensure gap frames' payloads are zeroed to avoid garbage. Update display logic to compute runtime from the hardware timestamp (microseconds -> seconds) instead of frame_seq. Small comment and struct documentation updates to reflect the new parametric binary layout.
```
</details>

<details>
<summary><b>Commit 22:</b> <code>3dba014</code> — Add EEPROM sentinel and robust calibration load/save</summary>

```
Hash:   3dba014
Date:   2026-06-14 14:20:13 +0330
Author: Alireza Sotoodeh

Add EEPROM sentinel and robust calibration load/save

Introduce an EEPROM magic sentinel (EEPROM_MAGIC_NUMBER / EEPROM_MAGIC_ADDR) and reserve space for it. saveCalibration() now writes the magic before calibration floats and loadCalibration() validates the magic first; if missing, it logs a warning, applies factory defaults (zero biases, unit scales) and aborts loading to avoid reading corrupt floats. Adjusts address offsets accordingly and adds a calibration header comment. This improves robustness of calibration persistence across reboots.
```
</details>

<details>
<summary><b>Commit 23:</b> <code>986aa4c</code> — Update Issues found by AI.md</summary>

```
Hash:   986aa4c
Date:   2026-06-14 12:02:47 +0330
Author: Alireza Sotoodeh

Update Issues found by AI.md
```
</details>

<details>
<summary><b>Commit 24:</b> <code>96593bd</code> — Initialize SD gap LogFrame before write</summary>

```
Hash:   96593bd
Date:   2026-06-14 12:02:43 +0330
Author: Alireza Sotoodeh

Initialize SD gap LogFrame before write

Fix SD recovery gap frame initialization to avoid writing garbage bytes. Previously the gap frame used 0xFFFFFFFF and left payload/identifiers uninitialized. Now the commit sets frame_seq to global_frame_counter, marks event_flag as 0xAA to identify the SD_GAP event, clears the IMU payload fields with memset, and forces a synced write to ensure a sterile gap marker and improve log integrity during SD recovery.
```
</details>

<details>
<summary><b>Commit 25:</b> <code>82eaf99</code> — Add spinlocks for frame counter and tag events</summary>

```
Hash:   82eaf99
Date:   2026-06-14 12:00:35 +0330
Author: Alireza Sotoodeh

Add spinlocks for frame counter and tag events

Introduce two portMUX_TYPE spinlocks (frameCounterMux, tagEventMux) and use portENTER_CRITICAL/portEXIT_CRITICAL around accesses to global_frame_counter and tag_event_triggered. Protects incrementing/initializing the frame sequence and setting/clearing the waypoint tag flag across cores (sensorTask and loggingTask) to prevent race conditions and inconsistent frame_seq or tag events on ESP32-S3 multicore operation. Comments added to clarify intent.
```
</details>

<details>
<summary><b>Commit 26:</b> <code>40f38a9</code> — Add calibration prototypes and reorder globals</summary>

```
Hash:   40f38a9
Date:   2026-06-14 11:54:38 +0330
Author: Alireza Sotoodeh

Add calibration prototypes and reorder globals

Move and consolidate several preprocessor defines and global declarations for clarity, add forward declarations for calibration functions (performCalibration, print_calibration, saveCalibration, loadCalibration) to avoid scope issues, and reorder UI/menu-related globals (MENU_ITEMS_COUNT, menuItems, menuCursor, subMenuMsg) and log filename. Also cleaned up spacing/indentation and removed duplicated QUEUE_LENGTH define. No functional logic changes intendedâ€”just reorganization and preparatory prototypes.
```
</details>

<details>
<summary><b>Commit 27:</b> <code>e184656</code> — Move OLED auto-off and trigger force UI update</summary>

```
Hash:   e184656
Date:   2026-06-14 11:36:52 +0330
Author: Alireza Sotoodeh

Move OLED auto-off and trigger force UI update

Relocates the OLED auto-off power management check in loggingTask from before the graphics rendering phase to after rendering completes. When the timeout is hit the code now sets power save, updates currentState, and sets force_update_ui = true to ensure the UI state is refreshed when entering sleep. Also includes a tiny whitespace cleanup in setup().
```
</details>

<details>
<summary><b>Commit 28:</b> <code>e9e168c</code> — Add bandwidth metrics, PSRAM sizing & IMU temp</summary>

```
Hash:   e9e168c
Date:   2026-06-14 11:34:07 +0330
Author: Alireza Sotoodeh

Add bandwidth metrics, PSRAM sizing & IMU temp

Introduce a parametric bandwidth/memory engine (DATA_FRAME_SIZE, SAMPLING_RATE_HZ, BYTES_PER_SECOND, BYTES_PER_HOUR, MB_PER_HOUR, QUEUE_LENGTH, PSRAM_BUFFER_SIZE_MB) to compute PSRAM buffer sizing and dynamic bandwidth. Add imu.temp to LogFrame, populate it in sensorTask, and display the stored temperature on the OLED (use receivedFrame.payload.imu.temp instead of calling mpu.getTemperature() directly). Replace the hardcoded 11.88 MB/hr with MB_PER_HOUR when estimating remaining SD hours, move/clean up the QUEUE_LENGTH definition, and print the computed PSRAM buffer size at setup. Update struct size/comments and perform minor formatting/whitespace cleanups.
```
</details>

<details>
<summary><b>Commit 29:</b> <code>39aaedf</code> — Add AI-generated code review report</summary>

```
Hash:   39aaedf
Date:   2026-06-14 11:06:37 +0330
Author: Alireza Sotoodeh

Add AI-generated code review report

Add Report/Issues found by AI.md containing a comprehensive AI review of the ESP32-S3 DeadReckoner firmware. The document consolidates findings from multiple reviewers (deep seek, ChatGPT, Claude), enumerates critical/major/minor issues (thread-safety, SD recovery, timing, data integrity, UI and power handling), provides recommended fixes and improvements, and includes overall scores. This file is intended to guide remediation and prioritize reliability and performance fixes.
```
</details>

<details>
<summary><b>Commit 30:</b> <code>aaf1c56</code> — Rewrite To Do list with structured sections</summary>

```
Hash:   aaf1c56
Date:   2026-06-13 23:41:37 +0330
Author: Alireza Sotoodeh

Rewrite To Do list with structured sections

Rework the DeadReckoner To Do list: cleaned up formatting, reorganized and consolidated items into clear sections (Core Data Logging & Architecture, UI/UX, MATLAB/Postâ€‘processing, Hardware). Updated and checked completed tasks (SD dynamic recovery, PSRAM buffering, frame size reduction, gap frames, queue fixes, pin relocations, MATLAB stitcher, safe shutdown, etc.), added concrete hardware recommendations (decoupling caps, I2C pullups, button debouncing), and clarified outstanding action items (MPU9250 health tracking, GPS integration, BMP280, watchâ€‘dog evaluation, ZUPT/SHS/LSTM research). Removed older unstructured entries and improved readability for easier tracking and prioritization.
```
</details>

<details>
<summary><b>Commit 31:</b> <code>5d6f761</code> — Purge dataQueue on log start and after wipe</summary>

```
Hash:   5d6f761
Date:   2026-06-13 23:29:43 +0330
Author: Alireza Sotoodeh

Purge dataQueue on log start and after wipe

Call xQueueReset(dataQueue) in loggingTask when creating a new log file and after clearing all logs. This critical fix clears stale sensor frames that may accumulate during menu interactions or formatting, ensuring new recordings start from a clean buffer and preventing leaked frames from being written to new sessions. Also includes small comment/formatting tweaks around the reset logic.
```
</details>

<details>
<summary><b>Commit 32:</b> <code>d2947c3</code> — Yield to watchdog during SD flush; clarify SD calc</summary>

```
Hash:   d2947c3
Date:   2026-06-13 23:24:30 +0330
Author: Alireza Sotoodeh

Yield to watchdog during SD flush; clarify SD calc

Remove unused lastPrintMillis variable. Add a flushCount and periodic vTaskDelay during dataQueue flushing so the RTOS Task Watchdog is fed every 500 frames, preventing hard resets when emptying large PSRAM-backed buffers to SD. Clarify SD free/remaining capacity calculations and document the dynamic bandwidth assumption (33 bytes * 100 Hz = 11.88 MB/hour). Minor comment and spacing adjustments.
```
</details>

<details>
<summary><b>Commit 33:</b> <code>b898876</code> — Adjust SD remaining-hours calculation</summary>

```
Hash:   b898876
Date:   2026-06-13 22:51:50 +0330
Author: Alireza Sotoodeh

Adjust SD remaining-hours calculation

Update the remaining-hours estimate to reflect the new data rate/format by changing the divisor from 20.16 to 11.88 (matches the optimized 33-byte packed frame architecture at 100Hz). Also keep the 64-bit cast for cluster arithmetic to avoid overflow on large SD cards and add an explanatory inline comment.
```
</details>

<details>
<summary><b>Commit 34:</b> <code>d18b758</code> — Remove xQueueReset to avoid race condition</summary>

```
Hash:   d18b758
Date:   2026-06-13 22:47:05 +0330
Author: Alireza Sotoodeh

Remove xQueueReset to avoid race condition

Delete the call to xQueueReset(dataQueue) in loggingTask and add a comment explaining the reason: resetting the queue could race with Core 0 which is buffering valid pre-fusion IMU frames, causing those frames to be lost. This preserves recently produced data when resuming logging and documents the critical fix.
```
</details>

<details>
<summary><b>Commit 35:</b> <code>756fa36</code> — Fix SD capacity overflow on large cards</summary>

```
Hash:   756fa36
Date:   2026-06-13 17:53:44 +0330
Author: Alireza Sotoodeh

Fix SD capacity overflow on large cards

Cast cluster*sector multiplication to 64-bit to avoid 32-bit overflow on large SD cards (>32GB). sd_free_mb and sd_total_mb are now computed from (uint64_t)freeClusters/totalClusters before division, preventing incorrect capacity values. Kept existing remaining-hours and GB calculations unchanged. Also cleaned up minor stray whitespace and preserved existing file/count resets and menu cursor initialization.
```
</details>

<details>
<summary><b>Commit 36:</b> <code>18df5bb</code> — Remove parent logs and nested recovery fragments</summary>

```
Hash:   18df5bb
Date:   2026-06-13 17:51:07 +0330
Author: Alireza Sotoodeh

Remove parent logs and nested recovery fragments

Enhance log-sweeping routine to delete both parent log files (DR_LOG_%03d.BIN) and any sequential child recovery fragments (%03d%03d.BIN). Iterates parent IDs 1â€“999 and for each parent loops over child fragments incrementing j until no more fragments exist, removing each found file. This prevents orphaned recovery chunks from accumulating on the SD card after crashes or rollovers and then resets the current log filename and global log/recovery IDs.
```
</details>

<details>
<summary><b>Commit 37:</b> <code>7c4c10b</code> — Use global frame counter and reset on log events</summary>

```
Hash:   7c4c10b
Date:   2026-06-13 17:48:29 +0330
Author: Alireza Sotoodeh

Use global frame counter and reset on log events

Introduce a volatile global_frame_counter and remove the local shadowed variable in sensorTask to ensure a single frame counter shared across tasks. Reset global_frame_counter when starting a new log, after recovery/rollover, and during setup so frame numbering is consistent across sessions. Also includes minor whitespace/comment cleanups.
```
</details>

<details>
<summary><b>Commit 38:</b> <code>c2042e5</code> — Add safe long-press shutdown and SD flush</summary>

```
Hash:   c2042e5
Date:   2026-06-13 17:33:15 +0330
Author: Alireza Sotoodeh

Add safe long-press shutdown and SD flush

Introduce a safe shutdown sequence triggered by a long press of the SELECT button. Added Press_to_ShutDown_MS (3000ms) and a volatile system_shutdown_requested flag; sensorTask will pause data generation when shutdown is requested. loggingTask now implements a smart long-press detector: holding SELECT for the threshold wakes the display, shows status, drains remaining frames from the PSRAM/data queue to the open log file, syncs and closes the file, updates LEDs/UI, and then halts the system loop to ensure it is safe to power off. Also adjusted short-press behavior to only trigger on quick releases to avoid accidental immediate actions.
```
</details>

<details>
<summary><b>Commit 39:</b> <code>aae3057</code> — Display elapsed time instead of frame seq</summary>

```
Hash:   aae3057
Date:   2026-06-13 17:23:08 +0330
Author: Alireza Sotoodeh

Display elapsed time instead of frame seq

Replace the raw frame sequence number with a human-readable elapsed time computed from frame_seq assuming a 100 Hz frame rate. The code now divides frame_seq by 100 to get total seconds, derives minutes and seconds, and formats them as T:MMM:SS (zero-padded) at the same display position. The MPU temperature display is left unchanged.
```
</details>

<details>
<summary><b>Commit 40:</b> <code>276a5f4</code> — Improve SD runtime recovery and UI feedback</summary>

```
Hash:   276a5f4
Date:   2026-06-13 17:12:42 +0330
Author: Alireza Sotoodeh

Improve SD runtime recovery and UI feedback

Add a small SD_recoverd_signal_MS macro and rework the runtime SD-card recovery path to be non-blocking and more user-friendly. Introduces a hardware alarm layer that pulses the red LED and buzzer (100ms on every 1000ms) while in SD failure state, and keeps that rhythm independent of slower recovery attempts. Recovery attempts are gated by Attempt_Runtime_SD_recovery_MS to avoid repeated sd.begin() timeouts; when recovery runs it wakes the display, shows the active filename, resets LEDs to green briefly, and uses SD_recoverd_signal_MS for the brief user-visible pause. Loop yields reduced to 10ms to prevent watchdog starvation while preserving the recovery trap.
```
</details>

<details>
<summary><b>Commit 41:</b> <code>a2c7a71</code> — Move BinReader tests, add V5, tighten jump filter</summary>

```
Hash:   a2c7a71
Date:   2026-06-13 16:55:30 +0330
Author: Alireza Sotoodeh

Move BinReader tests, add V5, tighten jump filter

Reorganize BinReader test assets into Code_deadreckoner/Matlab/3-BinReader/Tests (many test files and images renamed/moved) and add a new set of recovery test artifacts. Reduce the garbage-filter jump threshold in v4.m (jumps > 50000 -> jumps > 500) to detect smaller sequence gaps. Add a new V5.m: a rewritten BinReader pipeline that chains DR_LOG and recovery files, decodes packed frames, computes diagnostics (drops, RMS, stats), plots session summaries, and includes robustness fixes (sessionData naming, drop detection, clamp helper).
```
</details>

<details>
<summary><b>Commit 42:</b> <code>49fc4a0</code> — Add global log/recovery IDs and improved recovery naming</summary>

```
Hash:   49fc4a0
Date:   2026-06-13 16:54:36 +0330
Author: Alireza Sotoodeh

Add global log/recovery IDs and improved recovery naming

Introduce global_log_id and global_recovery_id to track main log number (X) and recovery instance (Y). Use these to generate recovery filenames ("%03d%03d.BIN") during SD recovery for O(1) name generation, display the active recovery filename to the user, and increment/reset counters appropriately across startup, new-file creation, clearing, and recovery flows.

Other changes: update SD root scan to use global_log_id, count both main and recovery-style files when estimating total files, reset/clear queues and UI flags at appropriate points, add a brief UI pause to show recovery feedback, and minor cleanup of comments and LED/buzzer control sequencing. These changes improve robustness and make recovered sessions easier to identify and resume safely.
```
</details>

<details>
<summary><b>Commit 43:</b> <code>a9abe2b</code> — Update To Do list.md</summary>

```
Hash:   a9abe2b
Date:   2026-06-13 16:53:58 +0330
Author: Alireza Sotoodeh

Update To Do list.md
```
</details>

<details>
<summary><b>Commit 44:</b> <code>12969b5</code> — Mark Queue Overflow and 100Hz write fix done</summary>

```
Hash:   12969b5
Date:   2026-06-12 21:40:34 +0330
Author: Alireza Sotoodeh

Mark Queue Overflow and 100Hz write fix done

Update Report/To Do list.md to mark the 'Queue Overflow' task and the 'remove 100HZ writing and fix it for 1HZ (memory usage issue)' task as completed. Minor formatting/spacing adjusted in the checklist; no functional code changes.
```
</details>

<details>
<summary><b>Commit 45:</b> <code>72ea622</code> — Report: update To Do list tasks and notes</summary>

```
Hash:   72ea622
Date:   2026-06-12 20:45:04 +0330
Author: Alireza Sotoodeh

Report: update To Do list tasks and notes

Mark several checklist items as done and add MPU9250 recovery details. SD card runtime failure and GPIO9/10 pin change were checked off. MPU9250 section was expanded and partially completed: keep data by using Frame Sequence + Gap Frame, switch to PSRAM and a compact save format, and add MATLAB code to reassemble fragments. Added an OLED power-reset issue entry, cleaned up formatting and minor checklist reordering.
```
</details>

<details>
<summary><b>Commit 46:</b> <code>2c07c5a</code> — Optimize LogFrame, add PSRAM queue & SD recovery</summary>

```
Hash:   2c07c5a
Date:   2026-06-12 20:44:48 +0330
Author: Alireza Sotoodeh

Optimize LogFrame, add PSRAM queue & SD recovery

Reduce LogFrame size by introducing a packed union payload to overlap IMU and GPS fields (forced 1-byte alignment), and update all producers/consumers to use the union layout. Add esp_heap_caps.h and implement a large PSRAM-backed static FreeRTOS queue (QUEUE_LENGTH = 50000) via heap_caps_malloc + xQueueCreateStatic, with fatal error handling if PSRAM allocation fails. Revise SD recovery to create REC_ prefixed recovery files and update current_log_filename, and adjust gap-frame clearing to zero the union payload correctly. Also update OLED display formatting for the new payload layout and minor housekeeping (last-edit comment).
```
</details>

<details>
<summary><b>Commit 47:</b> <code>f8cf9f5</code> — Move BinReader data to Tests and add v4</summary>

```
Hash:   f8cf9f5
Date:   2026-06-12 20:44:24 +0330
Author: Alireza Sotoodeh

Move BinReader data to Tests and add v4

Reorganize test artifacts by moving existing sample logs and MATLAB test scripts from data/... into Tests/... directories. Add a new test set (Tests/4-test recovery with PSRAM) containing additional .BIN and .png files, and introduce Code_deadreckoner/Matlab/BinReader/v4.m.

v4.m implements an improved merge-and-decoding flow: replaces strict sequence matching with an Absolute Distance Minimizer for smart stitching, adds a garbage-frame filter to trim corrupted SD-card tails, supports overlap allowances for PSRAM cases, and produces payload decoding, diagnostics and plots (acceleration, orientation, frame continuity) with drop/gap detection.
```
</details>

<details>
<summary><b>Commit 48:</b> <code>fba5d59</code> — Add PSRAM recovery test data and v3 MATLAB reader</summary>

```
Hash:   fba5d59
Date:   2026-06-12 20:40:06 +0330
Author: Alireza Sotoodeh

Add PSRAM recovery test data and v3 MATLAB reader

Add test dataset and a new MATLAB reader for PSRAM recovery testing. Files added under data/3-test recovery with PSRAM: three PNGs, DR_LOG_001.BIN, REC_001.BIN, REC_002.BIN, ResutCommandWindow.txt (sample run output), and v3.m. The v3.m script implements a Relational Sequence Chaining (Greedy Stitcher) to automatically merge crashed SD sessions without an RTC, extracts frame metadata, stitches fragments by sequence numbers (with safety gap checks), decodes IMU/orientation payloads, computes session stats (merged frames, recovery gaps, missing frames) and generates diagnostic plots.
```
</details>

<details>
<summary><b>Commit 49:</b> <code>d664e11</code> — Reorganize MATLAB files and add BIN parser</summary>

```
Hash:   d664e11
Date:   2026-06-12 19:39:13 +0330
Author: Alireza Sotoodeh

Reorganize MATLAB files and add BIN parser

Restructure project layout and add tools for log analysis.

- Move visualization scripts into Code_deadreckoner/Matlab/1-Visual (Visual.m, visual_v2.m) and add DR_LOG_001.BIN sample.
- Reorganize test result folders into Code_deadreckoner/Matlab/2-test result/ (Static_Drift, Dynamic Return-to-Zero, Vibration Rejection) to better group outputs.
- Relocate BinReader samples into Code_deadreckoner/Matlab/BinReader/data/1-first log/ and add a new recovery dataset under data/2-test the recovery (DR_LOG_001.BIN and 1-test the recovery.png).
- Add BinReader/data/2-test the recovery/v2.m: a new MATLAB parser/diagnostics script that decodes the 49-byte packed ESP32 log frames (sequence, quaternions, accel, GPS, event flags), detects SD gaps, user tags, dropped frames, and produces diagnostic plots.

This change cleans up folder organization and introduces a dedicated binary-log parser for recovery and integrity analysis.
```
</details>

<details>
<summary><b>Commit 50:</b> <code>f401a2b</code> — SD runtime recovery and pin remap</summary>

```
Hash:   f401a2b
Date:   2026-06-12 18:38:40 +0330
Author: Alireza Sotoodeh

SD runtime recovery and pin remap

Implement dynamic SD-card runtime recovery and remap a few unsafe pins. Adds attemptSDRecovery() and an sd_critical_error interceptor that detects write/sync failures, retries SPI re-init, reopens the same filename for append, and injects a gap recovery frame into the log when recovery succeeds. Introduces current_log_filename, recovery timing macros (Attempt_Runtime_SD_recovery_MS, Recheck_SD_beforeBoot_MS), and UI/LED/buzzer feedback during SD error retries. Also reorganizes pin/setting blocks, moves SD CS from GPIO10 to GPIO15, moves TAG button from GPIO9 to GPIO14, refactors LogFrame to use frame_seq, adds a global frame counter and related MPU retry timing macro, and tweaks logging/open logic to preserve and reuse the active log filename. Misc: updated header comment and small timing/RTOS adjustments for more responsive recovery handling.
```
</details>

<details>
<summary><b>Commit 51:</b> <code>5e9c419</code> — Update To Do list with finding the final issues</summary>

```
Hash:   5e9c419
Date:   2026-06-12 15:21:13 +0330
Author: Alireza Sotoodeh

Update To Do list with finding the final issues

Expand the project To Do list with runtime recovery items and hardware considerations. Added SD card "Runtime Failure" and SD Dynamic Recovery, MPU9250 Dynamic Recovery and an mpu_critical_error item for calibration; clarified safe shutdown and software solution. Added Queue Overflow handling suggestion (use frame_sequence_number instead of timestamp in LogFrame), GPS logging rate fix (100Hz -> 1Hz), watchdog consideration, BMP280 for Z-axis deficits, and change recommendation for GPIO9/10 (flash/strapping pins). New "Hardware challenges" section covers brownout mitigation (capacitance recommendations), I2C pull-up resistor (4.7k for MPU9250), and key debouncing (0.1ÂµF across key).
```
</details>

<details>
<summary><b>Commit 52:</b> <code>175ddba</code> — Update README.md</summary>

```
Hash:   175ddba
Date:   2026-06-12 14:44:16 +0330
Author: Alireza Sotoodeh

Update README.md
```
</details>

<details>
<summary><b>Commit 53:</b> <code>53ee269</code> — Update To Do list.md</summary>

```
Hash:   53ee269
Date:   2026-06-12 14:44:06 +0330
Author: Alireza Sotoodeh

Update To Do list.md
```
</details>

<details>
<summary><b>Commit 54:</b> <code>ae8b760</code> — Add comprehensive project reports and TODO</summary>

```
Hash:   ae8b760
Date:   2026-06-12 14:30:50 +0330
Author: Alireza Sotoodeh

Add comprehensive project reports and TODO

Add detailed documentation for the DeadReckoner project: new Progress.md (development phases and status), Report2.md (v2.0 full Progress & Architecture Report with hardware inventory, architecture, tests, binary logging format, and timeline), and To Do list.md (action items). Also rename/clean up existing report file to Report/Report.md and update firmware flashing configuration section. These changes consolidate design history, testing results, and next steps for development and GNSS integration.
```
</details>

<details>
<summary><b>Commit 55:</b> <code>109b15d</code> — DYNAMIC RECOVERY PROTOCOL for MPU reconnection</summary>

```
Hash:   109b15d
Date:   2026-06-11 22:18:56 +0330
Author: Alireza Sotoodeh

DYNAMIC RECOVERY PROTOCOL for MPU reconnection
```
</details>

<details>
<summary><b>Commit 56:</b> <code>fff0137</code> — Update Progress & Architecture Report.md</summary>

```
Hash:   fff0137
Date:   2026-06-11 22:01:44 +0330
Author: Alireza Sotoodeh

Update Progress & Architecture Report.md
```
</details>

<details>
<summary><b>Commit 57:</b> <code>61546f3</code> — implement MPU9250 critical disconnect trap and boot-time SD scanning</summary>

```
Hash:   61546f3
Date:   2026-06-11 21:09:23 +0330
Author: Alireza Sotoodeh

implement MPU9250 critical disconnect trap and boot-time SD scanning

Implemented a robust critical error handling mechanism for the MPU9250 sensor. The system now monitors I2C communication; upon a disconnection, it safely syncs/closes the log file and enters an infinite SOS loop with visual/audible alerts. Added a boot-time scanning sequence for the SD card to display existing log counts and the next filename, providing better pre-mission feedback. Removed legacy calibration prompts to streamline the startup sequence.
```
</details>

<details>
<summary><b>Commit 58:</b> <code>1e77d9b</code> — fix (Total: XX.X GB) in submenu SD card</summary>

```
Hash:   1e77d9b
Date:   2026-06-11 20:34:19 +0330
Author: Alireza Sotoodeh

fix (Total: XX.X GB) in submenu SD card
```
</details>

<details>
<summary><b>Commit 59:</b> <code>cc09b6b</code> — update menu - SD card sub menu</summary>

```
Hash:   cc09b6b
Date:   2026-06-11 20:26:26 +0330
Author: Alireza Sotoodeh

update menu - SD card sub menu

Total: XX.X GB
Free: XXXX MB
Time: XX.X Hrs
Files Count: XX
Create New File
Format / Clear
Back to Menu
i should fix (Total: XX.X GB)
```
</details>

<details>
<summary><b>Commit 60:</b> <code>2441549</code> — update menu - mute buzzer menu</summary>

```
Hash:   2441549
Date:   2026-06-11 17:26:05 +0330
Author: Alireza Sotoodeh

update menu - mute buzzer menu
```
</details>

<details>
<summary><b>Commit 61:</b> <code>82630a8</code> — update menu - display mode</summary>

```
Hash:   82630a8
Date:   2026-06-11 17:16:46 +0330
Author: Alireza Sotoodeh

update menu - display mode
```
</details>

<details>
<summary><b>Commit 62:</b> <code>34b6077</code> — added buzzer and LED for Tag and SD card failure</summary>

```
Hash:   34b6077
Date:   2026-06-11 16:49:15 +0330
Author: Alireza Sotoodeh

added buzzer and LED for Tag and SD card failure
```
</details>

<details>
<summary><b>Commit 63:</b> <code>a19f32f</code> — try to update oled-phase 4</summary>

```
Hash:   a19f32f
Date:   2026-06-10 22:22:11 +0330
Author: Alireza Sotoodeh

try to update oled-phase 4

1- added return home (if in sleep mode)
2- sleep oled after some time to save power
```
</details>

<details>
<summary><b>Commit 64:</b> <code>74f1cf1</code> — try to update oled- phase 3</summary>

```
Hash:   74f1cf1
Date:   2026-06-10 18:05:30 +0330
Author: Alireza Sotoodeh

try to update oled- phase 3

added Sd card calculation sub menu
```
</details>

<details>
<summary><b>Commit 65:</b> <code>a6f7e2a</code> — try to update oled phase 2</summary>

```
Hash:   a6f7e2a
Date:   2026-06-10 16:25:03 +0330
Author: Alireza Sotoodeh

try to update oled phase 2
```
</details>

<details>
<summary><b>Commit 66:</b> <code>e0a9af6</code> — try to update oled- phase 1</summary>

```
Hash:   e0a9af6
Date:   2026-06-10 16:14:20 +0330
Author: Alireza Sotoodeh

try to update oled- phase 1
```
</details>

<details>
<summary><b>Commit 67:</b> <code>20b4be7</code> — Update Progress & Architecture Report.md</summary>

```
Hash:   20b4be7
Date:   2026-06-10 14:59:53 +0330
Author: Alireza Sotoodeh

Update Progress & Architecture Report.md
```
</details>

<details>
<summary><b>Commit 68:</b> <code>0b6d062</code> — Create Lipo_VS_LIIon.png</summary>

```
Hash:   0b6d062
Date:   2026-06-09 15:46:21 +0330
Author: Alireza Sotoodeh

Create Lipo_VS_LIIon.png
```
</details>

<details>
<summary><b>Commit 69:</b> <code>06d4af0</code> — Create Li-ion-battery-discharge-voltage-curve.png</summary>

```
Hash:   06d4af0
Date:   2026-06-09 15:46:19 +0330
Author: Alireza Sotoodeh

Create Li-ion-battery-discharge-voltage-curve.png
```
</details>

<details>
<summary><b>Commit 70:</b> <code>3a02459</code> — first version of a bin file reader (MATLAB code)</summary>

```
Hash:   3a02459
Date:   2026-06-09 13:36:22 +0330
Author: Alireza Sotoodeh

first version of a bin file reader (MATLAB code)
```
</details>

<details>
<summary><b>Commit 71:</b> <code>62a3aa7</code> — update for SD card logger</summary>

```
Hash:   62a3aa7
Date:   2026-06-09 12:33:17 +0330
Author: Alireza Sotoodeh

update for SD card logger
```
</details>

<details>
<summary><b>Commit 72:</b> <code>2d598a3</code> — speed test Sd card</summary>

```
Hash:   2d598a3
Date:   2026-06-09 11:40:52 +0330
Author: Alireza Sotoodeh

speed test Sd card
```
</details>

<details>
<summary><b>Commit 73:</b> <code>63c2d1f</code> — added test analyzer for no SD card mode</summary>

```
Hash:   63c2d1f
Date:   2026-06-08 17:22:48 +0330
Author: Alireza Sotoodeh

added test analyzer for no SD card mode
```
</details>

<details>
<summary><b>Commit 74:</b> <code>b92c2d9</code> — Update Progress & Architecture Report.md</summary>

```
Hash:   b92c2d9
Date:   2026-06-08 16:57:49 +0330
Author: Alireza Sotoodeh

Update Progress & Architecture Report.md
```
</details>

<details>
<summary><b>Commit 75:</b> <code>ff103ec</code> — Final SD card Test (success!)</summary>

```
Hash:   ff103ec
Date:   2026-06-08 16:32:36 +0330
Author: Alireza Sotoodeh

Final SD card Test (success!)
```
</details>

<details>
<summary><b>Commit 76:</b> <code>f4be198</code> — SD card Test</summary>

```
Hash:   f4be198
Date:   2026-06-08 16:32:09 +0330
Author: Alireza Sotoodeh

SD card Test
```
</details>

<details>
<summary><b>Commit 77:</b> <code>56e2f99</code> — Create Chip_Identification_ESP.ino</summary>

```
Hash:   56e2f99
Date:   2026-06-08 13:04:57 +0330
Author: Alireza Sotoodeh

Create Chip_Identification_ESP.ino
```
</details>

<details>
<summary><b>Commit 78:</b> <code>571117f</code> — clean up and added some test for SD card</summary>

```
Hash:   571117f
Date:   2026-06-08 12:31:36 +0330
Author: Alireza Sotoodeh

clean up and added some test for SD card
```
</details>

<details>
<summary><b>Commit 79:</b> <code>c64fab2</code> — added SD card test (all methods has been failed)</summary>

```
Hash:   c64fab2
Date:   2026-06-08 11:04:07 +0330
Author: Alireza Sotoodeh

added SD card test (all methods has been failed)
```
</details>

<details>
<summary><b>Commit 80:</b> <code>71b1203</code> — added SD card Diagram</summary>

```
Hash:   71b1203
Date:   2026-06-05 21:10:39 +0330
Author: Alireza Sotoodeh

added SD card Diagram
```
</details>

<details>
<summary><b>Commit 81:</b> <code>38a5df4</code> — diagram SD card adaptors</summary>

```
Hash:   38a5df4
Date:   2026-06-05 20:15:23 +0330
Author: Alireza Sotoodeh

diagram SD card adaptors
```
</details>

<details>
<summary><b>Commit 82:</b> <code>d61bf42</code> — Standalone Sanity Check for SPI SD Card Module</summary>

```
Hash:   d61bf42
Date:   2026-06-05 20:14:00 +0330
Author: Alireza Sotoodeh

Standalone Sanity Check for SPI SD Card Module
```
</details>

<details>
<summary><b>Commit 83:</b> <code>95b4553</code> — Sanity Check for SPI SD Card Module</summary>

```
Hash:   95b4553
Date:   2026-06-05 03:57:14 +0330
Author: Alireza Sotoodeh

Sanity Check for SPI SD Card Module
```
</details>

<details>
<summary><b>Commit 84:</b> <code>4770619</code> — Hardware & Filter Validation (Test Results)</summary>

```
Hash:   4770619
Date:   2026-06-04 22:18:46 +0330
Author: Alireza Sotoodeh

Hardware & Filter Validation (Test Results)
```
</details>

<details>
<summary><b>Commit 85:</b> <code>49a098e</code> — Vibration Rejection test result</summary>

```
Hash:   49a098e
Date:   2026-06-04 22:18:32 +0330
Author: Alireza Sotoodeh

Vibration Rejection test result
```
</details>

<details>
<summary><b>Commit 86:</b> <code>490ca10</code> — Clean up and add a second test (Dynamic Return-to-Zero)</summary>

```
Hash:   490ca10
Date:   2026-06-04 21:09:55 +0330
Author: Alireza Sotoodeh

Clean up and add a second test (Dynamic Return-to-Zero)
```
</details>

<details>
<summary><b>Commit 87:</b> <code>3ff7a07</code> — Drift Test Visualizer</summary>

```
Hash:   3ff7a07
Date:   2026-06-04 16:28:32 +0330
Author: Alireza Sotoodeh

Drift Test Visualizer
```
</details>

<details>
<summary><b>Commit 88:</b> <code>4fe7d7c</code> — Update Progress & Architecture Report.md</summary>

```
Hash:   4fe7d7c
Date:   2026-06-04 16:28:10 +0330
Author: Alireza Sotoodeh

Update Progress & Architecture Report.md
```
</details>

<details>
<summary><b>Commit 89:</b> <code>6db47c8</code> — fix OLED</summary>

```
Hash:   6db47c8
Date:   2026-06-04 15:41:16 +0330
Author: Alireza Sotoodeh

fix OLED
```
</details>

<details>
<summary><b>Commit 90:</b> <code>e5a2f29</code> — ix issues before test</summary>

```
Hash:   e5a2f29
Date:   2026-06-04 15:33:59 +0330
Author: Alireza Sotoodeh

ix issues before test

Fixed EEPROM load bug (added setters) and migrated OLED to Hardware I2C (Wire1).
```
</details>

<details>
<summary><b>Commit 91:</b> <code>5fe3bf7</code> — Firmware Flashing Configuration (ESP32-S3 N16R8)</summary>

```
Hash:   5fe3bf7
Date:   2026-06-04 15:31:49 +0330
Author: Alireza Sotoodeh

Firmware Flashing Configuration (ESP32-S3 N16R8)
```
</details>

<details>
<summary><b>Commit 92:</b> <code>6b28855</code> — Update Progress & Architecture Report.md</summary>

```
Hash:   6b28855
Date:   2026-06-04 15:03:10 +0330
Author: Alireza Sotoodeh

Update Progress & Architecture Report.md
```
</details>

<details>
<summary><b>Commit 93:</b> <code>66e7771</code> — Sanity Check for OLED 0.91</summary>

```
Hash:   66e7771
Date:   2026-06-04 14:59:02 +0330
Author: Alireza Sotoodeh

Sanity Check for OLED 0.91
```
</details>

<details>
<summary><b>Commit 94:</b> <code>8a65805</code> — Basic I2C sanity check MPU9250</summary>

```
Hash:   8a65805
Date:   2026-06-04 14:46:46 +0330
Author: Alireza Sotoodeh

Basic I2C sanity check MPU9250
```
</details>

<details>
<summary><b>Commit 95:</b> <code>5b4c454</code> — Updated wiring diagram</summary>

```
Hash:   5b4c454
Date:   2026-06-04 13:05:46 +0330
Author: Alireza Sotoodeh

Updated wiring diagram
```
</details>

<details>
<summary><b>Commit 96:</b> <code>b099dad</code> — Update Progress & Architecture Report.md</summary>

```
Hash:   b099dad
Date:   2026-06-04 13:05:08 +0330
Author: Alireza Sotoodeh

Update Progress & Architecture Report.md
```
</details>

<details>
<summary><b>Commit 97:</b> <code>08459b3</code> — dual core with Free Atreus</summary>

```
Hash:   08459b3
Date:   2026-06-03 19:32:06 +0330
Author: Alireza Sotoodeh

dual core with Free Atreus
```
</details>

<details>
<summary><b>Commit 98:</b> <code>0ee83a3</code> — Update Progress & Architecture Report.md</summary>

```
Hash:   0ee83a3
Date:   2026-06-03 19:09:52 +0330
Author: Alireza Sotoodeh

Update Progress & Architecture Report.md
```
</details>

<details>
<summary><b>Commit 99:</b> <code>ef89423</code> — Create ESP32_S3.ino</summary>

```
Hash:   ef89423
Date:   2026-06-03 18:43:14 +0330
Author: Alireza Sotoodeh

Create ESP32_S3.ino
```
</details>

<details>
<summary><b>Commit 100:</b> <code>d77ccbc</code> — Create Progress & Architecture Report.md</summary>

```
Hash:   d77ccbc
Date:   2026-06-03 18:42:35 +0330
Author: Alireza Sotoodeh

Create Progress & Architecture Report.md
```
</details>

<details>
<summary><b>Commit 101:</b> <code>6ed1539</code> — added wiring diagram for ESP</summary>

```
Hash:   6ed1539
Date:   2026-06-03 17:00:09 +0330
Author: Alireza Sotoodeh

added wiring diagram for ESP
```
</details>

<details>
<summary><b>Commit 102:</b> <code>92d4cb3</code> — init_project(again)</summary>

```
Hash:   92d4cb3
Date:   2026-06-03 16:34:13 +0330
Author: Alireza Sotoodeh

init_project(again)
```
</details>

<details>
<summary><b>Commit 103:</b> <code>d7407d4</code> — Delete setup modules directory</summary>

```
Hash:   d7407d4
Date:   2026-06-03 15:53:09 +0330
Author: Alireza Sotoodeh

Delete setup modules directory
```
</details>

<details>
<summary><b>Commit 104:</b> <code>863ff07</code> — Delete main code directory</summary>

```
Hash:   863ff07
Date:   2026-06-03 15:52:57 +0330
Author: Alireza Sotoodeh

Delete main code directory
```
</details>

<details>
<summary><b>Commit 105:</b> <code>196cec4</code> — Delete .metadata directory</summary>

```
Hash:   196cec4
Date:   2026-06-03 15:52:38 +0330
Author: Alireza Sotoodeh

Delete .metadata directory
```
</details>

<details>
<summary><b>Commit 106:</b> <code>3b96d1d</code> — Merge pull request #8 from Alireza-Sotoodeh/NodeMUC8266</summary>

```
Hash:   3b96d1d
Date:   2025-08-09 11:17:13 +0330
Author: Alireza Sotoodeh

Merge pull request #8 from Alireza-Sotoodeh/NodeMUC8266

Node muc8266
```
</details>

<details>
<summary><b>Commit 107:</b> <code>3359107</code> — cleaning repo-05/18-11:16PM</summary>

```
Hash:   3359107
Date:   2025-08-09 11:16:43 +0330
Author: Alireza Sotoodeh

cleaning repo-05/18-11:16PM
```
</details>

<details>
<summary><b>Commit 108:</b> <code>2bc2774</code> — fixed the acceleration (linear acceleration)-05/17-4:57</summary>

```
Hash:   2bc2774
Date:   2025-08-08 16:57:22 +0330
Author: Alireza Sotoodeh

fixed the acceleration (linear acceleration)-05/17-4:57
```
</details>

<details>
<summary><b>Commit 109:</b> <code>b0166f0</code> — oled display fixed-05/17-11:19PM</summary>

```
Hash:   b0166f0
Date:   2025-08-08 11:19:45 +0330
Author: Alireza Sotoodeh

oled display fixed-05/17-11:19PM
```
</details>

<details>
<summary><b>Commit 110:</b> <code>ff11272</code> — 05/16-12:26PM</summary>

```
Hash:   ff11272
Date:   2025-08-07 12:26:53 +0330
Author: Alireza Sotoodeh

05/16-12:26PM

fixed the MATLAB code (closing the serial on exit)
```
</details>

<details>
<summary><b>Commit 111:</b> <code>04cdf7e</code> — 05/15-2:42PM</summary>

```
Hash:   04cdf7e
Date:   2025-08-06 14:43:03 +0330
Author: Alireza Sotoodeh

05/15-2:42PM

display and acceleration added
```
</details>

<details>
<summary><b>Commit 112:</b> <code>9cbcf16</code> — 05/15-2:17PM</summary>

```
Hash:   9cbcf16
Date:   2025-08-06 14:17:22 +0330
Author: Alireza Sotoodeh

05/15-2:17PM

setup basic oled
```
</details>

<details>
<summary><b>Commit 113:</b> <code>d69f889</code> — 05/14-8:33Pm</summary>

```
Hash:   d69f889
Date:   2025-08-05 20:33:51 +0330
Author: Alireza Sotoodeh

05/14-8:33Pm

now it can read from EEprom and save the callibration to it or load it
```
</details>

<details>
<summary><b>Commit 114:</b> <code>bd57081</code> — 05/14-7:59PM</summary>

```
Hash:   bd57081
Date:   2025-08-05 19:59:43 +0330
Author: Alireza Sotoodeh

05/14-7:59PM

fixd matlab code
```
</details>

<details>
<summary><b>Commit 115:</b> <code>a18b783</code> — Merge pull request #7 from Alireza-Sotoodeh/NodeMUC8266</summary>

```
Hash:   a18b783
Date:   2025-08-05 19:45:52 +0330
Author: Alireza Sotoodeh

Merge pull request #7 from Alireza-Sotoodeh/NodeMUC8266

Node muc8266
```
</details>

<details>
<summary><b>Commit 116:</b> <code>f623caf</code> — 05/14-7:43PM</summary>

```
Hash:   f623caf
Date:   2025-08-05 19:44:23 +0330
Author: Alireza Sotoodeh

05/14-7:43PM

fixed MATLAB code (opening serial problem)
```
</details>

<details>
<summary><b>Commit 117:</b> <code>dac7043</code> — 05/14_7:33PM</summary>

```
Hash:   dac7043
Date:   2025-08-05 19:34:07 +0330
Author: Alireza Sotoodeh

05/14_7:33PM

fixed MPU9250 comments and settings
```
</details>

<details>
<summary><b>Commit 118:</b> <code>3e039ce</code> — 05/14-5:19PM</summary>

```
Hash:   3e039ce
Date:   2025-08-05 17:19:11 +0330
Author: Alireza Sotoodeh

05/14-5:19PM

added comments for understating
```
</details>

<details>
<summary><b>Commit 119:</b> <code>8a9c9e9</code> — 05/14</summary>

```
Hash:   8a9c9e9
Date:   2025-08-05 15:57:33 +0330
Author: Alireza Sotoodeh

05/14

start
```
</details>

<details>
<summary><b>Commit 120:</b> <code>1b1623d</code> — 05/13-10:20Pm</summary>

```
Hash:   1b1623d
Date:   2025-08-04 10:20:12 +0330
Author: Alireza Sotoodeh

05/13-10:20Pm

codes working
```
</details>

<details>
<summary><b>Commit 121:</b> <code>5e9d19f</code> — start</summary>

```
Hash:   5e9d19f
Date:   2025-08-04 09:02:54 +0330
Author: Alireza Sotoodeh

start
```
</details>

<details>
<summary><b>Commit 122:</b> <code>31c9ae8</code> — save before start</summary>

```
Hash:   31c9ae8
Date:   2025-08-04 09:01:25 +0330
Author: Alireza Sotoodeh

save before start
```
</details>

<details>
<summary><b>Commit 123:</b> <code>9dc8987</code> — removed the project</summary>

```
Hash:   9dc8987
Date:   2025-08-03 15:42:08 +0330
Author: Alireza Sotoodeh

removed the project
```
</details>

<details>
<summary><b>Commit 124:</b> <code>bba6109</code> — 05/12-3:34PM</summary>

```
Hash:   bba6109
Date:   2025-08-03 15:35:11 +0330
Author: Alireza Sotoodeh

05/12-3:34PM

test the MPU9250 and filter and 3d visualization by Arduino and MATLAB
```
</details>

<details>
<summary><b>Commit 125:</b> <code>d44cee1</code> — 05/11-2:04PM</summary>

```
Hash:   d44cee1
Date:   2025-08-02 14:05:01 +0330
Author: Alireza Sotoodeh

05/11-2:04PM

adding the MPU9250 lib
```
</details>

<details>
<summary><b>Commit 126:</b> <code>b578859</code> — 05/11-2:02PM</summary>

```
Hash:   b578859
Date:   2025-08-02 14:02:40 +0330
Author: Alireza Sotoodeh

05/11-2:02PM

remove unnecessary files
```
</details>

<details>
<summary><b>Commit 127:</b> <code>da3fe11</code> — Update workbench.xmi</summary>

```
Hash:   da3fe11
Date:   2025-07-25 16:48:20 +0330
Author: Alireza Sotoodeh

Update workbench.xmi
```
</details>

<details>
<summary><b>Commit 128:</b> <code>0e84527</code> — Add DMP.c for MPU_9250 and update workspace state</summary>

```
Hash:   0e84527
Date:   2025-07-25 16:34:39 +0330
Author: Alireza Sotoodeh

Add DMP.c for MPU_9250 and update workspace state

Added the new DMP.c module for the MPU_9250 sensor in setup modules/DataSheets/MPU_9250/. Updated Eclipse workspace state to reflect recent file activity and editor state.
```
</details>

<details>
<summary><b>Commit 129:</b> <code>3cb9f77</code> — Revert "05/03-3:53PM"</summary>

```
Hash:   3cb9f77
Date:   2025-07-25 16:31:47 +0330
Author: Alireza Sotoodeh

Revert "05/03-3:53PM"

This reverts commit 35e3c884d70004a60c4bb66b258b43bdf60dd482.
```
</details>

<details>
<summary><b>Commit 130:</b> <code>35e3c88</code> — 05/03-3:53PM</summary>

```
Hash:   35e3c88
Date:   2025-07-25 15:53:44 +0330
Author: Alireza Sotoodeh

05/03-3:53PM

found the DMP
```
</details>

<details>
<summary><b>Commit 131:</b> <code>527e4d0</code> — Merge branch 'MPU9250' of https://github.com/Alireza-Sotoodeh/Navigation-STM32 into MPU9250</summary>

```
Hash:   527e4d0
Date:   2025-07-25 15:04:37 +0330
Author: Alireza Sotoodeh

Merge branch 'MPU9250' of https://github.com/Alireza-Sotoodeh/Navigation-STM32 into MPU9250
```
</details>

<details>
<summary><b>Commit 132:</b> <code>8df6c0a</code> — Update MPU_9250.md</summary>

```
Hash:   8df6c0a
Date:   2025-07-25 15:04:30 +0330
Author: Alireza Sotoodeh

Update MPU_9250.md
```
</details>

<details>
<summary><b>Commit 133:</b> <code>fccbc95</code> — 05/03-3:02PM</summary>

```
Hash:   fccbc95
Date:   2025-07-25 15:02:46 +0330
Author: Alireza Sotoodeh

05/03-3:02PM

want to start working on DMP farmwear
```
</details>

<details>
<summary><b>Commit 134:</b> <code>e1c129e</code> — 05/03-12:17PM</summary>

```
Hash:   e1c129e
Date:   2025-07-25 12:17:52 +0330
Author: Alireza Sotoodeh

05/03-12:17PM

Update MPU_9250.md
```
</details>

<details>
<summary><b>Commit 135:</b> <code>ac92d29</code> — 05/03-12:17PM</summary>

```
Hash:   ac92d29
Date:   2025-07-25 12:17:27 +0330
Author: Alireza Sotoodeh

05/03-12:17PM

Update MPU_9250.md
```
</details>

<details>
<summary><b>Commit 136:</b> <code>cfa9612</code> — 05/03-12:12PM</summary>

```
Hash:   cfa9612
Date:   2025-07-25 12:13:08 +0330
Author: Alireza Sotoodeh

05/03-12:12PM

fixed MPU9250.md
```
</details>

<details>
<summary><b>Commit 137:</b> <code>d7b0656</code> — 05/03-12:10PM</summary>

```
Hash:   d7b0656
Date:   2025-07-25 12:11:14 +0330
Author: Alireza Sotoodeh

05/03-12:10PM

adding datasheet MPU9250 and MPU9250.md
```
</details>

<details>
<summary><b>Commit 138:</b> <code>27d3701</code> — 05/03-11:22AM (save before start)</summary>

```
Hash:   27d3701
Date:   2025-07-25 11:23:10 +0330
Author: Alireza Sotoodeh

05/03-11:22AM (save before start)
```
</details>

<details>
<summary><b>Commit 139:</b> <code>b869740</code> — 05/02-7:38PM</summary>

```
Hash:   b869740
Date:   2025-07-24 19:39:06 +0330
Author: Alireza Sotoodeh

05/02-7:38PM

ready for test need to modify main.c
```
</details>

<details>
<summary><b>Commit 140:</b> <code>0a7b903</code> — 05/02-7:30PM</summary>

```
Hash:   0a7b903
Date:   2025-07-24 19:31:16 +0330
Author: Alireza Sotoodeh

05/02-7:30PM

start working but need saving first
```
</details>

<details>
<summary><b>Commit 141:</b> <code>04bcf97</code> — 05/01-8:53PM</summary>

```
Hash:   04bcf97
Date:   2025-07-23 20:54:00 +0330
Author: Alireza Sotoodeh

05/01-8:53PM

updated version
found some GitHub library too
should improve it
need testing
```
</details>

<details>
<summary><b>Commit 142:</b> <code>3f6cd2f</code> — 05/01-8:49</summary>

```
Hash:   3f6cd2f
Date:   2025-07-23 20:50:15 +0330
Author: Alireza Sotoodeh

05/01-8:49

first step
needs improvments
```
</details>

<details>
<summary><b>Commit 143:</b> <code>5b426fa</code> — Update README.md</summary>

```
Hash:   5b426fa
Date:   2025-07-23 11:05:54 +0330
Author: Alireza Sotoodeh

Update README.md
```
</details>

<details>
<summary><b>Commit 144:</b> <code>0455e92</code> — 04/29-9:44PM</summary>

```
Hash:   0455e92
Date:   2025-07-20 21:44:39 +0330
Author: Alireza Sotoodeh

04/29-9:44PM

fixing ReadME
```
</details>

<details>
<summary><b>Commit 145:</b> <code>3b1b5ac</code> — 04/27- 8:15 PM</summary>

```
Hash:   3b1b5ac
Date:   2025-07-18 20:16:29 +0330
Author: Alireza Sotoodeh

04/27- 8:15 PM

stopped working on MPu6500 because i need magnometer which can be found on MPU9250
just Updated README.md
```
</details>

<details>
<summary><b>Commit 146:</b> <code>4305256</code> — 04/27-6:03 PM</summary>

```
Hash:   4305256
Date:   2025-07-18 18:04:53 +0330
Author: Alireza Sotoodeh

04/27-6:03 PM

- fixed readme file
- Optimize full-scale accelerometer/gyro ranges (set to Â±8g and Â±1000dps for dynamic environments)
- Increase sample rate to 200 Hz (and editing the driver_MPU6500_dmp.c)
```
</details>

<details>
<summary><b>Commit 147:</b> <code>701ae4b</code> — 04/27-10:48 AM</summary>

```
Hash:   701ae4b
Date:   2025-07-18 10:48:51 +0330
Author: Alireza Sotoodeh

04/27-10:48 AM

Refactor and enhance MPU6500 inertial navigation code

- Fixed duplicate `mpu6500_receive_callback` definition in `main.c` to resolve compilation error.
- Removed redundant `mpu6500_dmp_init` call in `main.c` for robust initialization with retry logic.
- Improved error handling in `main.c` and `driver_mpu6500_dmp.c` with detailed debug messages and context.
- Added FIFO overflow handling and sensor data validation in `mpu6500_receive_callback`.
- Implemented EXTI interrupt handler for MPU6500 data ready interrupts.
- Set MPU6500 low-pass filter to `MPU6500_LOW_PASS_FILTER_2` in `driver_mpu6500_dmp.c` for better dynamic response.
- Removed redundant FIFO configuration in `driver_mpu6500_dmp.c`.
- Added offset application in `driver_mpu6500_dmp.c` to ensure accurate sensor data.
- Streamlined `main.c` for clarity and compatibility with updated driver.
- Recommended Madgwick filter for orientation estimation to reduce drift, with implementation guidance.

Fixes compilation errors, improves navigation accuracy, and prepares for future module integration (SD card, WiFi, Bluetooth).
```
</details>

<details>
<summary><b>Commit 148:</b> <code>62df236</code> — 04/26</summary>

```
Hash:   62df236
Date:   2025-07-17 22:30:28 +0330
Author: Alireza Sotoodeh

04/26
```
</details>

<details>
<summary><b>Commit 149:</b> <code>768b21c</code> — now code has no error</summary>

```
Hash:   768b21c
Date:   2025-07-16 08:01:08 +0330
Author: Alireza Sotoodeh

now code has no error
```
</details>

<details>
<summary><b>Commit 150:</b> <code>ec842c7</code> — 04/23 saving before going for a break!</summary>

```
Hash:   ec842c7
Date:   2025-07-14 11:33:26 +0330
Author: Alireza Sotoodeh

04/23 saving before going for a break!
```
</details>

<details>
<summary><b>Commit 151:</b> <code>2327c58</code> — before testing the MPU6500 commands</summary>

```
Hash:   2327c58
Date:   2025-07-14 10:47:22 +0330
Author: Alireza Sotoodeh

before testing the MPU6500 commands

all things has been set
file scr and interface are ready to go
```
</details>

<details>
<summary><b>Commit 152:</b> <code>e532bc4</code> — added all necceasry files</summary>

```
Hash:   e532bc4
Date:   2025-07-13 20:11:22 +0330
Author: Alireza Sotoodeh

added all necceasry files
```
</details>

<details>
<summary><b>Commit 153:</b> <code>8ac5a28</code> — save beffore going to my new brand branch</summary>

```
Hash:   8ac5a28
Date:   2025-07-13 15:01:44 +0330
Author: Alireza Sotoodeh

save beffore going to my new brand branch
```
</details>

<details>
<summary><b>Commit 154:</b> <code>9b63d4f</code> — adding scr MPU6500</summary>

```
Hash:   9b63d4f
Date:   2025-07-13 15:00:15 +0330
Author: Alireza Sotoodeh

adding scr MPU6500

driver and interface files has been added
```
</details>

<details>
<summary><b>Commit 155:</b> <code>d95b49d</code> — setup CubeIDE</summary>

```
Hash:   d95b49d
Date:   2025-07-13 14:56:47 +0330
Author: Alireza Sotoodeh

setup CubeIDE

now i have everything to get stated and working on MPU6500
```
</details>

<details>
<summary><b>Commit 156:</b> <code>4239272</code> — added MPU6500 library</summary>

```
Hash:   4239272
Date:   2025-07-13 14:52:23 +0330
Author: Alireza Sotoodeh

added MPU6500 library
```
</details>

<details>
<summary><b>Commit 157:</b> <code>b31009d</code> — removing all file(from the top)</summary>

```
Hash:   b31009d
Date:   2025-07-13 14:50:43 +0330
Author: Alireza Sotoodeh

removing all file(from the top)
```
</details>

<details>
<summary><b>Commit 158:</b> <code>2f0ded6</code> — fixing readme file</summary>

```
Hash:   2f0ded6
Date:   2025-07-13 14:26:16 +0330
Author: Alireza Sotoodeh

fixing readme file
```
</details>

<details>
<summary><b>Commit 159:</b> <code>adfc075</code> — initial MPU6500 and trying to test it</summary>

```
Hash:   adfc075
Date:   2025-07-13 14:24:16 +0330
Author: Alireza Sotoodeh

initial MPU6500 and trying to test it

trying to work with the library i found online
```
</details>

<details>
<summary><b>Commit 160:</b> <code>05d017f</code> — initial PJ</summary>

```
Hash:   05d017f
Date:   2025-07-12 20:39:48 +0330
Author: Alireza Sotoodeh

initial PJ

1404/04/21
```
</details>

<details>
<summary><b>Commit 161:</b> <code>bed8a35</code> — modified readme file</summary>

```
Hash:   bed8a35
Date:   2025-07-12 19:01:00 +0330
Author: Alireza Sotoodeh

modified readme file

trying to describe the path
```
</details>

<details>
<summary><b>Commit 162:</b> <code>d72b9ad</code> — Initial commit</summary>

```
Hash:   d72b9ad
Date:   2025-07-12 18:41:59 +0330
Author: Alireza Sotoodeh

Initial commit
```
</details>

---

*Report generated by `project_summarizer.py` on 2026-06-15 17:22:14*