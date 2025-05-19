/******************************************************************************
 * @file    data_logger.cpp
 * @author  Thomas Zoldowski
 * @date    May 19, 2025
 * @brief   Definitions for logging compost sensor data and warnings.
 ******************************************************************************/
#include "logic/data_logger.h"

void data_logger_init() {
    // TODO: mount SD card, open log file, etc.
}

void data_logger_update() {
    // TODO: flush or rotate logs as needed
}

void data_logger_log_reading(uint8_t idx, float temperature, float humidity) {
    // TODO: write a sensor reading to storage
}

void data_logger_log_warning(const char * msg) {
    // TODO: write a warning message to storage
}
