#include "desktop_input.h"
#include "desktop_launcher.h"

void desktop_input_handle_tab(shell_state_t* state, desktop_window_state_fn any_open_windows) {
    if (state->start_open != 0u) {
        if (state->focus == FOCUS_START) {
            state->focus = FOCUS_TASKBAR;
        } else if (state->focus == FOCUS_TASKBAR) {
            state->focus = any_open_windows() != 0u ? FOCUS_WINDOW : FOCUS_DESKTOP;
        } else if (state->focus == FOCUS_WINDOW && any_open_windows() != 0u) {
            state->focus = FOCUS_DESKTOP;
        } else {
            state->focus = FOCUS_START;
        }
    } else if (any_open_windows() != 0u) {
        if (state->focus == FOCUS_DESKTOP) {
            state->focus = FOCUS_TASKBAR;
        } else if (state->focus == FOCUS_TASKBAR) {
            state->focus = FOCUS_WINDOW;
        } else {
            state->focus = FOCUS_DESKTOP;
        }
    } else {
        state->focus = state->focus == FOCUS_DESKTOP ? FOCUS_TASKBAR : FOCUS_DESKTOP;
    }
    state->dirty = 1u;
}

void desktop_input_handle_arrows(shell_state_t* state, ui_key_t key) {
    if (state->focus == FOCUS_DESKTOP) {
        const u32 count = desktop_entry_count();
        if (key == KEY_UP && state->desktop_index > 0u) {
            state->desktop_index -= 1u;
        } else if (key == KEY_DOWN && state->desktop_index + 1u < count) {
            state->desktop_index += 1u;
        }
    } else if (state->focus == FOCUS_TASKBAR) {
        if (key == KEY_LEFT && state->taskbar_index > 0u) {
            state->taskbar_index -= 1u;
        } else if (key == KEY_RIGHT && state->taskbar_index + 1u < taskbar_entry_count()) {
            state->taskbar_index += 1u;
        }
    } else if (state->focus == FOCUS_START) {
        const u32 count = start_entry_count();
        if (key == KEY_UP && state->start_index > 0u) {
            state->start_index -= 1u;
        } else if (key == KEY_DOWN && state->start_index + 1u < count) {
            state->start_index += 1u;
        }
    }
    state->dirty = 1u;
}

void desktop_input_handle_enter(
    shell_state_t* state,
    shell_window_t* windows,
    u32* order,
    u32 window_count,
    desktop_view_open_fn open_view,
    desktop_windows_close_fn close_windows
) {
    if (state->focus == FOCUS_DESKTOP) {
        launcher_open_desktop_entry(state->desktop_index, open_view);
        return;
    }
    if (state->focus == FOCUS_TASKBAR) {
        desktop_session_activate_taskbar_entry(state, windows, order, window_count, state->taskbar_index, open_view);
        return;
    }
    if (state->focus == FOCUS_START) {
        if (state->start_index + 1u < start_entry_count()) {
            launcher_open_start_entry(state->start_index, open_view);
        } else {
            desktop_session_set_start_open(state, 0u);
            close_windows(state, windows, window_count);
            state->focus = FOCUS_DESKTOP;
            state->dirty = 1u;
        }
        return;
    }
    if (state->focus == FOCUS_WINDOW) {
        state->focus = FOCUS_TASKBAR;
        state->dirty = 1u;
    }
}

void desktop_input_handle_escape(
    shell_state_t* state,
    shell_window_t* windows,
    u32 window_count,
    desktop_window_state_fn any_open_windows
) {
    (void)window_count;
    if (state->start_open != 0u) {
        desktop_session_set_start_open(state, 0u);
        state->focus = FOCUS_TASKBAR;
    } else if (any_open_windows() != 0u) {
        desktop_session_close_active_window(state, windows, window_count);
        state->focus = FOCUS_DESKTOP;
    } else {
        state->focus = FOCUS_DESKTOP;
    }
    state->dirty = 1u;
}

void desktop_input_activate_desktop_entry(shell_state_t* state, u32 index, desktop_view_open_fn open_view) {
    state->desktop_index = index;
    state->focus = FOCUS_DESKTOP;
    launcher_open_desktop_entry(index, open_view);
}

void desktop_input_activate_start_entry(
    shell_state_t* state,
    shell_window_t* windows,
    u32 window_count,
    u32 index,
    desktop_view_open_fn open_view,
    desktop_windows_close_fn close_windows
) {
    state->start_index = index;
    state->focus = FOCUS_START;
    if (index + 1u < start_entry_count()) {
        launcher_open_start_entry(index, open_view);
    } else {
        desktop_session_set_start_open(state, 0u);
        close_windows(state, windows, window_count);
        state->focus = FOCUS_DESKTOP;
        state->dirty = 1u;
    }
}
