/******************************************************************************
 * @file    ui_manager.cpp
 * @author  Thomas Zoldowski
 * @date    May 10, 2025
 * @brief   UI manager implementation for Smart Composter screen navigation.
 *
 * This file implements screen switching logic using LVGL. It loads different
 * screens dynamically based on the current screen ID and user interactions.
 ******************************************************************************/

#include "ui_manager.h"
#include "screens/screen_home.h"
#include "screens/screen_sensors.h"
#include "screens/screen_warnings.h"
#include "screens/screen_motors.h"
#include "screens/screen_history.h"
#include "lvgl.h"

void ui_init(void) {
    lv_obj_t *screen = lv_scr_act();  // Get the active screen
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    create_home_screen(); // Default startup screen
}

void ui_switch_to(ScreenID screen) {
    switch (screen) {
        case SCREEN_HOME:
            create_home_screen();
            break;
        case SCREEN_SENSORS:
            create_sensor_screen();
            break;
        case SCREEN_WARNINGS:
            create_warnings_screen();
            break;
        case SCREEN_MOTORS:
            create_motor_screen();
            break;
        case SCREEN_HISTORY:
            create_history_screen();
            break;
        default:
            create_home_screen(); // Fallback
            break;
    }
}

