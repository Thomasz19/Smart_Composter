#include <Arduino.h>

#ifndef SETTINGS_STORAGE_H
#define SETTINGS_STORAGE_H
// This struct holds all of the user‐editable values
// that you want to survive a reboot.
typedef struct {
    // 3 sensors, each has temp_low, temp_high, hum_low
    float temp_low[3];
    float temp_high[3];
    float hum_low[3];

    // 4‐digit PIN string + null terminator
    char user_pin[5];  // e.g. "0742"

    // True if PIN protection is on; false if disabled
    bool pin_protection_enabled;

    // How many seconds the blower should run
    int blower_duration_sec;

    // How many seconds the pump should run
    int pump_duration_sec;
} Config;

// A single global instance that your UI code will reference.
// After loadConfig(), you should copy these into your sensor_thresh[],
// user_pin[], etc., and after changes, copy them back here and save.
extern Config config;

// Call this once (after mounting LittleFS) to load "/config.bin" if it exists.
// If it does not exist or is corrupt, loadConfig() will populate 'config'
// with sensible defaults and immediately write them out.
void loadConfig();

// Call this any time you (or the user) change a value in 'config'.
// It will open "/config.bin" (creating/truncating if needed) and write all bytes.
void saveConfig();

#endif /* SETTINGS_STORAGE_H */