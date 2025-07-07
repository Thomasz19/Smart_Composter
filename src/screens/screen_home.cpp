/******************************************************************************
 * @file    screen_home.cpp
 * @author  Thomas Zoldowski
 * @date    May 10, 2025
 * @brief   Implementation of the Home Screen UI.
 ******************************************************************************/

#include <Arduino.h>
#include "screens/screen_home.h"
#include "screens/screen_sensors.h"
#include <lvgl.h>
#include "GVSU_Logo.h"
#include "ui_manager.h"

LV_IMG_DECLARE(GVSU_Logo);

extern void global_input_event_cb(lv_event_t * e);


static lv_obj_t* home_screen = nullptr;

static void screen_touch_cb(lv_event_t * e) {
  Serial.println("▶ screen_touch_cb fired");
  handle_screen_selection("Sensor Overview");  // Set the next screen to Sensors
  Serial.println("◀ screen_touch_cb returned");
}

lv_obj_t* create_home_screen() {

    home_screen = lv_obj_create(NULL);
    
    // Set black background
    lv_obj_set_style_bg_color(home_screen, lv_color_hex(0x0032A0), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(home_screen, LV_OPA_COVER, LV_PART_MAIN);

    // Create logo image object
    lv_obj_t *logo = lv_img_create(home_screen);
    lv_img_set_src(logo, &GVSU_Logo);
    lv_obj_align(logo, LV_ALIGN_CENTER, 0, 0);

    // Register the global input event callback
    lv_obj_add_event_cb(home_screen, global_input_event_cb, LV_EVENT_ALL, NULL);

    // Add a full-screen transparent object for capturing touch
    lv_obj_t *touch_area = lv_obj_create(home_screen);
    lv_obj_remove_style_all(touch_area);  // Make it invisible
    lv_obj_set_size(touch_area, lv_pct(100), lv_pct(100));
    lv_obj_add_event_cb(touch_area, screen_touch_cb, LV_EVENT_CLICKED, NULL);

    return home_screen;
}

////lv_color_hex(0x0032A0)