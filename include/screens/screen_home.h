/******************************************************************************
 * @file    screen_home.h
 * @author  Thomas Zoldowski
 * @date    May 10, 2025
 * @brief   Header for the Home Screen UI.
 *
 * Layout:
 *  - Project title or logo at top center
 *  - Quick summary of system state (e.g. "System OK", time, etc.)
 *  - Buttons for navigation to other screens (Sensors, Warnings, etc.)
 ******************************************************************************/

#ifndef SCREEN_HOME_H
#define SCREEN_HOME_H

#include <lvgl.h>

lv_obj_t* create_home_screen(void);

extern char next_screen[32];  // Global variable to set the next screen

void update_home_screen(void);  // New function to handle flashing

#endif /* SCREEN_HOME_H */