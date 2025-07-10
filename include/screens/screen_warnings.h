/******************************************************************************
 * @file    screen_warnings.h
 * @author  Thomas Zoldowski
 * @date    May 10, 2025
 * @brief   Header for the Warnings and Alerts Screen UI.
 *
 * Layout:
 *  - List of active and recent warnings
 *  - Each item can include timestamp and type (e.g. temp, oxygen)
 *  - Color-coded or icon-tagged list entries
 ******************************************************************************/

#ifndef SCREEN_WARNINGS_H
#define SCREEN_WARNINGS_H

#include <lvgl.h>

enum FooterStatus {
    FOOTER_OK,
    FOOTER_WARNING
};

// Possible warnings bitmask
enum WarningMask {
    WARN_NONE          = 0,
    WARN_FRONT_DOOR    = 1 << 0,
    WARN_BACK_DOOR     = 1 << 1,
    WARN_LOADING_DOOR  = 1 << 2,
    WARN_HIGH_TEMP     = 1 << 3,
};

// Add more warnings as needed
void format_warnings(uint32_t mask, char *buf, size_t buf_sz, lv_obj_t *label);

/**
 * Create & show the Warnings screen.
 * Must be called once to initialize.
 */
lv_obj_t* create_warnings_screen(void);

/**
 * Append a new warning. Timestamp and desc must be null-terminated strings.
 * The table will grow by one row automatically.
 */
void add_warning(const char *description);


#endif /* SCREEN_WARNINGS_H */

// EOF