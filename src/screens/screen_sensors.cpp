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
static lv_obj_t *label_temp[6];
static lv_obj_t *label_hum[6];
static lv_obj_t *label_o2;
lv_obj_t *bar_sonar;
static lv_style_t style_bar_ind;
// SONAR UART buffer (SEN0208 protocol)
static uint8_t buffer_RTT[4];
static int32_t distanceMm = 0;
static float distanceInches = 0.0f;
static uint8_t pct;
// —————— Constants ——————
static const uint32_t SONAR_INTERVAL_MS    = 1000;  // 1 Hz sampling
static const uint8_t  AVG_WINDOW_SIZE      = 5;     // buffer 5 samples
static const uint8_t  OUTLIER_THRESHOLD_IN = 5;     // ignore >5 in jumps
static const uint8_t  COMPOSTER_HEIGHT_IN  = 44;    // total height in inches

// —————— Sonar buffering ——————
static unsigned int sonarBuf[AVG_WINDOW_SIZE];
static uint8_t      bufIdx          = 0;
static float        lastValidIn     = 0.0f;


lv_obj_t *sensor_screen = nullptr;

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
        // tell sensor_manager to read the mux + AHT20s:
    sensor_manager_update();

    // Check which devices ACK’d on-bus:
    ConnectionStatus status = sensor_manager_get_connection_status();

    for (int i = 0; i < 3; i++) {
        if (status.sensor[i]) {
            // read temperature (°C) and convert to °F
            float tempC = sensor_manager_get_temperature(i);
            float tempF = tempC * 9.0f / 5.0f + 32.0f;
            float hum   = sensor_manager_get_humidity(i);

            // Convert tempF to “tenths” fixed-point:
            int32_t temp10      = (int32_t)roundf(tempF * 10.0f);
            int32_t temp_whole  = temp10 / 10;             // integer part
            int32_t temp_decimal = abs(temp10 % 10);       // single decimal digit

            // Convert hum to “tenths” fixed-point:
            int32_t hum10       = (int32_t)roundf(hum * 10.0f);
            int32_t hum_whole   = hum10 / 10;
            int32_t hum_decimal = abs(hum10 % 10);

            // Use LV_PRId32 to print “whole.decimal”:
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
        } else {
            lv_label_set_text(label_temp[i], "Error");
            lv_label_set_text(label_hum[i],  "Error");
        }
    }

    if (status.o2) {
        // read O₂ level (percent)
        float o2          = sensor_manager_get_oxygen();
        int32_t o210       = (int32_t)roundf(o2 * 10.0f);
        int32_t o2_whole   = o210 / 10;
        int32_t o2_decimal= abs(o210 % 10);

        lv_label_set_text_fmt(
            label_o2,
            "%" LV_PRId32 ".%" LV_PRId32 "%%",
            o2_whole, o2_decimal
        );
    } else {
        lv_label_set_text(label_o2, "Error");
    }
    
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
    lv_obj_set_size(grid, lv_pct(100), 400);
    lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, 65);

    static lv_coord_t col_dsc[] = { 200, 200, 180, 180, LV_GRID_TEMPLATE_LAST };
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

    // Sonar Data
    // === Compost Level Bar on right side (col 3, rows 0-3) ===
    lv_style_init(&style_bar_ind);
    lv_style_set_bg_opa(&style_bar_ind, LV_OPA_COVER);
    lv_style_set_bg_color(&style_bar_ind, lv_palette_main(LV_PALETTE_RED));
    lv_style_set_bg_grad_color(&style_bar_ind, lv_palette_main(LV_PALETTE_GREEN));
    lv_style_set_bg_grad_dir(&style_bar_ind, LV_GRAD_DIR_VER);
    lv_style_set_radius(&style_bar_ind, 0);


    bar_sonar = lv_bar_create(sensor_screen);
    lv_obj_add_style(bar_sonar, &style_bar_ind, LV_PART_INDICATOR);
    lv_obj_set_size(bar_sonar, 40, 300);
    lv_obj_align(bar_sonar, LV_ALIGN_RIGHT_MID, -30, 10);
    lv_bar_set_range(bar_sonar, 0, 100);
    lv_bar_set_value(bar_sonar, 50, LV_ANIM_OFF);
    lv_obj_set_style_radius(bar_sonar, 0, LV_PART_MAIN);
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
    lv_bar_set_value(bar_sonar, pct, LV_ANIM_ON);
    return sensor_screen;
}

bool is_sensor_screen_active() {
    return lv_scr_act() == sensor_screen;
}

void update_sensor_screen() {
    update_sensor_values();
   
    //Serial.println("Update data");

}


/**
 * @brief  Called once per loop(); reads UART1 at 1 Hz, buffers + rejects outliers,
 *         every 5 samples computes avg distance → percent full → updates bar.
 */
void sonar_update_and_fill_bar() {
    float inches;
    if (!read_sonar_packet(inches)) return;
    Serial.print("Sonar (raw mm): ");
    Serial.println((int)(inches * 25.4));  // avoids float
    // Outlier rejection
    if (bufIdx==0) lastValidIn = inches;
    if (fabs(inches - lastValidIn) > OUTLIER_THRESHOLD_IN) return;
    lastValidIn = inches;
    Serial.println("made it 1");
    // Buffer
    sonarBuf[bufIdx++] = inches;
    if (bufIdx < AVG_WINDOW_SIZE) return;
    bufIdx = 0;
    Serial.println("made it 2");
    // Average
    float sum=0;
    for(int i=0;i<AVG_WINDOW_SIZE;i++) sum += sonarBuf[i];
    float avgIn = sum/AVG_WINDOW_SIZE;
    //Serial.println(avgIn);
    // Compute % full
    float levelIn = COMPOSTER_HEIGHT_IN - avgIn;
    levelIn = constrain(levelIn, 0, (float)COMPOSTER_HEIGHT_IN);
    pct = (uint8_t)roundf((levelIn/COMPOSTER_HEIGHT_IN)*100.0f);
    Serial.println("Pct: ");
    Serial.println(pct);

    // Update LVGL bar
    if (!bar_sonar) {
        Serial.println("Error: bar_sonar is NULL!");
        return;
    }
    if (lv_obj_is_valid(bar_sonar)) {
        lv_bar_set_value(bar_sonar, pct, LV_ANIM_ON);
        //lv_obj_invalidate(bar_sonar);
        Serial.println("updated bar");
    }else{
        Serial.println("Not valid bar :(");
    }
    
}

/**
 * @brief  Read raw sonar packet from Serial2, decode to inches.
 *         Uses SEN0208 header-CRC protocol.
 * @return true if a valid reading was obtained.
 */
bool read_sonar_packet(float &outInches) {
    // Need 4 bytes: header, high, low, checksum
    if (Serial2.available() < 4) return false;
    // Check header
    if (Serial2.peek() != 0xFF) {
        Serial2.read();
        return false;
    }
    // Read packet
    for (int i = 0; i < 4; ++i) buffer_RTT[i] = Serial2.read();
    uint8_t cs = buffer_RTT[0] + buffer_RTT[1] + buffer_RTT[2];
    if (buffer_RTT[3] != cs) return false;
    // Build mm, convert to inches
    distanceMm = (buffer_RTT[1] << 8) | buffer_RTT[2];
    distanceInches = distanceMm / 25.4f;
    outInches = distanceInches;
    return true;
}