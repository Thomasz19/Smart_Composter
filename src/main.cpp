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
*  - Sensor data display (temperature, humidity, CO₂, O₂)
*  - Warning and status notifications
*  - Pump/Blower control interface
*  - History logging view
*
* Dependencies:
*  - arduino_gigaDisplay
*  - arduino_gigaDisplayTouch
*  - Adafruit GFX Library
*  - LVGL
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
#include <NewPing.h>

// Network
#include "logic/network_manager.h"
#include "logic/actuator_manager.h"
#include "settings_storage.h"

// Screen Pointers
extern lv_obj_t* diag_screen;
extern lv_obj_t* sensor_screen;
//extern lv_obj_t* limit_switch_screen;

Arduino_H7_Video  Display(800, 480, GigaDisplayShield);
Arduino_GigaDisplayTouch  TouchDetector;

// ================= Prototype Functions =================
void global_input_event_cb(lv_event_t * e);
void Init_LittleFS(void);
void my_print(lv_log_level_t level, const char * buf);
// ================= Global Variables =================
unsigned long glast_input_time = 0;
static unsigned long last_sensor_update = 0;

// Instantiate the raw flash driver on its default pins
QSPIFBlockDevice root(QSPI_SO0, QSPI_SO1, QSPI_SO2, QSPI_SO3,  QSPI_SCK, QSPI_CS, QSPIF_POLARITY_MODE_1, 40000000);
mbed::MBRBlockDevice user_data(&root, 3);
mbed::LittleFileSystem user_data_fs("user");


// ================= WATCH DOG =================
mbed::Watchdog &watchdog = mbed::Watchdog::get_instance();


// ================= INIT SETUP =================
void setup() {
  Serial.begin(SERIAL_BAUDRATE);
  Serial2.begin(9600);

  delay(1000);
  Serial.println("DEBUG: Serial.println working");
  
  Display.begin();
  TouchDetector.begin();
  lv_init();

  // Mount LittleFS (or reformat if running for the first time)
  Init_LittleFS();

  // Load or create defaults
  loadConfig();

  // Initialize your UI modules, including screen_manual’s static globals
  settings_init_from_config();

  // Init Diagnostic sceeen
  create_diagnostics_screen();

  // Initialize sensors
  sensor_manager_init();

  // Init Pins
  Limit_Switch_Init();
  LED_Init();

  // Init Screens
  create_sensor_screen();
  create_warnings_screen();
  create_home_screen();

  // Setup watchdog
  watchdog.start(2000); // Enable the watchdog and configure the duration of the timeout (ms).
  lv_log_register_print_cb(my_print);
}



// ================= MAIN LOOP =================
void loop() {
  lv_timer_handler();
  // Poll sensor data at defined interval
  if (millis() - last_sensor_update >= SENSOR_UPDATE_INTERVAL_MS) {
    lv_obj_t* active = lv_screen_active();  // get the currently displayed screen

    // Diagnostics screen updates
    if (active == diag_screen) {
      sensor_manager_update();
      update_diagnostics_screen();
    }
    // Sensor screen updates
    else if (active == sensor_screen) {
      sensor_manager_update();
      update_sensor_screen();
     // sonar_update_and_fill_bar();
    }
    // Limit-switch screen updates
    // else if (active == limit_switch_screen) {
    //   Limit_Switch_update();
    // }

    LED_Update();
    
    last_sensor_update = millis();
  }

  // Inactivity timeout check
  if (millis() - glast_input_time > INACTIVITY_TIMEOUT_MS) {
    create_home_screen();
    glast_input_time = millis(); // Prevent repeated reloads
  }

 // update_footer_status(FOOTER_OK);
  watchdog.kick();
  //delay(5);
}

// ================= FUNCTIONS =================
void global_input_event_cb(lv_event_t * e) {
  glast_input_time = millis();  // Reset inactivity timer
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
