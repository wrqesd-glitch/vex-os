#ifndef VEX_INIT_DESKTOP_RUNTIME_H
#define VEX_INIT_DESKTOP_RUNTIME_H

#include "desktop_session.h"

typedef enum desktop_runtime_action {
    DESKTOP_RUNTIME_ACTION_NONE = 0,
    DESKTOP_RUNTIME_ACTION_INSTALL = 1,
    DESKTOP_RUNTIME_ACTION_OPEN = 2
} desktop_runtime_action_t;

typedef struct desktop_runtime_state {
    u32 busy;
    u32 frame;
    u32 frame_limit;
    desktop_runtime_action_t action;
    shell_view_t target_view;
} desktop_runtime_state_t;

typedef u32 (*desktop_runtime_installed_fn)(shell_view_t view);
typedef void (*desktop_runtime_view_fn)(shell_view_t view);

void desktop_runtime_init(desktop_runtime_state_t* runtime, shell_state_t* shell);
u32 desktop_runtime_request_install(
    desktop_runtime_state_t* runtime,
    shell_state_t* shell,
    shell_view_t view,
    desktop_runtime_installed_fn is_installed
);
u32 desktop_runtime_request_open(
    desktop_runtime_state_t* runtime,
    shell_state_t* shell,
    shell_view_t view,
    desktop_runtime_installed_fn is_installed
);
u32 desktop_runtime_tick(
    desktop_runtime_state_t* runtime,
    shell_state_t* shell,
    desktop_runtime_view_fn install_view,
    desktop_runtime_view_fn open_view
);

#endif
