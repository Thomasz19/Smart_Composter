#include "settings_storage.h"

// These must match the extern in settings_storage.h:
Config config;

// The LittleFileSystem instance and block‐device pointer come from your main code.
// Adjust names if yours are different.


// Path to the file we’ll store our settings in:
static const char *CONFIG_PATH = "/user/config.bin";

void loadConfig() {
    // Try opening /config.bin for read
    FILE *f = fopen(CONFIG_PATH, "rb");
    if (f) {
        // File exists → read exactly sizeof(Config) bytes. 
        size_t n = fread(&config, 1, sizeof(Config), f);
        fclose(f);
        if (n == sizeof(Config)) {
            // Successfully loaded everything
            return;
            Serial.println("[LFS]OK: Loaded from config file");
        }
        // If we read the wrong size, fall through and rewrite defaults.
    }

    // If we get here:
    // 1) config.bin didn’t exist, or
    // 2) its size was wrong / read failed.
    // So fill config[] with defaults, then saveConfig() to create the file.

    // --- DEFAULTS: change these as you like ---
    for (int i = 0; i < 3; i++) {
        config.temp_low[i]  = 130.0f;
        config.temp_high[i] = 160.0f;
        config.hum_low[i]   = 40.0f;
    }
    strcpy(config.user_pin, "0000");
    config.pin_protection_enabled = true;
    config.blower_duration_sec = 10;
    config.pump_duration_sec   = 10;
    config.activation_interval_min = 60; // 1 minute
    // ------------------------------------------

    uint32_t now = time(nullptr);
    config.lastPumpEpoch    = now;
    config.lastBlowerEpoch  = now;

    config.camera_delay_sec = 5;  // default camera delay
    config.send_interval_min = 15; // default send interval

    saveConfig();
}

void saveConfig() {
    // Open (or create/truncate) /config.bin for write
    FILE *f = fopen(CONFIG_PATH, "wb");
    if (!f) {
        Serial.println("[LFS]ERROR: Could not open config.bin for writing!");
        return;
    }
    Serial.println("[LFS]OK: Changed Saved");
    fwrite(&config, 1, sizeof(Config), f);
    fclose(f);
}  