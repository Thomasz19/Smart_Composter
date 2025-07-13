/******************************************************************************
 * @file    screen_sensors.h
 * @author  Thomas Zoldowski
 * @date    May 10, 2025
 * @brief   Header for the Sensor Data Screen UI.
 *
 * Layout:
 *  - Temperature and humidity values from each sensor (3 total)
 *  - I2C mux selection (if needed) shown or abstracted
 *  - Optional: icons or color indicators for sensor state
 ******************************************************************************/


#ifndef SCREEN_SENSORS_H
#define SCREEN_SENSORS_H

#include <lvgl.h>

// Create and return the sensor screen object
lv_obj_t* create_sensor_screen(void);

// update the sensor screen with current data
void update_sensor_screen(void);

void SensorDataToSerial();

void CameraDelayToSerial();

#endif /* SCREEN_SENSORS_H */
