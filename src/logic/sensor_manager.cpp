/******************************************************************************
 * @file    sensor_manager.cpp
 * @author  Thomas Zoldowski
 * @date    May 19, 2025
 * @brief   Definitions for initializing and reading compost sensors.
 ******************************************************************************/

#include <DFRobot_OxygenSensor.h>
#include "logic/sensor_manager.h"
#include "TCA9548.h"
#include <VL53L1X.h>

#define Oxygen_IICAddress ADDRESS_3
#define TOF_ADDRESS       0x29  // Default VL53L1X I2C address

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

void sensor_manager_init() {
    o2Channel = -1;
    Wire.begin();

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
        tca.selectChannel(sensor_channels[4 + j]);
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
    
}



void sensor_manager_update() {
    // AHT Sensors
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

    // VL53L1X readings
    for (uint8_t j = 0; j < 2; j++) {
        tca.selectChannel(sensor_channels[3 + j]);
        uint16_t dist = tof_sensors[j].readRangeContinuousMillimeters();
        tof_distance[j] = tof_sensors[j].timeoutOccurred() ? NAN : dist * 0.1f; // cm
    }

    // O2 Sensor
    if (o2Channel >= 0) {
        tca.selectChannel(o2Channel);
        oxygen_level = o2Sensor.getOxygenData(20);
    }

    // Deselect all channels to avoid bus conflicts
    tca.disableAllChannels();
}


float sensor_manager_get_temperature(uint8_t idx) {
    return (idx < 6) ? sensor_data[idx].temperature : NAN;
}

float sensor_manager_get_humidity(uint8_t idx) {
    return (idx < 6) ? sensor_data[idx].humidity : NAN;
}

float sensor_manager_get_oxygen(void) {
    return oxygen_level;
}

float sensor_manager_get_tof_distance(uint8_t idx) {
    return (idx < 2) ? tof_distance[idx] : NAN;
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

    // VL53L1X sensors
    for (uint8_t j = 0; j < 2; j++) {
        tca.selectChannel(sensor_channels[3 + j]);
        Wire.beginTransmission(TOF_ADDRESS);
        status.vl53[j] = (Wire.endTransmission() == 0);
    }

    // Check O₂ sensor
    if (o2Channel >= 0) {
            tca.selectChannel(o2Channel);
            Wire.beginTransmission(Oxygen_IICAddress);    // define O2_I2C_ADDRESS (e.g. 0x??)
            status.o2 = (Wire.endTransmission() == 0);
    }
    // Deselect all channels to avoid bus conflicts
    tca.disableAllChannels();

    return status;
}


void Limit_Switch_Init() {
    for (uint8_t i = 0; i < 5; ++i) {
        pinMode(LIMIT_SWITCH_PINS[i], INPUT);  // Externally pulled high
    }
}

void Limit_Switch_update() {
    for (uint8_t i = 0; i < 5; ++i) {
        int pin_state = digitalRead(LIMIT_SWITCH_PINS[i]);
        // LOW means switch is pressed (door closed)
        limit_switch_states[i] = (pin_state == LOW);

        if (limit_switch_states[i]) {
            Serial.print("Limit Switch ");
            Serial.print(i);
            Serial.println(" is CLOSED (door shut)");
        } else {
            Serial.print("Limit Switch ");
            Serial.print(i);
            Serial.println(" is OPEN (door open)");
        }
    }
}

// Accessor function to get switch state
bool Limit_Switch_isClosed(uint8_t index) {
    if (index < 5) return limit_switch_states[index];
    return false;
}