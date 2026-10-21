#include "diag_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "hardware/sync.h"

// Debug-only RAM buffer. The oldest data is dropped if it fills.
namespace {
constexpr uint32_t LOG_CAPACITY = 16384;
constexpr uint16_t REPORT_PAYLOAD_SIZE = 63;
constexpr uint8_t REPORT_DATA_SIZE = 61;
char log_bytes[LOG_CAPACITY];
uint32_t write_pos = 0;
uint32_t read_pos = 0;
volatile bool capture_host_logs = false;
}

extern "C" void feather_diag_capture_host(bool enable) {
    capture_host_logs = enable;
}

extern "C" int feather_diag_printf(char const* format, ...) {
    // TinyUSB logs device-side GET_REPORT for every read of this buffer.
    // Keep only logs emitted while tuh_task() is running; preserve
    // explicit startup diagnostics outside the host task.
    if (!capture_host_logs && strncmp(format, "[USB-DIAG]", 10) != 0) return 0;
    char line[256];
    va_list args;
    va_start(args, format);
    int written = vsnprintf(line, sizeof(line), format, args);
    va_end(args);
    if (written <= 0) return written;

    uint32_t count = (uint32_t) written;
    if (count >= sizeof(line)) count = sizeof(line) - 1;

    // Keep the critical section short. Avoid calling printf inside it.
    uint32_t saved = save_and_disable_interrupts();
    for (uint32_t i = 0; i < count; i++) {
        log_bytes[write_pos % LOG_CAPACITY] = line[i];
        write_pos++;
        if (write_pos - read_pos > LOG_CAPACITY) {
            read_pos = write_pos - LOG_CAPACITY;
        }
    }
    restore_interrupts(saved);
    return written;
}

extern "C" uint16_t feather_diag_read(uint8_t* out, uint16_t length) {
    if (!out || length < REPORT_PAYLOAD_SIZE) return 0;

    memset(out, 0, REPORT_PAYLOAD_SIZE);
    out[0] = 0xD6; // Protocol marker; the report ID is supplied separately.

    uint32_t saved = save_and_disable_interrupts();
    uint32_t available = write_pos - read_pos;
    uint8_t n = available > REPORT_DATA_SIZE ? REPORT_DATA_SIZE : (uint8_t) available;
    out[1] = n;
    for (uint8_t i = 0; i < n; i++) {
        out[2+i] = (uint8_t) log_bytes[(read_pos+i) % LOG_CAPACITY];
    }
    read_pos += n;
    restore_interrupts(saved);
    return REPORT_PAYLOAD_SIZE;
}
