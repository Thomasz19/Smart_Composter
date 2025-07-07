/******************************************************************************
 * @file    actuator_manager.h
 * @author  Thomas Zoldowski
 * @date    May 19, 2025
 * @brief   Declarations for controlling compost actuators.
 ******************************************************************************/
#ifndef LOGIC_ACTUATOR_MANAGER_H
#define LOGIC_ACTUATOR_MANAGER_H

#include <cstdint>

void LED_Init();

void LED_On();

void LED_Off();

/** @brief  Updates the LED status based on security state.
*         Call this periodically to reflect the current security status.
*/
void LED_Update();

/** @brief  Initializes the actuator scheduler.
*         Call this once in setup() to configure pins
*         and prepare the scheduler for use.
*/
void initActuatorScheduler();

/** @brief  Call this every loop() to manage pump and blower states.
*         It will activate the pump and blowers based on the configured intervals.
*/
void scheduleHourlyActuators();

#endif // LOGIC_ACTUATOR_MANAGER_H
