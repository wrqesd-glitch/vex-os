#include "desktop_registry.h"
#include "desktop_domain.h"
#include "desktop_app_runtime.h"

static u32 g_installed[VIEW_COUNT];
static u32 g_desktop_shortcut[VIEW_COUNT];
static u32 g_pinned[VIEW_COUNT];
static desktop_app_lifecycle_t g_lifecycle[VIEW_COUNT];
static shell_view_t g_shortcut_order[VIEW_COUNT] = {
    VIEW_HUB,
    VIEW_DIAGNOSTICS,
    VIEW_TESTS,
    VIEW_SERVICES,
    VIEW_TERMINAL,
    VIEW_CONSOLE
};

static desktop_app_lifecycle_t resting_lifecycle(shell_view_t view) {
    if (g_installed[(u32)view] == 0u) {
        return DESKTOP_APP_IDLE;
    }
    if (g_pinned[(u32)view] != 0u) {
        return DESKTOP_APP_PINNED;
    }
    return DESKTOP_APP_IDLE;
}

void desktop_registry_init(void) {
    for (u32 index = 0; index < VIEW_COUNT; ++index) {
        g_installed[index] = desktop_domain_is_installed((shell_view_t)index);
        g_desktop_shortcut[index] = 0u;
        g_pinned[index] = 0u;
        g_lifecycle[index] = DESKTOP_APP_IDLE;
    }
    g_desktop_shortcut[(u32)VIEW_HUB] = 1u;
    g_pinned[(u32)VIEW_HUB] = 1u;
    desktop_registry_install(VIEW_SERVICES);
    desktop_registry_pin(VIEW_SERVICES);
    g_desktop_shortcut[(u32)VIEW_SERVICES] = 1u;
    desktop_registry_install(VIEW_CONSOLE);
    desktop_registry_pin(VIEW_CONSOLE);
    g_desktop_shortcut[(u32)VIEW_CONSOLE] = 1u;
    for (u32 index = 0; index < VIEW_COUNT; ++index) {
        g_lifecycle[index] = resting_lifecycle((shell_view_t)index);
    }
}

u32 desktop_registry_is_installed(shell_view_t view) {
    return g_installed[(u32)view];
}

u32 desktop_registry_is_pinned(shell_view_t view) {
    return g_pinned[(u32)view];
}

void desktop_registry_install(shell_view_t view) {
    g_installed[(u32)view] = 1u;
    g_desktop_shortcut[(u32)view] = 1u;
    desktop_domain_install(view);
    g_lifecycle[(u32)view] = resting_lifecycle(view);
}

void desktop_registry_pin(shell_view_t view) {
    if (g_installed[(u32)view] == 0u) {
        return;
    }
    g_pinned[(u32)view] = 1u;
    if (g_lifecycle[(u32)view] != DESKTOP_APP_INSTALLING &&
        g_lifecycle[(u32)view] != DESKTOP_APP_LAUNCHING &&
        g_lifecycle[(u32)view] != DESKTOP_APP_ACTIVE) {
        g_lifecycle[(u32)view] = DESKTOP_APP_PINNED;
    }
}

void desktop_registry_begin_install(shell_view_t view) {
    if (g_installed[(u32)view] != 0u) {
        return;
    }
    g_lifecycle[(u32)view] = DESKTOP_APP_INSTALLING;
}

void desktop_registry_complete_install(shell_view_t view) {
    desktop_registry_install(view);
}

void desktop_registry_begin_launch(shell_view_t view) {
    if (g_installed[(u32)view] == 0u) {
        return;
    }
    g_lifecycle[(u32)view] = DESKTOP_APP_LAUNCHING;
}

void desktop_registry_sync_window(shell_view_t view, u32 open, u32 active) {
    if (g_installed[(u32)view] == 0u) {
        return;
    }

    if (open != 0u) {
        g_lifecycle[(u32)view] = active != 0u ? DESKTOP_APP_ACTIVE : DESKTOP_APP_OPEN;
        return;
    }

    if (g_lifecycle[(u32)view] == DESKTOP_APP_INSTALLING ||
        g_lifecycle[(u32)view] == DESKTOP_APP_LAUNCHING) {
        return;
    }
    g_lifecycle[(u32)view] = resting_lifecycle(view);
}

desktop_app_lifecycle_t desktop_registry_lifecycle(shell_view_t view) {
    return g_lifecycle[(u32)view];
}

const char* desktop_registry_lifecycle_label(shell_view_t view) {
    switch (g_lifecycle[(u32)view]) {
    case DESKTOP_APP_INSTALLING:
        return "INSTALLING";
    case DESKTOP_APP_PINNED:
        return "PINNED";
    case DESKTOP_APP_OPEN:
        return "OPEN";
    case DESKTOP_APP_LAUNCHING:
        return "LAUNCHING";
    case DESKTOP_APP_ACTIVE:
        return "ACTIVE";
    default:
        return g_installed[(u32)view] != 0u ? "READY" : "IDLE";
    }
}

static u32 count_shortcuts(void) {
    u32 count = 0u;
    for (u32 index = 0; index < VIEW_COUNT; ++index) {
        const shell_view_t view = g_shortcut_order[index];
        if (g_installed[(u32)view] != 0u && g_desktop_shortcut[(u32)view] != 0u) {
            ++count;
        }
    }
    return count;
}

static shell_view_t shortcut_at(u32 wanted) {
    u32 current = 0u;
    for (u32 index = 0; index < VIEW_COUNT; ++index) {
        const shell_view_t view = g_shortcut_order[index];
        if (g_installed[(u32)view] == 0u || g_desktop_shortcut[(u32)view] == 0u) {
            continue;
        }
        if (current == wanted) {
            return view;
        }
        ++current;
    }
    return VIEW_HUB;
}

u32 desktop_registry_desktop_entry_count(void) {
    return count_shortcuts();
}

shell_view_t desktop_registry_desktop_entry_view(u32 desktop_index) {
    return shortcut_at(desktop_index);
}

u32 desktop_registry_start_entry_count(void) {
    return count_shortcuts() + 1u;
}

shell_view_t desktop_registry_start_entry_view(u32 start_index) {
    return shortcut_at(start_index);
}

u32 desktop_registry_taskbar_entry_count(void) {
    u32 count = 1u;
    for (u32 index = 0; index < VIEW_COUNT; ++index) {
        const shell_view_t view = g_shortcut_order[index];
        if (view == VIEW_HUB || g_installed[index] == 0u) {
            continue;
        }
        if (g_pinned[index] != 0u ||
            g_lifecycle[index] == DESKTOP_APP_OPEN ||
            g_lifecycle[index] == DESKTOP_APP_LAUNCHING ||
            g_lifecycle[index] == DESKTOP_APP_ACTIVE) {
            ++count;
        }
    }
    return count;
}

shell_view_t desktop_registry_taskbar_entry_view(u32 taskbar_index) {
    if (taskbar_index == 0u) {
        return VIEW_HUB;
    }
    {
        u32 current = 1u;
        for (u32 index = 0; index < VIEW_COUNT; ++index) {
            const shell_view_t view = g_shortcut_order[index];
            if (view == VIEW_HUB || g_installed[index] == 0u) {
                continue;
            }
            if (g_pinned[index] == 0u &&
                g_lifecycle[index] != DESKTOP_APP_OPEN &&
                g_lifecycle[index] != DESKTOP_APP_LAUNCHING &&
                g_lifecycle[index] != DESKTOP_APP_ACTIVE) {
                continue;
            }
            if (current == taskbar_index) {
                return view;
            }
            ++current;
        }
    }
    return VIEW_HUB;
}

u32 desktop_registry_taskbar_index_for_view(shell_view_t view) {
    for (u32 index = 1u; index < desktop_registry_taskbar_entry_count(); ++index) {
        if (desktop_registry_taskbar_entry_view(index) == view) {
            return index;
        }
    }
    return 0u;
}

u32 desktop_registry_installed_count(void) {
    u32 count = 0u;
    for (u32 index = 0; index < VIEW_COUNT; ++index) {
        count += g_installed[index] != 0u ? 1u : 0u;
    }
    return count;
}

u32 desktop_registry_pinned_count(void) {
    u32 count = 0u;
    for (u32 index = 0; index < VIEW_COUNT; ++index) {
        if (g_installed[index] != 0u && g_pinned[index] != 0u) {
            ++count;
        }
    }
    return count;
}
