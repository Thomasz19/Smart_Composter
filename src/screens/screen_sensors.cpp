/******************************************************************************
 * @file    screen_sensors.cpp
 * @author  Thomas Zoldowski
 * @date    May 10, 2025
 * @brief   Implementation of the Sensor Data Screen UI.
 ******************************************************************************/

#include "config.h"
#include <lvgl.h>
#include <Arduino.h>
#include "screens/screen_sensors.h"
#include "screens/screen_warnings.h"
#include "screens/screen_manual.h"  
#include "ui_manager.h"

// ================= extern prototype functions =================
extern void global_input_event_cb(lv_event_t * e);

// ================= Global Variables =================
// Label handles for live updates
static lv_obj_t *label_temp[3];
static lv_obj_t *label_hum[3];
static lv_obj_t *label_o2;

static unsigned long last_update = 0;
static lv_obj_t *sensor_screen = NULL;

// Threshold struct
struct TempThresholds {
    float good_min;
    float good_max;
    float warn_max;
};

TempThresholds temp_thresholds = {15.0, 30.0, 35.0};

/*
functions
*/
static lv_color_t get_temp_color(float temp) { // WARNING: not used yet
    if (temp >= temp_thresholds.good_min && temp <= temp_thresholds.good_max)
        return lv_color_hex(0x1ac41f); // green
    else if (temp <= temp_thresholds.warn_max)
        return lv_color_hex(0xc4691a);  // Orange
    else
        return lv_color_hex(0xc41a1a); // red
}

//sensor readings (replace with real sensor code)
static void update_sensor_values() {
#if SIMULATION_MODE
    for (int i = 0; i < 3; i++) {
        float temp = 20.0 + random(-10, 10) * 0.1;
        float hum = 40.0 + random(-10, 10) * 0.1;
        lv_label_set_text_fmt(label_temp[i], "Temp: %.1f°C", temp);
        lv_label_set_text_fmt(label_hum[i], "Hum: %.1f%%", hum);
    }

    float o2 = 20.9 + random(-5, 5) * 0.1;
    lv_label_set_text_fmt(label_o2, "O₂ Level: %.1f%%", o2);
#else
    // TODO: Read real sensors here
#endif
}

void create_sensor_screen(void) {

    sensor_screen = lv_obj_create(NULL);
    lv_scr_load(sensor_screen);

    // Register the global input event callback
    //lv_obj_add_event_cb(sensor_screen, global_input_event_cb, LV_EVENT_ALL, NULL);

    // Background
    lv_obj_set_style_bg_color(sensor_screen, lv_color_hex(0xc0c9d9), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(sensor_screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(sensor_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(sensor_screen, LV_DIR_NONE);

    // create header
    create_header(sensor_screen, "Sensor Overview");

    // ===== Sensor Data Grid =====
    lv_obj_t *grid = lv_obj_create(sensor_screen);
    lv_obj_set_size(grid, lv_pct(100), 400);
    lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, 65);

    static lv_coord_t col_dsc[] = { 200, 240, 200, LV_GRID_TEMPLATE_LAST };
    static lv_coord_t row_dsc[] = { 60, 60, 60, 60, 60, LV_GRID_TEMPLATE_LAST };

    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);

    // Create a horizontal line between header row and sensor rows
    static lv_point_precise_t line_points[] = { {0, 0}, {700, 0} }; // Line from x=0 to x=320

    lv_obj_t *line = lv_line_create(sensor_screen);
    lv_line_set_points(line, line_points, 2);
    lv_obj_set_style_line_color(line, lv_color_black(), 0);
    lv_obj_set_style_line_width(line, 4, 0);
    lv_obj_align(line, LV_ALIGN_TOP_LEFT, 0, 90 + 60);  // Align just under row 0 of the grid

    // ----- Column Headers -----
    // Col 0 Lable
    lv_obj_t *label_col0 = lv_label_create(grid);
    lv_label_set_text(label_col0, "Sensors");
    lv_obj_set_grid_cell(label_col0, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_set_style_text_color(label_col0, lv_color_black(), 0);
    lv_obj_set_style_text_font(label_col0, &lv_font_montserrat_40, 0);
    
    // Col 1 Lable
    lv_obj_t *label_col1 = lv_label_create(grid);
    lv_label_set_text(label_col1, "Temp °F");
    lv_obj_set_grid_cell(label_col1, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_set_style_text_color(label_col1, lv_color_black(), 0);
    lv_obj_set_style_text_font(label_col1, &lv_font_montserrat_40, 0);

    // Col 2 Lable
    lv_obj_t *label_col2 = lv_label_create(grid);
    lv_label_set_text(label_col2, "Humidity (%)");
    lv_obj_set_grid_cell(label_col2, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_set_style_text_color(label_col2, lv_color_black(), 0);
    lv_obj_set_style_text_font(label_col2, &lv_font_montserrat_40, 0);

    

    // ----- Row Labels + Sensor Value Labels -----
    for (int i = 0; i < 3; i++) {
        int row = i + 1; // row 0 is header

        // Row titles (Sensor 1, 2, 3)
        lv_obj_t *label_sensor = lv_label_create(grid);
        lv_label_set_text_fmt(label_sensor, "Sensor %d", i + 1);
        lv_obj_set_grid_cell(label_sensor, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, row, 1);
        lv_obj_set_style_text_font(label_sensor, &lv_font_montserrat_40, 0);
        lv_obj_set_style_text_color(label_sensor, lv_color_black(), 0);

        // Temp
        label_temp[i] = lv_label_create(grid);
        lv_obj_set_grid_cell(label_temp[i], LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, row, 1);
        lv_obj_set_style_text_font(label_temp[i], &lv_font_montserrat_40, 0);
        lv_obj_set_style_text_color(label_temp[i], lv_color_black(), 0);

        // Humidity
        label_hum[i] = lv_label_create(grid);
        lv_obj_set_grid_cell(label_hum[i], LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, row, 1);
        lv_obj_set_style_text_font(label_hum[i], &lv_font_montserrat_40, 0);
        lv_obj_set_style_text_color(label_hum[i], lv_color_black(), 0);
    }

    // Line 
    lv_obj_t *line1 = lv_line_create(sensor_screen);
    lv_line_set_points(line1, line_points, 2);
    lv_obj_set_style_line_color(line1, lv_color_black(), 0);
    lv_obj_set_style_line_width(line1, 4, 0);
    lv_obj_align(line1, LV_ALIGN_TOP_LEFT, 0, 75 + 280);  // Align just under row 0 of the grid

    // O2 Data
    lv_obj_t *label_o2_title = lv_label_create(grid);
    lv_label_set_text(label_o2_title, "O2 Level");
    lv_obj_set_grid_cell(label_o2_title, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 4, 1);
    lv_obj_set_style_text_font(label_o2_title, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(label_o2_title, lv_color_black(), 0);

    label_o2 = lv_label_create(grid);
    lv_obj_set_grid_cell(label_o2, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 4, 1);
    lv_obj_set_style_text_font(label_o2, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(label_o2, lv_color_black(), 0);

    // Create footer
    create_footer(sensor_screen);

    //update_sensor_values(); // Initial values
}

bool is_sensor_screen_active() {
    return lv_scr_act() == sensor_screen;
}

void update_sensor_screen() {
    if (millis() - last_update > 1000) {
        last_update = millis();
        update_sensor_values();
    }
}