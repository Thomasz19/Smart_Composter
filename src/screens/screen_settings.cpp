/******************************************************************************
 * @file    screen_manual.cpp
 * @author  Thomas Zoldowski
 * @date    May 10, 2025
 * @brief   Implementation of the Motor Control and Status Screen UI.
 ******************************************************************************/

#include "screens/screen_manual.h"
#include "screens/screen_sensors.h"
#include "screens/screen_settings.h"
#include "screens/screen_warnings.h"

#include "ui_manager.h"
#include <string.h>
#include <Arduino.h>
#include <time.h>
#include "settings_storage.h"

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

enum ModalMode {
    MODAL_NONE,
    MODAL_SENSOR_PARAM,
    MODAL_PIN_UNLOCK,
    MODAL_PIN_CHANGE,
    MODAL_BLOWER_TIME,
    MODAL_PUMP_TIME 
};
static ModalMode modal_mode      = MODAL_NONE;

// Runtime PIN & state
static char user_pin[5] = "0000";
static bool pin_protection_enabled = true;
static bool security_unlocked       = false;

// Overlay and modal handles
static lv_obj_t *lock_overlay_tab1 = nullptr;
static lv_obj_t *lock_overlay_tab2 = nullptr;
static lv_obj_t *lock_overlay_tab3 = nullptr;

static int blower_duration_sec = 15;
static int pump_duration_sec   = 10;

// ---- Modal objects ----
static lv_obj_t *modal_bg = NULL;
static lv_obj_t *modal_ta = NULL;
static lv_obj_t *modal_kb = NULL;
static int        modal_field_id = -1;
static lv_obj_t * modal_target_btn  = NULL;

// Forward declarations
static void lock_overlay_cb(lv_event_t *e);
static void change_pin_btn_cb(lv_event_t *e);
static void show_modal_keypad(bool for_change);
static void params_btn_cb(lv_event_t *e);
static void modal_kb_event_cb(lv_event_t *e);
static void config_time_btn_cb(lv_event_t *e);

void logout_cb(lv_event_t *e) {
    security_unlocked = false;
    //ui_manager_load_login_screen();  // Optional: switch to login screen
}

lv_obj_t* create_settings_screen() {
    Serial.println("[UI] Creating Settings screen"); // Debug

    // Constants for layout
    const int HEADER_H   = 80;
    const int FOOTER_H   = 60;
    const int SCREEN_H   = 480;
    const int TABVIEW_H  = SCREEN_H - HEADER_H - FOOTER_H; // 350px

    // Setup Screen
    settings_screen = lv_obj_create(NULL);
    

    // create header + footer
    create_header(settings_screen, "Settings");
    create_footer(settings_screen);

    // Tabview on the left
    lv_obj_t *tabview = lv_tabview_create(settings_screen);
    lv_tabview_set_tab_bar_position(tabview, LV_DIR_LEFT);  
    lv_tabview_set_tab_bar_size    (tabview, 170);     

    // Size & position between header & footer
    lv_obj_set_size(tabview, lv_pct(100), TABVIEW_H);
    lv_obj_align   (tabview, LV_ALIGN_TOP_MID, 0, HEADER_H);
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

    // Tab 1 screen ----------------------------------------------------------------------------------
    Serial.println("[UI] Initializing Tab 1");
    setup_ui_tab1(tab_1);

    // Tab 2 screen ----------------------------------------------------------------------------------
    Serial.println("[UI] Initializing Tab 2");
    setup_ui_tab2(tab_2);
    
    // Tab 3 screen ----------------------------------------------------------------------------------
    Serial.println("[UI] Initializing Tab 3");
    setup_ui_tab3(tab_3); 

    return settings_screen;
}
void setup_ui_tab1(lv_obj_t *tab_1) 
{
    lv_obj_set_style_pad_all(tab_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t * grid = lv_obj_create(tab_1);
    lv_obj_set_size(grid, lv_pct(100), lv_pct(100));      // fill the tab area
    lv_obj_align   (grid, LV_ALIGN_TOP_MID, 0, 0);

    static lv_coord_t col_dsc[] = { lv_pct(1), lv_pct(33), lv_pct(33), lv_pct(33), LV_GRID_TEMPLATE_LAST };
    static lv_coord_t row_dsc[] = { lv_pct(1), lv_pct(33), lv_pct(33), lv_pct(33),  LV_GRID_TEMPLATE_LAST };  // header + 3 rows

    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, LV_PART_MAIN);

    if (pin_protection_enabled && !security_unlocked) {
        Serial.println("[UI] Creating lock overlay on Tab 1");
        lock_overlay_tab1 = lv_btn_create(tab_1);
        lv_obj_set_size(lock_overlay_tab1, lv_pct(95), lv_pct(95));
        lv_obj_align(lock_overlay_tab1, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_color(lock_overlay_tab1, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(lock_overlay_tab1, LV_OPA_70, LV_PART_MAIN);
        lv_obj_clear_flag(lock_overlay_tab1, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(lock_overlay_tab1, lock_overlay_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t *lbl = lv_label_create(lock_overlay_tab1);
        lv_label_set_text(lbl, "Tap to Unlock");
        lv_obj_center(lbl);
    }
    // Column titles
    const char *cols[] = {"Lev.","Low","High","Low"};
    for(int c = 0; c < 4; c++) {
        // Column header as plain label
        lv_obj_t *label_col = lv_label_create(grid);
        lv_label_set_text(label_col, cols[c]);
        lv_obj_align(label_col, LV_ALIGN_CENTER, 0, 0);

        // Place in row 0, column c
        lv_obj_set_grid_cell(label_col,
        LV_GRID_ALIGN_CENTER, c, 1,
        LV_GRID_ALIGN_CENTER, 0, 1);

        // styling
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
        intptr_t id = r*3 + 0;
        lv_obj_add_event_cb(btn, params_btn_cb, LV_EVENT_CLICKED, (void*)id);
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
        intptr_t id = r*3 + 1;
        lv_obj_add_event_cb(btn, params_btn_cb, LV_EVENT_CLICKED, (void*)id);
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
        intptr_t id = r*3 + 2;
        lv_obj_add_event_cb(btn, params_btn_cb, LV_EVENT_CLICKED, (void*)id);
        }
    }

}

void setup_ui_tab2(lv_obj_t *tab_2) {
    // Change PIN button
    
    Serial.println("Create Settings screen - Tab2"); // Debug
    lv_obj_t *btn_cp = lv_btn_create(tab_2);
    lv_obj_set_size(btn_cp, 300, 100);
    lv_obj_align(btn_cp, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_add_event_cb(btn_cp, change_pin_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_cp = lv_label_create(btn_cp);
    lv_label_set_text(lbl_cp, "Change PIN");
    lv_obj_center(lbl_cp);

    // Require PIN switch
    // not doing switch anymore unless they ask for it

    // Overlay
    if (pin_protection_enabled && !security_unlocked) {
        Serial.println("[UI] Creating lock overlay"); // Debug
        lock_overlay_tab2 = lv_btn_create(tab_2);
        lv_obj_set_size(lock_overlay_tab2, lv_pct(100), lv_pct(100));
        lv_obj_align(lock_overlay_tab2, LV_ALIGN_TOP_LEFT, 0, 0);
        lv_obj_set_style_bg_color(lock_overlay_tab2, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(lock_overlay_tab2, LV_OPA_70, LV_PART_MAIN);
        lv_obj_clear_flag(lock_overlay_tab2, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(lock_overlay_tab2, lock_overlay_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t *lbl = lv_label_create(lock_overlay_tab2);
        lv_label_set_text(lbl, "Tap to Unlock");
        lv_obj_center(lbl);
    }
}

void setup_ui_tab3(lv_obj_t *tab_3) 
{
    lv_obj_clean(tab_3);

    const char *labels[] = {"Blower", "Pump"};

    Serial.println("[UI] Setting up Config Tab");

    lv_obj_t *grid = lv_obj_create(tab_3);
    lv_obj_set_size(grid, lv_pct(100), lv_pct(100));
    lv_obj_align(grid, LV_ALIGN_TOP_LEFT, 0, 0);

    if (pin_protection_enabled && !security_unlocked) {
        Serial.println("[UI] Creating lock overlay tab 3"); // Debug
        lock_overlay_tab3 = lv_btn_create(tab_3);
        lv_obj_set_size(lock_overlay_tab3, lv_pct(100), lv_pct(100));
        lv_obj_align(lock_overlay_tab3, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_color(lock_overlay_tab3, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(lock_overlay_tab3, LV_OPA_70, LV_PART_MAIN);
        lv_obj_clear_flag(lock_overlay_tab3, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(lock_overlay_tab3, lock_overlay_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t *lbl = lv_label_create(lock_overlay_tab3);
        lv_label_set_text(lbl, "Tap to Unlock");
        lv_obj_center(lbl);
    }

    static lv_coord_t col_dsc[] = { lv_pct(40), lv_pct(60), LV_GRID_TEMPLATE_LAST };
    static lv_coord_t row_dsc[] = { lv_pct(25), lv_pct(37), lv_pct(37), LV_GRID_TEMPLATE_LAST };

    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    lv_obj_set_style_border_width(grid, 0, 0);   // Remove border
    lv_obj_set_style_pad_all(grid, 0, 0);        // Remove internal padding
    lv_obj_set_style_radius(grid, 0, 0);         // Remove rounded corners (optional)
    lv_obj_set_style_outline_width(grid, 0, 0);  // Remove outline if present
    lv_obj_set_style_bg_opa(grid , LV_OPA_TRANSP, 0);  // transparent background

    // Title row (Row 0 spanning 2 columns)
    lv_obj_t *title_lbl = lv_label_create(grid);
    lv_label_set_text(title_lbl, "Config On Time Per Hour");
    lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_align(title_lbl, LV_TEXT_ALIGN_LEFT, 0);

    // Span both columns: col 0 (start), span 2 columns
    lv_obj_set_grid_cell(title_lbl,
    LV_GRID_ALIGN_CENTER, 0, 2,   // col 0, span 2 columns
    LV_GRID_ALIGN_CENTER, 0, 1);  // row 0, span 1 row

    for (int i = 0; i < 2; i++) {
        int row = i + 1;  // row 1 = Blower, row 2 = Pump

        // Label (column 0)
        lv_obj_t *lbl = lv_label_create(grid);
        lv_label_set_text(lbl, labels[i]);
        lv_obj_set_grid_cell(lbl, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, row, 1);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_40, 0);

        // Button (column 1)
        lv_obj_t *btn = lv_btn_create(grid);
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, row, 1);
        lv_obj_set_style_pad_all(btn, 10, 0);
        lv_obj_set_user_data(btn, (void*)(intptr_t)i); // store row index: 0 = blower, 1 = pump
        lv_obj_add_event_cb(btn, config_time_btn_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t *btn_lbl = lv_label_create(btn);
        char buf[16];
        if (i == 0) {
            // Blower
            snprintf(buf, sizeof(buf), "%d sec", blower_duration_sec);
        } else {
            // Pump
            snprintf(buf, sizeof(buf), "%d sec", pump_duration_sec);
        }
        lv_label_set_text(btn_lbl, buf);
        lv_obj_center(btn_lbl);
    }

}

static void config_time_btn_cb(lv_event_t *e) {
    lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
    int index = (int)(intptr_t)lv_obj_get_user_data(btn); // 0=blower, 1=pump
    modal_mode = (index == 0) ? MODAL_BLOWER_TIME : MODAL_PUMP_TIME;
    modal_target_btn = btn;
    show_modal_keypad(false);
}

// Unlock overlay tapped
static void lock_overlay_cb(lv_event_t *e) {
    //lv_timer(10);
    modal_mode = MODAL_PIN_UNLOCK;
    show_modal_keypad(false);
}

static void change_pin_btn_cb(lv_event_t *e) {
    modal_mode = MODAL_PIN_CHANGE;
    show_modal_keypad(true);
}


static void params_btn_cb(lv_event_t * e) {
    modal_mode      = MODAL_SENSOR_PARAM;
    // remember who launched us and what field
    modal_target_btn = (lv_obj_t *)lv_event_get_target(e);
    modal_field_id   = (int)(intptr_t)lv_event_get_user_data(e);

    // translucent full‐screen bg
    modal_bg = lv_obj_create(lv_scr_act());
    lv_obj_set_size(modal_bg, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(modal_bg, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(modal_bg, LV_OPA_50, 0);

    // create close button
    lv_obj_t * close_btn = lv_btn_create(modal_bg);
    lv_obj_set_size(close_btn, 100, 100);
    lv_obj_align(close_btn, LV_ALIGN_TOP_RIGHT, -10, 10);

    lv_obj_t * close_lbl = lv_label_create(close_btn);
    lv_label_set_text(close_lbl, LV_SYMBOL_CLOSE);
    lv_obj_center(close_lbl);
    lv_obj_set_style_text_font(
    close_lbl,
    &lv_font_montserrat_48,
    LV_PART_MAIN | LV_STATE_DEFAULT
    );

    // Hook its click event to delete the overlay
    lv_obj_add_event_cb(close_btn, [](lv_event_t * e){
        LV_UNUSED(e);
        // Destroy the whole modal (bg → ta, kb, cancel get removed too)
        lv_obj_del(modal_bg);
        // Clear your globals
        modal_bg         = NULL;
        modal_ta         = NULL;
        modal_kb         = NULL;
        modal_target_btn = NULL;
        modal_field_id   = -1;
    }, LV_EVENT_CLICKED, NULL);

    // create textarea for numeric input
    modal_ta = lv_textarea_create(modal_bg);
    lv_obj_set_width(modal_ta, 200);
    lv_obj_align(modal_ta, LV_ALIGN_CENTER, 0, -90);
    lv_textarea_set_one_line(modal_ta, true);
    lv_textarea_set_max_length(modal_ta, 6);
    lv_textarea_set_text(modal_ta, "");
    lv_obj_set_style_text_font(
    modal_ta,
    &lv_font_montserrat_40,
    LV_PART_MAIN | LV_STATE_DEFAULT
    );
    // create keyboard
    modal_kb = lv_keyboard_create(modal_bg);
    lv_keyboard_set_mode(modal_kb, LV_KEYBOARD_MODE_NUMBER);
    lv_obj_set_style_text_font(
    modal_kb,
    &lv_font_montserrat_48,         // pick any size you’ve built into LVGL
    LV_PART_MAIN | LV_STATE_DEFAULT
    );
    lv_obj_set_size(modal_kb, lv_pct(100), lv_pct(60));
    lv_obj_align(modal_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(modal_kb, modal_ta);

    // when the user presses “Enter/OK”, we’ll get an LV_EVENT_READY
    lv_obj_add_event_cb(modal_kb, modal_kb_event_cb, LV_EVENT_READY, NULL);
}

void show_modal_keypad(bool for_change) {
    // Debug
    Serial.println("show_modal_keypad");
    // open kp
    modal_bg = lv_obj_create(lv_scr_act());
    lv_obj_set_size(modal_bg, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(modal_bg, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(modal_bg, LV_OPA_50, 0);

    lv_obj_t *close_btn = lv_btn_create(modal_bg);
    lv_obj_set_size(close_btn, 100, 100);
    lv_obj_align(close_btn, LV_ALIGN_TOP_RIGHT, -10, 10);

    lv_obj_t *close_lbl = lv_label_create(close_btn);
    lv_label_set_text(close_lbl, LV_SYMBOL_CLOSE);
    lv_obj_center(close_lbl);
    lv_obj_set_style_text_font(close_lbl, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(close_btn, [](lv_event_t *e) {
        lv_obj_del(modal_bg);
        modal_bg = modal_kb = modal_ta = modal_target_btn = nullptr;
        modal_field_id = -1;
    }, LV_EVENT_CLICKED, NULL);

    modal_ta = lv_textarea_create(modal_bg);
    lv_obj_set_width(modal_ta, 200);
    lv_obj_align(modal_ta, LV_ALIGN_CENTER, 0, -90);
    lv_textarea_set_one_line(modal_ta, true);
    lv_textarea_set_max_length(modal_ta, 6);
    lv_textarea_set_text(modal_ta, "");
    lv_obj_set_style_text_font(modal_ta, &lv_font_montserrat_40, LV_PART_MAIN | LV_STATE_DEFAULT);

    modal_kb = lv_keyboard_create(modal_bg);
    lv_keyboard_set_mode(modal_kb, LV_KEYBOARD_MODE_NUMBER);
    lv_obj_set_style_text_font(modal_kb, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_size(modal_kb, lv_pct(100), lv_pct(60));
    lv_obj_align(modal_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(modal_kb, modal_ta);

    lv_obj_add_event_cb(modal_kb, modal_kb_event_cb, LV_EVENT_READY, NULL);
}

static void modal_kb_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_READY) return;
    const char *txt = lv_textarea_get_text(modal_ta);

    switch (modal_mode) {
        case MODAL_SENSOR_PARAM: {
            float new_val = atof(txt);
            int sensor = modal_field_id / 3;
            int field = modal_field_id % 3;
            switch(field) {
                case 0:
                    sensor_thresh[sensor].temp_low  = new_val;
                    config.temp_low[sensor]         = new_val;
                    break;
                case 1:
                    sensor_thresh[sensor].temp_high = new_val;
                    config.temp_high[sensor]        = new_val;
                    break;
                case 2:
                    sensor_thresh[sensor].hum_low   = new_val;
                    config.hum_low[sensor]          = new_val;
                    break;
            }

            lv_obj_t * lbl = lv_obj_get_child((lv_obj_t *)modal_target_btn, 0);
            static char buf[16];
            if(field < 2) {  // temperature
                snprintf(buf, sizeof(buf), "%.1f°F", new_val);
            } else {         // humidity
                snprintf(buf, sizeof(buf), "%.1f%%", new_val);
            }
            lv_label_set_text(lbl, buf);
            lv_obj_center(lbl);
            // **Persist the change immediately:**
            saveConfig();
            break;
        }
        case MODAL_PIN_UNLOCK: {
            if (strcmp(txt, user_pin) == 0) {
                if (lock_overlay_tab1) {
                    lv_obj_del(lock_overlay_tab1);
                    lock_overlay_tab1 = nullptr;
                }
                if (lock_overlay_tab2) {
                    lv_obj_del(lock_overlay_tab2);
                    lock_overlay_tab2 = nullptr;
                }
                if (lock_overlay_tab3) {
                    lv_obj_del(lock_overlay_tab3);
                    lock_overlay_tab3 = nullptr;
                }
                security_unlocked = true;
            } else {
                printf("Entered PIN: %s, Stored PIN: %s\n", txt, user_pin);
                lv_obj_t *mbox = lv_msgbox_create(lv_scr_act());
                lv_obj_set_size(mbox, 300, 200);
                lv_msgbox_add_title(mbox, "Error");
                lv_msgbox_add_text(mbox, "Wrong PIN");
                lv_obj_center(mbox);
                lv_obj_set_style_text_font(mbox, &lv_font_montserrat_40, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_msgbox_add_close_button(mbox);
            }
            break;
        }
        case MODAL_PIN_CHANGE: {
            if (strlen(txt) == 4) {
                strcpy(user_pin, txt);       // update UI global
                strcpy(config.user_pin, txt); // copy into config
                saveConfig();
            }
            break;
        }
        case MODAL_BLOWER_TIME:
        case MODAL_PUMP_TIME: {
            int seconds = atoi(txt);
            if (seconds <= 0) seconds = 1;  // sanity check

            if (modal_mode == MODAL_BLOWER_TIME){
                blower_duration_sec = seconds;
                config.blower_duration_sec = seconds; // copy into config
                saveConfig();
            }
            else{
                pump_duration_sec = seconds;
                config.pump_duration_sec = seconds;  // copy into config
                saveConfig();
            }

            // Update button label
            lv_obj_t *lbl = lv_obj_get_child(modal_target_btn, 0);
            char buf[16];
            snprintf(buf, sizeof(buf), "%d sec", seconds);
            lv_label_set_text(lbl, buf);
            lv_obj_center(lbl);
            break;
        }
        default:
            break;
    }

    lv_obj_del(modal_kb);
    lv_obj_del(modal_ta);
    lv_obj_del(modal_bg);

    modal_mode = MODAL_NONE;
    modal_field_id = -1;
    modal_target_btn = nullptr;
}

// Call this once in setup(), after loadConfig(), to copy values into the UI’s private globals.
void settings_init_from_config() {
    // Copy sensor thresholds:
    for (int i = 0; i < 3; i++) {
        sensor_thresh[i].temp_low  = config.temp_low[i];
        sensor_thresh[i].temp_high = config.temp_high[i];
        sensor_thresh[i].hum_low   = config.hum_low[i];
    }

    // Copy PIN and lock state:
    strcpy(user_pin, config.user_pin);
    pin_protection_enabled = config.pin_protection_enabled;
    security_unlocked = false; // always start locked on reboot

    // Copy blower/pump times:
    blower_duration_sec = config.blower_duration_sec;
    pump_duration_sec   = config.pump_duration_sec;
}


bool check_pin() {
    return security_unlocked;
}