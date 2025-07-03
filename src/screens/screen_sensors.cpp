/******************************************************************************
 * @file    screen_sensors.cpp
 * @author  Thomas Zoldowski
 * @date    May 10, 2025
 * @brief   Implementation of the Sensor Data Screen UI.
 ******************************************************************************/

#include "config.h"

#include <Arduino.h>
#include "screens/screen_sensors.h"
#include "screens/screen_warnings.h"
#include "screens/screen_manual.h"  
#include "screens/screen_diagnostics.h" 
#include "ui_manager.h"
#include "logic/sensor_manager.h"

// ================= extern prototype functions =================
extern void global_input_event_cb(lv_event_t * e);

// ================= Global Variables =================
// Label handles for live updates
static lv_obj_t *label_temp[3];
static lv_obj_t *label_hum[3];
static lv_obj_t *label_o2;
static lv_obj_t *label_bar_pct;  // dynamic percentage label

static lv_obj_t *bar_level;  // Compost level bar
// Configuration for TOF buffering
static const float MAX_DEPTH_CM = 111.0f;         // maximum sensor range
static const int   BUF_SIZE     = 5;              // number of samples to average
static const float OUTLIER_THRESH_CM = 20.0f;      // ignore changes >20 cm

lv_obj_t *sensor_screen = nullptr;
// Maximum measurable compost depth in cm

int8_t o2Channel;
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
// static lv_color_t get_temp_color(float temp) { // WARNING: not used yet
//     if (temp >= temp_thresholds.good_min && temp <= temp_thresholds.good_max)
//         return lv_color_hex(0x1ac41f); // green
//     else if (temp <= temp_thresholds.warn_max)
//         return lv_color_hex(0xc4691a);  // Orange
//     else
//         return lv_color_hex(0xc41a1a); // red
// }

//sensor readings (replace with real sensor code)
static void update_sensor_values() {
    #if SIMULATION_MODE
        for (int i = 0; i < 6; i++) {
            // 1) Simulated temp/hum floats
            float tempC = 120.0f + (random(-10, 11) * 0.5f);
            float hum   = 40.0f + (random(-10, 11) * 0.1f);

            // 2) Convert to “tenths” integer
            int32_t temp10 = (int32_t)roundf(tempC * 10.0f);
            int32_t hum10  = (int32_t)roundf(hum   * 10.0f);

            // 3) Split into whole and decimal digits
            int32_t temp_whole   = temp10 / 10;             // e.g. 20
            int32_t temp_decimal = abs(temp10 % 10);        // e.g. 3  → shows 20.3

            int32_t hum_whole    = hum10  / 10;             // e.g. 40
            int32_t hum_decimal  = abs(hum10  % 10);        // e.g. 7  → shows 40.7%

            // 4) Format with LV_PRId32:
            lv_label_set_text_fmt(
            label_temp[i],
            "%" LV_PRId32 ".%" LV_PRId32 "°F",
            temp_whole, temp_decimal
            );
            lv_label_set_text_fmt(
            label_hum[i],
            "%" LV_PRId32 ".%" LV_PRId32 "%%",
            hum_whole, hum_decimal
            );
        }
        // O₂ example (if you want “O₂: xx.x%”)
        float o2_sim       = 20.9f + (random(-5, 6) * 0.1f);
        int32_t o2_tenths  = (int32_t)roundf(o2_sim * 10.0f);
        int32_t o2_whole   = o2_tenths / 10;
        int32_t o2_decimal = abs(o2_tenths % 10);
        lv_label_set_text_fmt(
        label_o2,
        "%" LV_PRId32 ".%" LV_PRId32 "%%",
        o2_whole, o2_decimal
        );
    #else
        
        data_mutex.lock();
        // copy into locals using your sensor_manager_* accessors
        float temps[3] = {
            sensor_manager_get_temperature(0),
            sensor_manager_get_temperature(1),
            sensor_manager_get_temperature(2)
        };
        float hums[3] = {
            sensor_manager_get_humidity(0),
            sensor_manager_get_humidity(1),
            sensor_manager_get_humidity(2)
        };
        float o2_local  = sensor_manager_get_oxygen();
        float tof_local = sensor_manager_get_tof_distance(0);
        data_mutex.unlock();


        // AHT20 readings
        Serial.println("AHT20 sensors:");
        for (int i = 0; i < 3; i++) {
            //if (s.sensor[i]) {
                float tempF = temps[i] * 9.0f/5.0f + 32.0f;
                int32_t t10 = roundf(tempF*10);
                int32_t h10 = roundf(hums[i]*10);
                lv_label_set_text_fmt(label_temp[i], "%d.%d°F", t10/10, abs(t10%10));
                lv_label_set_text_fmt(label_hum[i],  "%d.%d%%", h10/10, abs(h10%10));
            // } else {
            //     lv_label_set_text(label_temp[i], "Error");
            //     lv_label_set_text(label_hum[i],  "Error");
            // }
        }
        Serial.println("o2 sensor:");
        // O₂ reading
        //if (s.o2) {
            int32_t o210 = roundf(o2_local*10);
            lv_label_set_text_fmt(label_o2, "%d.%d%%", o210/10, abs(o210%10));
        //} else {
        //    lv_label_set_text(label_o2, "Error");
        //}

        Serial.println("VL53L1X sensors:");
        // Compost-level bar with buffer and outlier logic
        static float buf[BUF_SIZE];
        static uint8_t buf_idx = 0, buf_cnt = 0, outlier_count = 0;
        if (!isnan(tof_local)) {
            float avg = 0;
            if (buf_cnt > 0) {
                for (uint8_t k = 0; k < buf_cnt; k++) avg += buf[k];
                avg /= buf_cnt;
            }
            if (buf_cnt == 0 || fabs(tof_local - avg) <= OUTLIER_THRESH_CM) {
                // valid reading
                buf[buf_idx] = tof_local;
                buf_idx = (buf_idx + 1) % BUF_SIZE;
                if (buf_cnt < BUF_SIZE) buf_cnt++;
                outlier_count = 0;
            } else {
                // potential outlier
                outlier_count++;
                if (outlier_count >= BUF_SIZE) {
                    // sustained new value: reset buffer
                    for (uint8_t k = 0; k < BUF_SIZE; k++) buf[k] = tof_local;
                    buf_cnt = BUF_SIZE;
                    buf_idx = 0;
                    outlier_count = 0;
                }
            }
        }
        // Compute filtered average
        float avg_depth = 0;
        if (buf_cnt > 0) {
            for (uint8_t k = 0; k < buf_cnt; k++) avg_depth += buf[k];
            avg_depth /= buf_cnt;
        }

        // Map to bar 0..100
        int bar_val = (int)constrain((avg_depth / MAX_DEPTH_CM) * 100.0f, 0, 100);
        bar_val = 100 - bar_val; // Invert: 0% = full, 100% = empty
        lv_bar_set_value(bar_level, bar_val, LV_ANIM_OFF);

        // Update dynamic percentage label next to bar
        lv_label_set_text_fmt(label_bar_pct, "%d%%", bar_val);
        lv_area_t coords;
        lv_obj_get_coords(bar_level, &coords);
        int bar_h = coords.y2 - coords.y1;
        // position y at bar bottom minus proportion
        int y = coords.y2 - (bar_val * bar_h / 100);
        // offset label to left of bar and centered vertically
        lv_obj_set_pos(label_bar_pct, coords.x1 - 90, y - 8);
        Serial.println("Sensor update completed.");
    #endif
}
// =================== SCREEN DRIVER ===================
lv_obj_t* create_sensor_screen(void) {

    sensor_screen = lv_obj_create(NULL);
    
    
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
    lv_obj_set_size(grid, lv_pct(100), 300);
    lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, 65);

    static lv_coord_t col_dsc[] = { 200, 200, 180, LV_GRID_TEMPLATE_LAST };
    static lv_coord_t row_dsc[] = { 60, 60, 60, 60, LV_GRID_TEMPLATE_LAST };

    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);

    // Create a horizontal line between header row and sensor rows
    static lv_point_precise_t line_points[] = { {0, 400}, {0, 0} }; // Line from x=0 to x=320

    lv_obj_t *line = lv_line_create(sensor_screen);
    lv_line_set_points(line, line_points, 2);
    lv_obj_set_style_line_color(line, lv_color_black(), 0);
    lv_obj_set_style_line_width(line, 4, 0);
    lv_obj_align(line, LV_ALIGN_TOP_LEFT, 620, 80);  // Align just under row 0 of the grid
    
    // ----- Row Labels + Sensor Value Labels -----
    for (int i = 0; i < 3; i++) {
        // Row titles (Sensor 1, 2, 3)
        lv_obj_t *label_sensor = lv_label_create(grid);
        lv_label_set_text_fmt(label_sensor, "Sensor %d", i + 1);
        lv_obj_set_grid_cell(label_sensor, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, i, 1);
        lv_obj_set_style_text_font(label_sensor, &lv_font_montserrat_48, 0);
        lv_obj_set_style_text_color(label_sensor, lv_color_black(), 0);

        // Temp
        label_temp[i] = lv_label_create(grid);
        lv_obj_set_grid_cell(label_temp[i], LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, i, 1);
        lv_obj_set_style_text_font(label_temp[i], &lv_font_montserrat_48, 0);
        lv_obj_set_style_text_color(label_temp[i], lv_color_black(), 0);

        // Humidity
        label_hum[i] = lv_label_create(grid);
        lv_obj_set_grid_cell(label_hum[i], LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, i, 1);
        lv_obj_set_style_text_font(label_hum[i], &lv_font_montserrat_48, 0);
        lv_obj_set_style_text_color(label_hum[i], lv_color_black(), 0);
    }
    
    // Line 
    lv_obj_t *line1 = lv_line_create(sensor_screen);
    lv_line_set_points(line1, line_points, 2);
    lv_obj_set_style_line_color(line1, lv_color_black(), 0);
    lv_obj_set_style_line_width(line1, 4, 0);
    lv_obj_align(line1, LV_ALIGN_TOP_LEFT, 0, 400-37+60);  // Align just under row 0 of the grid

    // O2 Data
    lv_obj_t *label_o2_title = lv_label_create(grid);
    lv_label_set_text(label_o2_title, "O2 %");
    lv_obj_set_grid_cell(label_o2_title, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 3, 1);
    lv_obj_set_style_text_font(label_o2_title, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(label_o2_title, lv_color_black(), 0);

    label_o2 = lv_label_create(grid);
    lv_obj_set_grid_cell(label_o2, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 3, 1);
    lv_obj_set_style_text_font(label_o2, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(label_o2, lv_color_black(), 0);

   // Compost level bar on right
    bar_level = lv_bar_create(sensor_screen);
    lv_obj_set_size(bar_level,40,320);
    lv_obj_align(bar_level,LV_ALIGN_RIGHT_MID,-30, 5);
    lv_bar_set_range(bar_level,0,100);
    lv_bar_set_value(bar_level,0,LV_ANIM_OFF);
    lv_obj_set_style_radius(bar_level,0,LV_PART_MAIN);

    static lv_style_t si; lv_style_init(&si);
    lv_style_set_bg_opa(&si,LV_OPA_COVER);
    lv_style_set_bg_color(&si,lv_palette_main(LV_PALETTE_RED));
    lv_style_set_bg_grad_color(&si,lv_palette_main(LV_PALETTE_GREEN));
    lv_style_set_bg_grad_dir(&si,LV_GRAD_DIR_VER);
    lv_style_set_radius(&si,0);
    lv_obj_add_style(bar_level,&si,LV_PART_INDICATOR);

    // Dynamic percentage label
    label_bar_pct = lv_label_create(sensor_screen);
    lv_label_set_text(label_bar_pct, "0%% Full");
    lv_obj_set_style_text_font(label_bar_pct, &lv_font_montserrat_40, 0);
    
    // Create footer
    create_footer(sensor_screen);
    
    // Diagnostics button bottom-left
    lv_obj_t *btn_diag = lv_btn_create(sensor_screen);
    lv_obj_set_size(btn_diag, 140, 70);
    lv_obj_align(btn_diag, LV_ALIGN_BOTTOM_RIGHT, -200, -60);
    lv_obj_add_event_cb(btn_diag, [](lv_event_t* e) {
        LV_UNUSED(e);
        lv_scr_load(create_diagnostics_screen());
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_diag = lv_label_create(btn_diag);
    lv_label_set_text(lbl_diag, "Diagnostics");
    lv_obj_center(lbl_diag);
    
    update_sensor_values(); // Initial values
    
    return sensor_screen;
}

bool is_sensor_screen_active() {
    return lv_scr_act() == sensor_screen;
}

void update_sensor_screen() {
    update_sensor_values();
   
    //Serial.println("Update data");

}