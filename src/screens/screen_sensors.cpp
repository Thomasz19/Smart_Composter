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
#include "screens/screen_settings.h"

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
static int bar_val;
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
        sensor_manager_update();
        ConnectionStatus status = sensor_manager_get_connection_status();
        
        // AHT20 readings
        for (int i = 0; i < 3; i++) {
            if (label_temp[i] && label_hum[i]) {
                if (status.sensor[i]) {
                    float tempC = sensor_manager_get_temperature(i);
                    float tempF = tempC * 9.0f/5.0f + 32.0f;
                    float hum = sensor_manager_get_humidity(i);
                    int32_t t10 = roundf(tempF*10);
                    int32_t h10 = roundf(hum*10);
                    lv_label_set_text_fmt(label_temp[i], "%d.%d°F", t10/10, abs(t10%10));
                    lv_label_set_text_fmt(label_hum[i],  "%d.%d%%", h10/10, abs(h10%10));
                } else {
                    lv_label_set_text(label_temp[i], "Error");
                    lv_label_set_text(label_hum[i],  "Error");
                }
            }
     }
        // O₂ reading
        if (label_o2) {
            if (status.o2) {
                float o2 = sensor_manager_get_oxygen();
                int32_t o210 = roundf(o2*10);
                lv_label_set_text_fmt(label_o2, "%d.%d%%", o210/10, abs(o210%10));
            } else {
                lv_label_set_text(label_o2, "Error");
            }
        }

        //Serial.println("VL53L1X sensors:");
        // Compost-level bar with buffer and outlier logic
        static float buf[BUF_SIZE];
        static uint8_t buf_idx = 0, buf_cnt = 0, outlier_count = 0;
        float raw = status.vl53[0] ? sensor_manager_get_tof_distance(0) : NAN;
        if (!isnan(raw)) {
            float avg = 0;
            if (buf_cnt > 0) {
                for (uint8_t k = 0; k < buf_cnt; k++) avg += buf[k];
                avg /= buf_cnt;
            }
            if (buf_cnt == 0 || fabs(raw - avg) <= OUTLIER_THRESH_CM) {
                // valid reading
                buf[buf_idx] = raw;
                buf_idx = (buf_idx + 1) % BUF_SIZE;
                if (buf_cnt < BUF_SIZE) buf_cnt++;
                outlier_count = 0;
            } else {
                // potential outlier
                outlier_count++;
                if (outlier_count >= BUF_SIZE) {
                    // sustained new value: reset buffer
                    for (uint8_t k = 0; k < BUF_SIZE; k++) buf[k] = raw;
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
        if (bar_level && label_bar_pct) {
            // Map to bar 0..100
            bar_val = (int)constrain((avg_depth / MAX_DEPTH_CM) * 100.0f, 0, 100);
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
            lv_obj_set_pos(label_bar_pct, coords.x1 - 90, y - 24);
        }
        Serial.println("Sensor update completed.");
    #endif
}
// =================== SCREEN DRIVER ===================
lv_obj_t* create_sensor_screen(void) {

    lv_obj_t *sensor_screen = lv_obj_create(NULL);
    
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
    lv_obj_set_size(grid, lv_pct(100), 340);
    lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, 65);

    static lv_coord_t col_dsc[] = { 200, 200, 180, LV_GRID_TEMPLATE_LAST };
    static lv_coord_t row_dsc[] = { 50, 60, 60, 60, 60, LV_GRID_TEMPLATE_LAST };

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
    
    // Column titles
    lv_obj_t *label_temp_title = lv_label_create(grid);
    lv_label_set_text(label_temp_title, "Temp");
    lv_obj_set_grid_cell(label_temp_title, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_set_style_text_font(label_temp_title, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(label_temp_title, lv_color_black(), 0);

    lv_obj_t *label_hum_title = lv_label_create(grid);
    lv_label_set_text(label_hum_title, "Hum");
    lv_obj_set_grid_cell(label_hum_title, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_set_style_text_font(label_hum_title, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(label_hum_title, lv_color_black(), 0);

    // ----- Row Labels + Sensor Value Labels -----
    for (int i = 0; i < 3; i++) {
        // Row titles (Sensor 1, 2, 3)
        lv_obj_t *label_sensor = lv_label_create(grid);
        lv_label_set_text_fmt(label_sensor, "Sensor %d", i + 1);
        lv_obj_set_grid_cell(label_sensor, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, i+1, 1);
        lv_obj_set_style_text_font(label_sensor, &lv_font_montserrat_48, 0);
        lv_obj_set_style_text_color(label_sensor, lv_color_black(), 0);

        // Temp
        label_temp[i] = lv_label_create(grid);
        lv_obj_set_grid_cell(label_temp[i], LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, i+1, 1);
        lv_obj_set_style_text_font(label_temp[i], &lv_font_montserrat_48, 0);
        lv_obj_set_style_text_color(label_temp[i], lv_color_black(), 0);

        // Humidity
        label_hum[i] = lv_label_create(grid);
        lv_obj_set_grid_cell(label_hum[i], LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, i+1, 1);
        lv_obj_set_style_text_font(label_hum[i], &lv_font_montserrat_48, 0);
        lv_obj_set_style_text_color(label_hum[i], lv_color_black(), 0);
    }
    
    // Create a horizontal line at the bottom of the grid
    static lv_point_precise_t line_points1[] = { {0, 0}, {620, 0} };
    lv_obj_t * line1 = lv_line_create(sensor_screen);
    lv_line_set_points(line1, line_points1, 2);
    lv_obj_set_style_line_color(line1, lv_color_black(), 0);
    lv_obj_set_style_line_width(line1, 4, 0);
    lv_obj_set_pos(line1, 0, 350);

    // O2 Data
    lv_obj_t *label_o2_title = lv_label_create(grid);
    lv_label_set_text(label_o2_title, "O2 %");
    lv_obj_set_grid_cell(label_o2_title, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 4, 1);
    lv_obj_set_style_text_font(label_o2_title, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(label_o2_title, lv_color_black(), 0);

    label_o2 = lv_label_create(grid);
    lv_obj_set_grid_cell(label_o2, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 4, 1);
    lv_obj_set_style_text_font(label_o2, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(label_o2, lv_color_black(), 0);

   // Compost level bar on right
    bar_level = lv_bar_create(sensor_screen);
    lv_obj_set_size(bar_level,40,310);
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
    lv_label_set_text(label_bar_pct, "");
    lv_obj_set_style_text_font(label_bar_pct, &lv_font_montserrat_40, 0);
    
    
    
    // USB “Diagnostics” button at top-left
    lv_obj_t *btn_diag = lv_btn_create(sensor_screen);

    // 1) Size and position
    lv_obj_set_size(btn_diag, 80, 74);
    lv_obj_align(btn_diag, LV_ALIGN_TOP_RIGHT, -2, 2);

    // 2) Style the background
    lv_obj_set_style_bg_color(btn_diag, lv_color_hex(0x42649f), LV_PART_MAIN);
    lv_obj_set_style_bg_opa  (btn_diag, LV_OPA_COVER,           LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_diag, 0,                  LV_PART_MAIN);
    
    // 3) Load diagnostics on click
    lv_obj_add_event_cb(btn_diag, [](lv_event_t* e) {
        LV_UNUSED(e);
        lv_scr_load(create_diagnostics_screen());
    }, LV_EVENT_CLICKED, NULL);

    // 4) USB symbol label
    lv_obj_t *lbl_diag = lv_label_create(btn_diag);
    lv_label_set_text(lbl_diag, LV_SYMBOL_USB);
    lv_obj_set_style_text_color(lbl_diag, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_diag, &lv_font_montserrat_40, 0);
    lv_obj_center(lbl_diag);

    return sensor_screen;
}

void update_sensor_screen() {
    update_sensor_values();
   
    //Serial.println("Update data");

}

/** @brief Send sensor data to Serial for debugging
 * This function formats the sensor data and sends it over Serial.
 */
void SensorDataToSerial() {

    Serial.print("Data:");
    for (int i = 0; i < 3; i++) {
        float tempC = sensor_manager_get_temperature(i);
        float tempF = tempC * 9.0f / 5.0f + 32.0f;    // convert to °F
        Serial.print(tempF, 1);                      // print with 1 decimal
        Serial.print(",");
        float hum = sensor_manager_get_humidity(i);
        Serial.print(hum);
        Serial.print(",");
    }

    float o2 = sensor_manager_get_oxygen();
    Serial.print(o2);
    Serial.print(",");
    Serial.println(bar_val);
}
static int old_camera_delay = -1;

void CameraDelayToSerial() {
    // Print camera delay to Serial for debugging
    int camera_delay = getCameraDelay();
    

    if (camera_delay != old_camera_delay) {
        old_camera_delay = camera_delay;
        Serial.print("Delay:");
        Serial.println(camera_delay);
    }
}
