/******************************************************************************
 * @file    screen_history.cpp
 * @author  Thomas Zoldowski
 * @date    May 10, 2025
 * @brief   Implementation of the Historical Data Screen UI.
 ******************************************************************************/
#include <Arduino.h>
#include "screens/screen_history.h"
#include "ui_manager.h"
#include "screens/screen_warnings.h"

// Screen handle
static lv_obj_t* history_screen = nullptr;

/** @brief Create the History screen.
 *  @return Pointer to the created history screen object.
 */
lv_obj_t* create_history_screen(void) {
    Serial.println("[HS] Loading History Screen..."); // Debug
    history_screen = lv_obj_create(NULL);
    
    // create header + footer
    create_header(history_screen, "History");
    create_footer(history_screen);
    

    // Background color 
    lv_obj_set_style_bg_color(history_screen, lv_color_hex(0xc0c9d9), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(history_screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(history_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(history_screen, LV_DIR_NONE);
    
    lv_obj_t *lbl = lv_label_create(history_screen);
    lv_label_set_text(lbl, "$100 and i'll finish this screen");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_40, 0);
    lv_obj_center(lbl);

    Serial.println("[HS] Loading History Screen COMPLETE"); // Debug
    return history_screen;
}