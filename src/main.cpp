/******************************************************************************
* @file    main.cpp
* @author  Thomas Zoldowski
* @date    May 10, 2025
* @brief   Entry point for Smart Composter firmware.
*
* This file initializes the Arduino GIGA R1 WiFi and GIGA Display Shield,
* sets up the LVGL graphics environment, and handles screen transitions
* for monitoring and controlling the smart composting system.
*
* Features include:
*  - Sensor data display (temperature, humidity, CO2, O2)
*  - Warning and status notifications
*  - Pump/Blower control interface
*  - History logging view
*
* This firmware is designed to run on the Arduino GIGA R1 WiFi with the GIGA Display Shield.
* It uses the LVGL graphics library for rendering the user interface.
******************************************************************************/

// Ardunio
#include <Arduino.h>

// Mbed OS
#include <mbed.h>

// Display Driver
#include <lvgl.h>

// C Library
#include <stdio.h>
#include <string.h>
#include <errno.h>

// LittleFS (Mbed)
#include "QSPIFBlockDevice.h"
#include "MBRBlockDevice.h"
#include <LittleFileSystem.h>

// Local Files
#include "ui_manager.h"
#include "config.h"

// SCreens
#include "screens/screen_home.h"
#include "screens/screen_sensors.h"
#include "screens/screen_warnings.h"
#include "screens/screen_diagnostics.h"
#include "screens/screen_settings.h"

// LCD
#include "Arduino_H7_Video.h"
#include "Arduino_GigaDisplayTouch.h"

// Sensors
#include "logic/sensor_manager.h"

// Network
#include "logic/actuator_manager.h"
#include "settings_storage.h"

#define CHUNK_LINES 7

// Screen buffers
// These buffers are used to store the screen content for LVGL rendering
// Adjust the size according to your display resolution and needs
// For example, for a 800x480 display with 7 lines of content:
static lv_color_t buf1[800 * CHUNK_LINES];
static lv_color_t buf2[800 * CHUNK_LINES];  // optional second buffer

// screen timeout
unsigned long last_activity=0;

Arduino_H7_Video  Display(800, 480, GigaDisplayShield);
Arduino_GigaDisplayTouch  TouchDetector;

// ================= Prototype Functions =================
void global_input_event_cb(lv_event_t * e);
void Init_LittleFS(void);
void my_print(lv_log_level_t level, const char * buf);


// ================= Global Variables =================
unsigned long glast_input_time = 0;
unsigned long input_time = 0; // For serial output

static uint32_t lastSensorUpdate    = 0;
static uint32_t lastLEDUpdate       = 0;
static uint32_t lastSecurityCheck   = 0;
static uint32_t lastActuatorSchedule= 0;
static uint32_t camera_delay = 0; // For camera delay

constexpr uint32_t SENSOR_INTERVAL_MS      = 1000;
constexpr uint32_t LED_INTERVAL_MS         = 250;
constexpr uint32_t SECURITY_CHECK_MS       = 500;
constexpr uint32_t ACTUATOR_SCHEDULE_MS    = 1000; // hourly

// Instantiate the raw flash driver on its default pins
QSPIFBlockDevice root(QSPI_SO0, QSPI_SO1, QSPI_SO2, QSPI_SO3,  QSPI_SCK, QSPI_CS, QSPIF_POLARITY_MODE_1, 40000000);
mbed::MBRBlockDevice user_data(&root, 3);
mbed::LittleFileSystem user_data_fs("user");

// ================= WATCH DOG =================
mbed::Watchdog &watchdog = mbed::Watchdog::get_instance();

int selected_index = -1;

// ================= INIT SETUP =================
void setup() {
  Serial.begin(SERIAL_BAUDRATE);
  //Serial2.begin(9600);

  delay(2000);
  Serial.println("DEBUG: Serial.println working");
  
  // Initialize the display and touch controller
  Display.begin();
  TouchDetector.begin();
  lv_init();

  // Initialize the display driver
  lv_disp_t *disp = lv_display_get_default();
  lv_display_set_buffers(disp,buf1, buf2, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

  // Mount LittleFS (or reformat if running for the first time)
  Init_LittleFS();

  // Load or create defaults
  loadConfig();

  // Initialize your UI modules, including screen_manual’s static globals
  settings_init_from_config();
  Serial.println("5...................");


  Serial.println("10...................");
  // Initialize sensors
  sensor_manager_init();
  Serial.println("20...................");
  // Init Pins
  Limit_Switch_Init();
  LED_Init();
  initActuatorScheduler();

  // Init Screens
  Serial.println("30...................");

  Serial.println("40...................");;

  Serial.println("50...................");

  Serial.println("60...................");
  // Setup watchdog
  watchdog.start(2000); // Enable the watchdog and configure the duration of the timeout (ms).

  lv_log_register_print_cb(my_print);

  ui_init(); // Initialize the UI screens

  // Init Diagnostic screen
  handle_screen_selection("Home");
  //update_footer_status(FOOTER_OK);

  Serial.println("100...................");

}



// ================= MAIN LOOP =================
void loop() {
  lv_timer_handler();

  uint32_t now = millis();

  // Poll sensor data at defined interval
  if (now - lastSensorUpdate >= SENSOR_UPDATE_INTERVAL_MS) {
    sensor_manager_update();
    // Diagnostics screen updates
    if (is_diagnostics_screen_active()) { // Diagnostics screen is active
      update_diagnostics_screen();
    }
    // Sensor screen updates
    else if (selected_index == 0) { // Sensor screen is active
      update_sensor_screen();
      Serial.println("Sensor screen updated");
    }
    lastSensorUpdate = now;  // Reset the last sensor update time
  }

  // LED status update (2 Hz)
  if (now - lastLEDUpdate >= LED_INTERVAL_MS) {
    LED_Update();
    Limit_Switch_update();
    lastLEDUpdate = now;
  }
 
  // Hourly actuators and limit switches
  if (now - lastActuatorSchedule >= ACTUATOR_SCHEDULE_MS) {
    scheduleHourlyActuators();
    lastActuatorSchedule = now;
  }
    

  // Security PIN timeout (0.5 Hz)
  if (now - lastSecurityCheck >= SECURITY_CHECK_MS) {
    security_timeout_check();
    lastSecurityCheck = now;
  }

  // Inactivity timeout check
  if (now - glast_input_time > INACTIVITY_TIMEOUT_MS) {
    handle_screen_selection("Home"); // Go back to home screen after timeout
    glast_input_time = now; // Prevent repeated reloads
  }

  // Timeout for security PIN
  // Send Data to Raspberry Pi every 10 seconds
  if (now - input_time > getSendInterval()* 1000) {
    SensorDataToSerial();
    
    input_time = now;  // Reset the input time
  }
  CameraDelayToSerial();

  // Keep the watchdog alive
  watchdog.kick();
}

// ================= FUNCTIONS =================
void global_input_event_cb(lv_event_t * e) {
  glast_input_time = millis();  // Reset inactivity timer
  last_activity = millis();
}

// ================= LITTLEFS SETUP =================
void Init_LittleFS(void){
  // 1) Initialize root and the user_data partition
  int err = root.init();
  if (err) {
    Serial.println("root.init() failed");
    while (true) {}
  }
  if (user_data.init() != 0) {
    Serial.println("user_data.init() failed");
    while (true) {}
  }
  //  Serial.println("Mounting the filesystem…");
  err = user_data_fs.mount(&user_data);
  if (err) {
    Serial.println("Mount failed—reformatting…");
    int fmtErr = user_data_fs.reformat(&user_data);
    if (fmtErr) {
      Serial.print("Reformat failed: ");
      Serial.print(strerror(-fmtErr));
      Serial.print(" (");
      Serial.print(fmtErr);
      Serial.println(")");
      while (true) {}
    }
    // Now that LittleFS has been formatted, mount again:
    if (user_data_fs.mount(&user_data) != 0) {
      Serial.println("Mount after reformat still failed!");
      while (true) {}
    }
  }
  Serial.println("LittleFS mounted OK.");
 }

void my_print(lv_log_level_t level, const char * buf){
  Serial.println(buf);
}
