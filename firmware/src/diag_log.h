#ifndef _FEATHER_DIAG_LOG_H_
#define _FEATHER_DIAG_LOG_H_

#include <stdint.h>
#include <stdbool.h>

// Read-only feature report on HID configuration interface (one 63-byte payload).
#define REPORT_ID_DIAGNOSTICS 102

#ifdef __cplusplus
extern "C" {
#endif
int feather_diag_printf(char const* format, ...);
uint16_t feather_diag_read(uint8_t* out, uint16_t length);
// Capture TinyUSB host task logs, not USB-C device-side polling noise.
void feather_diag_capture_host(bool enable);
#ifdef __cplusplus
}
#endif

#endif
