#ifndef VEX_INIT_DESKTOP_PS2_H
#define VEX_INIT_DESKTOP_PS2_H

#include "desktop_input.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

enum {
    PS2_DATA_PORT = 0x60,
    PS2_STATUS_PORT = 0x64,
    PS2_STATUS_OUTPUT_FULL = 0x01,
    PS2_STATUS_AUX_FULL = 0x20
};

u8 ps2_port_read(u16 port);
void ps2_port_write(u16 port, u8 value);
u8 ps2_read_status(void);
u8 ps2_read_data(void);
u32 ps2_mouse_init(void);
ui_key_t ps2_decode_keyboard_scancode(u8 scancode, u32* extended_prefix);
char ps2_decode_keyboard_char(u8 scancode, u32* extended_prefix);

#endif
