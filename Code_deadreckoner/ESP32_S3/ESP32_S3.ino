// Last Edit: 2026-06-20
// Reason for Last Edit: xQueueReset on new file, SAMPLING_RATE_HZ wired to task timing, LogFrame zero-init, PSRAM-SD conflict note, OLED hot-plug comments, device header
// Author: Alireza Sotoodeh

/*
 * =========================================================================
 * PROJECT: DeadReckoner
 * VERSION: 2.1 (QueueReset, parametric task rate, zero-init frames, OLED hot-plug docs)
 * DEVICE: ESP32-S3 N16R8 (16MB Flash + 8MB Octal PSRAM)
 * =========================================================================
 * WIRING DIAGRAM
 * -------------------------------------------------------------------------
 * Component Pin | MCU Pin       | Note / Hardware Reasoning
 * -------------------------------------------------------------------------
 * MPU9250 VCC   | 3.3V          | Clean 3.3V power rail
 * MPU9250 GND   | GND           | Common system ground
 * MPU9250 SCL   | GPIO 5        | Hardware I2C (Wire) Clock (Requires external 4.7k pull-up)
 * MPU9250 SDA   | GPIO 4        | Hardware I2C (Wire) Data (Requires external 4.7k pull-up)
 * MPU9250 AD0   | GND           | Forces I2C Address to 0x68
 * MPU9250 NCS   | 3.3V          | SPI disable, forces I2C mode
 * MPU9250 FSYNC | GND           | Tied to GND to prevent floating noise
 * -------------------------------------------------------------------------
 * OLED VCC      | 3.3V          | 0.91-inch SSD1306 Power
 * OLED GND      | GND           | Common system ground
 * OLED SCL      | GPIO 7        | Software I2C (U8g2 Bit-bang on Core 1)
 * OLED SDA      | GPIO 6        | Software I2C (U8g2 Bit-bang on Core 1)
 * -------------------------------------------------------------------------
 * SD Card 3V3   | 3.3V          | DIRECT 3.3V ONLY (Bypassing 5V regulators)
 * SD Card GND   | GND           | Common system ground
 * SD Card CS    | GPIO 15       | FSPI CS0 (High-speed line)
 * SD Card MOSI  | GPIO 11       | FSPI MOSI
 * SD Card SCK   | GPIO 12       | FSPI SCK (Running at 20MHz)
 * SD Card MISO  | GPIO 13       | FSPI MISO
 * -------------------------------------------------------------------------
 * BTN SELECT    | GPIO 1        | Menu Enter/Toggle (Active-Low, Internal Pull-up)
 * BTN UP        | GPIO 2        | Menu Navigation (Active-Low, Internal Pull-up)
 * BTN DOWN      | GPIO 8        | Menu Navigation (Active-Low, Internal Pull-up)
 * BTN TAG       | GPIO 14       | Waypoint Marker (Active-Low, Internal Pull-up)
 * -------------------------------------------------------------------------
 * BUZZER (+)    | GPIO 21       | Active Buzzer (use 100-ohm series resistor)
 * LED RED       | GPIO 17       | Requires 220~330 ohm series resistor
 * LED GREEN     | GPIO 18       | Requires 220~330 ohm series resistor
 * LED CATHODE   | GND           | Center long pin of the 3-pin Common Cathode LED
 * =========================================================================
 */
 
/*////////////////////////////includes////////////////////////////*/
#include "MPU9250.h" //for setting up MPU9250
#include <EEPROM.h>  // ESP32 EEPROM wrapper library for loading calibration
#include <U8g2lib.h> // U8g2 library for OLED
#include <Wire.h>    // I2C library
#include <SPI.h>     // SPI library for SD Card
#include <SdFat.h>   // SdFat library for high-speed logging
#include "esp_heap_caps.h" // Required for explicit PSRAM memory allocation

// ESP32's FS.h (pulled by WebServer below) defines a conflicting `File` class
// and overrides FILE_WRITE with "w". The macro rename makes FS.h define its
// class as SdFat_File_ instead, avoiding the name clash with SdFat's File.
#define File SdFat_File_
#include <WiFi.h>    // WiFi AP for phone GPS pairing
#include <WebServer.h> // HTTP server for GPS web page
#include <DNSServer.h> // Captive portal DNS redirection
#undef File
#undef FILE_WRITE
#define FILE_WRITE (O_RDWR | O_CREAT | O_AT_END)

/*////////////////////////////defines////////////////////////////*/

// =========================================================================
// pin definition
// =========================================================================
  // Button Pins 
  #define BTN_SELECT_PIN 1
  #define BTN_UP_PIN 2
  #define BTN_DOWN_PIN 8
  #define BTN_TAG_PIN 14
  // MPU9250
  #define I2C_MPU_SDA 4
  #define I2C_MPU_SCL 5
  // 0.91 inch OLED
  #define I2C_OLED_SDA 6
  #define I2C_OLED_SCL 7
  // DIY SD Card 
  #define SD_CS_PIN 15
  #define SD_MOSI_PIN 11
  #define SD_SCK_PIN 12
  #define SD_MISO_PIN 13
  // Notification Pins
  #define BUZZER_PIN 21
  #define LED_RED_PIN 17                                  
  #define LED_GREEN_PIN 18                                
// =========================================================================
// setting definition
// =========================================================================
  // serial print
  #define bud_rate 115200
  // DIY SD card
  #define SPI_FREQ_MHZ 20                                 // max is 26MHZ
  #define Attempt_Runtime_SD_recovery_MS 3000
  #define Recheck_SD_beforeBoot_MS 3000                                
  #define SD_recoverd_signal_MS 50
  // MPU9250 setting
  MPU9250 mpu; // handler: allowing access to all library methods
  #define MPU9250_IMU_ADDRESS 0x68 												// Specifies the I2C slave address of the MPU9250:0x68 GND / 0x69 High
  #define MAGNETIC_DECLINATION 3.4	 											// angle between magnetic north and true north (should be fix for diffrent city)
  #define MPU9250_Accelerometer_Rang  A2G 								//select: A2G, A4G, A8G, A16G
  #define MPU9250_Gyroscope_Rang  G500DPS 								//select: G250DPS, G500DPS, G1000DPS, G2000DPS
  #define MPU9250_Magnetometer_resolution  M16BITS 				//select: M14BITS, M16BITS
  #define MPU9250_fifo_sample_rate  SMPL_1000HZ 					// Must be >= SAMPLING_RATE_HZ. Options: 1000, 500, 333, 250, 200, 167, 143, 125 Hz
  #define MPU9250_Gyroscope_filter_choice  0x01						//select: 0x00: Enables DLPF with 8kHz sample rate|0x01: Enables DLPF with 1kHz sample rate|0x02 or 0x03: Bypasses DLPF
  #define MPU9250_Gyroscope_DLPF_cutoff  DLPF_41HZ 				// Should be <= SAMPLING_RATE_HZ/2 (Nyquist). Options: 250, 184, 92, 41, 20, 10, 5, 3600 Hz
  #define MPU9250_Accelerometer_filter_choice  0x01				//select: 0x01 Enable, 0x00 bypass
  #define MPU9250_Accelerometer_DLPF_cutoff  DLPF_5HZ 		// Should be <= SAMPLING_RATE_HZ/2 (Nyquist). Options: 218, 99, 45, 21, 10, 5, 420 Hz
  #define MPU9250_filter_algorithm	MADGWICK 							//select: MADGWICK, MAHONY, NONE
  #define MPU9250_filter_iterations	10										//select: 1-50 higher better but may slow down
  #define attempt_recovery_MPU9250_MS 2000
  // OLED 
  U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ I2C_OLED_SCL, /* data=*/ I2C_OLED_SDA, /* reset=*/ U8X8_PIN_NONE);
  #define font_10_pixel u8g2_font_t0_15b_me
  #define font_8_pixel u8g2_font_helvB08_tf
  #define font_5_pixel u8g2_font_spleen5x8_me
  #define update_rate_oled 1500
  #define OLED_SLEEP_TIMEOUT_MS 20000                       
  // Notification Timings
  #define ALARM_BEEP_MS 100                                 // Duration of error beeps during SD failure      
  #define TAG_BEEP_MS 50                                    
  #define TAG_BLINK_MS 100                                  
  // buttons 
  #define Press_to_ShutDown_MS 3000
  // menu 
  #define MENU_ITEMS_COUNT 5
  // Phone GPS Pairing via WiFi AP
  #define GPS_AP_SSID   "DeadReckoner-S3"
  #define GPS_AP_PASS   "deadreckoner"
  #define GPS_AP_IP     192, 168, 4, 1
  #define GPS_AP_CHANNEL 1
  #define GPS_AP_HIDDEN  0
  #define GPS_AP_MAX_CONN 1
// =========================================================================
// PARAMETRIC BANDWIDTH & MEMORY ENGINE
// =========================================================================
  #define DATA_FRAME_SIZE            47     
  #define SAMPLING_RATE_HZ           100    
  #if SAMPLING_RATE_HZ < 1 || SAMPLING_RATE_HZ > 1000
    #error "SAMPLING_RATE_HZ must be between 1 and 1000 (MPU9250 max FIFO rate)"
  #endif
  #define BYTES_PER_SECOND           (DATA_FRAME_SIZE * SAMPLING_RATE_HZ)
  #define BYTES_PER_HOUR             ((uint64_t)BYTES_PER_SECOND * 3600)
  #define MB_PER_HOUR                ((float)BYTES_PER_HOUR / (1024.0 * 1024.0)) 
  #define QUEUE_LENGTH               50000  
  #define PSRAM_BUFFER_SIZE_MB       ((float)(QUEUE_LENGTH * DATA_FRAME_SIZE) / (1024.0 * 1024.0))
// =========================================================================
// Calibration
// =========================================================================
  #define EEPROM_MAGIC_NUMBER       0xDEAD
  #define EEPROM_MAGIC_ADDR         0

/*//////////////////////////// Function prototypes ////////////////////////////*/
void performCalibration();
void print_calibration();
void saveCalibration();
void loadCalibration();
uint16_t calcCRC16(const uint8_t* data, uint16_t len);
void writeLogFileHeader(File& file);
void startGPSAP();
void stopGPSAP();
void handleGPSRoot();
void handleGPSPost();
void handleGPSNotFound();
void writeGPSFrame(uint8_t event_flag, double lat, double lon, float alt, uint32_t time);
/*//////////////////////////// RTOS Data Structures ////////////////////////////*/

#pragma pack(push, 1) // Force absolute 1-byte alignment for all enclosed structures
// File header written at the start of every DR_LOG_xxx.BIN file for self-describing binary parsing
typedef struct {
    uint32_t magic;        // 4 Bytes: Validation magic 0xDEADC0DE
    uint8_t  version;      // 1 Byte:  Format version (1)
    uint8_t  frame_size;   // 1 Byte:  sizeof(LogFrame) for forward compatibility
    uint16_t sample_rate;  // 2 Bytes: SAMPLING_RATE_HZ at file creation
    uint64_t epoch_ms;     // 8 Bytes: millis() at file creation for absolute time anchoring
} FileHeader;  // 16 bytes total

// Optimized Parametric Binary structure using Union 
typedef struct {
    uint32_t frame_seq;  // 4 Bytes: Monotonic sequential index for frame drop tracking
    uint64_t timestamp;  // 8 Bytes: Relative microsecond timestamp from log start (esp_timer_get_time() - log_time_base)
    uint8_t event_flag;  // 1 Byte: 0=IMU, 1=TAG, 0xAA=SD_GAP, 0xBB=GPS
    
    // Memory Overlap: Total payload size strictly 32 Bytes
    union {
        struct {
            float q[4];
            float accel[3];
            float temp;
        } imu;
        
        struct {
            double lat;
            double lng;
            float  alt;
            uint32_t epoch; // Unix epoch seconds from phone
        } gps;
    } payload;
    
    uint16_t crc;
} LogFrame;
#pragma pack(pop) // Restore default compiler alignment

// UI State Machine Definitions
  enum UIState {
      STATE_LIVE_VIEW,
      STATE_MENU,
      STATE_SUBMENU_SD_INFO,    
      STATE_SUBMENU_DISPLAY,
      STATE_SUBMENU_MUTE,
      STATE_CONFIRM_FORMAT,
      STATE_CONFIRM_CREATE_FILE,
      STATE_GPS_PROMPT_START,    // Ask "Get GPS start?" at boot / new file / format
      STATE_GPS_PROMPT_END,      // Ask "Get GPS end?" during shutdown
      STATE_GPS_WAITING,         // WiFi AP active, waiting for phone data
      STATE_GPS_CONFIRM_EXIT     // "Exit GPS pairing?" [YES/NO]
  };
  volatile UIState currentState = STATE_LIVE_VIEW;

  const char* menuItems[MENU_ITEMS_COUNT] = {
      "1.Display Mode",
      "2.Mute Sounds",
      "3.SD Card Info",
      "4.Calibration",
      "5.Exit Menu"
  };
  int8_t menuCursor = 0; // Tracks selected menu item

// GPS Pairing globals
  bool gps_start_needed = false;   // Set true when a new file needs start GPS
  bool gps_end_needed = false;     // Set true during shutdown for end GPS
  bool gps_pending_file_creation = false; // Set true when file must be created after GPS prompt
  int8_t gps_prompt_cursor = 1;    // Cursor for YES/NO prompt (default NO)
  volatile bool gps_data_received = false; // Flag set by HTTP handler
  IPAddress gps_ap_ip(GPS_AP_IP);
  DNSServer gps_dns;
  WebServer gps_server(80);

// Inter-Core Communication Flags
  volatile uint8_t tag_event_pending = 0;  // counter to catch consecutive TAG presses
  volatile bool mpu_critical_error = false;
  volatile bool sd_critical_error = false;
  volatile bool system_shutdown_requested = false;
  volatile uint32_t global_frame_counter = 0;
  volatile uint32_t dropped_frames_count = 0;
  volatile uint64_t log_time_base = 0;

// Hardware spinlocks for multi-core thread safety
  portMUX_TYPE frameCounterMux = portMUX_INITIALIZER_UNLOCKED;
  portMUX_TYPE tagEventMux = portMUX_INITIALIZER_UNLOCKED; 

// Tracks the active file to resume appending after failure
  uint16_t global_log_id = 1;       // X: Main Log/Test Number (e.g., 001)
  uint16_t global_recovery_id = 1;  // Y: Recovery Instance Number (e.g., 002)
  uint16_t max_log_id = 1;          // Tracks the highest existing DR_LOG_xxx.BIN index
  char current_log_filename[20] = "DR_LOG_001.BIN";

// Phone GPS data for start/end anchoring via WiFi AP pairing
  typedef struct {
      bool     has_start;
      double   start_lat;
      double   start_lon;
      float    start_alt;
      uint32_t start_time;   // UNIX epoch seconds from phone
      bool     has_end;
      double   end_lat;
      double   end_lon;
      float    end_alt;
      uint32_t end_time;
  } PhoneGPSData;
  PhoneGPSData phone_gps = {false, 0, 0, 0, 0, false, 0, 0, 0, 0};

// FreeRTOS Handles & PSRAM Queue
  QueueHandle_t dataQueue;
  uint8_t *queueBuffer;      
  StaticQueue_t *queueStruct;
  TaskHandle_t sensorTaskHandle;
  TaskHandle_t loggingTaskHandle;

// SD Card Handlers
  SdFat sd;
  File logFile;

uint16_t calcCRC16(const uint8_t* data, uint16_t len) {
    uint16_t crc = 0xFFFF;
    while (len--) {
        crc ^= *data++;
        for (uint8_t i = 0; i < 8; i++) {
            crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : (crc >> 1);
        }
    }
    return crc;
}

/*//////////////////////////// FreeRTOS Tasks ////////////////////////////*/

// =========================================================================
// SENSOR CONFIGURATION HELPER
// =========================================================================
void configureMPUSettings(MPU9250Setting& setting) {
    setting.accel_fs_sel = ACCEL_FS_SEL::MPU9250_Accelerometer_Rang;
    setting.gyro_fs_sel = GYRO_FS_SEL::MPU9250_Gyroscope_Rang;
    setting.mag_output_bits = MAG_OUTPUT_BITS::MPU9250_Magnetometer_resolution; 
    setting.fifo_sample_rate = FIFO_SAMPLE_RATE::MPU9250_fifo_sample_rate; 
    setting.gyro_fchoice = MPU9250_Gyroscope_filter_choice;
    setting.gyro_dlpf_cfg = GYRO_DLPF_CFG::MPU9250_Gyroscope_DLPF_cutoff;
    setting.accel_fchoice = MPU9250_Accelerometer_filter_choice;
    setting.accel_dlpf_cfg = ACCEL_DLPF_CFG::MPU9250_Accelerometer_DLPF_cutoff;
}

// =========================================================================
// DYNAMIC RECOVERY PROTOCOL
// =========================================================================
void attemptMPURecovery() {
    // 1. Hardware-level I2C Bus Reset
    Wire.end();
    Wire.begin(I2C_MPU_SDA, I2C_MPU_SCL);
    Wire.setClock(400000);
    Wire.setTimeout(50);

    // 2. Re-initialize MPU Settings
    MPU9250Setting setting;
    configureMPUSettings(setting);

    // 3. Attempt Connection & Apply Filters
    if (mpu.setup(MPU9250_IMU_ADDRESS, setting)) {
        mpu.setMagneticDeclination(MAGNETIC_DECLINATION); 
        mpu.selectFilter(QuatFilterSel::MPU9250_filter_algorithm); 
        mpu.setFilterIterations(MPU9250_filter_iterations);
        
        loadCalibration(); // Re-apply EEPROM values
        mpu_critical_error = false; // Flag system as recovered
    }
}

// =========================================================================
// FILE HEADER WRITER
// =========================================================================
void writeLogFileHeader(File& file) {
    FileHeader h;
    h.magic = 0xDEADC0DE;
    h.version = 1;
    h.frame_size = sizeof(LogFrame);
    h.sample_rate = SAMPLING_RATE_HZ;
    h.epoch_ms = millis();
    file.write((uint8_t*)&h, sizeof(h));
}

// =========================================================================
// PHONE GPS PAIRING VIA WiFi AP
// =========================================================================

// Embedded HTML page for GPS manual entry (auto-fills date/time from browser JS)
const char gps_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>DeadReckoner GPS</title>
<style>
body{font-family:sans-serif;text-align:center;margin:20px;background:#222;color:#fff}
input{width:90%%;padding:8px;margin:5px;font-size:16px;border-radius:4px;border:1px solid #555;background:#333;color:#fff}
button{width:90%%;padding:12px;margin:10px;font-size:18px;background:#4CAF50;color:#fff;border:none;border-radius:5px;cursor:pointer}
button:hover{background:#45a049}
#status{padding:10px;margin:10px;border-radius:4px}
.ok{color:#8f8;background:#252}
.err{color:#f88;background:#522}
.info{color:#aaa;font-size:14px}
</style></head><body>
<h2>DeadReckoner GPS</h2>
<p class="info">Open your map app, long-press start location, copy lat/lon, and paste below.</p>
<p id="status" class="info">Enter GPS data and tap SET</p>
<form id="gpsForm" onsubmit="return sendGPS()">
<label>Latitude:</label><input type="text" id="lat" placeholder="e.g. 35.689506" required>
<label>Longitude:</label><input type="text" id="lon" placeholder="e.g. 51.389046" required>
<label>Altitude (m, optional):</label><input type="text" id="alt" placeholder="e.g. 1800">
<label>Date/Time (auto-filled, editable):</label><input type="text" id="time" placeholder="YYYY-MM-DD HH:MM:SS">
<button type="submit">SET LOCATION</button>
</form>
<p class="info">After SET, you can close this page and disconnect WiFi.</p>
<script>
function pad(n){return n.toString().padStart(2,'0')}
var d=new Date();
document.getElementById('time').value = d.getFullYear()+'-'+pad(d.getMonth()+1)+'-'+pad(d.getDate())+' '+pad(d.getHours())+':'+pad(d.getMinutes())+':'+pad(d.getSeconds());
function sendGPS(){var lat=parseFloat(document.getElementById('lat').value);var lon=parseFloat(document.getElementById('lon').value);var alt=parseFloat(document.getElementById('alt').value)||0;var parts=(document.getElementById('time').value).split(/[- :]/);var t=new Date(parts[0],parts[1]-1,parts[2],parts[3]||0,parts[4]||0,parts[5]||0);var time=Math.floor(t.getTime()/1000);var xhr=new XMLHttpRequest();xhr.open('POST','/gps',true);xhr.setRequestHeader('Content-Type','application/json');xhr.onload=function(){if(xhr.status==200){document.getElementById('status').innerHTML='GPS SET! You can close this page and disconnect WiFi.';document.getElementById('status').className='ok'}else{document.getElementById('status').innerHTML='Error: '+xhr.responseText;document.getElementById('status').className='err'}};xhr.onerror=function(){document.getElementById('status').innerHTML='Connection error';document.getElementById('status').className='err'};xhr.send(JSON.stringify({lat:lat,lon:lon,alt:alt,time:time}));return false}
</script></body></html>
)rawliteral";

void startGPSAP() {
    // Start WiFi in soft-AP mode with password
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(gps_ap_ip, gps_ap_ip, IPAddress(255, 255, 255, 0));
    WiFi.softAP(GPS_AP_SSID, GPS_AP_PASS, GPS_AP_CHANNEL, GPS_AP_HIDDEN, GPS_AP_MAX_CONN);
    
    // Start DNS server for captive portal (redirects all domains to ESP)
    gps_dns.start(53, "*", gps_ap_ip);
    
    // Configure HTTP server routes
    gps_server.on("/", handleGPSRoot);
    gps_server.on("/gps", HTTP_POST, handleGPSPost);
    gps_server.onNotFound(handleGPSNotFound);
    gps_server.begin();
    
    Serial.println("GPS AP started: " GPS_AP_SSID);
}

void stopGPSAP() {
    gps_dns.stop();
    gps_server.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    Serial.println("GPS AP stopped");
}

void handleGPSRoot() {
    gps_server.send_P(200, "text/html", gps_html);
}

void handleGPSPost() {
    if (!gps_server.hasArg("plain")) {
        gps_server.send(400, "text/plain", "No data received");
        return;
    }
    // Parse JSON from phone
    String body = gps_server.arg("plain");
    Serial.print("GPS POST received: ");
    Serial.println(body);
    
    // Extract lat, lon, alt, time using simple string search
    double lat = 0, lon = 0;
    float alt = 0;
    uint32_t time = 0;
    
    int idx;
    idx = body.indexOf("\"lat\":");
    if (idx >= 0) lat = String(body.substring(idx + 6)).toFloat();
    idx = body.indexOf("\"lon\":");
    if (idx >= 0) lon = String(body.substring(idx + 6)).toFloat();
    idx = body.indexOf("\"alt\":");
    if (idx >= 0) alt = String(body.substring(idx + 6)).toFloat();
    idx = body.indexOf("\"time\":");
    if (idx >= 0) time = String(body.substring(idx + 7)).toInt();
    
    // Store the GPS data based on prompt mode
    if (gps_start_needed) {
        phone_gps.has_start = true;
        phone_gps.start_lat = lat;
        phone_gps.start_lon = lon;
        phone_gps.start_alt = alt;
        phone_gps.start_time = time;
        // 0xBB frame is written later by the GPS_WAITING exit handler,
        // which creates the file first if needed (boot / new file / format).
        Serial.println("Start GPS saved");
    } else if (gps_end_needed) {
        phone_gps.has_end = true;
        phone_gps.end_lat = lat;
        phone_gps.end_lon = lon;
        phone_gps.end_alt = alt;
        phone_gps.end_time = time;
        // Write 0xCC frame immediately (file is still open during shutdown)
        if (logFile) {
            writeGPSFrame(0xCC, lat, lon, alt, time);
            logFile.sync();
        }
        Serial.println("End GPS saved");
    }
    
    gps_data_received = true;
    gps_server.send(200, "text/plain", "OK");
}

void handleGPSNotFound() {
    // Captive portal: redirect any domain to the GPS page
    gps_server.sendHeader("Location", "http://192.168.4.1/");
    gps_server.send(302, "text/plain", "");
}

void writeGPSFrame(uint8_t event_flag, double lat, double lon, float alt, uint32_t time) {
    // Write a GPS metadata frame using the existing LogFrame GPS union slot
    LogFrame gpsFrame = {};
    portENTER_CRITICAL(&frameCounterMux);
    gpsFrame.frame_seq = global_frame_counter++;
    portEXIT_CRITICAL(&frameCounterMux);
    gpsFrame.timestamp = esp_timer_get_time() - log_time_base;
    gpsFrame.event_flag = event_flag; // 0xBB=start, 0xCC=end
    gpsFrame.payload.gps.lat = lat;
    gpsFrame.payload.gps.lng = lon;
    gpsFrame.payload.gps.alt = alt;
    gpsFrame.payload.gps.epoch = time;
    gpsFrame.crc = calcCRC16((uint8_t*)&gpsFrame, sizeof(LogFrame) - sizeof(gpsFrame.crc));
    logFile.write((uint8_t*)&gpsFrame, sizeof(LogFrame));
}

// =========================================================================
// SD CARD DYNAMIC RECOVERY PROTOCOL
// =========================================================================
bool attemptSDRecovery() {
    logFile.close();
    SPI.end();
    
    // Allow SPI hardware to power-cycle before reinitialization
    vTaskDelay(pdMS_TO_TICKS(10));
    
    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    
    if (sd.begin(SD_CS_PIN, SD_SCK_MHZ(SPI_FREQ_MHZ))) {
        // === FIX ISSUE 9: Generate file name based on current validated recovery index ===
        snprintf(current_log_filename, sizeof(current_log_filename), "%03d%03d.BIN", global_log_id, global_recovery_id);
        
        logFile = sd.open(current_log_filename, FILE_WRITE);
        if (logFile) {
            // Commit on Success: Only increment index after file is verified open
            global_recovery_id++;
            sd_critical_error = false;
            return true;
        }
    }
    return false;
}

// =========================================================================
// Core 0 Task
// Strictly for high-speed sensor reading and mathematical fusion
// =========================================================================
void sensorTask(void *pvParameters) {
  LogFrame frame = {};
  unsigned long last_mpu_data_time = millis();  // Track last successful read
  unsigned long last_recovery_attempt = 0;      // Tracks MPU9250 recovery intervals
  TickType_t xLastWakeTime = xTaskGetTickCount();

  for(;;) {
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000 / SAMPLING_RATE_HZ));

    // === If shutdown is initiated ===
    if (system_shutdown_requested) {
        vTaskDelay(pdMS_TO_TICKS(100));
        continue;
    }

    // === INTERCEPTOR: CRITICAL MPU DISCONNECT ERROR ===
    if (mpu_critical_error) {
        if (millis() - last_recovery_attempt > attempt_recovery_MPU9250_MS) {
            last_recovery_attempt = millis();
            attemptMPURecovery();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
        continue; // Skip sensor reading until recovered
    }

    if (mpu.update()) {
      last_mpu_data_time = millis(); // Reset timeout counter
      // Thread-safe atomic read of multi-core shared variables to prevent 64-bit torn reads
      uint64_t local_time_base;
      portENTER_CRITICAL(&frameCounterMux);
      frame.frame_seq = global_frame_counter++;
      local_time_base = log_time_base;
      portEXIT_CRITICAL(&frameCounterMux);
      
      // Calculate high-precision timestamp relative to calibrated log time base
      frame.timestamp = esp_timer_get_time() - local_time_base;
      frame.event_flag = 0; // Default: 0 marks standard high-speed IMU packet
      
      // Updated syntax to target the optimized overlapping payload union
      frame.payload.imu.q[0] = mpu.getQuaternionW();
      frame.payload.imu.q[1] = mpu.getQuaternionX();
      frame.payload.imu.q[2] = mpu.getQuaternionY();
      frame.payload.imu.q[3] = mpu.getQuaternionZ();
      frame.payload.imu.accel[0] = mpu.getLinearAccX();
      frame.payload.imu.accel[1] = mpu.getLinearAccY();
      frame.payload.imu.accel[2] = mpu.getLinearAccZ();
      frame.payload.imu.temp = mpu.getTemperature();
      // Thread-safe isolation for inter-core Waypoint Tagging flags
      portENTER_CRITICAL(&tagEventMux);
      if (tag_event_pending > 0) {
          frame.event_flag = 1; // 1 marks user button interaction event
          tag_event_pending--;
      }
      portEXIT_CRITICAL(&tagEventMux);
      frame.crc = calcCRC16((uint8_t*)&frame, sizeof(LogFrame) - sizeof(frame.crc));
      // Monitor Queue Health and Track Silent Overflows
      if (xQueueSend(dataQueue, &frame, 0) != pdPASS) {
          dropped_frames_count++;
          digitalWrite(LED_RED_PIN, HIGH);
      } else if (!sd_critical_error && !mpu_critical_error) {
          digitalWrite(LED_RED_PIN, LOW);
      }
    } else {
      // DETECT SENSOR DISCONNECTION: No new data for 500ms
      if (millis() - last_mpu_data_time > 500) {
          mpu_critical_error = true;
      }
    }
  }
}

// =========================================================================
// Core 1 Task
// For OLED, Buttons, and heavy Flash/SD writing
// =========================================================================
void loggingTask(void *pvParameters) {
 // Zero-initialize the structure to prevent rendering garbage stack bytes before the first frame arrives
  LogFrame receivedFrame = {};
  unsigned long lastDisplayMillis = 0;
  unsigned long lastFlushMillis = 0;
  unsigned long lastBtnCheckMillis = 0; 
  int8_t displayCursor = 0; // Tracks selection inside Display Mode submenu
  int8_t muteCursor = 0; // Tracks selection inside Mute Sounds submenu
  int8_t confirmCursor = 1; // Tracks choice in Format Confirm menu (Default: 1 = NO)
  int8_t sdMenuCursor = 0; // Tracks selection inside the 8-item SD Submenu
  int8_t sdScrollOffset = 0; // Manages the scrolling window for 128x32 OLED
  uint16_t totalFilesCount = 0; // Stores computed BIN files count on SD

  // State tracking variables for Edge Detection
  bool selectWasPressed = false;
  bool upWasPressed = false;
  bool downWasPressed = false;
  bool tagWasPressed = false;
  // Flag for instant UI rendering
  bool force_update_ui = false;
  // Storage variables for SD calculations
  uint32_t sd_free_mb = 0;
  float sd_remain_hours = 0.0;
  float sd_total_gb = 0.0;
  // === Power Management Variables ===
  bool is_oled_sleeping = false;
  bool auto_off_enabled = true; // Default: OLED sleeps after 20s
  unsigned long last_interaction_millis = millis();
  // === Sound & Notification Management Variables ===
  bool is_muted = false; // Default: Sounds are ON
  unsigned long buzzer_turn_off_time = 0;
  unsigned long led_turn_off_time = 0;
  bool is_buzzer_on = false;
  bool is_led_on = false;
  bool error_handled = false; 
  unsigned long last_sd_recovery_attempt = 0;
  bool gps_boot_prompt_done = false; // One-shot boot GPS prompt flag

  // One-shot boot GPS start prompt before logging begins
  if (!gps_boot_prompt_done) {
      gps_boot_prompt_done = true;
      gps_start_needed = true;
      currentState = STATE_GPS_PROMPT_START;
      force_update_ui = true;
  }

  for(;;) {
    unsigned long currentMillis = millis();

    // === INTERCEPTOR: RUNTIME SD CARD FAILURE RECOVERY ===
    if (sd_critical_error) {
        // 1. HARDWARE ALARM LAYER (Absolute Non-Blocking)
        // Rhythmic SOS pattern: Strict 100ms ON every 1000ms cycle
        if ((currentMillis % 1000) < 100) {
            digitalWrite(LED_RED_PIN, HIGH);
            if (!is_muted) digitalWrite(BUZZER_PIN, HIGH);
        } else {
            digitalWrite(LED_RED_PIN, LOW);
            digitalWrite(BUZZER_PIN, LOW);
        }

        // 2. SLOW-RATE RECOVERY LAYER (Executes only once every 3000ms)
        // Prevents the 500ms sd.begin() hardware timeout from starving the alarm rhythm
        if (currentMillis - last_sd_recovery_attempt > Attempt_Runtime_SD_recovery_MS) {
            last_sd_recovery_attempt = currentMillis;
            is_oled_sleeping = false;
            u8g2.setPowerSave(0); 
            u8g2.clearBuffer();
            u8g2.drawStr(5, 12, "SD CARD ERR!");
            u8g2.drawStr(0, 26, "RETRIES ACTIVE...");
            u8g2.sendBuffer();

            // FIX: Force hardware alarms OFF before entering the blocking SD library function
            // This prevents the buzzer from screaming continuously if caught in the ON phase
            digitalWrite(LED_RED_PIN, LOW);
            digitalWrite(BUZZER_PIN, LOW);

            if (attemptSDRecovery()) {
                LogFrame gapFrame;
                
                // Fully clear the entire structure to prevent uninitialized temp/garbage bytes
                memset(&gapFrame, 0, sizeof(LogFrame));
                
                // Thread-safe capture of sequence counter and timebase
                uint64_t local_time_base;
                portENTER_CRITICAL(&frameCounterMux);
                gapFrame.frame_seq = global_frame_counter++;
                local_time_base = log_time_base;
                portEXIT_CRITICAL(&frameCounterMux);
                
                // Relative timestamp consistent with IMU frames
                gapFrame.timestamp = esp_timer_get_time() - local_time_base;    
                gapFrame.event_flag = 0xAA;
                gapFrame.crc = calcCRC16((uint8_t*)&gapFrame, sizeof(LogFrame) - sizeof(gapFrame.crc));
                
                // Force secure block write of the sterile gap marker
                logFile.write((uint8_t*)&gapFrame, sizeof(LogFrame));
                logFile.sync();
                
                // Reset hardware signaling to safe operational state
                digitalWrite(LED_RED_PIN, LOW);
                digitalWrite(BUZZER_PIN, LOW); 
                digitalWrite(LED_GREEN_PIN, HIGH);
                
                u8g2.clearBuffer();
                char recBuf[32];
                snprintf(recBuf, sizeof(recBuf), "Rec Active: %s", current_log_filename);
                u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(recBuf)) / 2, 20, recBuf);
                u8g2.sendBuffer();
                
                vTaskDelay(pdMS_TO_TICKS(SD_recoverd_signal_MS));
                digitalWrite(LED_GREEN_PIN, LOW);
                force_update_ui = true;
                last_interaction_millis = currentMillis;
                continue; 
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10)); // Yield execution to prevent watchdog starvation
        continue; // Force trap within recovery boundary
    }

    // === INTERCEPTOR: CRITICAL MPU DISCONNECT ERROR ===
    if (mpu_critical_error) {
        if (!error_handled) {
            // Safely close the SD log file to prevent data corruption
            if (logFile) { logFile.sync(); logFile.close(); }
            
            // Force wake OLED
            is_oled_sleeping = false;
            u8g2.setPowerSave(0); 
            u8g2.clearBuffer();
            u8g2.drawStr(5, 15, "CRITICAL ERROR!");
            u8g2.drawStr(0, 28, "MPU DISCONNECTED");
            u8g2.sendBuffer();
            
            error_handled = true;
        }
        
        // Non-blocking SOS Pattern (100ms ON, 100ms OFF)
        if ((currentMillis / 100) % 2 == 0) {
            digitalWrite(LED_RED_PIN, HIGH);
            if (!is_muted) digitalWrite(BUZZER_PIN, HIGH);
        } else {
            digitalWrite(LED_RED_PIN, LOW);
            digitalWrite(BUZZER_PIN, LOW);
        }
        
        vTaskDelay(pdMS_TO_TICKS(20)); // Yield to scheduler
        continue; // BYPASS THE REST OF THE LOOP!
        
    } else if (error_handled) {
        // === DYNAMIC RECOVERY TRIGGERED ===
        error_handled = false;
        digitalWrite(LED_RED_PIN, LOW);
        digitalWrite(BUZZER_PIN, LOW);
        
        // Create a new sequential log file to resume mission safely
        logFile = sd.open(current_log_filename, FILE_WRITE);
        
        // FIX ISSUE 5: Validate file pointer after MPU recovery to prevent silent write failures
        if (!logFile) {
            sd_critical_error = true;
        }
        
        // Feedback to User
        u8g2.clearBuffer();
        u8g2.drawStr(10, 15, "MPU RECOVERED!");
        u8g2.drawStr(5, 28, "Resuming Log...");
        u8g2.sendBuffer();
        vTaskDelay(pdMS_TO_TICKS(1500)); // Show message briefly
        
        // Reset UI State to live tracking
        force_update_ui = true;
        currentState = STATE_LIVE_VIEW;
        last_interaction_millis = currentMillis;
    }
    
    // === PHASE 0: Non-Blocking Hardware Notifications ===
    if (is_buzzer_on && (currentMillis >= buzzer_turn_off_time)) {
        digitalWrite(BUZZER_PIN, LOW);
        is_buzzer_on = false;
    }
    if (is_led_on && (currentMillis >= led_turn_off_time)) {
        digitalWrite(LED_GREEN_PIN, LOW); // Turn off Green LED
        is_led_on = false;
    }

    // === PHASE 1: Button Edge Detection & Debouncing ===
    bool selectTriggered = false, upTriggered = false, downTriggered = false;
    if (currentMillis - lastBtnCheckMillis > 50) {
      
      // TAG Button (Independent of OLED Sleep state)
      if (digitalRead(BTN_TAG_PIN) == LOW) {
        if (!tagWasPressed) { 
            // Protect writing to shared variable across cores
            portENTER_CRITICAL(&tagEventMux);
            if (tag_event_pending < 255) tag_event_pending++; // cap at 255 to avoid overflow
            portEXIT_CRITICAL(&tagEventMux);
            tagWasPressed = true;
            if (!selectWasPressed) last_interaction_millis = currentMillis; 
            
            // Trigger Fast Feedback (Non-Blocking)
            if (!is_muted) { 
                digitalWrite(BUZZER_PIN, HIGH);
                is_buzzer_on = true;
                buzzer_turn_off_time = currentMillis + TAG_BEEP_MS; 
            }
            
            // Green LED always flashes for TAG, even in stealth mode
            digitalWrite(LED_GREEN_PIN, HIGH);
            is_led_on = true;
            led_turn_off_time = currentMillis + TAG_BLINK_MS; 
        }
      } else { tagWasPressed = false; }

      // SELECT Button with Smart Long-Press Detection for Safe Shutdown
      if (digitalRead(BTN_SELECT_PIN) == LOW) {
        if (!selectWasPressed) { 
            // Button was just physically pressed down
            selectTriggered = false; // Do not trigger instantly on falling edge
            selectWasPressed = true;
            last_interaction_millis = currentMillis; // Mark press start time
        } else {
            // Button is being continuously HELD down
            if (currentMillis - last_interaction_millis > Press_to_ShutDown_MS) {
                // --- CRITICAL SAFE SHUTDOWN PROTOCOL ---
                system_shutdown_requested = true; // Signals Core 0 to halt data generation
                
                // Force wake OLED and notify user
                is_oled_sleeping = false;
                u8g2.setPowerSave(0); 
                u8g2.clearBuffer();
                u8g2.setFont(font_8_pixel);
                u8g2.drawStr(5, 12, "SAVING DATA...");
                u8g2.drawStr(0, 26, "DO NOT UNPLUG!");
                u8g2.sendBuffer();
                
                // FLUSH LAYER: Drain remaining frames from PSRAM Queue directly to SD Card
                LogFrame flushFrame = {};
                uint32_t remainingFrames = uxQueueMessagesWaiting(dataQueue);
                Serial.print("Shutdown active. Flushing frames to SD: "); Serial.println(remainingFrames);
                
                uint32_t flushCount = 0; // NEW: Counter to track and feed the Task Watchdog Timer
                while (xQueueReceive(dataQueue, &flushFrame, 0) == pdPASS) {
                    if (logFile) {
                        logFile.write((uint8_t*)&flushFrame, sizeof(LogFrame));
                    }
                    
                    flushCount++;
                    // CRITICAL INDUSTRIAL FIX: Yield every 500 frames to feed the RTOS Watchdog.
                    // Prevents a hard system reset/crash during massive 2.14MB PSRAM buffer flushing.
                    if (flushCount % 500 == 0) {
                        vTaskDelay(pdMS_TO_TICKS(5)); 
                    }
                }
                
                // === GPS END PROMPT ===
                gps_end_needed = true;
                int8_t gps_end_cursor = 1; // Default NO
                bool gps_end_done = false;
                
                while (!gps_end_done) {
                    u8g2.clearBuffer();
                    u8g2.setFont(font_8_pixel);
                    u8g2.drawStr(0, 8, "Get GPS End?");
                    u8g2.drawStr(20, 22, "YES");
                    u8g2.drawStr(20, 32, "NO");
                    u8g2.drawStr(8, (gps_end_cursor == 0) ? 22 : 32, ">");
                    u8g2.sendBuffer();
                    
                    delay(100); // Debounce + yield for WiFi background
                    
                    // UP/DOWN toggles cursor
                    if (digitalRead(BTN_UP_PIN) == LOW) {
                        gps_end_cursor = 0;
                        while (digitalRead(BTN_UP_PIN) == LOW) delay(10);
                    } else if (digitalRead(BTN_DOWN_PIN) == LOW) {
                        gps_end_cursor = 1;
                        while (digitalRead(BTN_DOWN_PIN) == LOW) delay(10);
                    }
                    
                    // SELECT confirms
                    if (digitalRead(BTN_SELECT_PIN) == LOW) {
                        delay(50);
                        if (digitalRead(BTN_SELECT_PIN) == LOW) {
                            gps_end_done = true;
                        }
                    }
                }
                
                if (gps_end_cursor == 0) {
                    // YES: Start WiFi AP to get end GPS
                    gps_data_received = false;
                    startGPSAP();
                    
                    u8g2.clearBuffer();
                    u8g2.drawStr(0, 8, "Connect Phone to");
                    u8g2.drawStr(0, 20, GPS_AP_SSID);
                    u8g2.drawStr(0, 31, "Open 192.168.4.1");
                    u8g2.sendBuffer();
                    Serial.println("GPS end: AP started, waiting for phone...");
                    
                    bool wifi_done = false;
                    bool wifi_timeout = false;
                    unsigned long wifi_wait_start = millis();
                    
                    while (!wifi_done && !wifi_timeout) {
                        gps_server.handleClient();
                        gps_dns.processNextRequest();
                        
                        if (gps_data_received) {
                            wifi_done = true;
                            u8g2.clearBuffer();
                            u8g2.drawStr(5, 12, "GPS End Saved!");
                            u8g2.drawStr(5, 26, "Shutting Down...");
                            u8g2.sendBuffer();
                            delay(1500);
                        }
                        
                        // Long timeout: 5 minutes (user-controlled via SELECT to exit)
                        for (int w = 0; w < 10; w++) {
                            delay(10); // Poll WiFi in 10ms chunks
                            gps_server.handleClient();
                            gps_dns.processNextRequest();
                            if (gps_data_received) break;
                        }
                        
                        // Check SELECT for manual exit
                        if (!wifi_done && digitalRead(BTN_SELECT_PIN) == LOW) {
                            delay(50);
                            if (digitalRead(BTN_SELECT_PIN) == LOW) {
                                wifi_done = true;
                                while (digitalRead(BTN_SELECT_PIN) == LOW) delay(10);
                            }
                        }
                    }
                    
                    stopGPSAP();
                }
                
                gps_end_needed = false;
                
                // Close file handler safely to lock file allocation tables
                if (logFile) {
                    logFile.sync();
                    logFile.close();
                }
                
                // Final UI Notification
                u8g2.clearBuffer();
                const char* offMsg = "SAFE TO POWER OFF";
                int xOff = (u8g2.getDisplayWidth() - u8g2.getStrWidth(offMsg)) / 2;
                u8g2.drawStr(xOff, 20, offMsg);
                u8g2.sendBuffer();
                
                // Hardware visual feedback: Solid Green LED signals absolute safety
                digitalWrite(LED_RED_PIN, LOW);
                digitalWrite(LED_GREEN_PIN, HIGH);
                for(int i = 0; i < 2; i++) {
                  digitalWrite(BUZZER_PIN, HIGH);
                  delay(ALARM_BEEP_MS);
                  digitalWrite(BUZZER_PIN, LOW);
                  delay(ALARM_BEEP_MS);
                }
                digitalWrite(BUZZER_PIN, LOW);
                
                // Optional: Insert Power Latch GPIO clear command here to cut battery physically
                // digitalWrite(POWER_LATCH_PIN, LOW);
                
                while(1) {
                    vTaskDelay(pdMS_TO_TICKS(1000)); // Lock system safely forever
                }
            }
        }
      } else { 
        if (selectWasPressed) {
            // Button was RELEASED. Verify if it was a valid short press (< 1 second)
            if (currentMillis - last_interaction_millis < 1000) {
                selectTriggered = true;
                force_update_ui = true;
            }
        }
        selectWasPressed = false;
      }

      // UP Button
      if (digitalRead(BTN_UP_PIN) == LOW) {
        if (!upWasPressed) { 
            upTriggered = true;
            force_update_ui = true; upWasPressed = true;
            if (!selectWasPressed) last_interaction_millis = currentMillis; 
        }
      } else { upWasPressed = false; }

      // DOWN Button
      if (digitalRead(BTN_DOWN_PIN) == LOW) {
        if (!downWasPressed) { 
            downTriggered = true;
            force_update_ui = true; downWasPressed = true;
            if (!selectWasPressed) last_interaction_millis = currentMillis; 
        }
      } else { downWasPressed = false; }

      lastBtnCheckMillis = currentMillis;
    }

    // === INTERCEPTOR: The First-Press Trap Solver ===
    if (is_oled_sleeping && (selectTriggered || upTriggered || downTriggered)) {
        // WAKE UP SEQUENCE
        is_oled_sleeping = false;
        
        // Full hardware re-initialization to recover from hot-plug or power loss.
        // Required because OLED is not soldered — a physical reconnect invalidates the I2C device state.
        u8g2.begin();
        u8g2.setFont(font_8_pixel);
        u8g2.setPowerSave(0);
        
        currentState = STATE_LIVE_VIEW;
        // Return to Home Principle
        force_update_ui = true;
        // CONSUME the button presses so they don't leak into Phase 2 (Menu)
        selectTriggered = false;
        upTriggered = false;
        downTriggered = false;
    }

    // === PHASE 2: UI State Machine ===
    if (currentState == STATE_LIVE_VIEW) {
      if (selectTriggered) {
        // Re-initialize OLED hardware on entering menu to recover lost connection instantly.
        // This handles hot-plug: if the OLED (non-soldered) was disconnected and reattached,
        // begin() re-discovers it via I2C init. setPowerSave(0) alone won't recover a lost device.
        u8g2.begin();
        u8g2.setFont(font_8_pixel);
        u8g2.setPowerSave(0);
        
        currentState = STATE_MENU;
        menuCursor = 0;
        force_update_ui = true;
      }
    } 
    else if (currentState == STATE_MENU) {
      if (upTriggered) {
        menuCursor--;
        if (menuCursor < 0) menuCursor = MENU_ITEMS_COUNT - 1;
      }
      if (downTriggered) {
        menuCursor++;
        if (menuCursor >= MENU_ITEMS_COUNT) menuCursor = 0;
      }
      
      if (selectTriggered) {
        // Handle Menu Actions
        if (menuCursor == 0) {
            // Action: Enter Display Mode Submenu instead of toggling instantly
            currentState = STATE_SUBMENU_DISPLAY;
            displayCursor = auto_off_enabled ? 1 : 0;
            force_update_ui = true;
            last_interaction_millis = currentMillis;
        }
        else if (menuCursor == 1) {
            // Action: Enter Mute Sounds Submenu instead of toggling instantly
            currentState = STATE_SUBMENU_MUTE;
            muteCursor = is_muted ? 1 : 0; // Focus on current system state
            force_update_ui = true;
            last_interaction_millis = currentMillis;
        }
        else if (menuCursor == 2) {
            // Action: Enter SD Manager Submenu & Compute statistics dynamically
            u8g2.clearBuffer();
            u8g2.drawStr(10, 20, "Calculating...");
            u8g2.sendBuffer();
            
            if (sd.card() && sd.vol() && sd.card()->errorCode() == 0) { 
                uint32_t freeClusters = sd.vol()->freeClusterCount();
                uint32_t totalClusters = sd.vol()->clusterCount();
                uint32_t sectorsPerCluster = sd.vol()->sectorsPerCluster();
                
                // Cast to 64-bit unsigned integer to prevent arithmetic overflow on large SD cards (>32GB)
                sd_free_mb = (uint32_t)(((uint64_t)freeClusters * sectorsPerCluster) / 2048);
                // Fixed parametric bandwidth tracking based on 47-Byte packets
                sd_remain_hours = (float)sd_free_mb / MB_PER_HOUR; 
                
                uint32_t sd_total_mb = (uint32_t)(((uint64_t)totalClusters * sectorsPerCluster) / 2048);
                sd_total_gb = (float)sd_total_mb / 1024.0;

                // === High-Speed Filtered Directory Iteration via openNext() ===
                totalFilesCount = 0;
                File rootDir;
                if (rootDir.open("/", O_RDONLY)) {
                    File file;
                    char nameBuf[25];
                    while (file.openNext(&rootDir, O_RDONLY)) {
                        if (!file.isDir()) {
                            file.getName(nameBuf, sizeof(nameBuf));
                            int len = strlen(nameBuf);
                            
                            // Validate .BIN extension first
                            if (len >= 4 && strcasecmp(nameBuf + len - 4, ".BIN") == 0) {
                                // 1. Check for standard Parent logs (e.g., DR_LOG_001.BIN)
                                if (strncmp(nameBuf, "DR_LOG_", 7) == 0) {
                                    totalFilesCount++;
                                } 
                                // FIX ISSUE 8: 2. Check for Recovery logs (e.g., 001001.BIN -> exactly 10 chars)
                                else if (len == 10) {
                                    bool isRecoveryFile = true;
                                    for (int i = 0; i < 6; i++) {
                                        if (!isdigit(nameBuf[i])) {
                                            isRecoveryFile = false;
                                            break;
                                        }
                                    }
                                    if (isRecoveryFile) {
                                        totalFilesCount++;
                                    }
                                }
                            }
                        }
                        file.close();
                    }
                    rootDir.close();
                }
            } else {
                sd_free_mb = 0; sd_remain_hours = 0.0; sd_total_gb = 0.0; totalFilesCount = 0;
            }
            currentState = STATE_SUBMENU_SD_INFO;
            sdMenuCursor = 0; // Reset scroll cursors
            sdScrollOffset = 0;
            force_update_ui = true;
        }
        else if (menuCursor == 3) { 
          // Action: Calibration
          vTaskSuspend(sensorTaskHandle);
          performCalibration();
          print_calibration();
          saveCalibration();
          xQueueReset(dataQueue);

          // Write gap frame (0xAA) to mark calibration event before resuming sensor task.
          // Sensor task is suspended — no race on frame_counter or esp_timer.
          {
              LogFrame gapFrame = {};
              gapFrame.frame_seq = global_frame_counter++;
              gapFrame.timestamp = esp_timer_get_time() - log_time_base;
              gapFrame.event_flag = 0xAA;
              gapFrame.crc = calcCRC16((uint8_t*)&gapFrame, sizeof(LogFrame) - sizeof(gapFrame.crc));
              xQueueSend(dataQueue, &gapFrame, 0);
          }

          vTaskResume(sensorTaskHandle);
          force_update_ui = true; 
          currentState = STATE_LIVE_VIEW;
        } 
        else if (menuCursor == 4) {
          // Action: Exit Menu
          currentState = STATE_LIVE_VIEW;
          force_update_ui = true;
        }
      }
    }
    else if (currentState == STATE_SUBMENU_SD_INFO) {
      // Navigation inside the expanded 8-item SD menu (Indices 0 to 7)
      if (upTriggered) {
        sdMenuCursor--;
        if (sdMenuCursor < 0) sdMenuCursor = 7; // Loop back to 8th item (Back)
        force_update_ui = true;
      }
      if (downTriggered) {
        sdMenuCursor++;
        if (sdMenuCursor > 7) sdMenuCursor = 0; // Loop back to 1st item (Total)
        force_update_ui = true;
      }
      
      // Calculate scroll offset dynamically for 3-line display window
      if (sdMenuCursor < sdScrollOffset) {
        sdScrollOffset = sdMenuCursor;
      } else if (sdMenuCursor >= sdScrollOffset + 3) {
        sdScrollOffset = sdMenuCursor - 2;
      }

      // SELECT Actions mapped correctly after adding the Drops item
      if (selectTriggered) {
        if (sdMenuCursor >= 0 && sdMenuCursor <= 4) {
          // FIX ISSUE 1: Lines 1 to 5 (including Drops) jump safely to Live View
          currentState = STATE_LIVE_VIEW;
          force_update_ui = true;
          last_interaction_millis = currentMillis;
        }
        else if (sdMenuCursor == 5) {
          // Option 6: Create New File -> Enter confirmation trap
          currentState = STATE_CONFIRM_CREATE_FILE;
          confirmCursor = 1; 
          force_update_ui = true;
        }
        else if (sdMenuCursor == 6) {
          // Option 7: Format / Clear -> Enter confirmation trap
          currentState = STATE_CONFIRM_FORMAT;
          confirmCursor = 1; 
          force_update_ui = true;
        }
        else if (sdMenuCursor == 7) {
          // Option 8: Back to Menu -> Safe escape to main menu
          currentState = STATE_MENU;
          menuCursor = 2; 
          force_update_ui = true;
        }
      }
    }
    else if (currentState == STATE_SUBMENU_DISPLAY) {
      if (upTriggered || downTriggered) {
        displayCursor = (displayCursor == 0) ? 1 : 0;
        force_update_ui = true;
      }
      
      if (selectTriggered) {
        if (displayCursor == 0) {
          auto_off_enabled = false; // Always ON
        } else {
          auto_off_enabled = true; // Auto Off 20s
        }
        
        currentState = STATE_LIVE_VIEW; // Direct redirect to Home
        force_update_ui = true;
        last_interaction_millis = currentMillis;
      }
    }
    else if (currentState == STATE_SUBMENU_MUTE) {
      // Navigation inside Mute Menu
      if (upTriggered || downTriggered) {
        muteCursor = (muteCursor == 0) ? 1 : 0;
        force_update_ui = true;
      }
      
      // Selection inside Mute Menu
      if (selectTriggered) {
        if (muteCursor == 0) {
          is_muted = false; // Sounds: ON
        } else {
          is_muted = true; // Sounds: MUTED
        }
        
        currentState = STATE_LIVE_VIEW; // Direct redirect to Home
        force_update_ui = true;
        last_interaction_millis = currentMillis;
      }
    } 
    else if (currentState == STATE_CONFIRM_CREATE_FILE) {
      // Confirmation trap for creating a new file
      if (upTriggered || downTriggered) {
        confirmCursor = (confirmCursor == 0) ? 1 : 0;
        force_update_ui = true;
      }
      if (selectTriggered) {
        if (confirmCursor == 1) {
          currentState = STATE_SUBMENU_SD_INFO;
          force_update_ui = true;
        } else {
          // 1. Safely sync and close current file to protect FAT table
          if (logFile) {
              logFile.sync();
              logFile.close();
          }
          
          // 2. Safely increment the file ceiling tracker to guarantee a new name
          if (max_log_id < 999) {
              max_log_id++; 
              global_log_id = max_log_id;
          } else {
              global_log_id = 999;
          }
          
          // 3. Reset recovery and sequence trackers safely
          global_recovery_id = 1;
          
          portENTER_CRITICAL(&frameCounterMux);
          global_frame_counter = 0;
          log_time_base = esp_timer_get_time();
          portEXIT_CRITICAL(&frameCounterMux);
          
          // Purge queued frames to prevent old data from leaking into the new file
          xQueueReset(dataQueue);

          // Defer file creation: prompt user for GPS start before opening new file
          gps_pending_file_creation = true;
          gps_start_needed = true;
          currentState = STATE_GPS_PROMPT_START;
          force_update_ui = true;
        }
      }
    }
    else if (currentState == STATE_CONFIRM_FORMAT) {
      // Confirmation trap for formatting/deleting logs
      if (upTriggered || downTriggered) {
        confirmCursor = (confirmCursor == 0) ? 1 : 0;
        force_update_ui = true;
      }
      if (selectTriggered) {
        if (confirmCursor == 1) {
          currentState = STATE_SUBMENU_SD_INFO;
          force_update_ui = true;
        } else {
          // Safe Execution: Suspend Core 0 sensor task during massive SD wiping operations
          vTaskSuspend(sensorTaskHandle);
          // Execute Smart Delete Protocol
          u8g2.clearBuffer();
          u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth("Clearing Logs...")) / 2, 20, "Clearing Logs...");
          u8g2.sendBuffer();
          
          if (logFile) logFile.close();
          char delFilename[20];
          // Enhanced sweeping protocol to remove BOTH Parent logs and Orphaned recovery fragments
          for (int i = 1; i <= 999; i++) {
            // 1. Delete the Parent Log File
            snprintf(delFilename, sizeof(delFilename), "DR_LOG_%03d.BIN", i);
            if (sd.exists(delFilename)) {
              sd.remove(delFilename);
            }
            
            // 2. Nested Sweep: Sequentially search and destroy all child recovery chunks [i][j].BIN
            int j = 1;
            while (true) {
              snprintf(delFilename, sizeof(delFilename), "%03d%03d.BIN", i, j);
              if (sd.exists(delFilename)) {
                sd.remove(delFilename);
                j++;
              } else {
                break;
              }
              
              // Yield to feed WDT between recovery file deletes
              vTaskDelay(pdMS_TO_TICKS(2));
            }
            
            // Yield to feed WDT on every parent iteration
            vTaskDelay(pdMS_TO_TICKS(3));
          }
          
          strcpy(current_log_filename, "DR_LOG_001.BIN");
          global_log_id = 1;
          global_recovery_id = 1;
          max_log_id = 1; // Reset the ceiling index tracker after card formatting
          
          // Thread-safe initialization of sequence tracker on memory wipe
          portENTER_CRITICAL(&frameCounterMux);
          global_frame_counter = 0;
          log_time_base = esp_timer_get_time();
          portEXIT_CRITICAL(&frameCounterMux);
          
          // Purge the queue after memory wipe to prevent leaking stale data
          xQueueReset(dataQueue);
          
          // Resume Core 0 task safely after system configuration is restored
          vTaskResume(sensorTaskHandle);
          
          u8g2.clearBuffer();
          u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth("All Logs Cleared!")) / 2, 20, "All Logs Cleared!");
          u8g2.sendBuffer();
          vTaskDelay(pdMS_TO_TICKS(800));
          
          // Defer file creation: prompt user for GPS start before opening new file
          gps_pending_file_creation = true;
          gps_start_needed = true;
          currentState = STATE_GPS_PROMPT_START;
          force_update_ui = true;
        }
      }
    }

    // === GPS START/END PROMPT STATES ===
    else if (currentState == STATE_GPS_PROMPT_START || currentState == STATE_GPS_PROMPT_END) {
      if (upTriggered || downTriggered) {
        gps_prompt_cursor = (gps_prompt_cursor == 0) ? 1 : 0;
        force_update_ui = true;
      }
      if (selectTriggered) {
        if (gps_prompt_cursor == 1) {
          // NO: Skip GPS pairing, clear the needed flag
          if (currentState == STATE_GPS_PROMPT_START) {
              gps_start_needed = false;
              phone_gps.has_start = false; // Discard any stale GPS data
          } else {
              gps_end_needed = false;
          }
          // If file creation was pending (Create New File / Format), create file now
          if (gps_pending_file_creation) {
              char filename[20];
              snprintf(filename, sizeof(filename), "DR_LOG_%03d.BIN", global_log_id);
              strcpy(current_log_filename, filename);
              logFile = sd.open(current_log_filename, FILE_WRITE);
              if (!logFile) {
                  sd_critical_error = true;
              } else {
                  writeLogFileHeader(logFile);
              }
              gps_pending_file_creation = false;
              u8g2.clearBuffer();
              char flashBuf[25];
              snprintf(flashBuf, sizeof(flashBuf), "Created: LOG_%03d", global_log_id);
              u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(flashBuf)) / 2, 20, flashBuf);
              u8g2.sendBuffer();
              if (!is_muted) {
                  digitalWrite(BUZZER_PIN, HIGH);
                  vTaskDelay(pdMS_TO_TICKS(100));
                  digitalWrite(BUZZER_PIN, LOW);
              } else {
                  vTaskDelay(pdMS_TO_TICKS(600));
              }
          }
          currentState = STATE_LIVE_VIEW;
          force_update_ui = true;
        } else {
          // YES: Start WiFi AP and wait for phone GPS data
          gps_data_received = false;
          currentState = STATE_GPS_WAITING;
          force_update_ui = true;
          // Display "Starting AP..." before blocking WiFi init
          u8g2.clearBuffer();
          u8g2.drawStr(5, 12, "Starting AP...");
          u8g2.drawStr(5, 26, "Wait for WiFi");
          u8g2.sendBuffer();
          startGPSAP();
        }
      }
    }
    else if (currentState == STATE_GPS_WAITING) {
      // Poll the HTTP server for incoming GPS data
      gps_server.handleClient();
      gps_dns.processNextRequest();
      
      // Check if phone sent GPS data
      if (gps_data_received) {
          stopGPSAP();
          gps_data_received = false;
          if (gps_start_needed) {
              gps_start_needed = false;
          }
          if (gps_end_needed) {
              gps_end_needed = false;
          }
          // If file creation was pending, create file before writing GPS frame
          if (gps_pending_file_creation) {
              char filename[20];
              snprintf(filename, sizeof(filename), "DR_LOG_%03d.BIN", global_log_id);
              strcpy(current_log_filename, filename);
              logFile = sd.open(current_log_filename, FILE_WRITE);
              if (!logFile) {
                  sd_critical_error = true;
              } else {
                  writeLogFileHeader(logFile);
              }
              gps_pending_file_creation = false;
          }
          // Write 0xBB GPS start frame (file existed or was just created above)
          if (phone_gps.has_start && logFile) {
              writeGPSFrame(0xBB, phone_gps.start_lat, phone_gps.start_lon, phone_gps.start_alt, phone_gps.start_time);
              logFile.sync();
              phone_gps.has_start = false;
          }
          currentState = STATE_LIVE_VIEW;
          force_update_ui = true;
          last_interaction_millis = currentMillis;
          // Show confirmation briefly
          u8g2.clearBuffer();
          u8g2.drawStr(5, 12, "GPS Received!");
          u8g2.drawStr(5, 26, "Disconnect Phone");
          u8g2.sendBuffer();
          vTaskDelay(pdMS_TO_TICKS(2000));
      }
      
      // Allow user to exit via SELECT + confirmation
      if (selectTriggered) {
          currentState = STATE_GPS_CONFIRM_EXIT;
          gps_prompt_cursor = 1; // Default NO
          force_update_ui = true;
      }
    }
    else if (currentState == STATE_GPS_CONFIRM_EXIT) {
      if (upTriggered || downTriggered) {
        gps_prompt_cursor = (gps_prompt_cursor == 0) ? 1 : 0;
        force_update_ui = true;
      }
      if (selectTriggered) {
        if (gps_prompt_cursor == 1) {
          // NO: Return to waiting
          currentState = STATE_GPS_WAITING;
          force_update_ui = true;
        } else {
          // YES: Exit GPS pairing
          stopGPSAP();
          gps_data_received = false;
          if (gps_start_needed) gps_start_needed = false;
          if (gps_end_needed) gps_end_needed = false;
          phone_gps.has_start = false; // Discard any received GPS data
          // If file creation was pending, create file now (no GPS data)
          if (gps_pending_file_creation) {
              char filename[20];
              snprintf(filename, sizeof(filename), "DR_LOG_%03d.BIN", global_log_id);
              strcpy(current_log_filename, filename);
              logFile = sd.open(current_log_filename, FILE_WRITE);
              if (!logFile) {
                  sd_critical_error = true;
              } else {
                  writeLogFileHeader(logFile);
              }
              gps_pending_file_creation = false;
              u8g2.clearBuffer();
              char flashBuf[25];
              snprintf(flashBuf, sizeof(flashBuf), "Created: LOG_%03d", global_log_id);
              u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(flashBuf)) / 2, 20, flashBuf);
              u8g2.sendBuffer();
              if (!is_muted) {
                  digitalWrite(BUZZER_PIN, HIGH);
                  vTaskDelay(pdMS_TO_TICKS(100));
                  digitalWrite(BUZZER_PIN, LOW);
              } else {
                  vTaskDelay(pdMS_TO_TICKS(600));
              }
          }
          currentState = STATE_LIVE_VIEW;
          force_update_ui = true;
          last_interaction_millis = currentMillis;
        }
      }
    }

    // === PHASE 3: Pull Data from Queue & SD Logging ===
    if (xQueueReceive(dataQueue, &receivedFrame, pdMS_TO_TICKS(10)) == pdPASS) {
      if (logFile) {
        size_t bytesWritten = logFile.write((uint8_t*)&receivedFrame, sizeof(LogFrame));
        
        // Error Detection: If bytes written do not match the expected struct size, hardware link is broken
        if (bytesWritten != sizeof(LogFrame)) {
            sd_critical_error = true; // Trigger immediate dynamic recovery interceptor
        }
      } else {
          sd_critical_error = true; // File pointer lost, enter error mode
      }
      
      if (currentMillis - lastFlushMillis > 5000) { 
        if (logFile) {
            if (!logFile.sync()) {
                sd_critical_error = true;
            }
        }
        lastFlushMillis = currentMillis;
      }
    } 

      // === PHASE 4: Graphics Rendering ===
      if (!is_oled_sleeping) {
          if ((currentMillis - lastDisplayMillis > update_rate_oled) || force_update_ui) {
            force_update_ui = false;
            u8g2.clearBuffer();
            
            if (currentState == STATE_LIVE_VIEW) {
              char buf[32];
              // Fixed Syntax & Compressed Layout for 128x32 OLED (Two-Column Grid Optimization)
              // Row 1 (Y=8): Quaternions W & X
              snprintf(buf, sizeof(buf), "Qw:%.2f", receivedFrame.payload.imu.q[0]); u8g2.drawStr(0, 8, buf);
              snprintf(buf, sizeof(buf), "Qx:%.2f", receivedFrame.payload.imu.q[1]); u8g2.drawStr(64, 8, buf);
              
              // Row 2 (Y=19): Quaternions Y & Z
              snprintf(buf, sizeof(buf), "Qy:%.2f", receivedFrame.payload.imu.q[2]); u8g2.drawStr(0, 19, buf);
              snprintf(buf, sizeof(buf), "Qz:%.2f", receivedFrame.payload.imu.q[3]); u8g2.drawStr(64, 19, buf);
              
              // Row 3 (Y=31): Real-time run duration & Non-blocking MPU Temperature
              uint32_t total_secs = receivedFrame.timestamp / 1000000; 
              uint32_t mins = total_secs / 60;
              uint32_t secs = total_secs % 60;
              snprintf(buf, sizeof(buf), "T:%03lu:%02lu", mins, secs); u8g2.drawStr(0, 31, buf);
              snprintf(buf, sizeof(buf), "T:%.1fC", receivedFrame.payload.imu.temp); u8g2.drawStr(76, 31, buf);
            } 
            else if (currentState == STATE_MENU) {
              u8g2.drawStr(0, 8, "--- MENU ---");
              u8g2.drawStr(10, 20, menuItems[menuCursor]);
              u8g2.drawStr(0, 20, ">");
            }
            
            else if (currentState == STATE_SUBMENU_SD_INFO) {
              char lines[8][32];
              snprintf(lines[0], 32, "1.Total: %.1f GB", sd_total_gb); 
              snprintf(lines[1], 32, "2.Free: %lu MB", sd_free_mb);
              snprintf(lines[2], 32, "3.Time: %.1f Hrs", sd_remain_hours);
              snprintf(lines[3], 32, "4.Files Count: %u", totalFilesCount);
              snprintf(lines[4], 32, "5.Drops: %lu", dropped_frames_count);
              snprintf(lines[5], 32, "6.Create New File");
              snprintf(lines[6], 32, "7.Format / Clear");
              snprintf(lines[7], 32, "8.Back to Menu");
              
              for (int i = 0; i < 3; i++) {
                int lineIndex = sdScrollOffset + i;
                int yPos = 10 + (i * 10); 
                u8g2.drawStr(12, yPos, lines[lineIndex]);
                
                if (lineIndex == sdMenuCursor) {
                  u8g2.drawStr(2, yPos, ">");
                }
              }
            }
            else if (currentState == STATE_CONFIRM_CREATE_FILE) {
              u8g2.drawStr(0, 8, "Create New File?");
              u8g2.drawStr(20, 22, "YES");
              u8g2.drawStr(20, 32, "NO");
              u8g2.drawStr(8, (confirmCursor == 0) ? 22 : 32, ">");
            }
            else if (currentState == STATE_CONFIRM_FORMAT) {
              u8g2.drawStr(0, 8, "Clear All Logs?");
              u8g2.drawStr(20, 22, "YES");
              u8g2.drawStr(20, 32, "NO");
              u8g2.drawStr(8, (confirmCursor == 0) ? 22 : 32, ">");
            }
            else if (currentState == STATE_SUBMENU_DISPLAY) {
              u8g2.drawStr(0, 8, "Disp Mode:");
              u8g2.drawStr(15, 20, "Always ON");
              u8g2.drawStr(15, 30, "Auto Off 20s");
              u8g2.drawStr(3, (displayCursor == 0) ? 20 : 30, ">");
            }
            else if (currentState == STATE_SUBMENU_MUTE) {
              u8g2.drawStr(0, 8, "Sound Opt:");
              u8g2.drawStr(15, 20, "Sounds: ON");
              u8g2.drawStr(15, 30, "Sounds: MUTED");
              u8g2.drawStr(3, (muteCursor == 0) ? 20 : 30, ">");
            }
            else if (currentState == STATE_GPS_PROMPT_START) {
              u8g2.drawStr(0, 8, "Get GPS Start?");
              u8g2.drawStr(20, 22, "YES");
              u8g2.drawStr(20, 32, "NO");
              u8g2.drawStr(8, (gps_prompt_cursor == 0) ? 22 : 32, ">");
            }
            else if (currentState == STATE_GPS_PROMPT_END) {
              u8g2.drawStr(0, 8, "Get GPS End?");
              u8g2.drawStr(20, 22, "YES");
              u8g2.drawStr(20, 32, "NO");
              u8g2.drawStr(8, (gps_prompt_cursor == 0) ? 22 : 32, ">");
            }
            else if (currentState == STATE_GPS_WAITING) {
              u8g2.drawStr(0, 8, "Waiting for Phone");
              u8g2.drawStr(0, 18, GPS_AP_SSID);
              u8g2.drawStr(0, 30, "192.168.4.1");
            }
            else if (currentState == STATE_GPS_CONFIRM_EXIT) {
              u8g2.drawStr(0, 8, "Exit GPS Pairing?");
              u8g2.drawStr(20, 22, "YES");
              u8g2.drawStr(20, 32, "NO");
              u8g2.drawStr(8, (gps_prompt_cursor == 0) ? 22 : 32, ">");
            }
            
            u8g2.sendBuffer();
            lastDisplayMillis = currentMillis;
          }
      }
      // Keep OLED awake during GPS pairing — don't sleep and don't change state
      if (auto_off_enabled && !is_oled_sleeping && currentState != STATE_GPS_WAITING
          && currentState != STATE_GPS_PROMPT_START && currentState != STATE_GPS_PROMPT_END
          && currentState != STATE_GPS_CONFIRM_EXIT
          && (currentMillis - last_interaction_millis > OLED_SLEEP_TIMEOUT_MS)) {
          is_oled_sleeping = true;
          u8g2.setPowerSave(1); 
          currentState = STATE_LIVE_VIEW; 
          force_update_ui = true;
      }
  } 
} 

/*////////////////////////////setup////////////////////////////*/
void setup() 
{ 
  Serial.begin(bud_rate);
  // Initialize Hardware I2C for MPU9250 with explicit pins for ESP32-S3
  Wire.begin(I2C_MPU_SDA, I2C_MPU_SCL); 
  Wire.setClock(400000); // Boost I2C to 400kHz for maximum IMU read speed
	Wire.setTimeout(50); //  Prevents Hardware I2C Bus Hang if wire is pulled

  // Initialize Buttons with internal pull-ups
  pinMode(BTN_SELECT_PIN, INPUT_PULLUP);
  pinMode(BTN_UP_PIN, INPUT_PULLUP);
  pinMode(BTN_DOWN_PIN, INPUT_PULLUP);
  pinMode(BTN_TAG_PIN, INPUT_PULLUP);

  // Initialize OLED
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(font_10_pixel);
  u8g2.drawStr(15, 25, "<< Boot up >>");
  u8g2.sendBuffer();
  delay(1000);
  u8g2.setFont(font_8_pixel);

  // Initialize Notification Hardware (Standard LED & Buzzer) FIRST!
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  pinMode(LED_RED_PIN, OUTPUT);
  digitalWrite(LED_RED_PIN, LOW);
  pinMode(LED_GREEN_PIN, OUTPUT);
  digitalWrite(LED_GREEN_PIN, LOW);

  // Initialize SD Card with Critical Error Feedback (Blocking Loop)
  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  // Trap the system here indefinitely until the SD card is successfully mounted
  while (!sd.begin(SD_CS_PIN, SD_SCK_MHZ(SPI_FREQ_MHZ))) {
    Serial.println("CRITICAL: SD Card Mount Failed! Waiting for SD...");
    // Render Error Message on OLED (Center-Aligned)
    u8g2.clearBuffer();
    const char* line1 = "SD Card Error!";
    const char* line2 = "Insert/Check SD";
    int x1 = (u8g2.getDisplayWidth() - u8g2.getStrWidth(line1)) / 2;
    int x2 = (u8g2.getDisplayWidth() - u8g2.getStrWidth(line2)) / 2;
    u8g2.drawStr(x1, 12, line1);
    u8g2.drawStr(x2, 28, line2);
    u8g2.sendBuffer();
    // Hardware Alarm: Red LED + Beeps using global parameter
    digitalWrite(LED_RED_PIN, HIGH);
    for(int i = 0; i < 2; i++) {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(ALARM_BEEP_MS);
      digitalWrite(BUZZER_PIN, LOW);
      delay(ALARM_BEEP_MS);
    }
    delay(Recheck_SD_beforeBoot_MS);
    digitalWrite(LED_RED_PIN, LOW); 
  } 
  
  // === Dynamic Sequential Logging Architecture ===
  // Render "Scanning SD..." while the ESP computes existing files
  u8g2.clearBuffer();
  u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth("Scanning SD...")) / 2, 20, "Scanning SD...");
  u8g2.sendBuffer();

  char filename[20];
  global_log_id = 1;
  // Scan the SD root directory to find the next available sequential number
  while (true) {
    snprintf(filename, sizeof(filename), "DR_LOG_%03d.BIN", global_log_id);
    if (!sd.exists(filename)) break;
    global_log_id++;
    if (global_log_id > 999) {
      Serial.println("ERROR: Log file limit reached (999). Overwriting DR_LOG_999.BIN");
      global_log_id = 999;
      break;
    }
  }
  max_log_id = global_log_id;       // Store the baseline for O(1) instantaneous dynamic creations
  global_recovery_id = 1;           // Reset recovery counter 
  global_frame_counter = 0;         // Reset frame counter
  log_time_base = esp_timer_get_time(); // Set baseline for the very first boot log file
  strcpy(current_log_filename, filename); 
  logFile = sd.open(current_log_filename, FILE_WRITE);

  if (logFile) {
    writeLogFileHeader(logFile);
    Serial.print("Success: Opened new log file -> ");
    Serial.println(filename);
    
    // Render dynamic SD statistics before mission starts
    u8g2.clearBuffer();
    char scanBuf[25];
    snprintf(scanBuf, sizeof(scanBuf), "Found: %d Logs", global_log_id - 1);
    u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(scanBuf)) / 2, 12, scanBuf);
    
    snprintf(scanBuf, sizeof(scanBuf), "Next: LOG_%03d", global_log_id);
    u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(scanBuf)) / 2, 28, scanBuf);
    u8g2.sendBuffer();
    delay(2000); // 2-second delay to let the user read the info
    
  } else {
    Serial.println("CRITICAL ERROR: Failed to create sequential log file!");
  }

  MPU9250Setting setting;
  configureMPUSettings(setting);

  while (!mpu.setup(MPU9250_IMU_ADDRESS, setting)) {
    Serial.println("CRITICAL: MPU connection failed. Check Wiring!");
    u8g2.clearBuffer();
    u8g2.drawStr(15, 15, "MPU9250 Failed!");
    u8g2.drawStr(20, 28, "Check Wiring");
    u8g2.sendBuffer();
    
    // Fast SOS pattern for Boot-time MPU Error
    for(int i = 0; i < 15; i++) {
        digitalWrite(LED_RED_PIN, HIGH);
        digitalWrite(BUZZER_PIN, HIGH);
        delay(100);
        digitalWrite(LED_RED_PIN, LOW);
        digitalWrite(BUZZER_PIN, LOW);
        delay(50);
    }
    delay(1000); // Brief pause before retrying
  }						
  mpu.setMagneticDeclination(MAGNETIC_DECLINATION); 
  mpu.selectFilter(QuatFilterSel::MPU9250_filter_algorithm); 
  mpu.setFilterIterations(MPU9250_filter_iterations);
  // Initialize EEPROM for ESP32 (Allocate 128 bytes)
  EEPROM.begin(128);
  // Load calibration from EEPROM on startup
  Serial.println("Loading calibration from EEPROM...");
  loadCalibration();
  print_calibration();
  u8g2.setFont(font_5_pixel);
  // =========================================================================
  // PSRAM ALLOCATION & QUEUE CREATION
  // =========================================================================
  // 1. Allocate the 1.6MB data buffer strictly in the external PSRAM
  queueBuffer = (uint8_t *)heap_caps_malloc(QUEUE_LENGTH * sizeof(LogFrame), MALLOC_CAP_SPIRAM);
  
  // 2. Allocate the Queue Manager struct strictly in internal 8-bit RAM (for scheduler speed)
  queueStruct = (StaticQueue_t *)heap_caps_malloc(sizeof(StaticQueue_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

  if (queueBuffer == NULL || queueStruct == NULL) {
      Serial.println("CRITICAL: Failed to allocate PSRAM for Queue! System Halted.");
      u8g2.clearBuffer();
      u8g2.drawStr(10, 15, "PSRAM ERROR!");
      u8g2.sendBuffer();
      while(1); // Trap the system here if PSRAM is defective or disabled in Arduino settings
  }

  // 3. Create the Static Queue using the allocated memory
  dataQueue = xQueueCreateStatic(QUEUE_LENGTH, sizeof(LogFrame), queueBuffer, queueStruct);
  Serial.print("SUCCESS: Buffer allocated in PSRAM. Total size (MB): ");
  Serial.println(PSRAM_BUFFER_SIZE_MB);

  // Pin Sensor Task to Core 0 (Highest Priority)
  xTaskCreatePinnedToCore(sensorTask, "SensorTask", 8192, NULL, 2, &sensorTaskHandle, 0);
  // Pin Logging Task to Core 1
  xTaskCreatePinnedToCore(loggingTask, "LoggingTask", 8192, NULL, 1, &loggingTaskHandle, 1);
}

/*////////////////////////////loop////////////////////////////*/
void loop() 
{ 
  // Main loop is intentionally left empty. FreeRTOS tasks now manage the entire system architecture.
  vTaskDelete(NULL);
}

// =========================================================================
// CALIBRATION FUNCTIONS 
// =========================================================================

void performCalibration() {
  u8g2.clearBuffer();
  u8g2.drawStr(0, 15, "Accel/Gyro Cal");
  u8g2.drawStr(0, 25, "Keep Still 5s");
  u8g2.sendBuffer();
  mpu.verbose(true);
  vTaskDelay(pdMS_TO_TICKS(5000));
  mpu.calibrateAccelGyro();
  u8g2.clearBuffer();
  u8g2.drawStr(0, 15, "Mag Cal");
  u8g2.drawStr(0, 25, "Figure 8 - 5s");
  u8g2.sendBuffer();
  vTaskDelay(pdMS_TO_TICKS(5000));
  mpu.calibrateMag();
  mpu.verbose(false);
}

void print_calibration() {
  Serial.println("< calibration parameters >");
  Serial.println("accel bias [g]: ");
  Serial.print(mpu.getAccBiasX() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY); Serial.print(", ");
  Serial.print(mpu.getAccBiasY() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY); Serial.print(", ");
  Serial.println(mpu.getAccBiasZ() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY);
  Serial.println("gyro bias [deg/s]: ");
  Serial.print(mpu.getGyroBiasX() / (float)MPU9250::CALIB_GYRO_SENSITIVITY);
  Serial.print(", ");
  Serial.print(mpu.getGyroBiasY() / (float)MPU9250::CALIB_GYRO_SENSITIVITY); Serial.print(", ");
  Serial.println(mpu.getGyroBiasZ() / (float)MPU9250::CALIB_GYRO_SENSITIVITY);
  Serial.println("mag bias [mG]: ");
  Serial.print(mpu.getMagBiasX()); Serial.print(", ");
  Serial.print(mpu.getMagBiasY()); Serial.print(", ");
  Serial.println(mpu.getMagBiasZ());
  Serial.println("mag scale []: ");
  Serial.print(mpu.getMagScaleX());
  Serial.print(", ");
  Serial.print(mpu.getMagScaleY()); Serial.print(", ");
  Serial.println(mpu.getMagScaleZ());
  
  u8g2.clearBuffer();
  char buf[32];
  snprintf(buf, sizeof(buf), "Acc: %.2f,%.2f,%.2f",
           mpu.getAccBiasX() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY,
           mpu.getAccBiasY() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY,
           mpu.getAccBiasZ() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY);
  u8g2.drawStr(0, 15, buf);
  snprintf(buf, sizeof(buf), "Gyr: %.1f,%.1f,%.1f",
           mpu.getGyroBiasX() / (float)MPU9250::CALIB_GYRO_SENSITIVITY,
           mpu.getGyroBiasY() / (float)MPU9250::CALIB_GYRO_SENSITIVITY,
           mpu.getGyroBiasZ() / (float)MPU9250::CALIB_GYRO_SENSITIVITY);
  u8g2.drawStr(0, 25, buf);
  u8g2.sendBuffer();
  vTaskDelay(pdMS_TO_TICKS(2000));
}

void saveCalibration() {
  // Write the confirmation sentinel first to validate future boots
  uint16_t magic = EEPROM_MAGIC_NUMBER;
  EEPROM.put(EEPROM_MAGIC_ADDR, magic);
  
  // FIX ISSUE 2: Offset the data allocation by the size of the magic number
  int addr = sizeof(uint16_t);
  
  EEPROM.put(addr, mpu.getAccBiasX()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getAccBiasY()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getAccBiasZ()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getGyroBiasX()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getGyroBiasY()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getGyroBiasZ()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getMagBiasX()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getMagBiasY()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getMagBiasZ()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getMagScaleX()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getMagScaleY()); addr += sizeof(float);
  EEPROM.put(addr, mpu.getMagScaleZ()); addr += sizeof(float);
  
  // CRC over magic + all floats to detect silent corruption
  uint8_t calData[50];
  for (int i = 0; i < 50; i++) calData[i] = EEPROM.read(i);
  uint16_t crc = calcCRC16(calData, 50);
  EEPROM.put(50, crc);
  EEPROM.commit();
}

void loadCalibration() {
  uint16_t loadedMagic = 0;
  EEPROM.get(EEPROM_MAGIC_ADDR, loadedMagic);
  
  // Defensive Check: Validate if EEPROM has been calibrated before
  if (loadedMagic != EEPROM_MAGIC_NUMBER) {
    Serial.println("WARNING: No valid calibration found in EEPROM. Using factory defaults.");
    u8g2.clearBuffer();
    u8g2.drawStr(10, 12, "No Valid Cal");
    u8g2.drawStr(5, 28, "Using Defaults");
    u8g2.sendBuffer();
    digitalWrite(LED_RED_PIN, HIGH);
    for(int i = 0; i < 2; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(ALARM_BEEP_MS);
        digitalWrite(BUZZER_PIN, LOW);
        delay(ALARM_BEEP_MS);
    }
    digitalWrite(LED_RED_PIN, LOW);
    mpu.setAccBias(0.0, 0.0, 0.0);
    mpu.setGyroBias(0.0, 0.0, 0.0);
    mpu.setMagBias(0.0, 0.0, 0.0);
    mpu.setMagScale(1.0, 1.0, 1.0);
    return; // Abort loading to protect the Madgwick filter from corrupt floats
  }

  uint16_t storedCrc;
  EEPROM.get(50, storedCrc);
  uint8_t calData[50];
  for (int i = 0; i < 50; i++) calData[i] = EEPROM.read(i);
  if (calcCRC16(calData, 50) != storedCrc) {
    Serial.println("WARNING: Calibration CRC mismatch! Data corrupted. Using factory defaults.");
    u8g2.clearBuffer();
    u8g2.drawStr(10, 12, "Cal CRC Error");
    u8g2.drawStr(5, 28, "Using Defaults");
    u8g2.sendBuffer();
    digitalWrite(LED_RED_PIN, HIGH);
    for(int i = 0; i < 2; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(ALARM_BEEP_MS);
        digitalWrite(BUZZER_PIN, LOW);
        delay(ALARM_BEEP_MS);
    }
    digitalWrite(LED_RED_PIN, LOW);
    mpu.setAccBias(0.0, 0.0, 0.0);
    mpu.setGyroBias(0.0, 0.0, 0.0);
    mpu.setMagBias(0.0, 0.0, 0.0);
    mpu.setMagScale(1.0, 1.0, 1.0);
    return;
  }

  Serial.println("Valid calibration found. Loading from EEPROM...");
  u8g2.clearBuffer();
  u8g2.drawStr(10, 12, "Valid Cal Found");
  u8g2.drawStr(5, 28, "Loading EEPROM");
  u8g2.sendBuffer();
  vTaskDelay(pdMS_TO_TICKS(1500));

  int addr = sizeof(uint16_t);
  float accBiasX, accBiasY, accBiasZ;
  float gyroBiasX, gyroBiasY, gyroBiasZ;
  float magBiasX, magBiasY, magBiasZ;
  float magScaleX, magScaleY, magScaleZ;

  EEPROM.get(addr, accBiasX); addr += sizeof(float);
  EEPROM.get(addr, accBiasY); addr += sizeof(float);
  EEPROM.get(addr, accBiasZ); addr += sizeof(float);
  EEPROM.get(addr, gyroBiasX); addr += sizeof(float);
  EEPROM.get(addr, gyroBiasY); addr += sizeof(float);
  EEPROM.get(addr, gyroBiasZ); addr += sizeof(float);
  EEPROM.get(addr, magBiasX); addr += sizeof(float);
  EEPROM.get(addr, magBiasY); addr += sizeof(float);
  EEPROM.get(addr, magBiasZ); addr += sizeof(float);
  EEPROM.get(addr, magScaleX); addr += sizeof(float);
  EEPROM.get(addr, magScaleY); addr += sizeof(float);
  EEPROM.get(addr, magScaleZ); addr += sizeof(float);

  mpu.setAccBias(accBiasX, accBiasY, accBiasZ);
  mpu.setGyroBias(gyroBiasX, gyroBiasY, gyroBiasZ);
  mpu.setMagBias(magBiasX, magBiasY, magBiasZ);
  mpu.setMagScale(magScaleX, magScaleY, magScaleZ);
  
  Serial.println("Loaded calibration values from EEPROM:");
  Serial.print("Acc Bias X: "); Serial.println(accBiasX);
  Serial.print("Acc Bias Y: "); Serial.println(accBiasY);
  Serial.print("Acc Bias Z: "); Serial.println(accBiasZ);
  Serial.print("Gyro Bias X: "); Serial.println(gyroBiasX);
  Serial.print("Gyro Bias Y: "); Serial.println(gyroBiasY);
  Serial.print("Gyro Bias Z: "); Serial.println(gyroBiasZ);
  Serial.print("Mag Bias X: "); Serial.println(magBiasX);
  Serial.print("Mag Bias Y: "); Serial.println(magBiasY);
  Serial.print("Mag Bias Z: "); Serial.println(magBiasZ);
  Serial.print("Mag Scale X: "); Serial.println(magScaleX);
  Serial.print("Mag Scale Y: "); Serial.println(magScaleY);
  Serial.print("Mag Scale Z: "); Serial.println(magScaleZ);
  Serial.println("STATUS: Calibration successfully applied to internal filter.");
}