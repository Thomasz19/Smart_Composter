/******************************************************************************
 * @file    sensor_manager.cpp
 * @author  Thomas Zoldowski
 * @date    May 19, 2025
 * @brief   Definitions for initializing and reading compost sensors.
 ******************************************************************************/

#include <DFRobot_OxygenSensor.h>
#include "logic/sensor_manager.h"
#include "screens/screen_sensors.h"
#include "screens/screen_warnings.h"
#include "ui_manager.h"
#include "TCA9548.h"
#include <VL53L1X.h>
#include "mbed.h"

#define Oxygen_IICAddress ADDRESS_3
#define TOF_ADDRESS       0x29  // Default VL53L1X I2C address

const uint8_t TMP117_ADDR       = 0x48;   // 0x48–0x4B depending on ADR pin
const uint8_t TMP117_TEMP_REG   = 0x00;   // Temperature Result register
float         boardTempF        = NAN;    // make this global so UI can read it

// Limit Switches
constexpr uint8_t LIMIT_SWITCH_PINS[5] = { D0, D1, D2, D3, D4 };
bool limit_switch_states[5] = {false, false, false, false, false};

// Gravity O₂ sensor
static DFRobot_OxygenSensor o2Sensor;
static float oxygen_level = NAN;
extern int8_t o2Channel;

// Structure to hold sensor readings
struct SensorData {
    float temperature;
    float humidity;
};


// VL53L1X Time-of-Flight sensors (ports 4 & 5 on the mux)
static VL53L1X tof_sensors[2];
static float tof_distance[2] = { NAN, NAN };

// I2C multiplexer on address 0x70
static TCA9548 tca(0x70);
// Array of AHT20 sensor objects
static Adafruit_AHTX0 aht_sensors[3];
// Array to store latest readings
static SensorData sensor_data[3];
// Corresponding TCA9548 channels for each sensor
static const uint8_t sensor_channels[8] = {0, 1, 2, 3, 4, 5, 6, 7};
// AHT20 I2C address
static const uint8_t AHT20_ADDRESS = 0x38;

// Mutex to protect shared data
rtos::Mutex           data_mutex;
ConnectionStatus      latest_status;
float                 latest_temps[3];
float                 latest_hums[3];
float                 latest_o2;
float                 latest_depth_cm;

/** @brief Initialize the sensor manager.
 * This function initializes the I2C bus, the TCA9548 multiplexer, and all sensors.
 * It also sets up the O₂ sensor and VL53L1X sensors.
 */
void sensor_manager_init() {
    o2Channel = -1;
    Wire.begin();           // Initialize I2C bus

    if (tca.begin() == false)
    {
        Serial.println("COULD NOT CONNECT TO MULTIPLEXER");
    }
    else{
        Serial.println("MULTIPLEXER DETECTED"); 
    }

    // Initialize AHT20 sensors
    Serial.println("Initializing AHT20 sensors...");
    for (uint8_t i = 0; i < 3; i++) {
        tca.selectChannel(sensor_channels[i]);
        if (!aht_sensors[i].begin()) {
            Serial.print("AHT20 #");
            Serial.print(i);
            Serial.println(" not found!");
        } else {
            Serial.print("AHT20 #");
            Serial.print(i);
            Serial.println(" initialized.");
        }
    }
    
    // Initialize VL53L1X TOF sensors
    Serial.println("Initializing VL53L1X sensors...");
    for (uint8_t j = 0; j < 2; j++) {
        tca.selectChannel(sensor_channels[3 + j]);
        tof_sensors[j].setBus(&Wire);
        if (tof_sensors[j].init()) {
            tof_sensors[j].setAddress(TOF_ADDRESS);
            Serial.print("VL53L1X #"); Serial.print(j); Serial.println(" initialized.");
            tof_sensors[j].setTimeout(500);
            tof_sensors[j].startContinuous(50);
        } else {
            Serial.print("VL53L1X #"); 
            Serial.print(j); Serial.println(" not found!");
        }
    }

    // Initialize O₂ sensor
    Serial.println("Initializing SEN0322 sensor...");
    tca.selectChannel(sensor_channels[5]);
    if (o2Sensor.begin(Oxygen_IICAddress) == 0) {
        Serial.print("O₂ sensor initialized on channel ");
        Serial.println(sensor_channels[5]);
        o2Channel = sensor_channels[5];
    } else {
        Serial.println("O₂ sensor not found on channel 5!");
    }

    // Deselect all channels to avoid bus conflicts
    tca.disableAllChannels();

    Wire.beginTransmission(TMP117_ADDR);
    Wire.write(0x01);            // Configuration register
    Wire.write(0x06); Wire.write(0x00);   // 0x0600 = continuous, 15‑Hz, avg = 1
    Wire.endTransmission();
    
}


/** @brief Update sensor readings.
 * This function reads data from all sensors and updates the sensor_data array.
 * It also checks the connection status of each sensor.
 */
void sensor_manager_update() {
    // Only read sensors that acknowledged on the bus
    ConnectionStatus status = sensor_manager_get_connection_status();

    // AHT20 Sensors (ports 0-2)
    for (uint8_t i = 0; i < 3; i++) {
        tca.selectChannel(sensor_channels[i]);
        if (status.sensor[i]) {
            sensors_event_t hum_evt, tmp_evt;
            if (aht_sensors[i].getEvent(&hum_evt, &tmp_evt)) {
                sensor_data[i].humidity    = hum_evt.relative_humidity;
                sensor_data[i].temperature = tmp_evt.temperature;
            } else {
                sensor_data[i].humidity    = NAN;
                sensor_data[i].temperature = NAN;
            }
        } else {
            sensor_data[i].humidity    = NAN;
            sensor_data[i].temperature = NAN;
        }
    }
    //Serial.println("AHT20 sensors updated.");

    // VL53L1X Sensors (ports 3-4)
    for (uint8_t j = 0; j < 2; j++) {
        tca.selectChannel(sensor_channels[3 + j]);
        if (status.vl53[j]) {
            uint16_t mm = tof_sensors[j].readRangeContinuousMillimeters();
            tof_distance[j] = tof_sensors[j].timeoutOccurred() ? NAN : mm * 0.1f; // cm
            // Serial.print("VL53L1X #");
            // Serial.print(j);
            // Serial.print(": ");
        } else {
            tof_distance[j] = NAN;
        }
    }
    //Serial.println("VL53L1X sensors updated.");

    // O₂ Sensor (port 5)
    if (status.o2) {
        tca.selectChannel(sensor_channels[5]);
        oxygen_level = o2Sensor.getOxygenData(20);
    } else {
        oxygen_level = NAN;
    }
   // Serial.println("O₂ sensor updated.");

    // Deselect all channels to avoid conflicts
    tca.disableAllChannels();

    Wire.beginTransmission(TMP117_ADDR);
    Wire.write(TMP117_TEMP_REG);

    // Send a repeated‑START (false) so we can immediately read
    if (Wire.endTransmission(false) == 0 &&
        Wire.requestFrom(TMP117_ADDR, (uint8_t)2) == 2)            // 2 bytes, MSB first
    {
        int16_t raw = (Wire.read() << 8) | Wire.read();            // combine MSB/LSB
        boardTempF  = raw * 0.0078125f * 9.0f / 5.0f + 32.0f;       // °F conversion
    }
    else {
        boardTempF = NAN;
}

}

/** @brief Get the latest external temperature in Fahrenheit.
 * @return The external temperature in Fahrenheit, or NAN if not available.
 * This function retrieves the board temperature from the TMP117 sensor.
 */
float getExternalTemperature() {
    return boardTempF; // Return the board temperature in Fahrenheit
}

/** @brief Get the latest temperature reading for a specific sensor.
 * @param idx Index of the sensor (0-2).
 * @return Temperature in Celsius, or NAN if the sensor is not available.
 * This function retrieves the temperature reading from the sensor_data array.
 */
float sensor_manager_get_temperature(uint8_t idx) {
    return (idx < 6) ? sensor_data[idx].temperature : NAN;
}

/** @brief Get the latest humidity reading for a specific sensor.
 * @param idx Index of the sensor (0-2).
 * @return Humidity in percentage, or NAN if the sensor is not available.
 * This function retrieves the humidity reading from the sensor_data array.
 */
float sensor_manager_get_humidity(uint8_t idx) {
    return (idx < 6) ? sensor_data[idx].humidity : NAN;
}

/** @brief Get the latest O₂ concentration reading.
 * @return O₂ concentration in percentage, or NAN if the sensor is not available.
 * This function retrieves the oxygen concentration from the o2Sensor object.
 */
float sensor_manager_get_oxygen(void) {
    return oxygen_level;
}

/** @brief Get the latest distance reading from a VL53L1X sensor.
 * @param idx Index of the VL53L1X sensor (0-1).
 * @return Distance in centimeters, or NAN if the sensor is not available.
 * This function retrieves the distance reading from the tof_distance array.
 */
float sensor_manager_get_tof_distance(uint8_t idx) {
    return (idx < 2) ? tof_distance[idx] : NAN;
}

/** @brief Get the connection status of all sensors.
 * @return ConnectionStatus structure containing the status of each sensor.
 * This function checks the connection status of the TCA9548 multiplexer and all sensors.
 */
ConnectionStatus sensor_manager_get_connection_status() {
    
    ConnectionStatus status = {
        .mux    = false,
        .sensor = { false, false, false },
        .o2     = false,
        .vl53   = { false, false },
    };
    
    // Test multiplexer at 0x70
    Wire.beginTransmission(0x70);
    status.mux = (Wire.endTransmission() == 0);

    // Test each sensor behind the mux
    for (uint8_t i = 0; i < 3; i++) {
        tca.selectChannel(sensor_channels[i]);
        Wire.beginTransmission(AHT20_ADDRESS);
        status.sensor[i] = (Wire.endTransmission() == 0);
    }
    
    // VL53L1X sensors
    for (uint8_t j = 0; j < 2; j++) {
        tca.selectChannel(sensor_channels[3 + j]);
        Wire.beginTransmission(TOF_ADDRESS);
        status.vl53[j] = (Wire.endTransmission() == 0);
    }
    
    // Check O₂ sensor
    tca.selectChannel(sensor_channels[5]);
    Wire.beginTransmission(Oxygen_IICAddress);    // define O2_I2C_ADDRESS (e.g. 0x??)
    status.o2 = (Wire.endTransmission() == 0);
        
    // Deselect all channels to avoid bus conflicts
    tca.disableAllChannels();

    return status;
}

/** @brief Get the latest connection status.
 * @return ConnectionStatus structure containing the latest status of each sensor.
 * This function retrieves the latest connection status of the sensors
 * and updates the latest_status variable.
 */
void Limit_Switch_Init() {
    for (uint8_t i = 0; i < 5; ++i) {
        pinMode(LIMIT_SWITCH_PINS[i], INPUT);  // Externally pulled high
    }
}

/** @brief Update the limit switch states and check for door openings.
 * This function reads the state of each limit switch and updates the limit_switch_states array.
 * It also checks if any door has been opened and adds a warning if necessary.
 */
void Limit_Switch_update() {
    uint32_t mask = WARN_NONE;
    static bool prev_closed[5] = { false, false, false, false, false };

    for (uint8_t i = 0; i < 5; ++i) {
        bool closed = (digitalRead(LIMIT_SWITCH_PINS[i]) == HIGH);
        limit_switch_states[i] = closed;

        // edge: only fire when we go from open → closed
        if (closed) {
            // home in on which door
            if (i == 0 || i == 1) {
                mask |= WARN_FRONT_DOOR;
                }
            else if (i == 2 || i == 3) {
                mask |= WARN_BACK_DOOR;
                }
            else { // i == 4
                mask |= WARN_LOADING_DOOR;
            }
        }
        if (closed && !prev_closed[i]) {
            // home in on which door
            if (i == 0 || i == 1) {
                Serial.println("Unloaded");
                add_warning("Unloaded front door Opened");  // Add warning for front door
            }
            else if (i == 2 || i == 3) {
                Serial.println("Unloaded");
                add_warning("Unloaded back door Opened");  // Add warning for back door
            }
            else { // i == 4
                Serial.println("Loaded");
                add_warning("Loaded loading door Opened");  // Add warning for loading door
            }
        }







        // remember for next time
        prev_closed[i] = closed;
    }

    update_footer_status(mask);
}


/** @brief Check if a specific limit switch is closed.
 * @param index Index of the limit switch (0-4).
 * @return True if the limit switch is closed, false otherwise.
 * This function checks the state of a specific limit switch and returns its status.
 */
bool Limit_Switch_isClosed(uint8_t index) {
    if (index < 5) return limit_switch_states[index];
    return false;
}
