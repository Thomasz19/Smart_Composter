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


LV_IMG_DECLARE(GVSU_Logo);

extern void global_input_event_cb(lv_event_t * e);

static void screen_touch_cb(lv_event_t * e) {
  create_sensor_screen();  // Switch to sensor screen on any touch
}

void create_home_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_scr_load(screen);

    // Set black background
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x0032A0), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    // Create logo image object
    lv_obj_t *logo = lv_img_create(screen);
    lv_img_set_src(logo, &GVSU_Logo);
    lv_obj_align(logo, LV_ALIGN_CENTER, 0, 0);

    // Register the global input event callback
    lv_obj_add_event_cb(screen, global_input_event_cb, LV_EVENT_ALL, NULL);

    // Add a full-screen transparent object for capturing touch
    lv_obj_t *touch_area = lv_obj_create(screen);
    lv_obj_remove_style_all(touch_area);  // Make it invisible
    lv_obj_set_size(touch_area, lv_pct(100), lv_pct(100));
    lv_obj_add_event_cb(touch_area, screen_touch_cb, LV_EVENT_CLICKED, NULL);
}

////lv_color_hex(0x0032A0)