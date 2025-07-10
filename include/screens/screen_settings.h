/******************************************************************************
 * @file    screen_settings.h
 * @author  Thomas Zoldowski
 * @date    May 10, 2025
 * @brief   Header for the Motor Control and Status Screen UI.
 *
 * Layout:
 *  - Toggle switches for Blower 1, Blower 2, and Pump
 *  - Status indicators (e.g. ON/OFF, active runtime)
 *  - Optional: RPM readout if using Hall sensor for feedback
 ******************************************************************************/

#ifndef SCREEN_SETTINGS_H
#define SCREEN_SETTINGS_H

#include <lvgl.h>

lv_obj_t* create_settings_screen();

void setup_ui_tab2();
void setup_ui_tab3();
void setup_ui_tab1();

void settings_init_from_config(void);

bool check_pin();

void logout_cb(lv_event_t *e);

void security_timeout_check();

/**
 *  A helper you can directly use as an LV_EVENT_CLICKED callback
 */
void lock_overlay_cb(lv_event_t *e);

/** @brief  Returns the user-configured blower ON time (seconds). */
uint16_t getBlowerOnTime();

/** @brief  Returns the user-configured pump ON time (seconds). */
uint16_t getPumpOnTime();

uint16_t getActivationInterval();

uint16_t getCameraDelay();

uint16_t getSendInterval();

uint16_t getTempHighThreshold(int sensor_id);

uint16_t getHumLowThreshold(int sensor_id);

extern unsigned long last_activity;

#endif /* SCREEN_MOTORS_H */
