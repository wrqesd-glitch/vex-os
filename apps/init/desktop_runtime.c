#include "desktop_runtime.h"
#include "desktop_registry.h"

enum {
    INSTALL_FRAME_LIMIT = 10u,
    OPEN_FRAME_LIMIT = 6u
};

static void sync_shell(shell_state_t* shell, const desktop_runtime_state_t* runtime) {
    shell->launch_busy = runtime->busy;
    shell->launch_action = (u32)runtime->action;
    shell->launch_view = runtime->target_view;
    if (runtime->busy == 0u || runtime->frame_limit == 0u) {
        shell->launch_progress = 0u;
        return;
    }
    shell->launch_progress = (runtime->frame * 100u) / runtime->frame_limit;
}

static void clear_runtime(desktop_runtime_state_t* runtime, shell_state_t* shell) {
    runtime->busy = 0u;
    runtime->frame = 0u;
    runtime->frame_limit = 0u;
    runtime->action = DESKTOP_RUNTIME_ACTION_NONE;
    runtime->target_view = VIEW_HUB;
    sync_shell(shell, runtime);
}

void desktop_runtime_init(desktop_runtime_state_t* runtime, shell_state_t* shell) {
    runtime->busy = 0u;
    runtime->frame = 0u;
    runtime->frame_limit = 0u;
    runtime->action = DESKTOP_RUNTIME_ACTION_NONE;
    runtime->target_view = VIEW_HUB;
    sync_shell(shell, runtime);
}

u32 desktop_runtime_request_install(
    desktop_runtime_state_t* runtime,
    shell_state_t* shell,
    shell_view_t view,
    desktop_runtime_installed_fn is_installed
) {
    if (runtime->busy != 0u || is_installed == 0 || is_installed(view) != 0u) {
        return 0u;
    }
    runtime->busy = 1u;
    runtime->frame = 0u;
    runtime->frame_limit = INSTALL_FRAME_LIMIT;
    runtime->action = DESKTOP_RUNTIME_ACTION_INSTALL;
    runtime->target_view = view;
    desktop_registry_begin_install(view);
    shell->focus = FOCUS_WINDOW;
    shell->dirty = 1u;
    sync_shell(shell, runtime);
    return 1u;
}

u32 desktop_runtime_request_open(
    desktop_runtime_state_t* runtime,
    shell_state_t* shell,
    shell_view_t view,
    desktop_runtime_installed_fn is_installed
) {
    if (runtime->busy != 0u || is_installed == 0 || is_installed(view) == 0u) {
        return 0u;
    }
    runtime->busy = 1u;
    runtime->frame = 0u;
    runtime->frame_limit = OPEN_FRAME_LIMIT;
    runtime->action = DESKTOP_RUNTIME_ACTION_OPEN;
    runtime->target_view = view;
    desktop_registry_begin_launch(view);
    shell->focus = FOCUS_WINDOW;
    shell->dirty = 1u;
    sync_shell(shell, runtime);
    return 1u;
}

u32 desktop_runtime_tick(
    desktop_runtime_state_t* runtime,
    shell_state_t* shell,
    desktop_runtime_view_fn install_view,
    desktop_runtime_view_fn open_view
) {
    if (runtime->busy == 0u) {
        return 0u;
    }

    if (runtime->frame < runtime->frame_limit) {
        ++runtime->frame;
    }
    sync_shell(shell, runtime);
    shell->dirty = 1u;

    if (runtime->frame < runtime->frame_limit) {
        return 1u;
    }

    if (runtime->action == DESKTOP_RUNTIME_ACTION_INSTALL && install_view != 0) {
        desktop_registry_complete_install(runtime->target_view);
        install_view(runtime->target_view);
    } else if (runtime->action == DESKTOP_RUNTIME_ACTION_OPEN && open_view != 0) {
        open_view(runtime->target_view);
    }
    clear_runtime(runtime, shell);
    shell->dirty = 1u;
    return 1u;
}
