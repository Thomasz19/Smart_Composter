/******************************************************************************
 * @file    screen_manual.cpp
 * @author  Thomas Zoldowski
 * @date    May 10, 2025
 * @brief   Implementation of the Motor Control and Status Screen UI.
 ******************************************************************************/

#include "screens/screen_manual.h"
#include "screens/screen_sensors.h"
#include "ui_manager.h"


static lv_obj_t* manual_screen = nullptr;

void create_manual_control_screen() {
    manual_screen = lv_obj_create(NULL);
    lv_scr_load(manual_screen);

     

    // Background color (optional)
    lv_obj_set_style_bg_color(manual_screen, lv_color_hex(0xc0c9d9), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(manual_screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(manual_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(manual_screen, LV_DIR_NONE);

    // Header Bar 
    lv_obj_t *header = lv_obj_create(manual_screen);
    lv_obj_set_size(header, lv_pct(100), 80);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x42649f), 0); 
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(header, LV_DIR_NONE); // Prevent any scrolling

    // Title
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Manual Override");
    lv_obj_center(title);
    lv_obj_set_style_text_color(title, lv_color_hex(0xc0c9d9), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_48, 0);

    // create drop down menu
    create_global_dropdown(manual_screen);
    
    // // Create arrow button in top-left
    // lv_obj_t *arrow_btn = lv_btn_create(manual_screen); 
    // lv_obj_set_size(arrow_btn, 80, 80);
    // lv_obj_align(arrow_btn, LV_ALIGN_TOP_LEFT, 0, 0);  // 10 px from top and left
    // lv_obj_add_event_cb(arrow_btn, [](lv_event_t * e) {
    //     create_sensor_screen();  
    // }, LV_EVENT_CLICKED, NULL);

    // // Add arrow icon to the button
    // lv_obj_t *arrow_icon = lv_label_create(arrow_btn);
    // lv_label_set_text(arrow_icon, LV_SYMBOL_LEFT);  // or LV_SYMBOL_LEFT if you're going "back"
    // lv_obj_center(arrow_icon);
    // lv_obj_set_style_bg_color(arrow_icon, lv_color_hex(0x103978), 0); 

}