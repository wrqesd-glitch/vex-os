#include "desktop_layout.h"

u32 point_in_rect(u32 x, u32 y, const ui_rect_t* rect) {
    return x >= rect->x && y >= rect->y &&
           x < rect->x + rect->width &&
           y < rect->y + rect->height;
}

ui_rect_t desktop_icon_rect(u32 index) {
    ui_rect_t rect;
    rect.x = 22u;
    rect.y = 80u + index * 124u;
    rect.width = 96u;
    rect.height = 118u;
    return rect;
}

ui_rect_t taskbar_button_rect(u32 index, u32 desktop_height) {
    ui_rect_t rect;
    u32 x = 8u;
    rect.y = desktop_height - 42u;
    rect.height = 34u;
    for (u32 current = 0; current <= index; ++current) {
        rect.x = x;
        if (current == 0u) {
            rect.width = 92u;
        } else {
            const shell_view_t view = taskbar_entry_view(current);
            rect.width = view == VIEW_TESTS ? 132u : 112u;
        }
        if (current == index) {
            return rect;
        }
        x += rect.width + 8u;
    }
    return rect;
}

ui_rect_t start_menu_rect(u32 desktop_height) {
    ui_rect_t rect;
    rect.x = 10u;
    rect.y = desktop_height - 406u;
    rect.width = 280u;
    rect.height = 344u;
    return rect;
}

ui_rect_t start_item_rect(u32 index, u32 desktop_height) {
    ui_rect_t rect = start_menu_rect(desktop_height);
    rect.x += 12u;
    rect.y += 76u + index * 38u;
    rect.width = 256u;
    rect.height = 28u;
    return rect;
}

ui_rect_t active_window_rect(const shell_state_t* state) {
    ui_rect_t rect;
    rect.x = state->window_x;
    rect.y = state->window_y;
    rect.width = state->window_width;
    rect.height = state->window_height;
    return rect;
}

ui_rect_t window_rect_for_slot(const shell_window_t* windows, u32 slot) {
    ui_rect_t rect;
    rect.x = windows[slot].render_visible != 0u ? windows[slot].render_x : windows[slot].x;
    rect.y = windows[slot].render_visible != 0u ? windows[slot].render_y : windows[slot].y;
    rect.width = windows[slot].render_visible != 0u ? windows[slot].render_width : windows[slot].width;
    rect.height = windows[slot].render_visible != 0u ? windows[slot].render_height : windows[slot].height;
    return rect;
}

ui_rect_t window_title_rect(const shell_state_t* state) {
    ui_rect_t rect = active_window_rect(state);
    rect.height = 34u;
    return rect;
}

ui_rect_t window_close_rect(const shell_state_t* state) {
    const ui_rect_t window = active_window_rect(state);
    ui_rect_t rect;
    rect.x = window.x + window.width - 38u;
    rect.y = window.y + 10u;
    rect.width = 14u;
    rect.height = 14u;
    return rect;
}

ui_rect_t window_minimize_rect(const shell_state_t* state) {
    const ui_rect_t window = active_window_rect(state);
    ui_rect_t rect;
    rect.x = window.x + window.width - 60u;
    rect.y = window.y + 10u;
    rect.width = 14u;
    rect.height = 14u;
    return rect;
}

ui_rect_t explorer_card_rect(const shell_state_t* state, u32 row, u32 column) {
    const ui_rect_t window = active_window_rect(state);
    ui_rect_t rect;
    rect.x = window.x + 228u + column * 204u;
    rect.y = window.y + 72u + row * 176u;
    rect.width = 184u;
    rect.height = 160u;
    return rect;
}
