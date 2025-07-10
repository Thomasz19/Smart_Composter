/******************************************************************************
 * @file    actuator_manager.cpp
 * @author  Thomas Zoldowski
 * @date    May 19, 2025
 * @brief   Definitions for controlling compost actuators.
 ******************************************************************************/
#include "logic/actuator_manager.h"
#include <Arduino.h>
#include "screens/screen_settings.h"
#include "settings_storage.h"
#include "screens/screen_manual.h"
#include "logic/sensor_manager.h"

// LED pins
const int LED_PINS[] = {28, 30, 32};

// ─────────────────────────────────────────────────────────────────────────────
// Pin defs
static constexpr uint8_t PUMP_PIN     = D27;
static constexpr uint8_t BLOWER1_PIN  = D25;
static constexpr uint8_t BLOWER2_PIN  = D23;

// ─────────────────────────────────────────────────────────────────────────────
// State for pump
static unsigned long  lastPumpMillis  = 0;
static bool           pumpActive      = false;
static unsigned long  pumpEndMillis   = 0;

// State for blower sequence
enum BlowState { BLOW_IDLE, BLOW_RUN1, BLOW_RUN2 };
static BlowState     blowState        = BLOW_IDLE;
static unsigned long lastBlowerMillis = 0;
static unsigned long blowStartMillis  = 0;

// 30 minutes re-arm for pump, in seconds:
static constexpr uint32_t PUMP_REARM_INTERVAL = 30 * 60;  
static constexpr uint32_t BLOWER_REARM_INTERVAL = 30 * 60;
// Edge-detect flag for blower-by-temperature:
static bool blower_temp_triggered = false;

// LED Functions
void LED_Init() {
    for (int i = 0; i < 3; i++) {
        pinMode(LED_PINS[i], OUTPUT);
        digitalWrite(LED_PINS[i], LOW); // Turn off initially
        //pinMode(LED_PINS[i], INPUT_PULLDOWN);
    }
}

void LED_On() {
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PINS[i], HIGH);
    }
}

void LED_Off() {
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PINS[i], LOW);
    }
}

/// Call once from setup()
void initActuatorScheduler() {
  pinMode(PUMP_PIN,    OUTPUT);
  pinMode(BLOWER1_PIN, OUTPUT);
  pinMode(BLOWER2_PIN, OUTPUT);

  digitalWrite(PUMP_PIN,    LOW);
  digitalWrite(BLOWER1_PIN, LOW);
  digitalWrite(BLOWER2_PIN, LOW);

  lastPumpMillis   = millis();
  lastBlowerMillis = millis();
}

void LED_Update() {

    // check if security unlocked and activate buttons
    if (check_pin()) LED_On();
    else LED_Off();

}


/**
 * @brief Schedule hourly actuators (pump and blower).
 * This function checks the current time against the last activation times
 * and activates the pump and blower as per the configured intervals.
 */
void scheduleHourlyActuators() {
    uint32_t nowSec = (uint32_t)time(nullptr);
    uint32_t Interval = getActivationInterval();

   // Serial.println(nowSec-config.lastPumpEpoch);

    bool dry = false;
    for(int i = 0; i < 3; i++) {
        
        if(sensor_manager_get_humidity(i) < getHumLowThreshold(i)) {
            dry = true;
            break;
        }
    }

    // ── Pump (only if not mid‐blower) ─────────────────────
    if (!pumpActive 
        && blowState == BLOW_IDLE
        && dry
        && (nowSec - config.lastPumpEpoch) >= PUMP_REARM_INTERVAL) 
    {
        pumpActive          = true;
        pumpEndMillis       = millis() + (unsigned long)getPumpOnTime() * 1000UL;
        config.lastPumpEpoch = nowSec;        // persist the trigger time
        digitalWrite(PUMP_PIN, HIGH);
        Serial.println("[Actuator1] Starting pump...");
        saveConfig();
    }

    if (pumpActive && millis() >= pumpEndMillis) {
        pumpActive = false;
        digitalWrite(PUMP_PIN, LOW);
        Serial.println("[Actuator1] Pump done, turning off.");
    }

    // ── Blower sequence ───────────────────────────────────
    bool overTemp = false;
    //Serial.print("[Actuator2] Checking temperature thresholds: ");
    // for(int i = 0; i < 3; i++) {
    //     Serial.println("Sensor ");
    //     Serial.print(i);
    //     Serial.print(": ");
    //     Serial.print(sensor_manager_get_temperature(i)* 9.0f / 5.0f + 32.0f);
    //     Serial.println("°F");
    //     Serial.print("Threshold: ");
    //     Serial.println(getTempHighThreshold(i));
    // }
    for(int i = 0; i < 3; i++) {
        float tempC = sensor_manager_get_temperature(i);
        float tempF = tempC * 9.0f / 5.0f + 32.0f;
        if(tempF >= getTempHighThreshold(i)) {
            overTemp = true;
            break;
        }
    }
    // If back below threshold, clear the one-shot flag
    if(!overTemp) blower_temp_triggered = false;

    // Serial.println(overTemp);
    // Serial.println(nowSec - config.lastBlowerEpoch);
    // Serial.println(Interval);

    if (blowState == BLOW_IDLE
        && !pumpActive
        && (nowSec - config.lastBlowerEpoch) >= Interval) 
    {
        Serial.println("[Actuator1] Starting blower 1...");
        // start blower1
        blowState          = BLOW_RUN1;
        blowStartMillis    = millis();
        config.lastBlowerEpoch = nowSec;      // persist the trigger time
        digitalWrite(BLOWER1_PIN, HIGH);
        saveConfig();
    } else if ((overTemp && !pumpActive) && (nowSec - config.lastBlowerEpoch) >= Interval/3) {
        blowState             = BLOW_RUN1;
        blowStartMillis       = millis();
        config.lastBlowerEpoch = nowSec;
        digitalWrite(BLOWER1_PIN, HIGH);
        Serial.println("[Actuator] Starting blower due to HIGH temp");
        saveConfig();
        blower_temp_triggered = true;
    }

    // blower1 timeout → blower2
    if (blowState == BLOW_RUN1
        && (millis() - blowStartMillis) >= (unsigned long)getBlowerOnTime() * 1000UL)
    {
        digitalWrite(BLOWER1_PIN, LOW);
        blowState       = BLOW_RUN2;
        blowStartMillis = millis();
        digitalWrite(BLOWER2_PIN, HIGH);
        Serial.println("[Actuator2] Blower1 done, starting blower2...");
    }

    // blower2 timeout → done
    if (blowState == BLOW_RUN2
        && (millis() - blowStartMillis) >= (unsigned long)getBlowerOnTime() * 1000UL)
    {
        digitalWrite(BLOWER2_PIN, LOW);
        blowState      = BLOW_IDLE;
        Serial.println("[Actuator2] Blower2 done, sequence complete.");
    }
    ActuatorStatusToSerial();
    updateManualScreenLEDs(pumpActive, blowState);
}

void ActuatorStatusToSerial() {
    // remember last state across calls
    static bool prevPumpActive      = false;
    static bool prevBlowerRun1      = false;
    bool       currBlowerRun1       = (blowState == BLOW_RUN1);

    // Pump just turned on?
    if (pumpActive && !prevPumpActive) {
        Serial.println("Pump:");
    }

    // Blower1 just turned on?
    if (currBlowerRun1 && !prevBlowerRun1) {
        Serial.println("Blower:");
    }

    // update saved state
    prevPumpActive  = pumpActive;
    prevBlowerRun1  = currBlowerRun1;
}



