#include "port.h"
#include "../../include/vex/kernel.h"

enum {
    PS2_DATA_PORT = 0x60,
    PS2_STATUS_PORT = 0x64,
    PS2_STATUS_OUTPUT_FULL = 0x01,
    PS2_SCANCODE_EXTENDED = 0xE0
};

static u32 g_extended_prefix;

void ps2_keyboard_init(void) {
    g_extended_prefix = 0u;
    while ((in8(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_FULL) != 0u) {
        (void)in8(PS2_DATA_PORT);
    }
}

vex_key_event_t ps2_keyboard_poll(void) {
    if ((in8(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_FULL) == 0u) {
        return VEX_KEY_NONE;
    }

    const u8 scancode = in8(PS2_DATA_PORT);
    if (scancode == PS2_SCANCODE_EXTENDED) {
        g_extended_prefix = 1u;
        return VEX_KEY_NONE;
    }

    if ((scancode & 0x80u) != 0u) {
        g_extended_prefix = 0u;
        return VEX_KEY_NONE;
    }

    if (g_extended_prefix != 0u) {
        g_extended_prefix = 0u;
        switch (scancode) {
        case 0x48: return VEX_KEY_UP;
        case 0x50: return VEX_KEY_DOWN;
        case 0x4B: return VEX_KEY_LEFT;
        case 0x4D: return VEX_KEY_RIGHT;
        default: return VEX_KEY_NONE;
        }
    }

    switch (scancode) {
    case 0x01: return VEX_KEY_ESCAPE;
    case 0x0F: return VEX_KEY_TAB;
    case 0x1C: return VEX_KEY_ENTER;
    default: return VEX_KEY_NONE;
    }
}
