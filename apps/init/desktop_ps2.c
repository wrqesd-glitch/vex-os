#include "desktop_ps2.h"

enum {
    PS2_SCANCODE_EXTENDED = 0xE0,
    PS2_WAIT_LIMIT = 100000u
};

u8 ps2_port_read(u16 port) {
    u8 value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "d"(port));
    return value;
}

void ps2_port_write(u16 port, u8 value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "d"(port));
}

u8 ps2_read_status(void) {
    return ps2_port_read(PS2_STATUS_PORT);
}

u8 ps2_read_data(void) {
    return ps2_port_read(PS2_DATA_PORT);
}

static u32 ps2_wait_write(void) {
    for (u32 spin = 0; spin < PS2_WAIT_LIMIT; ++spin) {
        if ((ps2_read_status() & 0x02u) == 0u) {
            return 1u;
        }
    }
    return 0u;
}

static void ps2_flush_output(void) {
    for (u32 spin = 0; spin < PS2_WAIT_LIMIT; ++spin) {
        if ((ps2_read_status() & PS2_STATUS_OUTPUT_FULL) == 0u) {
            return;
        }
        (void)ps2_read_data();
    }
}

static u32 mouse_write(u8 value) {
    if (ps2_wait_write() == 0u) {
        return 0u;
    }
    ps2_port_write(PS2_STATUS_PORT, 0xD4u);
    if (ps2_wait_write() == 0u) {
        return 0u;
    }
    ps2_port_write(PS2_DATA_PORT, value);
    return 1u;
}

static u32 mouse_read_ack(void) {
    for (u32 spin = 0; spin < 100000u; ++spin) {
        if ((ps2_read_status() & PS2_STATUS_OUTPUT_FULL) != 0u) {
            const u8 value = ps2_read_data();
            if (value == 0xFAu) {
                return 1u;
            }
        }
    }
    return 0u;
}

u32 ps2_mouse_init(void) {
    ps2_flush_output();
    if (ps2_wait_write() == 0u) {
        return 0u;
    }
    ps2_port_write(PS2_STATUS_PORT, 0xA8u);
    if (mouse_write(0xF6u) == 0u || mouse_read_ack() == 0u) {
        return 0u;
    }
    if (mouse_write(0xF4u) == 0u || mouse_read_ack() == 0u) {
        return 0u;
    }
    return 1u;
}

ui_key_t ps2_decode_keyboard_scancode(u8 scancode, u32* extended_prefix) {
    if (scancode == PS2_SCANCODE_EXTENDED) {
        *extended_prefix = 1u;
        return KEY_NONE;
    }
    if ((scancode & 0x80u) != 0u) {
        *extended_prefix = 0u;
        return KEY_NONE;
    }
    if (*extended_prefix != 0u) {
        *extended_prefix = 0u;
        switch (scancode) {
        case 0x48: return KEY_UP;
        case 0x50: return KEY_DOWN;
        case 0x4B: return KEY_LEFT;
        case 0x4D: return KEY_RIGHT;
        default: return KEY_NONE;
        }
    }
    switch (scancode) {
    case 0x01: return KEY_ESCAPE;
    case 0x0E: return KEY_BACKSPACE;
    case 0x0F: return KEY_TAB;
    case 0x1C: return KEY_ENTER;
    default: return KEY_NONE;
    }
}

char ps2_decode_keyboard_char(u8 scancode, u32* extended_prefix) {
    if (scancode == PS2_SCANCODE_EXTENDED) {
        *extended_prefix = 1u;
        return 0;
    }
    if ((scancode & 0x80u) != 0u) {
        *extended_prefix = 0u;
        return 0;
    }
    if (*extended_prefix != 0u) {
        *extended_prefix = 0u;
        return 0;
    }
    switch (scancode) {
    case 0x02: return '1';
    case 0x03: return '2';
    case 0x04: return '3';
    case 0x05: return '4';
    case 0x06: return '5';
    case 0x07: return '6';
    case 0x08: return '7';
    case 0x09: return '8';
    case 0x0A: return '9';
    case 0x0B: return '0';
    case 0x0C: return '-';
    case 0x10: return 'q';
    case 0x11: return 'w';
    case 0x12: return 'e';
    case 0x13: return 'r';
    case 0x14: return 't';
    case 0x15: return 'y';
    case 0x16: return 'u';
    case 0x17: return 'i';
    case 0x18: return 'o';
    case 0x19: return 'p';
    case 0x1E: return 'a';
    case 0x1F: return 's';
    case 0x20: return 'd';
    case 0x21: return 'f';
    case 0x22: return 'g';
    case 0x23: return 'h';
    case 0x24: return 'j';
    case 0x25: return 'k';
    case 0x26: return 'l';
    case 0x2C: return 'z';
    case 0x2D: return 'x';
    case 0x2E: return 'c';
    case 0x2F: return 'v';
    case 0x30: return 'b';
    case 0x31: return 'n';
    case 0x32: return 'm';
    case 0x33: return ',';
    case 0x34: return '.';
    case 0x35: return '/';
    case 0x39: return ' ';
    default: return 0;
    }
}
