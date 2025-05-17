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

void create_footer(lv_obj_t *parent);
void update_footer_status(FooterStatus status);
void create_warnings_screen(void);

#endif /* SCREEN_WARNINGS_H */
