/******************************************************************************
 * @file    screen_manual.h
 * @author  Thomas Zoldowski
 * @date    May 10, 2025
 * @brief   Header for the Motor Control and Status Screen UI.
 *
 * Layout:
 *  - Toggle switches for Blower 1, Blower 2, and Pump
 *  - Status indicators (e.g. ON/OFF, active runtime)
 *  - Optional: RPM readout if using Hall sensor for feedback
 ******************************************************************************/

#ifndef SCREEN_MANUAL_H
#define SCREEN_MANUAL_H

#include <lvgl.h>

// Create and return the manual control screen object
lv_obj_t* create_manual_control_screen(void);

// Check if manual control screen is currently active
void updateManualScreenLEDs(bool pumpActive, int blowState);


#endif /* SCREEN_MOTORS_H */
