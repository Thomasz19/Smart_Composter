/******************************************************************************
 * @file screen_diagnostics.h
 * @brief Header file for the diagnostics screen functionality.
 * 
 * This file contains declarations for functions related to creating,
 * checking, and updating the diagnostics screen in the application.
 ******************************************************************************/

#ifndef SCREEN_DIAGNOSTICS_H
#define SCREEN_DIAGNOSTICS_H

#include <lvgl.h>

// Create and return the diagnostics screen object
lv_obj_t* create_diagnostics_screen(void);

// Check if diagnostics screen is currently active
bool is_diagnostics_screen_active(void);

// Refresh connection status on the diagnostics screen
void update_diagnostics_screen(void);

#endif // SCREEN_DIAGNOSTICS_H