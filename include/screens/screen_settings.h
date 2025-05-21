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

void create_settings_screen();
void setup_ui_tab2(lv_obj_t *tab_2);
void setup_ui_tab3(lv_obj_t *tab_3);
void setup_ui_tab1(lv_obj_t *tab_1);

#endif /* SCREEN_MOTORS_H */
