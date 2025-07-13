/******************************************************************************
 * @file    ui_manager.cpp
 * @author  Thomas Zoldowski
 * @date    May 10, 2025
 * @brief   UI manager implementation for Smart Composter screen navigation.
 *
 * This file implements screen switching logic using LVGL. It loads different
 * screens dynamically based on the current screen ID and user interactions.
 ******************************************************************************/

#include "ui_manager.h"
#include "screens/screen_home.h"
#include "screens/screen_sensors.h"
#include "screens/screen_warnings.h"
#include "screens/screen_manual.h"
#include "screens/screen_history.h"
#include "screens/screen_settings.h"
#include <Arduino.h>
#include <string.h>

extern void global_input_event_cb(lv_event_t * e);

static lv_style_t dropdown_list_style;
static bool dropdown_style_initialized = false;


static lv_obj_t *current_screen = nullptr;

// Global selected index for dropdown menu
lv_obj_t *home_screen = nullptr;
lv_obj_t *sensor_screen = nullptr;
lv_obj_t *manual_screen = nullptr;
lv_obj_t *warnings_screen = nullptr;
lv_obj_t *settings_screen = nullptr;

// Footer variables
static bool footer_flash_state = false;
static uint32_t last_footer_flash = 0;
static uint32_t prev_warning_mask = -1;
static const uint32_t FOOTER_FLASH_INTERVAL = 500;

lv_obj_t *global_footer;
lv_obj_t *global_footer_label;

// Global dropdown menu selection index
lv_obj_t *dropdown = nullptr;

/*
Function: Drop down selection even handler
*/
static void dropdown_event_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target_obj(e);

    // Open drop down menu and Add styling
    if (code == LV_EVENT_CLICKED) {
        lv_dropdown_open(obj);
        Serial.println("[GDL] opening dropdown"); // Debug
        lv_obj_t *list = lv_dropdown_get_list(obj);
        if (list) {
            // Apply custom style only when the list is available
            lv_obj_add_style(list, &dropdown_list_style, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_max_height(list, LV_SIZE_CONTENT, 0);
            lv_obj_set_scroll_dir(list, LV_DIR_NONE);
            lv_obj_set_height(list, 71 * 4);
        }
    }

    // Change screen
    if(code == LV_EVENT_VALUE_CHANGED) {
        char buf[32];
       
        lv_dropdown_get_selected_str(obj, buf, sizeof(buf));
        Serial.print("[GDL] Selected: ");
        Serial.println(buf);
        handle_screen_selection(buf);
        lv_obj_set_parent(global_footer, current_screen);
        lv_obj_set_align(global_footer, LV_ALIGN_BOTTOM_MID);
    }
}

/** @brief Create a global dropdown menu for screen selection.
 *  This function initializes a dropdown menu that allows users to switch between different screens.
 *  It sets the dropdown's options and styles, and adds an event handler for selection changes.
 *  @param parent Pointer to the parent object where the dropdown will be created.
 */
void create_global_dropdown(lv_obj_t *parent) {

    ensure_dropdown_style();

    Serial.println("Create global dropdown"); // Debug

    dropdown = lv_dropdown_create(parent);

    lv_obj_set_size(dropdown, 80, 76);  // width: 150px, height: 40px
    lv_obj_align(dropdown, LV_ALIGN_TOP_LEFT, 2, 2);
    lv_dropdown_set_symbol(dropdown, LV_SYMBOL_LIST);
    //lv_dropdown_set_max_height(dropdown, 400);
    lv_dropdown_set_options_static(dropdown,
    "Sensor Overview\n"
    "Manual Control\n"
    "Warnings\n"
    "Settings"
    );

    lv_obj_set_style_bg_color(dropdown, lv_color_hex(0x42649f), 0);
    lv_obj_set_style_bg_opa(dropdown, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(dropdown, lv_color_white(), 0);
    lv_obj_set_style_border_width(dropdown, 0, 0);  // No border

    // Hide the selected option text 
    
    lv_dropdown_set_text(dropdown, "");

    lv_obj_set_style_text_font(dropdown, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
    
   
    // Click on menu button handler
    //lv_obj_add_event_cb(dropdown, dropdown_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(dropdown, dropdown_event_handler, LV_EVENT_ALL, NULL);

}

/** @brief Handle screen selection based on the dropdown menu.
 *  This function changes the current screen based on the selected label from the dropdown menu.
 *  It updates the selected index and loads the corresponding screen without animation.
 *  @param selected_label The label of the selected screen from the dropdown menu.
 */
void handle_screen_selection(const char *selected_label) {
    Serial.println("[Screen Handler] changing Screens..."); // Debug
    int new_index = -1;

    if      (!strcmp(selected_label, "Sensor Overview"))   new_index = 0;
    else if (!strcmp(selected_label, "Manual Control"))    new_index = 1;
    else if (!strcmp(selected_label, "Warnings"))          new_index = 2;
    else if (!strcmp(selected_label, "Settings"))          new_index = 3;
    else if (!strcmp(selected_label, "Home"))              new_index = 4;
    else new_index = 0;
    Serial.print("[Screen Handler] ");
    Serial.println(selected_label);
    Serial.print("[Screen Handler] new index: ");
    Serial.println(new_index);
    Serial.print("[Screen Handler] selected index: ");
    Serial.println(selected_index);
    if(new_index == selected_index) return; // no change

    selected_index = new_index;

    // if it’s changed, actually switch
    if (new_index >= 0) {
       
        switch (new_index) {
            case 0: current_screen = sensor_screen;         break;
            case 1: current_screen = manual_screen;         break;
            case 2: current_screen = warnings_screen;       break;
            case 3: current_screen = settings_screen;       break;
            case 4: current_screen = home_screen;           break;
            default: current_screen = sensor_screen;        break;
        }

        // Load the new screen without animation
        if(current_screen) {
            // Load new screen and delete old screen
            lv_screen_load_anim(current_screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false); 
            lv_dropdown_set_selected_highlight(dropdown, selected_index);
        }
    }
    Serial.println("[Screen Handler] Change Complete");
}

/** @brief Initialize the user interface
 * This function creates all the screens and sets up the global dropdown menu.
 */
void ui_init() {
    
    home_screen     = create_home_screen();
    sensor_screen   = create_sensor_screen();
    manual_screen   = create_manual_control_screen();
    warnings_screen = create_warnings_screen();
    create_footer(warnings_screen);
    settings_screen = create_settings_screen();
    create_footer(sensor_screen);
}

/** @brief Ensure the dropdown style is initialized.
 * This function checks if the dropdown style has been initialized.
 * If not, it initializes the style with custom properties.
 */
void ensure_dropdown_style() {
    if (!dropdown_style_initialized) {
        lv_style_init(&dropdown_list_style);
        lv_style_set_text_font(&dropdown_list_style, &lv_font_montserrat_48);
        lv_style_set_bg_color(&dropdown_list_style, lv_color_hex(0x42649F));
        lv_style_set_bg_grad_color(&dropdown_list_style, lv_color_hex(0xA3B7E4));
        lv_style_set_bg_grad_dir(&dropdown_list_style, LV_GRAD_DIR_HOR);
        dropdown_style_initialized = true;
    }
}

/** @brief Create the header bar at the top of the screen.
 *  This function creates a header bar with a title and a global dropdown menu.
 *  @param parent Pointer to the parent object where the header will be created.
 *  @param title_txt The title text to display in the header.
 *  This function sets the header's background color, text color, and font.
 */
void create_header(lv_obj_t *parent, const char *title_txt) {
    Serial.println("[gH] creating header"); // Debug
    // Header Bar
    lv_obj_t *header = lv_obj_create(parent);
    lv_obj_set_size(header, lv_pct(100), 80);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x42649f), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER,    LV_PART_MAIN);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(header, LV_DIR_NONE);

    // Title Label
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, title_txt);
    lv_obj_center(title);
    lv_obj_set_style_text_color(title, lv_color_hex(0xc0c9d9), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_48,  0);

    // Global navigation dropdown (icon-only)
    Serial.println("[gH] creating GDL"); // Debug
    
    create_global_dropdown(parent);
}

/** @brief Create a footer bar at the bottom of the screen.
 *  This function creates a footer bar with a label to display system status.
 *  @param parent Pointer to the parent object where the footer will be created.
 *  This function sets the footer's background color, text color, and font.
 */
void create_footer(lv_obj_t *parent) {
    lv_obj_t *scr = parent ? parent : lv_scr_act();
    global_footer = lv_obj_create(scr);
    lv_obj_set_size(global_footer, lv_pct(100), 60);
    lv_obj_align(global_footer, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(global_footer, lv_color_hex(0x1AC41F), 0);
    lv_obj_set_style_bg_opa(global_footer, LV_OPA_COVER, 0);
    lv_obj_clear_flag(global_footer, LV_OBJ_FLAG_SCROLLABLE);

    global_footer_label = lv_label_create(global_footer);
    lv_label_set_long_mode(global_footer_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(global_footer_label, lv_pct(100));
    lv_obj_align(global_footer_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(global_footer_label, "ALL SYSTEMS NOMINAL");
    lv_obj_set_style_text_color(global_footer_label, lv_color_hex(0x094211), 0);
    lv_obj_set_style_text_font(global_footer_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_align(global_footer_label, LV_TEXT_ALIGN_CENTER, 0);
}

/** @brief Update the footer status with the current warning mask.
 *  This function formats the warning mask into a string and updates the footer label.
 *  It also handles flashing the footer background color based on the warning state.
 *  @param warning_mask The current warning mask to display in the footer.
 *  This function checks if the warning mask has changed and updates the footer accordingly.
 */
void update_footer_status(uint32_t warning_mask) {
    if(!global_footer || !global_footer_label) return;
    // Only update the text when the mask actually changes:
    char buf[128];
    format_warnings(warning_mask, buf, sizeof(buf), global_footer_label);
    if (buf[0] == '\0') {
        // If the buffer is empty, just return
        return;
    }
    lv_label_set_text(global_footer_label, buf);
    prev_warning_mask = warning_mask;

    // If no warnings, show green and bail out
    if (warning_mask == WARN_NONE) {
        lv_obj_set_style_bg_color(global_footer, lv_color_hex(0x1AC41F), 0);
        lv_obj_set_style_text_align(global_footer_label, LV_TEXT_ALIGN_CENTER, 0);
        // reset flash state so it restarts clean next time
        footer_flash_state = false;
        last_footer_flash = millis();
        return;
    }

    // Otherwise, flash between dark and bright red every interval
    uint32_t t = millis();
    if (t - last_footer_flash >= FOOTER_FLASH_INTERVAL) {
        last_footer_flash = t;
        footer_flash_state = !footer_flash_state;
        lv_color_t c = footer_flash_state
                      ? lv_color_hex(0x8B0000)
                      : lv_color_hex(0xFF0000);
        lv_obj_set_style_bg_color(global_footer, c, 0);
        lv_obj_set_style_text_color(global_footer_label, lv_color_hex(0xFFFFFF), 0);
        
    }
    
}

//EOF