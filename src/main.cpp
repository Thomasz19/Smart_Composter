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

 //Includes --------------------------------------------------------------------
#include <Arduino.h>
#include <lvgl.h>
#include <stdio.h>
#include <mbed.h>

#include "fault_handler.h"
#include "ui_manager.h"
#include "config.h"

#include "screens/screen_home.h"
#include "screens/screen_sensors.h"
#include "screens/screen_warnings.h"

#include "Arduino_H7_Video.h"
#include "Arduino_GigaDisplayTouch.h"

#include "logic/sensor_manager.h"

Arduino_H7_Video  Display(800, 480, GigaDisplayShield);
Arduino_GigaDisplayTouch  TouchDetector;

// ================= Prototype Functions =================
void global_input_event_cb(lv_event_t * e);
static unsigned long last_sensor_update = 0;

// ================= Global Variables =================
unsigned long glast_input_time = 0;

// ================= WATCH DOG =================
mbed::Watchdog &watchdog = mbed::Watchdog::get_instance();

// ================= INIT SETUP =================
void setup() {
  Serial.begin(SERIAL_BAUDRATE);
  Serial.println("Serial.println working");

  // Initialize sensors
  sensor_manager_init();

  Display.begin();
  TouchDetector.begin();
  
  create_warnings_screen();
  create_home_screen();
  add_warning("Test");

  // Setup watchdog
  watchdog.start(2000); // Enable the watchdog and configure the duration of the timeout (ms).

}

// ================= MAIN LOOP =================
void loop() {
  lv_timer_handler();

  // Poll sensor data at defined interval
  if (millis() - last_sensor_update >= SENSOR_UPDATE_INTERVAL_MS) {
    sensor_manager_update();
    last_sensor_update = millis();
  }

  // Inactivity timeout check
  if (millis() - glast_input_time > INACTIVITY_TIMEOUT_MS) {
    create_home_screen();
    glast_input_time = millis(); // Prevent repeated reloads
  }

  update_footer_status(FOOTER_OK);
  watchdog.kick();
  delay(5);
}

// ================= FUNCTIONS =================
void global_input_event_cb(lv_event_t * e) {
  glast_input_time = millis();  // Reset inactivity timer
}
