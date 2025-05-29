
#ifndef SCREEN_DIAGNOSTICS_H
#define SCREEN_DIAGNOSTICS_H

#include <lvgl.h>

// Create and return the diagnostics screen object
lv_obj_t* create_diagnostics_screen(void);

// Check if diagnostics screen is currently active
bool is_diagnostics_screen_active(void);

// Refresh connection status on the diagnostics screen
void update_diagnostics_screen(void);

#endif // SCREEN_DIAGNOSTICS_H