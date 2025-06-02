/******************************************************************************
 * @file    sensor_manager.cpp
 * @author  Thomas Zoldowski
 * @date    May 19, 2025
 * @brief   Definitions for initializing and reading compost sensors.
 ******************************************************************************/
#include "logic/sensor_manager.h"
#include "TCA9548.h"


// Structure to hold sensor readings
struct SensorData {
    float temperature;
    float humidity;
};

// I2C multiplexer on address 0x70
static TCA9548 tca(0x70);
// Array of AHT20 sensor objects
static Adafruit_AHTX0 aht_sensors[3];
// Array to store latest readings
static SensorData sensor_data[3];
// Corresponding TCA9548 channels for each sensor
static const uint8_t sensor_channels[3] = {0, 1, 2};
// AHT20 I2C address
static const uint8_t AHT20_ADDRESS = 0x38;

void sensor_manager_init() {
    Wire.begin();
    if (tca.begin() == false)
    {
        Serial.println("COULD NOT CONNECT TO MULTIPLEXER");
    }
    else{
        Serial.println("MULTIPLEXER DETECTED"); 
    }
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
    // Deselect all channels to avoid bus conflicts
    tca.disableAllChannels();
}

void sensor_manager_update() {
    for (uint8_t i = 0; i < 3; i++) {
        tca.selectChannel(sensor_channels[i]);
        sensors_event_t humidity_event, temp_event;
        if (aht_sensors[i].getEvent(&humidity_event, &temp_event)) {
            sensor_data[i].humidity    = humidity_event.relative_humidity;
            sensor_data[i].temperature = temp_event.temperature;
        } else {
            sensor_data[i].humidity    = NAN;
            sensor_data[i].temperature = NAN;
        }
    }
    tca.disableAllChannels();
}

float sensor_manager_get_temperature(uint8_t idx) {
    return (idx < 3) ? sensor_data[idx].temperature : NAN;
}

float sensor_manager_get_humidity(uint8_t idx) {
    return (idx < 3) ? sensor_data[idx].humidity : NAN;
}

// Checks whether the mux and each sensor are present on the bus
ConnectionStatus sensor_manager_get_connection_status() {
    ConnectionStatus status = {false, {false, false, false}};

    // Test multiplexer at 0x70
    Wire.beginTransmission(0x70);
    status.mux = (Wire.endTransmission() == 0);

    // Test each sensor behind the mux
    for (uint8_t i = 0; i < 3; i++) {
        tca.selectChannel(sensor_channels[i]);
        Wire.beginTransmission(AHT20_ADDRESS);
        status.sensor[i] = (Wire.endTransmission() == 0);
    }
    tca.disableAllChannels();

    return status;
}
