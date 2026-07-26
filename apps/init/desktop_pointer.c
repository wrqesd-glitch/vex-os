#include "desktop_pointer.h"

static void close_active_window(shell_state_t* state, shell_window_t* windows, u32 window_count) {
    desktop_session_close_active_window(state, windows, window_count);
}

u32 desktop_pointer_is_hot(
    const shell_state_t* state,
    const shell_window_t* windows,
    u32 window_count,
    const desktop_pointer_routes_t* routes
) {
    const u32 x = state->cursor_x;
    const u32 y = state->cursor_y;
    if (routes->taskbar_hit(x, y) != 0u ||
        routes->desktop_icon_hit(x, y) != 0u ||
        routes->start_item_hit(x, y) != 0u ||
        routes->hub_card_hit(x, y) != 0u) {
        return 1u;
    }
    if (desktop_session_any_open_windows(windows, window_count) != 0u &&
        (routes->window_close_hit(x, y) != 0u ||
         routes->window_minimize_hit(x, y) != 0u ||
         routes->window_title_hit(x, y) != 0u)) {
        return 1u;
    }
    return 0u;
}

void desktop_pointer_handle_click(
    shell_state_t* state,
    shell_window_t* windows,
    u32* order,
    u32 window_count,
    const desktop_pointer_routes_t* routes
) {
    const u32 x = state->cursor_x;
    const u32 y = state->cursor_y;
    const u32 taskbar_hit = routes->taskbar_hit(x, y);

    if (taskbar_hit != 0u) {
        desktop_session_activate_taskbar_entry(state, windows, order, window_count, taskbar_hit - 1u, routes->open_view);
        return;
    }

    if (state->start_open != 0u) {
        if (routes->start_menu_hit(x, y) != 0u) {
            const u32 item_hit = routes->start_item_hit(x, y);
            if (item_hit != 0u) {
                routes->activate_start_entry(item_hit - 1u);
                return;
            }
            state->focus = FOCUS_START;
            state->dirty = 1u;
            return;
        }
        desktop_session_set_start_open(state, 0u);
        state->focus = desktop_session_any_open_windows(windows, window_count) != 0u ? FOCUS_WINDOW : FOCUS_DESKTOP;
        state->dirty = 1u;
    }

    if (desktop_session_any_open_windows(windows, window_count) != 0u) {
        if (routes->window_close_hit(x, y) != 0u) {
            close_active_window(state, windows, window_count);
            return;
        }

        if (routes->window_minimize_hit(x, y) != 0u) {
            desktop_session_minimize_active_window(state, windows, window_count);
            return;
        }

        {
            const u32 slot_hit = routes->window_slot_hit(x, y);
            if (slot_hit != 0u) {
                const u32 slot = slot_hit - 1u;
                routes->activate_window_slot(slot);
                {
                    const u32 card_hit = routes->hub_card_hit(x, y);
                    if (card_hit != 0u && card_hit <= hub_card_count()) {
                        routes->activate_hub_card(card_hit - 1u);
                        return;
                    }
                }
                if (routes->window_title_hit(x, y) != 0u) {
                    state->dragging_window = 1u;
                    state->drag_offset_x = x - state->window_x;
                    state->drag_offset_y = y - state->window_y;
                    return;
                }
                return;
            }
        }
    }

    {
        const u32 icon_hit = routes->desktop_icon_hit(x, y);
        if (icon_hit != 0u) {
            const u32 index = icon_hit - 1u;
            if (state->focus == FOCUS_DESKTOP && state->desktop_index == index) {
                routes->activate_desktop_entry(index);
            } else {
                state->desktop_index = index;
                state->focus = FOCUS_DESKTOP;
                state->dirty = 1u;
            }
            return;
        }
    }

    state->focus = FOCUS_DESKTOP;
    state->dirty = 1u;
}

void desktop_pointer_handle_packet(
    shell_state_t* state,
    shell_window_t* windows,
    u32* order,
    u32 window_count,
    const u8 packet[3],
    const desktop_pointer_routes_t* routes
) {
    (void)order;
    const u8 flags = packet[0];
    const int dx = (int)(signed char)packet[1];
    const int dy = (int)(signed char)packet[2];
    const u32 old_x = state->cursor_x;
    const u32 old_y = state->cursor_y;
    const u32 old_left = state->mouse_left_down;
    const u32 width = routes->framebuffer_width();
    const u32 height = routes->framebuffer_height();
    int next_x = (int)state->cursor_x + dx;
    int next_y = (int)state->cursor_y - dy;

    if (next_x < 0) {
        next_x = 0;
    } else if ((u32)next_x >= width) {
        next_x = (int)(width - 1u);
    }
    if (next_y < 0) {
        next_y = 0;
    } else if ((u32)next_y >= height) {
        next_y = (int)(height - 1u);
    }

    state->cursor_x = (u32)next_x;
    state->cursor_y = (u32)next_y;
    state->mouse_left_down = flags & 0x01u;

    if (state->dragging_window != 0u && state->mouse_left_down != 0u) {
        const u32 max_x = width > state->window_width + 12u ? width - state->window_width - 12u : 0u;
        const u32 max_y = height > state->window_height + 60u ? height - state->window_height - 60u : 0u;
        int window_x = (int)state->cursor_x - (int)state->drag_offset_x;
        int window_y = (int)state->cursor_y - (int)state->drag_offset_y;
        if (window_x < 12) {
            window_x = 12;
        } else if ((u32)window_x > max_x) {
            window_x = (int)max_x;
        }
        if (window_y < 24) {
            window_y = 24;
        } else if ((u32)window_y > max_y) {
            window_y = (int)max_y;
        }
        state->window_x = (u32)window_x;
        state->window_y = (u32)window_y;
        desktop_session_commit_active_window_geometry(state, windows);
    }

    if (state->cursor_x != old_x || state->cursor_y != old_y || state->mouse_left_down != old_left) {
        state->dirty = 1u;
    }
    if (old_left == 0u && state->mouse_left_down != 0u) {
        desktop_pointer_handle_click(state, windows, order, window_count, routes);
    } else if (old_left != 0u && state->mouse_left_down == 0u) {
        state->dragging_window = 0u;
    }
}
