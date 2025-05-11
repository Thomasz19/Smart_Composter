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

void create_sensor_screen(void);

#endif /* SCREEN_SENSORS_H */
