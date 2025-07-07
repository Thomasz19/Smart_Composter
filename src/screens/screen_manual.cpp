/******************************************************************************
 * @file    screen_manual.cpp
 * @author  Thomas Zoldowski
 * @date    May 10, 2025
 * @brief   Implementation of the Motor Control and Status Screen UI.
 ******************************************************************************/

#include "screens/screen_manual.h"
#include "screens/screen_sensors.h"
#include "screens/screen_warnings.h"
#include "ui_manager.h"
#include <string.h>
#include <Arduino.h>
#include "screens/screen_settings.h"

static lv_obj_t* manual_screen = nullptr;
lv_obj_t *logout_btn = nullptr; // Logout button handle

// LED handles for each motor
lv_obj_t *led[3];
// Forward for keypad callback
//static void show_keypad_cb(lv_event_t * e);



lv_obj_t* create_manual_control_screen() {
    manual_screen = lv_obj_create(NULL);
    

    // Debug
    Serial.println("Create manual screen");

    // Background color 
    lv_obj_set_style_bg_color(manual_screen, lv_color_hex(0xc0c9d9), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(manual_screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(manual_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(manual_screen, LV_DIR_NONE);
    
    // create header
    create_header(manual_screen, "Manual Override");
    //==================================================================
    // ===== Motor List =====
    // A vertical flex container beneath the header
    lv_obj_t *motor_cont = lv_obj_create(manual_screen);
    lv_obj_set_size(motor_cont, lv_pct(50), 330);
    lv_obj_align(motor_cont, LV_ALIGN_TOP_LEFT, 0, 75);
    lv_obj_set_flex_flow(motor_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_opa(motor_cont, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(motor_cont, 0, LV_PART_MAIN);
    lv_obj_clear_flag(motor_cont, LV_OBJ_FLAG_SCROLLABLE);

    // For each motor: LED + label
    const char *names[3] = {"Blower 1", "Blower 2", "Pump"};
    for(int i = 0; i < 3; i++) {
        lv_obj_t *row = lv_obj_create(motor_cont);
        lv_obj_set_size(row, lv_pct(100), 100);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row,
        LV_FLEX_ALIGN_START,  // justify children along the row (left)
        LV_FLEX_ALIGN_CENTER, // align children vertically in the middle
        LV_FLEX_ALIGN_CENTER  // align the row itself if it's in a parent flex
        );
        lv_obj_set_style_pad_column(row, 40, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        // LED indicator
        led[i] = lv_led_create(row);
        lv_obj_set_size(led[i], 80, 80);
        lv_led_off(led[i]);  // default off; turn on when running

        // Motor name
        lv_obj_t *lbl = lv_label_create(row);
        lv_label_set_text(lbl, names[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_48, 0);
        lv_obj_set_style_text_color(lbl, lv_color_black(), 0);
    }

    // ===== Activate Button Controls =====
    lv_obj_t *act_btn = lv_btn_create(manual_screen);
    lv_obj_set_size(act_btn, 350, 300);
    // align to right, middle of motor container
    lv_obj_align(act_btn, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_bg_color(act_btn, lv_color_hex(0x0e43b7), 0); 
    lv_obj_add_event_cb(act_btn, lock_overlay_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *act_lbl = lv_label_create(act_btn);
    lv_label_set_text(act_lbl, "ACTIVATE\nBUTTON\nCONTROLS");
    lv_obj_set_style_text_align(act_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(act_lbl);
    lv_obj_set_style_text_font(act_lbl, &lv_font_montserrat_48, 0);
    

    // ===== Logout Button =====
    logout_btn = lv_btn_create(manual_screen);
    lv_obj_set_size(logout_btn, 150, 60);
    lv_obj_align(logout_btn, LV_ALIGN_TOP_RIGHT, -20, 10);
    lv_obj_set_style_bg_color(logout_btn, lv_color_hex(0xff4d4d), 0); // red button
    lv_obj_add_event_cb(logout_btn, logout_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(logout_btn, LV_OBJ_FLAG_HIDDEN);
    if (check_pin()) {
        lv_obj_clear_flag(logout_btn, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_t *logout_lbl = lv_label_create(logout_btn);
    lv_label_set_text(logout_lbl, "Logout");
    lv_obj_center(logout_lbl);
    lv_obj_set_style_text_font(logout_lbl, &lv_font_montserrat_40, 0);


    create_footer(manual_screen);
    return manual_screen;
}

void updateManualScreenLEDs(bool pumpActive, int blowState) {
    if (!manual_screen) return;

    // pump LED
    if (pumpActive)      lv_led_on(led[2]);
    else                 lv_led_off(led[2]);

    // blower1 LED
    if (blowState == 1) lv_led_on(led[0]);
    else                         lv_led_off(led[0]);

    // blower2 LED
    if (blowState == 2) lv_led_on(led[1]);
    else                         lv_led_off(led[1]);

    if (check_pin() && logout_btn) {
      // unlocked → make sure it’s visible
      lv_obj_clear_flag(logout_btn, LV_OBJ_FLAG_HIDDEN);
    } else {
      // locked → hide it
      lv_obj_add_flag(logout_btn, LV_OBJ_FLAG_HIDDEN);
    }

}