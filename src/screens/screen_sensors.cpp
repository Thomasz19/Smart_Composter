/******************************************************************************
 * @file    screen_sensors.cpp
 * @author  Thomas Zoldowski
 * @date    May 10, 2025
 * @brief   Implementation of the Sensor Data Screen UI.
 ******************************************************************************/


#include "screens/screen_sensors.h"
#include <lvgl.h>

extern void global_input_event_cb(lv_event_t * e);

void create_sensor_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    // Set background color to black
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    lv_scr_load(screen);  // Load this screen

    // Register the global input event callback
    lv_obj_add_event_cb(screen, global_input_event_cb, LV_EVENT_ALL, NULL);

    // Add a centered label for now
    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text(label, "Sensor Screen");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}