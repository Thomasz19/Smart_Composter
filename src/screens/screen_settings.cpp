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

    // Header Bar 
    lv_obj_t *header = lv_obj_create(settings_screen);
    lv_obj_set_size(header, lv_pct(100), 80);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x42649f), 0); 
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(header, LV_DIR_NONE); // Prevent any scrolling

    // Title
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Settings");
    lv_obj_center(title);
    lv_obj_set_style_text_color(title, lv_color_hex(0xc0c9d9), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_48, 0);

    // create drop down menu
    create_global_dropdown(settings_screen);
    
}