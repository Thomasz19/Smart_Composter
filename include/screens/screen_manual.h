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

void create_manual_control_screen(void);

#endif /* SCREEN_MOTORS_H */
