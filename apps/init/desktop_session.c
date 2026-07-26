#include "desktop_session.h"

enum {
    TRANSITION_NONE = 0,
    TRANSITION_OPEN = 1,
    TRANSITION_CLOSE = 2,
    TRANSITION_MINIMIZE = 3,
    WINDOW_TRANSITION_FRAMES = 8u,
    START_TRANSITION_FRAMES = 7u,
    START_MENU_MAX_HEIGHT = 344u
};

static u32 lerp_u32(u32 from, u32 to, u32 step, u32 max_step) {
    if (max_step == 0u) {
        return to;
    }
    return from + ((to - from) * step) / max_step;
}

u32 desktop_session_any_open_windows(const shell_window_t* windows, u32 window_count) {
    for (u32 index = 0; index < window_count; ++index) {
        if (windows[index].open != 0u) {
            return 1u;
        }
    }
    return 0u;
}

u32 desktop_session_any_visible_windows(const shell_window_t* windows, u32 window_count) {
    for (u32 index = 0; index < window_count; ++index) {
        if (windows[index].render_visible != 0u) {
            return 1u;
        }
    }
    return 0u;
}

static void set_window_render_to_target(shell_window_t* window) {
    window->render_x = window->x;
    window->render_y = window->y;
    window->render_width = window->width;
    window->render_height = window->height;
}

void desktop_session_close_all_windows(shell_state_t* state, shell_window_t* windows, u32 window_count) {
    for (u32 index = 0; index < window_count; ++index) {
        windows[index].open = 0u;
        windows[index].minimized = 0u;
        windows[index].render_visible = 0u;
        windows[index].animating = 0u;
        windows[index].transition_kind = TRANSITION_NONE;
        windows[index].transition_frame = 0u;
        set_window_render_to_target(&windows[index]);
    }
    state->window_open = 0u;
    state->dragging_window = 0u;
}

void desktop_session_sync_active_window_geometry(shell_state_t* state, const shell_window_t* windows) {
    const u32 slot = (u32)state->active_view;
    state->window_open = windows[slot].open != 0u || windows[slot].minimized != 0u;
    state->window_x = windows[slot].x;
    state->window_y = windows[slot].y;
    state->window_width = windows[slot].width;
    state->window_height = windows[slot].height;
}

void desktop_session_commit_active_window_geometry(const shell_state_t* state, shell_window_t* windows) {
    const u32 slot = (u32)state->active_view;
    windows[slot].x = state->window_x;
    windows[slot].y = state->window_y;
    windows[slot].width = state->window_width;
    windows[slot].height = state->window_height;
    if (windows[slot].animating == 0u) {
        set_window_render_to_target(&windows[slot]);
    }
}

void desktop_session_bring_window_to_front(u32 slot, u32* order, u32 window_count) {
    u32 next_order[8];
    u32 out = 0u;

    for (u32 i = 0; i < window_count; ++i) {
        if (order[i] != slot) {
            next_order[out++] = order[i];
        }
    }
    next_order[out] = slot;
    for (u32 i = 0; i < window_count; ++i) {
        order[i] = next_order[i];
    }
}

void desktop_session_activate_window_slot(
    shell_state_t* state,
    const shell_window_t* windows,
    u32* order,
    u32 window_count,
    u32 slot
) {
    state->active_view = (shell_view_t)slot;
    state->focus = FOCUS_WINDOW;
    desktop_session_bring_window_to_front(slot, order, window_count);
    desktop_session_sync_active_window_geometry(state, windows);
    state->dirty = 1u;
}

void desktop_session_init_windows(
    shell_state_t* state,
    shell_window_t* windows,
    u32* order,
    u32 window_count,
    u32 desktop_width,
    u32 desktop_height
) {
    const u32 base_width = desktop_width - 206u;
    const u32 base_height = desktop_height - 154u;

    for (u32 index = 0; index < window_count; ++index) {
        windows[index].open = 0u;
        windows[index].minimized = 0u;
        windows[index].render_visible = 0u;
        windows[index].animating = 0u;
        windows[index].transition_kind = TRANSITION_NONE;
        windows[index].transition_frame = 0u;
        windows[index].kind = (shell_view_t)index;
        windows[index].x = 180u + index * 22u;
        windows[index].y = 86u + index * 18u;
        windows[index].width = base_width;
        windows[index].height = base_height;
        windows[index].render_x = windows[index].x;
        windows[index].render_y = windows[index].y;
        windows[index].render_width = windows[index].width;
        windows[index].render_height = windows[index].height;
        order[index] = index;
    }
    state->start_visible = 0u;
    state->start_animating = 0u;
    state->start_animation_frame = 0u;
    state->start_render_height = 0u;
    desktop_session_sync_active_window_geometry(state, windows);
}

void desktop_session_set_start_open(shell_state_t* state, u32 open) {
    if (open != 0u) {
        state->start_open = 1u;
        state->start_visible = 1u;
        state->start_animating = 1u;
        state->start_animation_frame = 0u;
        state->start_render_height = 32u;
    } else if (state->start_visible != 0u) {
        state->start_open = 0u;
        state->start_animating = 1u;
        state->start_animation_frame = 0u;
    } else {
        state->start_open = 0u;
        state->start_render_height = 0u;
    }
    state->dirty = 1u;
}

void desktop_session_open_view(
    shell_state_t* state,
    shell_window_t* windows,
    u32* order,
    u32 window_count,
    shell_view_t view
) {
    const u32 slot = (u32)view;
    if (slot >= window_count) {
        return;
    }
    if (windows[slot].open != 0u && windows[slot].animating == 0u) {
        desktop_session_activate_window_slot(state, windows, order, window_count, slot);
        return;
    }
    windows[slot].open = 1u;
    windows[slot].minimized = 0u;
    windows[slot].render_visible = 1u;
    windows[slot].animating = 1u;
    windows[slot].transition_kind = TRANSITION_OPEN;
    windows[slot].transition_frame = 0u;
    windows[slot].render_width = windows[slot].width * 3u / 5u;
    windows[slot].render_height = windows[slot].height * 3u / 5u;
    windows[slot].render_x = windows[slot].x + (windows[slot].width - windows[slot].render_width) / 2u;
    windows[slot].render_y = windows[slot].y + 38u + (windows[slot].height - windows[slot].render_height) / 3u;
    state->active_view = view;
    state->window_open = 1u;
    state->focus = FOCUS_WINDOW;
    state->dragging_window = 0u;
    desktop_session_bring_window_to_front(slot, order, window_count);
    desktop_session_sync_active_window_geometry(state, windows);
    state->dirty = 1u;
}

void desktop_session_activate_taskbar_entry(
    shell_state_t* state,
    shell_window_t* windows,
    u32* order,
    u32 window_count,
    u32 taskbar_index,
    void (*open_view)(shell_view_t view)
) {
    state->taskbar_index = taskbar_index;
    state->focus = FOCUS_TASKBAR;

    if (taskbar_index == 0u) {
        desktop_session_set_start_open(state, state->start_open == 0u ? 1u : 0u);
        state->focus = state->start_open != 0u ? FOCUS_START : FOCUS_TASKBAR;
        state->dirty = 1u;
        return;
    }

    {
        const shell_view_t view = taskbar_entry_view(taskbar_index);
        const u32 slot = (u32)view;
        if (view_is_installed(view) == 0u) {
            return;
        }
        if (slot >= window_count) {
            if (open_view != 0) {
                open_view(view);
            }
            return;
        }
        if (windows[slot].minimized != 0u) {
            if (open_view != 0) {
                open_view(view);
            }
            return;
        }
        if (windows[slot].open != 0u) {
            if ((u32)state->active_view == slot && state->focus == FOCUS_WINDOW) {
                desktop_session_minimize_active_window(state, windows, window_count);
                return;
            }
            desktop_session_activate_window_slot(state, windows, order, window_count, slot);
            return;
        }
        if (open_view != 0) {
            open_view(view);
        }
    }
}

void desktop_session_close_active_window(shell_state_t* state, shell_window_t* windows, u32 window_count) {
    const u32 slot = (u32)state->active_view;
    if (slot >= window_count) {
        state->window_open = 0u;
        state->dragging_window = 0u;
        state->focus = FOCUS_DESKTOP;
        state->dirty = 1u;
        return;
    }
    windows[slot].open = 0u;
    windows[slot].minimized = 0u;
    windows[slot].render_visible = 1u;
    windows[slot].animating = 1u;
    windows[slot].transition_kind = TRANSITION_CLOSE;
    windows[slot].transition_frame = 0u;
    state->window_open = desktop_session_any_open_windows(windows, window_count);
    state->dragging_window = 0u;
    state->focus = desktop_session_any_visible_windows(windows, window_count) != 0u ? FOCUS_WINDOW : FOCUS_DESKTOP;
    state->dirty = 1u;
}

void desktop_session_minimize_active_window(shell_state_t* state, shell_window_t* windows, u32 window_count) {
    const u32 slot = (u32)state->active_view;
    if (slot >= window_count) {
        return;
    }
    windows[slot].open = 0u;
    windows[slot].minimized = 1u;
    windows[slot].render_visible = 1u;
    windows[slot].animating = 1u;
    windows[slot].transition_kind = TRANSITION_MINIMIZE;
    windows[slot].transition_frame = 0u;
    state->window_open = desktop_session_any_open_windows(windows, window_count);
    state->dragging_window = 0u;
    state->focus = FOCUS_TASKBAR;
    state->dirty = 1u;
}

u32 desktop_session_tick(shell_state_t* state, shell_window_t* windows, u32 window_count) {
    u32 dirty = 0u;

    if (state->start_animating != 0u) {
        if (state->start_open != 0u) {
            state->start_render_height = lerp_u32(32u, START_MENU_MAX_HEIGHT, state->start_animation_frame, START_TRANSITION_FRAMES);
        } else {
            state->start_render_height = lerp_u32(START_MENU_MAX_HEIGHT, 24u, state->start_animation_frame, START_TRANSITION_FRAMES);
        }
        ++state->start_animation_frame;
        dirty = 1u;
        if (state->start_animation_frame > START_TRANSITION_FRAMES) {
            state->start_animating = 0u;
            if (state->start_open != 0u) {
                state->start_render_height = START_MENU_MAX_HEIGHT;
                state->start_visible = 1u;
            } else {
                state->start_render_height = 0u;
                state->start_visible = 0u;
            }
        }
    }

    for (u32 index = 0; index < window_count; ++index) {
        shell_window_t* window = &windows[index];
        if (window->animating == 0u) {
            if (window->render_visible != 0u) {
                set_window_render_to_target(window);
            }
            continue;
        }

        if (window->transition_kind == TRANSITION_OPEN) {
            const u32 start_width = window->width * 3u / 5u;
            const u32 start_height = window->height * 3u / 5u;
            const u32 start_x = window->x + (window->width - start_width) / 2u;
            const u32 start_y = window->y + 38u + (window->height - start_height) / 3u;
            window->render_width = lerp_u32(start_width, window->width, window->transition_frame, WINDOW_TRANSITION_FRAMES);
            window->render_height = lerp_u32(start_height, window->height, window->transition_frame, WINDOW_TRANSITION_FRAMES);
            window->render_x = lerp_u32(start_x, window->x, window->transition_frame, WINDOW_TRANSITION_FRAMES);
            window->render_y = lerp_u32(start_y, window->y, window->transition_frame, WINDOW_TRANSITION_FRAMES);
        } else {
            const u32 target_width = window->transition_kind == TRANSITION_MINIMIZE ? window->width / 5u : window->width / 2u;
            const u32 target_height = window->transition_kind == TRANSITION_MINIMIZE ? 28u : window->height / 3u;
            const u32 target_x = window->transition_kind == TRANSITION_MINIMIZE ? window->x + window->width / 3u : window->x + window->width / 4u;
            const u32 target_y = window->transition_kind == TRANSITION_MINIMIZE ? window->y + window->height - 26u : window->y + window->height / 3u;
            window->render_width = lerp_u32(window->width, target_width, window->transition_frame, WINDOW_TRANSITION_FRAMES);
            window->render_height = lerp_u32(window->height, target_height, window->transition_frame, WINDOW_TRANSITION_FRAMES);
            window->render_x = lerp_u32(window->x, target_x, window->transition_frame, WINDOW_TRANSITION_FRAMES);
            window->render_y = lerp_u32(window->y, target_y, window->transition_frame, WINDOW_TRANSITION_FRAMES);
        }

        ++window->transition_frame;
        dirty = 1u;
        if (window->transition_frame > WINDOW_TRANSITION_FRAMES) {
            window->animating = 0u;
            window->transition_kind = TRANSITION_NONE;
            if (window->open != 0u) {
                set_window_render_to_target(window);
                window->render_visible = 1u;
            } else {
                window->render_visible = 0u;
            }
        }
    }

    if (dirty != 0u) {
        state->dirty = 1u;
    }
    return dirty;
}
