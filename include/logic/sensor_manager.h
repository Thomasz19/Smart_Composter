/******************************************************************************
 * @file    sensor_manager.h
 * @author  Thomas Zoldowski
 * @date    May 19, 2025
 * @brief   Declarations for initializing and reading compost sensors.
 ******************************************************************************/
#ifndef LOGIC_SENSOR_MANAGER_H
#define LOGIC_SENSOR_MANAGER_H

#include <cstdint>

/** Initialize all compost sensors (I²C, ADC channels, etc.). */
void sensor_manager_init();

/** Poll/update all sensor readings. Call this periodically. */
void sensor_manager_update();

/** Get the latest temperature (°C) for sensor `idx` (0-based). */
float sensor_manager_get_temperature(uint8_t idx);

/** Get the latest humidity (%) for sensor `idx` (0-based). */
float sensor_manager_get_humidity(uint8_t idx);

#endif // LOGIC_SENSOR_MANAGER_H
