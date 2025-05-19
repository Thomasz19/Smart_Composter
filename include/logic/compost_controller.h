/******************************************************************************
 * @file    compost_controller.h
 * @author  Thomas Zoldowski
 * @date    May 19, 2025
 * @brief   Declarations for the compost control algorithm.
 ******************************************************************************/
#ifndef LOGIC_COMPOST_CONTROLLER_H
#define LOGIC_COMPOST_CONTROLLER_H

/** Initialize the compost control algorithm (state, timers, thresholds). */
void compost_controller_init();

/** Run one iteration of the compost control loop. Call frequently. */
void compost_controller_task();

/**
 * @brief Override default temperature thresholds.
 * @param low   Lower limit in °C.
 * @param high  Upper limit in °C.
 */
void compost_controller_set_temp_thresholds(float low, float high);

/**
 * @brief Override default humidity threshold.
 * @param low  Lower limit in %.
 */
void compost_controller_set_humidity_threshold(float low);

#endif // LOGIC_COMPOST_CONTROLLER_H
