/******************************************************************************
 * @file    screen_history.cpp
 * @author  Thomas Zoldowski
 * @date    May 10, 2025
 * @brief   Implementation of the Historical Data Screen UI.
 ******************************************************************************/

#include "screens/screen_history.h"
#include "ui_manager.h"

#include <lvgl.h>

static lv_obj_t* history_screen = nullptr;

void create_history_screen() {
    history_screen = lv_obj_create(NULL);
    lv_scr_load(history_screen);

    

    // Background color (optional)
    lv_obj_set_style_bg_color(history_screen, lv_color_hex(0xc0c9d9), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(history_screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(history_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(history_screen, LV_DIR_NONE);

    // create header
    create_header(history_screen, "History");
}