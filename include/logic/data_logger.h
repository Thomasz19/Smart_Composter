/******************************************************************************
 * @file    data_logger.h
 * @author  Thomas Zoldowski
 * @date    May 19, 2025
 * @brief   Declarations for logging compost sensor data and warnings.
 ******************************************************************************/
#ifndef LOGIC_DATA_LOGGER_H
#define LOGIC_DATA_LOGGER_H

#include <cstdint>

/** Initialize the data logger (mount SD, open file, etc.). */
void data_logger_init();

/** Flush or rotate logs as needed. Call periodically. */
void data_logger_update();

/**
 * @brief Record a single sensor reading.
 * @param idx          Sensor index.
 * @param temperature  Temperature in °F.
 * @param humidity     Relative humidity in %.
 */
void data_logger_log_reading(uint8_t idx, float temperature, float humidity);

/**
 * @brief Record a warning or fault message.
 * @param msg  Null-terminated string message.
 */
void data_logger_log_warning(const char * msg);

#endif // LOGIC_DATA_LOGGER_H
