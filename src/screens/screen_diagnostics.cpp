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
#include "ui_manager.h"

// Screen and label handles
static lv_obj_t* diag_screen = NULL;
static lv_obj_t* label_sensor_status[6];
static lv_obj_t* label_status_mux;

lv_obj_t* create_diagnostics_screen(void) {

    diag_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(diag_screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(diag_screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(diag_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(diag_screen, LV_DIR_NONE);

    create_header(diag_screen, "Diagnostics");

    // ===== Sensor Data Grid =====
    lv_obj_t *grid = lv_obj_create(diag_screen);
    lv_obj_set_size(grid, lv_pct(100), 384);
    lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, 80);

    static lv_coord_t col_dsc[] = { 200, 400, LV_GRID_TEMPLATE_LAST };
    static lv_coord_t row_dsc[] = { 48, 48, 48, 48, 48, 48, 48, 48, LV_GRID_TEMPLATE_LAST };

    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    
    // Enable Scrolling
    lv_obj_add_flag(grid, LV_OBJ_FLAG_SCROLLABLE);          // allow scrolling
    lv_obj_set_scroll_dir(grid, LV_DIR_VER);                // vertical scroll only
    lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_AUTO); // show scrollbar when needed

    // Create Mux row
    lv_obj_t *label_mux_title = lv_label_create(grid);
    lv_label_set_text(label_mux_title, "TCA9548A:");
    lv_obj_set_grid_cell(label_mux_title, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_set_style_text_font(label_mux_title, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(label_mux_title, lv_color_hex(0x32c935), 0);

    label_status_mux = lv_label_create(grid);
    lv_obj_set_grid_cell(label_status_mux, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_set_style_text_font(label_status_mux, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(label_status_mux, lv_color_hex(0x32c935), 0);

    // Create a row for each aht20 sensor
    for (uint8_t i = 0; i < 6; i++) {
        lv_obj_t *row = lv_label_create(grid);
        lv_label_set_text_fmt(row, "AHT20 #%d:", i + 1);
        lv_obj_set_grid_cell(row, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, i+1, 1);
        lv_obj_set_style_text_font(row, &lv_font_montserrat_40, 0);
        lv_obj_set_style_text_color(row, lv_color_hex(0x32c935), 0);

        label_sensor_status[i] = lv_label_create(grid);
        lv_obj_set_grid_cell(label_sensor_status[i], LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, i+1, 1);
        lv_obj_set_style_text_font(label_sensor_status[i], &lv_font_montserrat_40, 0);
        lv_obj_set_style_text_color(label_sensor_status[i], lv_color_hex(0x32c935), 0);
    }

    // Create 02 sensor row
    lv_obj_t *label_o2_title = lv_label_create(grid);
    lv_label_set_text(label_o2_title, "Oxygen:");
    lv_obj_set_grid_cell(label_o2_title, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 7, 1);
    lv_obj_set_style_text_font(label_o2_title, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(label_o2_title, lv_color_hex(0x32c935), 0);

    // label_status_mux = lv_label_create(grid);
    // lv_obj_set_grid_cell(label_status_mux, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 7, 1);
    // lv_obj_set_style_text_font(label_status_mux, &lv_font_montserrat_40, 0);
    // lv_obj_set_style_text_color(label_status_mux, lv_color_hex(0x32c935), 0);

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
    for (uint8_t i = 0; i < 6; i++) {
        if (status.sensor[i]) {
            lv_label_set_text(label_sensor_status[i], "Connected");
            lv_obj_set_style_text_color(label_sensor_status[i], lv_color_hex(0x32c935), LV_PART_MAIN);
        } else {
            lv_label_set_text(label_sensor_status[i], "Disconnected");
            lv_obj_set_style_text_color(label_sensor_status[i], lv_color_hex(0xc41a1a), LV_PART_MAIN);
        }
    }
}