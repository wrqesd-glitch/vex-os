#include "port.h"
#include "../../include/vex/kernel.h"

enum {
    COM1 = 0x3F8
};

void serial_init(void) {
    out8(COM1 + 1, 0x00);
    out8(COM1 + 3, 0x80);
    out8(COM1 + 0, 0x03);
    out8(COM1 + 1, 0x00);
    out8(COM1 + 3, 0x03);
    out8(COM1 + 2, 0xC7);
    out8(COM1 + 4, 0x0B);
}

void serial_write_char(char value) {
    while ((in8(COM1 + 5) & 0x20u) == 0u) {
    }
    out8(COM1, (u8)value);
}
