/******************************************************************************
 * @file GVSU_Logo.h
 * @author Thomas Zoldowski
 * @date   May 10, 2025
 * @brief  Header file for the GVSU logo image used in the UI.
 *
 * This file declares the GVSU logo image for use with LVGL.
 * It provides an external declaration for the image symbol,
 * allowing it to be used in the UI components.
 ******************************************************************************/

#ifndef GVSU_LOGO_H
#define GVSU_LOGO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

// External declaration for LVGL to find the image symbol
LV_IMG_DECLARE(GVSU_Logo);

#ifdef __cplusplus
}
#endif

#endif /* GVSU_LOGO_H */
