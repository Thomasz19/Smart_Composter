/******************************************************************************
 * @file    actuator_manager.cpp
 * @author  Thomas Zoldowski
 * @date    May 19, 2025
 * @brief   Definitions for controlling compost actuators.
 ******************************************************************************/
#include "logic/actuator_manager.h"

void actuator_manager_init() {
    // TODO: initialize PWM, GPIOs, etc.
}

void actuator_manager_set_blower_speed(uint8_t pct) {
    // TODO: set blower PWM duty to pct%
}

void actuator_manager_open_valve() {
    // TODO: drive valve open
}

void actuator_manager_close_valve() {
    // TODO: drive valve closed
}

void actuator_manager_set_indicator(bool on) {
    // TODO: turn indicator LED on/off
}
