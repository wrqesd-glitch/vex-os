#ifndef VEX_INIT_DESKTOP_LAYOUT_H
#define VEX_INIT_DESKTOP_LAYOUT_H

#include "desktop_session.h"

typedef struct ui_rect {
    u32 x;
    u32 y;
    u32 width;
    u32 height;
} ui_rect_t;

u32 point_in_rect(u32 x, u32 y, const ui_rect_t* rect);
ui_rect_t desktop_icon_rect(u32 index);
ui_rect_t taskbar_button_rect(u32 index, u32 desktop_height);
ui_rect_t start_menu_rect(u32 desktop_height);
ui_rect_t start_item_rect(u32 index, u32 desktop_height);
ui_rect_t active_window_rect(const shell_state_t* state);
ui_rect_t window_rect_for_slot(const shell_window_t* windows, u32 slot);
ui_rect_t window_title_rect(const shell_state_t* state);
ui_rect_t window_close_rect(const shell_state_t* state);
ui_rect_t window_minimize_rect(const shell_state_t* state);
ui_rect_t explorer_card_rect(const shell_state_t* state, u32 row, u32 column);

#endif
