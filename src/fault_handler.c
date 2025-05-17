/*
Not working... idk
*/


#include "fault_handler.h"
#include <Arduino.h>

// WARNING: yes there is a warning here but this function is not used so im not going to fix it yet
void mbed_error_hook(const mbed_error_ctx *ctx) { 
    printf("\n=== SYSTEM FAULT DETECTED ===\n");
    printf("Status     : 0x%08lX\n", ctx->error_status);
    printf("File       : %s\n", ctx->error_filename ? ctx->error_filename : "Unknown");
    printf("Line       : %d\n", ctx->error_line_number);
    printf("Address    : 0x%08lX\n", (uint32_t)ctx->error_address);
    printf("Thread ID  : 0x%08lX\n", (uint32_t)ctx->thread_id);

    // TODO: Write to EEPROM or flash for post-reboot diagnostics

    // Freeze or reset
    while (1);  // Or NVIC_SystemReset();
}

void check_and_report_previous_fault() {
    // Placeholder for future implementation:
    // Read crash info from EEPROM and print or display
}
