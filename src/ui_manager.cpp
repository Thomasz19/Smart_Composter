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
#include "screens/screen_manual.h"
#include "screens/screen_history.h"
#include "screens/screen_settings.h"

#include <string.h>

//static lv_obj_t *dropdown = nullptr;
extern void global_input_event_cb(lv_event_t * e);

static lv_style_t dropdown_list_style;
int selected_index = 0;
static lv_obj_t *dropdown = nullptr;   // The global dropdown instance


static void dropdown_event_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target_obj(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
        char buf[32];
        lv_dropdown_get_selected_str(obj, buf, sizeof(buf));
        handle_screen_selection(buf);
    }
}

void create_global_dropdown(lv_obj_t *parent) {
    dropdown = lv_dropdown_create(parent);
    lv_obj_set_size(dropdown, 80, 76);  // width: 150px, height: 40px
    lv_obj_align(dropdown, LV_ALIGN_TOP_LEFT, 2, 2);
    lv_dropdown_set_symbol(dropdown, LV_SYMBOL_LIST);
    //lv_dropdown_set_max_height(dropdown, 400);
    lv_dropdown_set_options_static(dropdown,
    "Sensor Overview\n"
    "Manual Control\n"
    "Warnings\n"
    "History\n"
    "Settings"
    );

    lv_obj_set_style_bg_color(dropdown, lv_color_hex(0x42649f), 0);
    lv_obj_set_style_bg_opa(dropdown, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(dropdown, lv_color_white(), 0);
    lv_obj_set_style_border_width(dropdown, 0, 0);  // No border

    // Hide the selected option text 
    lv_dropdown_set_selected(dropdown, 0);
    lv_dropdown_set_text(dropdown, "");

    lv_obj_add_event_cb(dropdown, dropdown_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_set_style_text_font(dropdown, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // drop down listed items
    lv_style_init(&dropdown_list_style);
    lv_style_set_text_font(&dropdown_list_style, &lv_font_montserrat_48);
    lv_style_set_bg_color(&dropdown_list_style, lv_color_hex(0xc0c9d9));
    // Set horizontal gradient from dark blue to light blue
    lv_style_set_bg_color(&dropdown_list_style, lv_color_hex(0x42649F));
    lv_style_set_bg_grad_color(&dropdown_list_style, lv_color_hex(0xA3B7E4));
    lv_style_set_bg_grad_dir(&dropdown_list_style, LV_GRAD_DIR_HOR);
    lv_obj_t * list = lv_dropdown_get_list(dropdown); /* Get list */
    lv_dropdown_open(dropdown);  // Force list to be created (must be open at least once)

    if (list) {
        lv_obj_add_style(list, &dropdown_list_style, LV_PART_MAIN | LV_STATE_DEFAULT);
        // Prevent scrolling
        lv_obj_set_style_max_height(list, LV_SIZE_CONTENT, 0); // Prevent internal height limits
        lv_obj_set_scroll_dir(list, LV_DIR_NONE);              // Disable scrolling
        lv_obj_set_height(list, 50 * 5);  
    }

    lv_dropdown_close(dropdown);  // Optional: close after styling
    
    
}

void handle_screen_selection(const char *selected_label) {
    if (strcmp(selected_label, "Sensor Overview") == 0) {
        create_sensor_screen();
        selected_index = 0;
    } else if (strcmp(selected_label, "Manual Control") == 0) {
        create_manual_control_screen();
        selected_index = 1;
    } else if (strcmp(selected_label, "Warnings") == 0) {
        create_warnings_screen();
        selected_index = 2;
    } else if (strcmp(selected_label, "History") == 0) {
        create_history_screen();
        selected_index = 3;
    } else if (strcmp(selected_label, "Settings") == 0) {
        create_settings_screen();
        selected_index = 4;
    }
    lv_dropdown_set_selected(dropdown, selected_index);

}

