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




lv_obj_t* create_sensor_screen(void);

void update_sensor_screen(void);

bool is_sensor_screen_active(void);

void sonar_update_and_fill_bar();

bool read_sonar_packet(float &outInches);

void SensorDataToSerial();

void CameraDelayToSerial();

#endif /* SCREEN_SENSORS_H */
