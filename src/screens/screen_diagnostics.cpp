/******************************************************************************
 * @file    screen_diagnostics.cpp
 * @author  Thomas Zoldowski
 * @date    May 29, 2025
 * @brief   Implementation of the Diagnostics Screen UI.
 ******************************************************************************/

#include "config.h"
#include <Arduino.h>
#include "screens/screen_diagnostics.h"
#include "screens/screen_warnings.h"
#include "logic/sensor_manager.h"
#include "logic/sensor_manager.h"
#include "ui_manager.h"

// Screen and label handles
static lv_obj_t* diag_screen = NULL;
static lv_obj_t* label_sensor_status[3];
static lv_obj_t* lbl;
static lv_obj_t* label_status_mux;

lv_obj_t* create_diagnostics_screen(void) {

    diag_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(diag_screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(diag_screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(diag_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(diag_screen, LV_DIR_NONE);

    create_header(diag_screen, "Diagnostics");

    // Container for status rows
    lv_obj_t* cont = lv_obj_create(diag_screen);
    lv_obj_set_size(cont, lv_pct(80), 330);
    lv_obj_align(cont, LV_ALIGN_TOP_LEFT, 0, 75);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_PART_MAIN);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(cont, LV_DIR_NONE);

    // Create MUX row
    lv_obj_t* mux_row = lv_obj_create(cont);
    lv_obj_set_size(mux_row, lv_pct(100), 65);
    lv_obj_set_layout(mux_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(mux_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(mux_row, 0, 0);
    lv_obj_set_style_bg_opa(mux_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(mux_row, 0, LV_PART_MAIN);
    lv_obj_set_scroll_dir(mux_row, LV_DIR_NONE);

    lbl = lv_label_create(mux_row);
    lv_label_set_text_fmt(lbl, "TCA9548A:  ");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0x32c935), LV_PART_MAIN);

    label_status_mux = lv_label_create(mux_row);
    lv_obj_set_style_text_font(label_status_mux, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(label_status_mux, lv_color_hex(0x32c935), LV_PART_MAIN);

    // Create a row for each sensor
    for (uint8_t i = 0; i < 3; i++) {
        lv_obj_t* row = lv_obj_create(cont);
        lv_obj_set_size(row, lv_pct(100), 65);
        lv_obj_set_layout(row, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_gap(row, 0, 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
        lv_obj_set_scroll_dir(row, LV_DIR_NONE);

        lbl = lv_label_create(row);
        lv_label_set_text_fmt(lbl, "Sensor %d:   ", i + 1);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_40, 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x32c935), LV_PART_MAIN);

        label_sensor_status[i] = lv_label_create(row);
        lv_obj_set_style_text_font(label_sensor_status[i], &lv_font_montserrat_40, 0);
        lv_obj_set_style_text_color(label_sensor_status[i], lv_color_hex(0x32c935), LV_PART_MAIN);
    }
    return diag_screen;
}

bool is_diagnostics_screen_active(void) {
    return lv_scr_act() == diag_screen;
}

void update_diagnostics_screen(void) {

    ConnectionStatus status = sensor_manager_get_connection_status();
    // Update MUX status
    if (status.mux) {
        lv_label_set_text(label_status_mux, "Connected");
        lv_obj_set_style_text_color(label_status_mux, lv_color_hex(0x32c935), LV_PART_MAIN);
    } else {
        lv_label_set_text(label_status_mux, "Disconnected");
        lv_obj_set_style_text_color(label_status_mux, lv_color_hex(0xc41a1a), LV_PART_MAIN);
    }

    // Update sensor statuses
    for (uint8_t i = 0; i < 3; i++) {
        if (status.sensor[i]) {
            lv_label_set_text(label_sensor_status[i], "Connected");
            lv_obj_set_style_text_color(label_sensor_status[i], lv_color_hex(0x32c935), LV_PART_MAIN);
        } else {
            lv_label_set_text(label_sensor_status[i], "Disconnected");
            lv_obj_set_style_text_color(label_sensor_status[i], lv_color_hex(0xc41a1a), LV_PART_MAIN);
        }
    }
}