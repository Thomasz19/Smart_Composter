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

#define MINUTE1 60UL  // 1 minute in milliseconds
#define MINUTE5 300UL // 5 minutes in milliseconds
#define HOUR1 3600UL // 1 hour in milliseconds



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


// ─────────────────────────────────────────────────────────────────────────────
/// Call this every loop()
void scheduleHourlyActuators() {
    uint32_t nowSec = (uint32_t)time(nullptr);

    uint32_t Interval = getActivationInterval();
    // Serial.print("[Actuator1] Interval: ");
    // Serial.println(Interval);

    // switch (blowState) {
    //     case BLOW_IDLE:   Serial.println("IDLE"); break;
    //     case BLOW_RUN1:   Serial.println("RUN1"); break;
    //     case BLOW_RUN2:   Serial.println("RUN2"); break;
    // }

    //Serial.println(nowSec);
    Serial.println(nowSec-config.lastPumpEpoch);
  // ── Pump (only if not mid‐blower) ─────────────────────
    if (!pumpActive 
        && blowState == BLOW_IDLE
        && (nowSec - config.lastPumpEpoch) >= Interval) 
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

    updateManualScreenLEDs(pumpActive, blowState);
}


