#ifndef VEX_INIT_DESKTOP_INPUT_H
#define VEX_INIT_DESKTOP_INPUT_H

#include "desktop_session.h"

typedef enum ui_key {
    KEY_NONE = 0,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_TAB,
    KEY_ENTER,
    KEY_ESCAPE,
    KEY_BACKSPACE
} ui_key_t;

typedef u32 (*desktop_window_state_fn)(void);
typedef void (*desktop_view_open_fn)(shell_view_t view);
typedef void (*desktop_windows_close_fn)(shell_state_t* state, shell_window_t* windows, u32 window_count);

void desktop_input_handle_tab(shell_state_t* state, desktop_window_state_fn any_open_windows);
void desktop_input_handle_arrows(shell_state_t* state, ui_key_t key);
void desktop_input_handle_enter(
    shell_state_t* state,
    shell_window_t* windows,
    u32* order,
    u32 window_count,
    desktop_view_open_fn open_view,
    desktop_windows_close_fn close_windows
);
void desktop_input_handle_escape(
    shell_state_t* state,
    shell_window_t* windows,
    u32 window_count,
    desktop_window_state_fn any_open_windows
);
void desktop_input_activate_desktop_entry(shell_state_t* state, u32 index, desktop_view_open_fn open_view);
void desktop_input_activate_start_entry(
    shell_state_t* state,
    shell_window_t* windows,
    u32 window_count,
    u32 index,
    desktop_view_open_fn open_view,
    desktop_windows_close_fn close_windows
);

#endif
