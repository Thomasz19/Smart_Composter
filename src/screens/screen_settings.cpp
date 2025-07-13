/******************************************************************************
 * @file    screen_settings.cpp
 * @author  Thomas Zoldowski
 * @date    May 10, 2025
 * @brief   Implementation of the Motor Control and Status Screen UI.
 ******************************************************************************/

#include "screens/screen_settings.h"
#include "screens/screen_warnings.h"

#include "ui_manager.h"
#include <string.h>
#include <Arduino.h>
#include <time.h>
#include "settings_storage.h"
#include <stdint.h>


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

// ---- Runtime configuration data ----
enum ModalMode {
    MODAL_NONE,
    MODAL_SENSOR_PARAM,
    MODAL_PIN_UNLOCK,
    MODAL_PIN_CHANGE,
    MODAL_BLOWER_TIME,
    MODAL_PUMP_TIME,
    MODAL_ACTIVATION_INTERVAL,
    MODAL_CAMERA_DELAY,
    MODAL_SEND_INTERVAL
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

// Tab handles
static lv_obj_t *tab_1 = nullptr;
static lv_obj_t *tab_2 = nullptr;
static lv_obj_t *tab_3 = nullptr;

// ---- Configuration parameters ----
static int blower_duration_sec = 15;
static int pump_duration_sec   = 10;
static int activation_interval_min = 1;
static int camera_delay_sec      = 60;   // or your default
static int send_interval_min     = 5;  // or your default

// ---- Modal objects ----
static lv_obj_t *modal_bg = NULL;
static lv_obj_t *modal_ta = NULL;
static lv_obj_t *modal_kb = NULL;
static int        modal_field_id = -1;
static lv_obj_t * modal_target_btn  = NULL;

// Forward declarations
static void change_pin_btn_cb(lv_event_t *e);
static void show_modal_keypad(bool for_change);
static void params_btn_cb(lv_event_t *e);
static void modal_kb_event_cb(lv_event_t *e);
static void config_time_btn_cb(lv_event_t *e);
static void config_interval_btn_cb(lv_event_t *e);
static void config_camera_delay_cb(lv_event_t *e);
static void config_send_interval_cb(lv_event_t *e);
static void show_lock_overlays(void);

void logout_cb(lv_event_t *e) {
    security_unlocked = false;
    //ui_manager_load_login_screen();  // Optional: switch to login screen
}

/** @brief Create the Settings screen.
 *  This function initializes the settings screen with tabs for sensors, authentication, and configuration.
 *  @return Pointer to the created settings screen object.
 */
lv_obj_t* create_settings_screen() {
    //Serial.println("[UI] Creating Settings screen"); // Debug

    // Constants for layout
    const int HEADER_H   = 80;
    const int FOOTER_H   = 60;
    const int SCREEN_H   = 480;
    const int TABVIEW_H  = SCREEN_H - HEADER_H - FOOTER_H; // 350px

    // Setup Screen
    lv_obj_t* settings_screen = lv_obj_create(NULL);

    // create header + footer
    create_header(settings_screen, "Settings");

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

    tab_1 = lv_tabview_add_tab(tabview, "Sensors");
    tab_2 = lv_tabview_add_tab(tabview, "Auth.");
    tab_3 = lv_tabview_add_tab(tabview, "Config");
    
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
    //Serial.println("[UI] Initializing Tab 1");
    setup_ui_tab1();

    // Tab 2 screen ----------------------------------------------------------------------------------
    //Serial.println("[UI] Initializing Tab 2");
    setup_ui_tab2();
    
    // Tab 3 screen ----------------------------------------------------------------------------------
    //
    setup_ui_tab3(); 
    Serial.println("[UI] Initialized Tab 3");

    return settings_screen;
}


//==================================|-------|====================================================
//==================================| Tab 1 |====================================================
//==================================|_______|====================================================

/** @brief Setup UI for Tab 1 (Sensors)
 * 
 * This function creates the grid layout for sensor thresholds and
 * adds buttons to adjust high/low temperature and humidity thresholds.
 */
void setup_ui_tab1() 
{
    lv_obj_set_style_pad_all(tab_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t * grid = lv_obj_create(tab_1);
    lv_obj_set_size(grid, lv_pct(100), lv_pct(100));      // fill the tab area
    lv_obj_align   (grid, LV_ALIGN_TOP_MID, 0, 0);

    static lv_coord_t col_dsc[] = { lv_pct(10), lv_pct(45), lv_pct(45), LV_GRID_TEMPLATE_LAST };
    static lv_coord_t row_dsc[] = { lv_pct(1), lv_pct(33), lv_pct(33), lv_pct(33),  LV_GRID_TEMPLATE_LAST };  // header + 3 rows

    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, LV_PART_MAIN);

    if (pin_protection_enabled && !security_unlocked) {
        //Serial.println("[UI] Creating lock overlay on Tab 1");
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
    const char *cols[] = {"Level","High","Low"};
    for(int c = 0; c < 3; c++) {
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

        /* If you want to add buttons for the low temp threadshold, 
        uncomment this section and make appropriate changes to the rows and columns*/
        
        // Low (temp_low)
        // {
        // char buf[16];
        // snprintf(buf, sizeof(buf), "%.1f°F", sensor_thresh[r].temp_low);
        // lv_obj_t * btn = lv_btn_create(grid);
        // lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, row, 1);
        // lv_obj_set_style_pad_all(btn, 1, 0);  // 4 px padding on every side
        // //lv_obj_set_size      (btn, lv_pct(25), lv_pct(25));
        // lv_obj_t *lbl = lv_label_create(btn);
        // lv_label_set_text(lbl, buf);
        // lv_obj_center(lbl);
        // intptr_t id = r*3 + 0;
        // lv_obj_add_event_cb(btn, params_btn_cb, LV_EVENT_CLICKED, (void*)id);
        // }
        // High (temp_high)
        {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f°F", sensor_thresh[r].temp_high);
        lv_obj_t * btn = lv_btn_create(grid);
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, row, 1);
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
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, row, 1);
        lv_obj_set_style_pad_all(btn, 1, 0);  // 4 px padding on every side
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, buf);
        lv_obj_center(lbl);
        intptr_t id = r*3 + 2;
        lv_obj_add_event_cb(btn, params_btn_cb, LV_EVENT_CLICKED, (void*)id);
        }
    }

}

//==================================|-------|====================================================
//==================================| Tab 2 |====================================================
//==================================|_______|====================================================

/**
 * @brief Callback for the Change PIN button.
 * Opens a modal keypad for changing the PIN.
 */
void setup_ui_tab2() {
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
        Serial.println("[UI] Lock overlay created on Tab 2"); // Debug
    }
}

//==================================|-------|====================================================
//==================================| Tab 3 |====================================================
//==================================|_______|====================================================

/**
 * @brief Setup the third tab of the Settings screen.
 * This tab contains configuration options for activation intervals, blower, and pump.
 * It also includes a lock overlay if PIN protection is enabled and not unlocked.
 */
void setup_ui_tab3() 
{
    //lv_obj_clean(tab_3);

    const char *labels[] = {"Interval", "Blower Duration", "Pump Duration"};

   Serial.println("[UI] Creating Settings screen - Tab3"); // Debug

    lv_obj_t *grid = lv_obj_create(tab_3);
    lv_obj_set_size(grid, lv_pct(100), lv_pct(100));
    lv_obj_align(grid, LV_ALIGN_TOP_LEFT, 0, 0);

    if (pin_protection_enabled && !security_unlocked) {
        //Serial.println("[UI] Creating lock overlay tab 3"); // Debug
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

    // Enable vertical scrolling with automatic scrollbar
    lv_obj_set_scroll_dir(grid, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_AUTO);

    lv_obj_set_style_size(grid,       12, 5,          LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(grid,     LV_OPA_COVER, LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_color(grid,   lv_palette_main(LV_PALETTE_GREY), LV_PART_SCROLLBAR);

    // Define 8 rows at 50px each, plus the separator row at 10px:
    static lv_coord_t col_dsc[] = { lv_pct(65), lv_pct(35), LV_GRID_TEMPLATE_LAST };
    static lv_coord_t row_dsc[] = {
        55,   // Row 0: Config On Times title
        55,   // Row 1: Interval
        55,   // Row 2: Blower
        55,   // Row 3: Pump
        10,   // Row 4: separator line
        55,   // Row 5: Data To Server title
        55,   // Row 6: Camera Delay
        55,   // Row 7: Data Send Interval
        LV_GRID_TEMPLATE_LAST
    };

    // Set the grid layout with the defined column and row descriptors
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);

    // Styling the grid
    lv_obj_set_style_border_width(grid, 0, 0);   // Remove border
    lv_obj_set_style_pad_all(grid, 0, 0);        // Remove internal padding
    lv_obj_set_style_radius(grid, 0, 0);         // Remove rounded corners (optional)
    lv_obj_set_style_outline_width(grid, 0, 0);  // Remove outline if present
    lv_obj_set_style_bg_opa(grid , LV_OPA_TRANSP, 0);  // transparent background

    // Title row (Row 0 spanning 2 columns)
    lv_obj_t *title_lbl = lv_label_create(grid);
    lv_label_set_text(title_lbl, "Config On Times");
    lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_align(title_lbl, LV_TEXT_ALIGN_LEFT, 0);

    // Span both columns: col 0 (start), span 2 columns
    lv_obj_set_grid_cell(title_lbl,
    LV_GRID_ALIGN_CENTER, 0, 2,   // col 0, span 2 columns
    LV_GRID_ALIGN_CENTER, 0, 1);  // row 0, span 1 row

    // Rows 1-3: Interval, Blower, Pump----------------------
    for (int i = 0; i < 3; i++) {
        int row = i + 1;  // row 1 = Blower, row 2 = Pump

        // Left‐side label
        lv_obj_t *lbl = lv_label_create(grid);
        lv_label_set_text(lbl, labels[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_36, 0);
        lv_obj_set_grid_cell(lbl,
            LV_GRID_ALIGN_START, 0, 1,
            LV_GRID_ALIGN_CENTER, row, 1);

        if (i == 0) {
            // Activation Interval: button that launches modal KB
            lv_obj_t *btn = lv_btn_create(grid);
            lv_obj_set_user_data(btn, (void*)(intptr_t)MODAL_ACTIVATION_INTERVAL);
            lv_obj_add_event_cb(btn, config_interval_btn_cb, LV_EVENT_CLICKED, NULL);
            lv_obj_set_style_pad_all(btn, 6, 0);
            lv_obj_set_width(btn, 100);
            lv_obj_set_grid_cell(btn,
                LV_GRID_ALIGN_STRETCH, 1, 1,
                LV_GRID_ALIGN_CENTER, row, 1);

            lv_obj_t *btn_lbl = lv_label_create(btn);
            char buf[16];
            snprintf(buf, sizeof(buf), "%d min", activation_interval_min);
            lv_label_set_text(btn_lbl, buf);
            lv_obj_center(btn_lbl);
        }
        else {
            // Blower (i==1) or Pump (i==2)
            int idx = i - 1;  // 0=Blower, 1=Pump
            lv_obj_t *btn = lv_btn_create(grid);
            lv_obj_set_user_data(btn, (void*)(intptr_t)(idx == 0
                ? MODAL_BLOWER_TIME
                : MODAL_PUMP_TIME));
            lv_obj_add_event_cb(btn, config_time_btn_cb, LV_EVENT_CLICKED, NULL);
            lv_obj_set_style_pad_all(btn, 6, 0);
            lv_obj_set_grid_cell(btn,
                LV_GRID_ALIGN_STRETCH, 1, 1,
                LV_GRID_ALIGN_CENTER, row, 1);

            lv_obj_t *btn_lbl = lv_label_create(btn);
            char buf[16];
            int dur = (idx == 0 ? blower_duration_sec : pump_duration_sec);
            snprintf(buf, sizeof(buf), "%d sec", dur);
            lv_label_set_text(btn_lbl, buf);
            lv_obj_center(btn_lbl);
        }
    }
    // ——— Row 4: separator line ———
    static lv_point_precise_t line_points[] = { {0, 0}, {lv_obj_get_width(grid), 0} }; // Line from x=0 to x=400

    lv_obj_t *line = lv_line_create(grid);
    lv_obj_set_grid_cell(line, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_CENTER, 4, 1);
    lv_obj_set_style_line_color(line, lv_color_black(), 0);
    lv_obj_set_style_line_width(line, 8, 0);
    lv_line_set_points(line, line_points, 2);

    // ——— Row 5: Data To Server title ———
    lv_obj_t *title2 = lv_label_create(grid);
    lv_label_set_text(title2, "Data To Server");
    lv_obj_set_style_text_font(title2, &lv_font_montserrat_40, 0);
    lv_obj_set_grid_cell(title2,
        LV_GRID_ALIGN_CENTER, 0, 2,
        LV_GRID_ALIGN_CENTER, 5, 1);

    // ——— Row 6: Camera Delay ———
    // Left label
    lv_obj_t *lbl = lv_label_create(grid);
    lv_label_set_text(lbl, "Cam Delay");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_36, 0);
    lv_obj_set_grid_cell(lbl,
        LV_GRID_ALIGN_START, 0, 1,
        LV_GRID_ALIGN_CENTER, 6, 1);

    // Button
    lv_obj_t *btn = lv_btn_create(grid);
    lv_obj_set_user_data(btn, (void*)(intptr_t)MODAL_CAMERA_DELAY);
    lv_obj_add_event_cb(btn, config_camera_delay_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_pad_all(btn, 6, 0);
    lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, 6, 1);

    char buf[16];
    lv_obj_t *btn_lbl = lv_label_create(btn);
    snprintf(buf, sizeof(buf), "%d sec", camera_delay_sec);
    lv_label_set_text(btn_lbl, buf);
    lv_obj_center(btn_lbl);
    

    // ——— Row 7: Data Send Interval ———
    
    // Left label
    lv_obj_t *DataLabel = lv_label_create(grid);
    lv_label_set_text(DataLabel, "Send Interval");
    lv_obj_set_style_text_font(DataLabel, &lv_font_montserrat_36, 0);
    lv_obj_set_grid_cell(DataLabel,
        LV_GRID_ALIGN_START, 0, 1,
        LV_GRID_ALIGN_CENTER, 7, 1);

    // Button
    lv_obj_t *DataBtn = lv_btn_create(grid);
    lv_obj_set_user_data(DataBtn, (void*)(intptr_t)MODAL_SEND_INTERVAL);
    lv_obj_add_event_cb(DataBtn, config_send_interval_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_pad_all(DataBtn, 6, 0);
    lv_obj_set_grid_cell(DataBtn,
        LV_GRID_ALIGN_STRETCH, 1, 1,
        LV_GRID_ALIGN_CENTER, 7, 1);

    lv_obj_t *DataBtn_lbl = lv_label_create(DataBtn);
    snprintf(buf, sizeof(buf), "%d min", send_interval_min);
    lv_label_set_text(DataBtn_lbl, buf);
    lv_obj_center(DataBtn_lbl);

}

/** @brief Callback for the camera delay button
 */
static void config_camera_delay_cb(lv_event_t *e) {
    lv_obj_t *btn = (lv_obj_t*)lv_event_get_target(e);
    modal_mode = MODAL_CAMERA_DELAY;
    modal_target_btn = btn;
    show_modal_keypad(false);
}

/** @brief Callback for the send interval button
 */
static void config_send_interval_cb(lv_event_t *e) {
    lv_obj_t *btn = (lv_obj_t*)lv_event_get_target(e);
    modal_mode = MODAL_SEND_INTERVAL;
    modal_target_btn = btn;
    show_modal_keypad(false);
}

/** @brief Callback for the keyboard event when the user presses "Enter/OK"
    * This is where we handle the input from the keypad
    */
static void config_time_btn_cb(lv_event_t *e) {
    lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
    int index = (int)(intptr_t)lv_obj_get_user_data(btn); // 0=blower, 1=pump
    modal_mode = (index == 0) ? MODAL_BLOWER_TIME : MODAL_PUMP_TIME;
    modal_target_btn = btn;
    show_modal_keypad(false);
}

/** @brief Callback for the keyboard event when the user presses "Enter/OK"
    * This is where we handle the input from the keypad
    */
static void config_interval_btn_cb(lv_event_t *e) {
    lv_obj_t *btn = (lv_obj_t*)lv_event_get_target(e);
    modal_mode = MODAL_ACTIVATION_INTERVAL;
    modal_target_btn = btn;
    show_modal_keypad(false);
}

/** @brief Callback for the keyboard event when the user presses "Enter/OK"
    * This is where we handle the input from the keypad
    */
void lock_overlay_cb(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target_obj(e);
    // Disable further clicks immediately
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    //Serial.println("[UI] Lock overlay tapped"); // Debug
    modal_mode = MODAL_PIN_UNLOCK;
    show_modal_keypad(false);
    Serial.println("[UI] Lock overlay tapped"); // Debug
}

/** @brief Callback for the change PIN button
    * This is where we handle the input from the keypad
    */
static void change_pin_btn_cb(lv_event_t *e) {
    modal_mode = MODAL_PIN_CHANGE;
    show_modal_keypad(true);
    Serial.println("[UI] Change PIN button tapped"); // Debug
}

/** @brief Callback for the keyboard event when the user presses "Enter/OK"
    * This is where we handle the input from the keypad
    */
static void params_btn_cb(lv_event_t * e) {
    Serial.println("[UI] Params button clicked"); // Debug
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
        if(modal_bg == nullptr) {
            Serial.println("Error: modal_bg is NULL");
            return;
        }
        lv_obj_del(modal_bg);
        // Clear your globals
        modal_bg         = nullptr;
        modal_ta         = nullptr;
        modal_kb         = nullptr;
        modal_target_btn = nullptr;
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
    Serial.println("[UI] Params button callback executed"); // Debug
}

/**
 * @brief Show a modal keypad for numeric input.
 * @param for_change If true, the keypad is used to change the PIN.
 */
void show_modal_keypad(bool for_change) {
    static bool modal_in_progress = false;
    if (modal_in_progress) return;
    modal_in_progress = true;
    // Debug
    Serial.println("show_modal_keypad");
    // open kp
    if (modal_bg) {
        lv_obj_del(modal_bg);
        modal_bg = nullptr;
        modal_kb = nullptr;
        modal_ta = nullptr;
        modal_target_btn = nullptr;
        modal_field_id = -1;
    }

    lv_obj_t *parent = lv_scr_act();
    if (!parent) {
        Serial.println("Error: No active screen for modal parent!");
        modal_in_progress = false;
        return;
    }

    // Create a translucent full‐screen background
    modal_bg = lv_obj_create(parent);

    if (!modal_bg) {
        Serial.println("Error: modal_bg is NULL");
        return;
    }
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
        if(modal_bg) {
        lv_obj_del(modal_bg);
        modal_bg = nullptr;
        modal_kb = nullptr;
        modal_ta = nullptr;
        modal_target_btn = nullptr;
        modal_field_id = -1;
    }
    }, LV_EVENT_CLICKED, NULL);

    // create textarea for numeric input
    modal_ta = lv_textarea_create(modal_bg);
    lv_obj_set_width(modal_ta, 200);
    lv_obj_align(modal_ta, LV_ALIGN_CENTER, 0, -90);
    lv_textarea_set_one_line(modal_ta, true);
    lv_textarea_set_max_length(modal_ta, 6);
    lv_textarea_set_text(modal_ta, "");
    lv_obj_set_style_text_font(modal_ta, &lv_font_montserrat_40, LV_PART_MAIN | LV_STATE_DEFAULT);

    // create keyboard
    modal_kb = lv_keyboard_create(modal_bg);
    lv_keyboard_set_mode(modal_kb, LV_KEYBOARD_MODE_NUMBER);
    lv_obj_set_style_text_font(modal_kb, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_size(modal_kb, lv_pct(100), lv_pct(60));
    lv_obj_align(modal_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(modal_kb, modal_ta);

    lv_obj_add_event_cb(modal_kb, modal_kb_event_cb, LV_EVENT_READY, NULL);
    Serial.println("[UI] Modal keypad shown"); // Debug
    modal_in_progress = false;
}

/** @brief Callback for the modal keyboard event when the user presses "Enter/OK"
 * This is where we handle the input from the keypad
 */
static void modal_kb_event_cb(lv_event_t *e) {
    Serial.println("[UI] Modal keyboard event callback"); // Debug
    if (!modal_bg || !modal_ta || !modal_kb) return;
    if (lv_event_get_code(e) != LV_EVENT_READY) return;
    const char *txt = lv_textarea_get_text(modal_ta);
    Serial.println("[DEBUG] Modal keyboard input");
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

                // reset inactivity timer right now
                extern unsigned long last_activity;
                last_activity = millis();

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
                // Show error message box
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
        case MODAL_ACTIVATION_INTERVAL: {
            int minutes = atoi(txt);
            if (minutes < 1) minutes = 1;

            // Update your globals & config
            activation_interval_min            = minutes;
            config.activation_interval_min     = minutes;
            saveConfig();

            // Update the button’s label
            lv_obj_t *lbl = lv_obj_get_child((lv_obj_t*)modal_target_btn, 0);
            char buf[16];
            snprintf(buf, sizeof(buf), "%d min", minutes);
            lv_label_set_text(lbl, buf);
            lv_obj_center(lbl);
            break;
            }
        case MODAL_CAMERA_DELAY: {
            int seconds = atoi(txt);
            if (seconds < 1) seconds = 1;

            // Update your globals & config
            camera_delay_sec            = seconds;
            config.camera_delay_sec     = seconds;
            saveConfig();

            // Update the button’s label
            lv_obj_t *lbl = lv_obj_get_child((lv_obj_t*)modal_target_btn, 0);
            char buf[16];
            snprintf(buf, sizeof(buf), "%d s", seconds);
            lv_label_set_text(lbl, buf);
            lv_obj_center(lbl);
            break;
        }

        case MODAL_SEND_INTERVAL: {
            int minutes = atoi(txt);
            if (minutes < 1) minutes = 1;

            // Update your globals & config
            send_interval_min            = minutes;
            config.send_interval_min     = minutes;
            saveConfig();

            // Update the button’s label
            lv_obj_t *lbl = lv_obj_get_child((lv_obj_t*)modal_target_btn, 0);
            char buf[16];
            snprintf(buf, sizeof(buf), "%d min", minutes);
            lv_label_set_text(lbl, buf);
            lv_obj_center(lbl);
            break;
        }
        default:
            break;
    }

    if (modal_bg) {
        lv_obj_del(modal_bg); // This deletes modal_bg and all its children (modal_kb, modal_ta, etc.)
        modal_bg = nullptr;
        modal_kb = nullptr;
        modal_ta = nullptr;
        modal_target_btn = nullptr;
        modal_field_id = -1;
    }
    modal_mode = MODAL_NONE; // Reset modal mode
}

/**
 * @brief Get the current blower on time.
 * @return The current blower on time in seconds.
 */
uint16_t getBlowerOnTime() {
    // returns the current numeric value of the blower-time spinbox
    return blower_duration_sec;
}

/**
 * @brief Get the current pump on time.
 * @return The current pump on time in seconds.
 */
uint16_t getPumpOnTime() {
    // returns the current numeric value of the pump-time spinbox
    return pump_duration_sec;
}

/**
 * @brief Get the current activation interval in seconds.
 * @return The current activation interval in seconds.
 */
uint16_t getActivationInterval() {
    // returns the current numeric value of the activation interval
    return activation_interval_min * 60; // convert minutes to seconds
}

/**
 * @brief Get the current camera delay in seconds.
 * @return The current camera delay in seconds.
 */
uint16_t getCameraDelay() {
    // returns the current numeric value of the camera delay
    return camera_delay_sec;
}

/**
 * @brief Get the current data send interval in seconds.
 * @return The current data send interval in seconds.
 */
uint16_t getSendInterval() {
    // returns the current numeric value of the data send interval
    return send_interval_min * 60; // convert minutes to seconds
}

/**
 * @brief Get the current high temperature threshold for a sensor.
 * @param sensor_id The ID of the sensor (0, 1, or 2).
 * @return The current high temperature threshold in degrees Fahrenheit.
 */
uint16_t getTempHighThreshold(int sensor_id) {
    // returns the current numeric value of the high temperature threshold
    return sensor_thresh[sensor_id].temp_high;
}

/**
 * @brief Get the current low temperature threshold for a sensor.
 * @param sensor_id The ID of the sensor (0, 1, or 2).
 * @return The current low temperature threshold in degrees Fahrenheit.
 */
uint16_t getHumLowThreshold(int sensor_id) {
    // returns the current numeric value of the low humidity threshold
    return sensor_thresh[sensor_id].hum_low;
}
/**
 * @brief Initialize settings from the configuration.
 * This function copies sensor thresholds, PIN, lock state, and blower/pump times
 * from the config structure to the global settings variables.
 */
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
    activation_interval_min = config.activation_interval_min;

    camera_delay_sec = config.camera_delay_sec;
    send_interval_min = config.send_interval_min;
}

/**
 * @brief Check if the PIN is currently unlocked.
 * @return True if the PIN is unlocked, false otherwise.
 */
bool check_pin() {
    return security_unlocked;
}

/**
 * @brief Check if the PIN protection is enabled.
 * @return True if PIN protection is enabled, false otherwise.
 */
void security_timeout_check() {
    // This function can be called periodically to check if the security should be locked again
    // For example, if the user has been inactive for a certain period
    unsigned long now = millis();

    if (security_unlocked && (now - last_activity > 60000*5)) { // 5 minutes of inactivity
        security_unlocked = false;
        show_lock_overlays();
        logout_cb(nullptr); // Call logout to reset UI state
    }
    
}

static void show_lock_overlays(void) {
    if(!pin_protection_enabled) return;

    // Tab 1
    if(!security_unlocked && lock_overlay_tab1 == NULL) {
        lock_overlay_tab1 = lv_btn_create(tab_1);
        lv_obj_set_size(lock_overlay_tab1, lv_pct(95), lv_pct(95));
        lv_obj_align(lock_overlay_tab1, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_color(lock_overlay_tab1, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(lock_overlay_tab1, LV_OPA_70, LV_PART_MAIN);
        lv_obj_add_event_cb(lock_overlay_tab1, lock_overlay_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t *lbl = lv_label_create(lock_overlay_tab1);
        lv_label_set_text(lbl, "Tap to Unlock");
        lv_obj_center(lbl);
    }

    // Tab 2
    if(!security_unlocked && lock_overlay_tab2 == NULL) {
        lock_overlay_tab2 = lv_btn_create(tab_2);
        lv_obj_set_size(lock_overlay_tab2, lv_pct(100), lv_pct(100));
        lv_obj_align(lock_overlay_tab2, LV_ALIGN_TOP_LEFT, 0, 0);
        lv_obj_set_style_bg_color(lock_overlay_tab2, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(lock_overlay_tab2, LV_OPA_70, LV_PART_MAIN);
        lv_obj_add_event_cb(lock_overlay_tab2, lock_overlay_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t *lbl = lv_label_create(lock_overlay_tab2);
        lv_label_set_text(lbl, "Tap to Unlock");
        lv_obj_center(lbl);
    }

    // Tab 3
    if(!security_unlocked && lock_overlay_tab3 == NULL) {
        lock_overlay_tab3 = lv_btn_create(tab_3);
        lv_obj_set_size(lock_overlay_tab3, lv_pct(100), lv_pct(100));
        lv_obj_align(lock_overlay_tab3, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_color(lock_overlay_tab3, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(lock_overlay_tab3, LV_OPA_70, LV_PART_MAIN);
        lv_obj_add_event_cb(lock_overlay_tab3, lock_overlay_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t *lbl = lv_label_create(lock_overlay_tab3);
        lv_label_set_text(lbl, "Tap to Unlock");
        lv_obj_center(lbl);
    }
}

// ps. sorry for the long code, but I wanted to keep it all together
//     I hope it’s still readable and understandable!