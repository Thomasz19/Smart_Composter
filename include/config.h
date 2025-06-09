/******************************************************************************
 * @file    config.h
 * @author  Thomas Zoldowski
 * @date    May 10, 2025
 * @brief   Pin and configuration definitions for Smart Composter system.
 *
 * This header file contains all hardware-related configuration macros for 
 * the Arduino GIGA R1 WiFi platform, including pin assignments for sensors,
 * motors, display parameters, and communication buses.
 *
 * Centralizing these definitions allows for easier maintenance, readability,
 * and hardware abstraction across the firmware.
 ******************************************************************************/

/*
here i define the color pallet of the LCD stuff

*/




#ifndef CONFIG_H_
#define CONFIG_H_


// ======= SIMULATION MODE =======
#define SIMULATION_MODE    0   // Set to 1 to enable fake sensor data; 0 = real sensors

// Sensor thresholds (adjust as needed)
#define TEMP_GOOD_MIN     15.0
#define TEMP_GOOD_MAX     30.0
#define TEMP_WARN_MAX     35.0

// ========== GENERAL ==========
#define SERIAL_BAUDRATE     115200

// Sensor polling interval (milliseconds)
#define SENSOR_UPDATE_INTERVAL_MS 1000

// Inactivity timeout (milliseconds)
#define INACTIVITY_TIMEOUT_MS 2400000  // 20 sec

// ========== I2C MUX AND SENSOR ARRAY ==========
#define I2C_MUX_ADDR        0x70  // TCA9548A default I2C address
#define NUM_SENSOR_NODES    3     // Number of temperature/humidity sensor groups

// I2C pins (typically hardware I2C for Arduino GIGA)
//#define I2C_SDA             D20
//#define I2C_SCL             D21


// ========== ULTRASONIC SENSOR ==========
#define PIN_ULTRASONIC_TRIG    D9
#define PIN_ULTRASONIC_ECHO    D10


// ========== 120VAC DEVICE CONTROL (via SSR) ==========
#define SSR_PIN_BLOWER_1       D2
#define SSR_PIN_BLOWER_2       D3
#define SSR_PIN_PUMP           D4


// ========== CONTROL BUTTONS ==========
#define BTN_PIN_BLOWER_1       D5
#define BTN_PIN_BLOWER_2       D6
#define BTN_PIN_PUMP           D7


// ========== ROTARY ENCODER ==========
#define ROTARY_PIN_CLK         D8   // Clock (A)
#define ROTARY_PIN_DT          D11  // Data (B)
#define ROTARY_PIN_SW          D12  // Switch (press)

// Optionally, define debounce delay for buttons and encoder
#define DEBOUNCE_MS            50


// ========== DISPLAY ==========
#define DISPLAY_WIDTH          800
#define DISPLAY_HEIGHT         480


// ========== TIMING CONFIG ==========
#define SENSOR_UPDATE_INTERVAL_MS   1000
#define SCREEN_REFRESH_INTERVAL_MS  500

#endif /* CONFIG_H_ */
