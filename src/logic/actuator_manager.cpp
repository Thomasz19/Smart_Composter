/******************************************************************************
 * @file    actuator_manager.cpp
 * @author  Thomas Zoldowski
 * @date    May 19, 2025
 * @brief   Definitions for controlling compost actuators.
 ******************************************************************************/
#include "logic/actuator_manager.h"
#include <Arduino.h>
#include "screens/screen_settings.h"

// LED pins
const int LED_PINS[] = {28, 30, 32};



// LED Functions
void LED_Init() {
    for (int i = 0; i < 3; i++) {
        pinMode(LED_PINS[i], OUTPUT);
        digitalWrite(LED_PINS[i], LOW); // Turn off initially
        //pinMode(LED_PINS[i], INPUT_PULLDOWN);
    }
}

void LED_On() {
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PINS[i], HIGH);
    }
}

void LED_Off() {
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PINS[i], LOW);
    }
}

void actuator_manager_init() {

}

void LED_Update() {

    // check if security unlocked and activate buttons
    if (check_pin()) LED_On();
    else LED_Off();

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
