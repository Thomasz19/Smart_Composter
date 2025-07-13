/******************************************************************************
 * @file    screen_warnings.cpp
 * @author  Thomas Zoldowski
 * @date    May 10, 2025
 * @brief   Implementation of the Warnings and Alerts Screen UI.
 ******************************************************************************/

#include "screens/screen_warnings.h"
#include "ui_manager.h"

#include <time.h>
#include <lvgl.h>
#include <Arduino.h>  // for millis()

static void warnings_table_draw_cb(lv_event_t * e);

// warning screen
// Max number of warnings we keep in memory
#define MAX_WARNINGS 20

// Storage for text so table cells point to valid memory
static char ts_buf    [MAX_WARNINGS+1][32];
static char desc_buf  [MAX_WARNINGS+1][64];

// LVGL objects
static lv_obj_t * warnings_table  = nullptr;

const int HEADER_H  = 80;
const int FOOTER_H  = 60;
const int SCREEN_H  = 480;
const int TABLE_H   = SCREEN_H - HEADER_H - FOOTER_H;

// How many warnings we’ve added so far
static int warning_count = 0;

/** @brief Add a warning to the warnings table.
 *  This function adds a new warning to the table, updating the display and managing the warning count.
 *  @param desc The description of the warning.
 */
static void warnings_table_draw_cb(lv_event_t * e)
{
    // Get the low-level draw task and its base descriptor
    lv_draw_task_t * task = lv_event_get_draw_task(e);
    lv_draw_dsc_base_t * base_dsc = (lv_draw_dsc_base_t *)lv_draw_task_get_draw_dsc(task);

    // We only want to re-style the cell backgrounds & labels
    if(base_dsc->part != LV_PART_ITEMS) return;

    // Which row/col is being drawn?
    uint32_t row = base_dsc->id1;
    uint32_t col = base_dsc->id2;
    /*Make the texts in the first cell center aligned*/
        if(row == 0) {
            lv_draw_label_dsc_t * label_draw_dsc = lv_draw_task_get_label_dsc(task);
            if(label_draw_dsc) {
                label_draw_dsc->align = LV_TEXT_ALIGN_CENTER;
            }
            lv_draw_fill_dsc_t * fill_draw_dsc = lv_draw_task_get_fill_dsc(task);
            if(fill_draw_dsc) {
                fill_draw_dsc->color = lv_color_mix(lv_color_hex(0xde6a6a), fill_draw_dsc->color, LV_OPA_COVER);
                fill_draw_dsc->opa = LV_OPA_COVER;
            }
        }
        /*In the first column align the texts to the right*/
        else if(col == 0) {
            lv_draw_label_dsc_t * label_draw_dsc = lv_draw_task_get_label_dsc(task);
            if(label_draw_dsc) {
                label_draw_dsc->align = LV_TEXT_ALIGN_LEFT;
            }
        }

        /*Make every 2nd row grayish*/
        if((row != 0 && row % 2) == 0) {
            lv_draw_fill_dsc_t * fill_draw_dsc = lv_draw_task_get_fill_dsc(task);
            if(fill_draw_dsc) {
                fill_draw_dsc->color = lv_color_mix(lv_color_hex(0xd4d4d4), fill_draw_dsc->color, LV_OPA_80);
                fill_draw_dsc->opa = LV_OPA_COVER;
            }
        }else{
            lv_draw_fill_dsc_t * fill_draw_dsc = lv_draw_task_get_fill_dsc(task);
            if(fill_draw_dsc) {
                fill_draw_dsc->color = lv_color_mix(lv_color_hex(0xc0c9d9), fill_draw_dsc->color, LV_OPA_80);
                fill_draw_dsc->opa = LV_OPA_COVER;
            }
        }
}

/** @brief Create the Warnings screen.
 *  This function initializes the warnings screen with a table to display warnings.
 *  @return Pointer to the created warnings screen object.
 */
lv_obj_t* create_warnings_screen() {

    lv_obj_t* warnings_screen = lv_obj_create(NULL);

    // Background color 
    lv_obj_set_style_bg_color(warnings_screen, lv_color_hex(0xc0c9d9), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(warnings_screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(warnings_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(warnings_screen, LV_DIR_NONE);

    // create header + footer
    create_header(warnings_screen, "Warnings");

    // Create a table
    warnings_table = lv_table_create(warnings_screen);
    lv_table_set_col_cnt(warnings_table, 2);
    lv_table_set_row_cnt(warnings_table, 1);

    // Set column widths (percent of total)
    lv_table_set_col_width(warnings_table, 0, 200);        // 200px for timestamp
    lv_table_set_col_width(warnings_table, 1, 600);        // 600px for description

    // Populate header
    lv_table_set_cell_value(warnings_table, 0, 0, "Time");
    lv_table_set_cell_value(warnings_table, 0, 1, "Description");

    // Size & position between header and footer
    lv_obj_set_size(warnings_table, lv_pct(100), TABLE_H);
    lv_obj_align   (warnings_table, LV_ALIGN_TOP_MID, 0, HEADER_H);
    lv_obj_set_style_text_font(warnings_table, &lv_font_montserrat_36, 0);

    // Style the table
    lv_obj_add_event_cb(warnings_table, warnings_table_draw_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_obj_add_flag(warnings_table, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    lv_obj_set_scroll_dir(warnings_table, LV_DIR_VER);
    
    //add_warning("Test");
    return warnings_screen;
}

/** @brief Format warnings into a human-readable string.
 *  This function formats the warning mask into a string for display.
 *  @param mask The warning mask to format.
 *  @param buf The buffer to store the formatted string.
 *  @param buf_sz The size of the buffer.
 *  @param label The label to update with the formatted string.
 * */
void format_warnings(uint32_t mask, char *buf, size_t buf_sz, lv_obj_t *label) {
    if(mask == WARN_NONE) {
        snprintf(buf, buf_sz, "ALL SYSTEMS NOMINAL");
        lv_obj_set_style_text_color(label, lv_color_hex(0x094211), 0);
        return;
    }
    buf[0] = '\0';
    bool first = true;
    auto append = [&](const char* msg) {
        if(!first) strncat(buf, ", ", buf_sz - strlen(buf) - 1);
        strncat(buf, msg, buf_sz - strlen(buf) - 1);
        first = false;
    };
    if(mask & WARN_FRONT_DOOR)   append("FRONT UNLOADING DOOR OPEN");
    if(mask & WARN_BACK_DOOR)    append("BACK UNLOADING DOOR OPEN");
    if(mask & WARN_LOADING_DOOR) append("LOADING DOOR OPEN");
    if(mask & WARN_HIGH_TEMP)    append("HIGH TEMP");
}

/** @brief Add a warning to the warnings table.
 *  This function adds a new warning to the table, updating the display and managing the warning count.
 *  @param description The description of the warning.
 */
void add_warning(const char *description) {
    if(!warnings_table) return;

    // 1) Generate current time “HH:MM:SS” into a temporary buffer
    char new_ts[16];
    time_t now = time(NULL);
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    strftime(new_ts, sizeof(new_ts), "%H:%M:%S", &tm_info);

    // 2) If not full yet, grow the table by one row
    if(warning_count < MAX_WARNINGS) {
        warning_count++;
        lv_table_set_row_cnt(warnings_table, warning_count + 1); // +1 for header
    }

    // 3) Shift existing entries down one row (1→2, 2→3, …)
    //    If at capacity, row MAX_WARNINGS is dropped
    for(int i = warning_count; i >= 2; --i) {
        strcpy(ts_buf[i],   ts_buf[i - 1]);
        strcpy(desc_buf[i], desc_buf[i - 1]);
    }

    // 4) Insert the new entry at row 1
    strcpy(ts_buf[1], new_ts);
    strncpy(desc_buf[1], description, sizeof(desc_buf[0]) - 1);
    desc_buf[1][sizeof(desc_buf[0]) - 1] = '\0';

    // 5) Refresh the visible rows (rows 1..warning_count)
    for(int r = 1; r <= warning_count; ++r) {
        lv_table_set_cell_value(warnings_table, r, 0, ts_buf[r]);
        lv_table_set_cell_value(warnings_table, r, 1, desc_buf[r]);
    }
}

