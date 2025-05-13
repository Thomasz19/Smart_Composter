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
 *  - arduino_gigaDisplay_GFX
 *  - arduinoGraphics
 *  - Adafruit GFX Library
 *  - LVGL
 ******************************************************************************/

 //Includes --------------------------------------------------------------------
#include <Arduino.h>
#include <lvgl.h>
#include "ui_manager.h"
#include "config.h"
#include "screens/screen_home.h"
#include "Arduino_H7_Video.h"
#include "Arduino_GigaDisplayTouch.h"

Arduino_H7_Video  Display(800, 480, GigaDisplayShield);
Arduino_GigaDisplayTouch  TouchDetector;

//Prototype Functions ---------------------------------------------------------
void global_input_event_cb(lv_event_t * e);


// Global Variables -----------------------------------------------------------
unsigned long glast_input_time = 0;
const unsigned long gINACTIVITY_TIMEOUT = 10000; // 1 minute



void setup() {
  Serial.begin(SERIAL_BAUDRATE);

  Display.begin();
  TouchDetector.begin();

  create_home_screen();
}

void loop() {
  lv_timer_handler();

  // Inactivity timeout check
  if (millis() - glast_input_time > gINACTIVITY_TIMEOUT) {
    create_home_screen();
    glast_input_time = millis(); // Prevent repeated reloads
  }

}

void global_input_event_cb(lv_event_t * e) {
  glast_input_time = millis();  // Reset inactivity timer
}
