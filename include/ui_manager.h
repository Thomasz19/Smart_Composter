/******************************************************************************
 * @file    ui_manager.h
 * @author  Thomas Zoldowski
 * @date    May 10, 2025
 * @brief   UI manager header for Smart Composter screen navigation.
 *
 * This file declares the interface for initializing and switching between
 * LVGL screen components. It provides an abstraction for screen transitions,
 * enabling a modular and maintainable UI flow.
 *
 * Functions:
 *  - ui_init(): Initializes the display and default screen
 *  - ui_switch_to(ScreenID): Switches to a specified screen
 ******************************************************************************/

#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <lvgl.h>

void create_global_dropdown(lv_obj_t *parent);

void handle_screen_selection(const char *selected_label);

/**
 * @brief Create a standard header bar on the given parent screen.
 * @param parent     The screen or container to attach the header to.
 * @param title_txt  The title string to display centered in the header.
 */
void create_header(lv_obj_t *parent, const char *title_txt);

void create_footer(lv_obj_t *parent);
void update_footer_status(uint32_t warning_mask);

void ensure_dropdown_style();

extern int       selected_index;

void ui_init();

#endif /* UI_MANAGER_H */
