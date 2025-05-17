/******************************************************************************
 * @file    screen_manual.cpp
 * @author  Thomas Zoldowski
 * @date    May 10, 2025
 * @brief   Implementation of the Motor Control and Status Screen UI.
 ******************************************************************************/

#include "screens/screen_manual.h"
#include "screens/screen_sensors.h"
#include "screens/screen_settings.h"
#include "ui_manager.h"

#include <lvgl.h>

static lv_obj_t* settings_screen = nullptr;

void create_settings_screen() {
    settings_screen = lv_obj_create(NULL);
    lv_scr_load(settings_screen);

    

    // Background color (optional)
    lv_obj_set_style_bg_color(settings_screen, lv_color_hex(0xc0c9d9), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(settings_screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(settings_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(settings_screen, LV_DIR_NONE);

    // create header
    create_header(settings_screen, "Settings");
    
}