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
#include <Arduino.h>
#include <string.h>

extern void global_input_event_cb(lv_event_t * e);

static lv_style_t dropdown_list_style;

static lv_obj_t *dropdown = nullptr;   // The global dropdown instance
static int       selected_index=-1;  // default to Sensor Overview
static lv_obj_t *current_screen = nullptr;

/*
Function: Drop down selection even handler
*/
static void dropdown_event_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target_obj(e);

    // Open drop down menu and Add styling
    if (code == LV_EVENT_CLICKED) {
        lv_dropdown_open(dropdown);
        Serial.println("[GDL] opening dropdown"); // Debug
        lv_obj_t *list = lv_dropdown_get_list(obj);
        if (list) {
            // Apply custom style only when the list is available
            lv_obj_add_style(list, &dropdown_list_style, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_max_height(list, LV_SIZE_CONTENT, 0);
            lv_obj_set_scroll_dir(list, LV_DIR_NONE);
            lv_obj_set_height(list, 71 * 5);
        }
    }

    // Change screen
    if(code == LV_EVENT_VALUE_CHANGED) {
        char buf[32];
        lv_dropdown_get_selected_str(obj, buf, sizeof(buf));
        handle_screen_selection(buf);
    }
}

/*
Function: Create the drop down menu button with its items
*/
void create_global_dropdown(lv_obj_t *parent) {
    
    Serial.println("Create global dropdown"); // Debug

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

    lv_obj_set_style_text_font(dropdown, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // drop down listed items
    lv_style_init(&dropdown_list_style);
    lv_style_set_text_font(&dropdown_list_style, &lv_font_montserrat_48);
    lv_style_set_bg_color(&dropdown_list_style, lv_color_hex(0xc0c9d9));
    // Set horizontal gradient from dark blue to light blue
    lv_style_set_bg_color(&dropdown_list_style, lv_color_hex(0x42649F));
    lv_style_set_bg_grad_color(&dropdown_list_style, lv_color_hex(0xA3B7E4));
    lv_style_set_bg_grad_dir(&dropdown_list_style, LV_GRAD_DIR_HOR);
    
    // Click on menu button handler
    lv_obj_add_event_cb(dropdown, dropdown_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(dropdown, dropdown_event_handler, LV_EVENT_CLICKED, NULL);
}

/*
Function: Handles the selection of each item in the drop down menu
*/
void handle_screen_selection(const char *selected_label) {
    Serial.println("[GDL] changing Screens..."); // Debug
    int new_index = -1;
    

    if      (!strcmp(selected_label, "Sensor Overview"))   new_index = 0;
    else if (!strcmp(selected_label, "Manual Control"))    new_index = 1;
    else if (!strcmp(selected_label, "Warnings"))          new_index = 2;
    else if (!strcmp(selected_label, "History"))           new_index = 3;
    else if (!strcmp(selected_label, "Settings"))          new_index = 4;
    else if (!strcmp(selected_label, "Home"))              new_index = 5;
    else new_index = 0;
    Serial.print("[LVGL] ");
    Serial.println(new_index);

    if(new_index == selected_index) return; // no change

    lv_obj_t* old_screen = current_screen;

    // if it’s changed, actually switch
    if (new_index >= 0) {
       
        switch (new_index) {
            case 0: current_screen = create_sensor_screen();         break;
            case 1: current_screen = create_manual_control_screen(); break;
            case 2: current_screen = create_warnings_screen();       break;
            case 3: current_screen = create_history_screen();        break;
            case 4: current_screen = create_settings_screen();       break;
            default: current_screen = create_sensor_screen();        break;
        }

        if(current_screen) {
            lv_screen_load(current_screen);
            selected_index = new_index;
        }

        if(old_screen) {
            lv_obj_del_async(old_screen);
        }
    }
    if (dropdown) {
      lv_dropdown_set_selected(dropdown, selected_index);
      Serial.println("[GDL] closing dropdown"); // Debug
      lv_dropdown_close(dropdown);  // close the list to avoid re-opening during its event
    }
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);

    Serial.print(F("[LVGL] Used: "));
    Serial.print(mon.total_size - mon.free_size);
    Serial.print(F(" bytes | Free: "));
    Serial.print(mon.free_size);
    Serial.print(F(" bytes | Largest Free: "));
    Serial.print(mon.free_biggest_size);
    Serial.print(F(" bytes | Fragmentation: "));
    Serial.print(mon.frag_pct);
    Serial.println(F("%"));
}

void create_header(lv_obj_t *parent, const char *title_txt) {
    Serial.println("[gH] creating header"); // Debug
    // Header Bar
    lv_obj_t *header = lv_obj_create(parent);
    lv_obj_set_size(header, lv_pct(100), 80);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x42649f), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER,    LV_PART_MAIN);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(header, LV_DIR_NONE);

    // Title Label
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, title_txt);
    lv_obj_center(title);
    lv_obj_set_style_text_color(title, lv_color_hex(0xc0c9d9), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_48,  0);

    // Global navigation dropdown (icon-only)
    Serial.println("[gH] creating GDL"); // Debug
    create_global_dropdown(parent);
}
//EOF