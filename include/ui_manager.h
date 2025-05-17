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

#endif /* UI_MANAGER_H */
