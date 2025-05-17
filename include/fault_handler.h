#ifndef FAULT_HANDLER_H_
#define FAULT_HANDLER_H_

#include "mbed_error.h"

// Called automatically by Mbed if MBED_CONF_PLATFORM_ERROR_HOOK_ENABLED is set
void mbed_error_hook(const mbed_error_ctx *ctx);

// Optional: call on boot to report last crash (future extension)
void check_and_report_previous_fault();

#endif // FAULT_HANDLER_H_
