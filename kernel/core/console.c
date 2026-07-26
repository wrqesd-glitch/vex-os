#include "../include/vex/kernel.h"

void console_write(const char* text) {
    while (*text != '\0') {
        if (*text == '\n') {
            serial_write_char('\r');
        }
        serial_write_char(*text++);
        framebuffer_console_write_char(text[-1]);
    }
}

void console_write_line(const char* text) {
    console_write(text);
    console_write("\n");
}

void console_write_hex(u64 value) {
    static const char digits[] = "0123456789ABCDEF";
    console_write("0x");
    for (i64 shift = 60; shift >= 0; shift -= 4) {
        const char digit = digits[(value >> (u64)shift) & 0xFu];
        serial_write_char(digit);
        framebuffer_console_write_char(digit);
    }
}

void console_write_u64(u64 value) {
    char buffer[32];
    u32 index = 0;
    if (value == 0) {
        serial_write_char('0');
        framebuffer_console_write_char('0');
        return;
    }

    while (value > 0) {
        buffer[index++] = (char)('0' + (value % 10u));
        value /= 10u;
    }
    while (index > 0) {
        const char digit = buffer[--index];
        serial_write_char(digit);
        framebuffer_console_write_char(digit);
    }
}
