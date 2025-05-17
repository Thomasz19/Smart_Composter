/******************************************************************************
 * @file    screen_warnings.cpp
 * @author  Thomas Zoldowski
 * @date    May 10, 2025
 * @brief   Implementation of the Warnings and Alerts Screen UI.
 ******************************************************************************/

#include "screens/screen_warnings.h"
#include "ui_manager.h"

#include <Arduino.h>  // for millis()

static lv_obj_t *footer_bar = nullptr;
static lv_obj_t *footer_label = nullptr;
static bool footer_flash_state = false;
static unsigned long last_footer_flash = 0;
static FooterStatus last_status = FOOTER_OK;
static const unsigned long FOOTER_FLASH_INTERVAL = 500;


static lv_obj_t* warnings_screen = nullptr;

void create_warnings_screen() {
    warnings_screen = lv_obj_create(NULL);
    lv_scr_load(warnings_screen);

    

    // Background color 
    lv_obj_set_style_bg_color(warnings_screen, lv_color_hex(0xc0c9d9), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(warnings_screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(warnings_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(warnings_screen, LV_DIR_NONE);

    // Header Bar 
    lv_obj_t *header = lv_obj_create(warnings_screen);
    lv_obj_set_size(header, lv_pct(100), 80);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x42649f), 0); 
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(header, LV_DIR_NONE); // Prevent any scrolling

    // Title
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Warnings");
    lv_obj_center(title);
    lv_obj_set_style_text_color(title, lv_color_hex(0xc0c9d9), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_48, 0);

    // create drop down menu
    create_global_dropdown(warnings_screen);
}


void create_footer(lv_obj_t *parent) {
    footer_bar = lv_obj_create(parent);
    lv_obj_set_size(footer_bar, lv_pct(100), 60);
    lv_obj_align(footer_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(footer_bar, lv_color_hex(0x1ac41f), 0);
    lv_obj_set_style_bg_opa(footer_bar, LV_OPA_COVER, 0);
    lv_obj_clear_flag(footer_bar, LV_OBJ_FLAG_SCROLLABLE);

    footer_label = lv_label_create(footer_bar);
    lv_label_set_text(footer_label, "ALL SYSTEMS NOMINAL");
    lv_obj_center(footer_label);
    lv_obj_set_style_text_color(footer_label, lv_color_hex(0x094211), 0);
    lv_obj_set_style_text_font(footer_label, &lv_font_montserrat_48, 0);
}

void update_footer_status(FooterStatus status) {
    if (!footer_bar || !footer_label) return;

    if (status == FOOTER_OK) {
        lv_obj_set_style_bg_color(footer_bar, lv_color_hex(0x1ac41f), 0);
        lv_label_set_text(footer_label, "ALL SYSTEMS NOMINAL");
        last_status = FOOTER_OK;
    } else {
        if (last_status != FOOTER_WARNING) {
            lv_label_set_text(footer_label, "SYSTEM WARNING DETECTED");
            last_status = FOOTER_WARNING;
        }

        if (millis() - last_footer_flash > FOOTER_FLASH_INTERVAL) {
            last_footer_flash = millis();
            footer_flash_state = !footer_flash_state;
            lv_color_t flash_color = footer_flash_state
                                    ? lv_color_hex(0x8B0000)
                                    : lv_color_hex(0xFF0000);
            lv_obj_set_style_bg_color(footer_bar, flash_color, 0);
        }
    }
}