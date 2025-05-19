/******************************************************************************
 * @file    screen_manual.cpp
 * @author  Thomas Zoldowski
 * @date    May 10, 2025
 * @brief   Implementation of the Motor Control and Status Screen UI.
 ******************************************************************************/
/*
Things that need to be added:
- keypad
- password protection
- Money because doing this shit for free should be illegal
*/
#include "screens/screen_manual.h"
#include "screens/screen_sensors.h"
#include "screens/screen_settings.h"
#include "screens/screen_warnings.h"

#include "ui_manager.h"

#include <lvgl.h>
#include <string.h>
#include <Arduino.h>

static lv_obj_t* settings_screen = nullptr;

// ---- Runtime threshold data ----
static struct {
    float temp_low;
    float temp_high;
    float hum_low;
} sensor_thresh[3] = {
    {15.0, 30.0, 40.0},
    {15.0, 30.0, 40.0},
    {130.0, 160.0, 20.0},
};

// ---- Spinbox handles ----
static lv_obj_t *sb_temp_low[3];
static lv_obj_t *sb_temp_high[3];
static lv_obj_t *sb_hum_low[3];

// ---- PIN storage ----
static char user_pin[5] = "0000";

// ---- Modal objects ----
static lv_obj_t * modal_bg;
static lv_obj_t * modal_ta;
static lv_obj_t * modal_kb;
static bool       modal_is_for_pin_change = false;
static int        modal_sensor_index = -1;

// Forward declarations
static void params_btn_cb(lv_event_t *e);
static void change_pin_btn_cb(lv_event_t *e);
static void modal_kb_event_cb(lv_event_t *e);



void create_settings_screen() {
    // Constants for layout
    const int HEADER_H   = 80;
    const int FOOTER_H   = 60;
    const int SCREEN_H   = 480;
    const int TABVIEW_H  = SCREEN_H - HEADER_H - FOOTER_H; // 350px

    // Setup Screen
    settings_screen = lv_obj_create(NULL);
    lv_scr_load(settings_screen);

    // create header + footer
    create_header(settings_screen, "Settings");
    create_footer(settings_screen);

    // Tabview on the left
    lv_obj_t *tabview = lv_tabview_create(settings_screen);
    lv_tabview_set_tab_bar_position(tabview, LV_DIR_LEFT);  // left tabs :contentReference[oaicite:1]{index=1}
    lv_tabview_set_tab_bar_size    (tabview, 170);           // 80px wide

    // Size & position between header & footer
    lv_obj_set_size(tabview, lv_pct(100), TABVIEW_H);
    lv_obj_align   (tabview, LV_ALIGN_TOP_MID, 0, HEADER_H);
    //lv_obj_set_style_bg_color(tabview, lv_color_hex(0xc0c9d9), LV_PART_MAIN);
    lv_obj_set_style_text_font(tabview, &lv_font_montserrat_40, 0);

    lv_obj_t * tab_buttons = lv_tabview_get_tab_bar(tabview);
    lv_obj_set_style_bg_color(tab_buttons, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
    lv_obj_set_style_text_color(tab_buttons, lv_palette_lighten(LV_PALETTE_GREY, 5), 0);
    lv_obj_set_style_border_side(tab_buttons, LV_BORDER_SIDE_RIGHT, LV_PART_ITEMS | LV_STATE_CHECKED);

    lv_obj_t *tab_1 = lv_tabview_add_tab(tabview, "Sensors");
    lv_obj_t *tab_2 = lv_tabview_add_tab(tabview, "Auth.");
    lv_obj_t *tab_3 = lv_tabview_add_tab(tabview, "Config");
    
    // Set font size for each tab
    lv_obj_set_style_text_font(tab_1, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_font(tab_2, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_font(tab_3, &lv_font_montserrat_40, 0);

    // Set Background Color for each Tab
    lv_obj_set_style_bg_color(tab_1, lv_color_hex(0xc0c9d9), 0);
    lv_obj_set_style_bg_opa(tab_1, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(tab_2, lv_color_hex(0xc0c9d9), 0);
    lv_obj_set_style_bg_opa(tab_2, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(tab_3, lv_color_hex(0xc0c9d9), 0);
    lv_obj_set_style_bg_opa(tab_3, LV_OPA_COVER, 0);

    // Tab 1 screen
    // ---- inside create_settings_screen(), after lv_tabview_add_tab(tv,"Sensors") ----
    lv_obj_set_style_pad_all(tab_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t * grid = lv_obj_create(tab_1);
    lv_obj_set_size(grid, lv_pct(100), lv_pct(100));      // fill the tab area
    lv_obj_align   (grid, LV_ALIGN_TOP_MID, 0, 0);

    static lv_coord_t col_dsc[] = { lv_pct(1), lv_pct(33), lv_pct(33), lv_pct(33), LV_GRID_TEMPLATE_LAST };
    static lv_coord_t row_dsc[] = { lv_pct(1), lv_pct(33), lv_pct(33), lv_pct(33),  LV_GRID_TEMPLATE_LAST };  // header + 3 rows

    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, LV_PART_MAIN);

    // Column titles
    const char *cols[] = {"Sen.","Low","High","Low"};
    for(int c = 0; c < 4; c++) {
        // Column header as plain label
        lv_obj_t *label_col = lv_label_create(grid);
        lv_label_set_text(label_col, cols[c]);
        lv_obj_align(label_col, LV_ALIGN_CENTER, 0, 0);

        // Place in row 0, column c
        lv_obj_set_grid_cell(label_col,
        LV_GRID_ALIGN_CENTER, c, 1,
        LV_GRID_ALIGN_CENTER, 0, 1);

        // Optional styling
        lv_obj_set_style_text_font(label_col, &lv_font_montserrat_40, 0);
        lv_obj_set_style_text_color(label_col, lv_color_black(), 0);
    }

    // Row headers + data buttons
    for(int r = 0; r < 3; r++) {
        int row = r + 1;
        // Row header (“1”, “2”, “3”)
        {
        // Row title as plain label
        lv_obj_t *label_row = lv_label_create(grid);
        lv_label_set_text_fmt(label_row, "%d", r + 1);

        // Place it in column 0, this row
        lv_obj_set_grid_cell(label_row,
            LV_GRID_ALIGN_CENTER, 0, 1,
            LV_GRID_ALIGN_CENTER, row, 1);

        // Optionally style it
        lv_obj_set_style_text_font(label_row, &lv_font_montserrat_40, 0);
        lv_obj_set_style_text_color(label_row, lv_color_black(), 0);
        lv_obj_align(label_row, LV_ALIGN_CENTER, 0, 0);
        }
        // Low (temp_low)
        {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f°F", sensor_thresh[r].temp_low);
        lv_obj_t * btn = lv_btn_create(grid);
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, row, 1);
        lv_obj_set_style_pad_all(btn, 1, 0);  // 4 px padding on every side
        //lv_obj_set_size      (btn, lv_pct(25), lv_pct(25));
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, buf);
        lv_obj_center(lbl);
        }
        // High (temp_high)
        {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f°F", sensor_thresh[r].temp_high);
        lv_obj_t * btn = lv_btn_create(grid);
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, row, 1);
        lv_obj_set_style_pad_all(btn, 1, 0);  // 4 px padding on every side
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, buf);
        lv_obj_center(lbl);
        }
        // Low (hum_low)
        {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f%%", sensor_thresh[r].hum_low);
        lv_obj_t * btn = lv_btn_create(grid);
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, 3, 1, LV_GRID_ALIGN_STRETCH, row, 1);
        lv_obj_set_style_pad_all(btn, 1, 0);  // 4 px padding on every side
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, buf);
        lv_obj_center(lbl);
        }
    }
    lv_obj_t *lbl;
    // Tab 2 screen
    lbl = lv_label_create(tab_2);
    lv_label_set_text(lbl, "Change your 4-digit PIN here");
    lv_obj_center(lbl);
    lv_obj_set_style_bg_color(lbl, lv_color_hex(0xc0c9d9), LV_PART_MAIN);

    // Tab 3 screen
    lbl = lv_label_create(tab_3);
    lv_label_set_text(lbl, "Configure Blowers Here");
    lv_obj_center(lbl);
    lv_obj_set_style_bg_color(lbl, lv_color_hex(0xc0c9d9), LV_PART_MAIN);

}



