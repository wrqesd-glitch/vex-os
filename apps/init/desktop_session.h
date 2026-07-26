#ifndef VEX_INIT_DESKTOP_SESSION_H
#define VEX_INIT_DESKTOP_SESSION_H

#include "desktop_catalog.h"

typedef unsigned int u32;

typedef enum focus_area {
    FOCUS_DESKTOP = 0,
    FOCUS_TASKBAR = 1,
    FOCUS_START = 2,
    FOCUS_WINDOW = 3
} focus_area_t;

typedef struct shell_state {
    shell_view_t active_view;
    shell_view_t launch_view;
    focus_area_t focus;
    u32 start_open;
    u32 start_visible;
    u32 start_animating;
    u32 start_animation_frame;
    u32 start_render_height;
    u32 launch_busy;
    u32 launch_action;
    u32 launch_progress;
    u32 window_open;
    u32 desktop_index;
    u32 hub_index;
    u32 taskbar_index;
    u32 start_index;
    u32 cursor_x;
    u32 cursor_y;
    u32 mouse_left_down;
    u32 window_x;
    u32 window_y;
    u32 window_width;
    u32 window_height;
    u32 dragging_window;
    u32 drag_offset_x;
    u32 drag_offset_y;
    u32 dirty;
} shell_state_t;

typedef struct shell_window {
    u32 open;
    u32 minimized;
    u32 render_visible;
    u32 animating;
    u32 transition_kind;
    u32 transition_frame;
    shell_view_t kind;
    u32 x;
    u32 y;
    u32 width;
    u32 height;
    u32 render_x;
    u32 render_y;
    u32 render_width;
    u32 render_height;
} shell_window_t;

u32 desktop_session_any_open_windows(const shell_window_t* windows, u32 window_count);
u32 desktop_session_any_visible_windows(const shell_window_t* windows, u32 window_count);
void desktop_session_close_all_windows(shell_state_t* state, shell_window_t* windows, u32 window_count);
void desktop_session_sync_active_window_geometry(shell_state_t* state, const shell_window_t* windows);
void desktop_session_commit_active_window_geometry(const shell_state_t* state, shell_window_t* windows);
void desktop_session_bring_window_to_front(u32 slot, u32* order, u32 window_count);
void desktop_session_activate_window_slot(
    shell_state_t* state,
    const shell_window_t* windows,
    u32* order,
    u32 window_count,
    u32 slot
);
void desktop_session_init_windows(
    shell_state_t* state,
    shell_window_t* windows,
    u32* order,
    u32 window_count,
    u32 desktop_width,
    u32 desktop_height
);
void desktop_session_set_start_open(shell_state_t* state, u32 open);
void desktop_session_open_view(
    shell_state_t* state,
    shell_window_t* windows,
    u32* order,
    u32 window_count,
    shell_view_t view
);
void desktop_session_activate_taskbar_entry(
    shell_state_t* state,
    shell_window_t* windows,
    u32* order,
    u32 window_count,
    u32 taskbar_index,
    void (*open_view)(shell_view_t view)
);
void desktop_session_close_active_window(shell_state_t* state, shell_window_t* windows, u32 window_count);
void desktop_session_minimize_active_window(shell_state_t* state, shell_window_t* windows, u32 window_count);
u32 desktop_session_tick(shell_state_t* state, shell_window_t* windows, u32 window_count);

#endif
