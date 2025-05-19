/******************************************************************************
 * @file    actuator_manager.h
 * @author  Thomas Zoldowski
 * @date    May 19, 2025
 * @brief   Declarations for controlling compost actuators.
 ******************************************************************************/
#ifndef LOGIC_ACTUATOR_MANAGER_H
#define LOGIC_ACTUATOR_MANAGER_H

#include <cstdint>

/** Initialize all actuators (PWM for blowers, GPIO for valves, LEDs, etc.). */
void actuator_manager_init();

/** Set blower speed as a percentage (0–100%). */
void actuator_manager_set_blower_speed(uint8_t pct);

/** Open the compost aeration valve. */
void actuator_manager_open_valve();

/** Close the compost aeration valve. */
void actuator_manager_close_valve();

/** Turn the indicator LED on or off. */
void actuator_manager_set_indicator(bool on);

#endif // LOGIC_ACTUATOR_MANAGER_H
