#ifndef VEX_INIT_DESKTOP_CATALOG_H
#define VEX_INIT_DESKTOP_CATALOG_H

typedef unsigned int u32;

/* Extend with new view types. Each view needs:
 *  - domain descriptor in desktop_domain.c
 *  - runtime state + update handler in desktop_app_runtime.c
 *  - render function in desktop_views.c
 *  - keyboard handler in desktop_app_runtime_handle_key()
 *  - catalog/registry entries for install/pin/lifecycle
 */

typedef enum shell_view {
    VIEW_HUB = 0,
    VIEW_DIAGNOSTICS = 1,
    VIEW_TESTS = 2,
    VIEW_SERVICES = 3,
    VIEW_TERMINAL = 4,
    VIEW_CONSOLE = 5
} shell_view_t;

typedef enum desktop_app_lifecycle {
    DESKTOP_APP_IDLE = 0,
    DESKTOP_APP_INSTALLING = 1,
    DESKTOP_APP_PINNED = 2,
    DESKTOP_APP_OPEN = 3,
    DESKTOP_APP_LAUNCHING = 4,
    DESKTOP_APP_ACTIVE = 5
} desktop_app_lifecycle_t;

u32 view_is_installed(shell_view_t view);
u32 view_is_installable(shell_view_t view);
u32 view_is_pinned(shell_view_t view);
desktop_app_lifecycle_t view_lifecycle(shell_view_t view);
const char* view_lifecycle_label(shell_view_t view);
const char* view_label(shell_view_t view);
const char* view_subtitle(shell_view_t view);
void install_view(shell_view_t view);
void pin_view(shell_view_t view);
u32 desktop_entry_count(void);
shell_view_t desktop_entry_view(u32 desktop_index);
u32 start_entry_count(void);
shell_view_t start_entry_view(u32 start_index);
u32 taskbar_entry_count(void);
shell_view_t taskbar_entry_view(u32 taskbar_index);
u32 taskbar_index_for_view(shell_view_t view);
u32 hub_card_count(void);
shell_view_t hub_card_view(u32 card_index);
u32 installed_view_count(void);
u32 pinned_view_count(void);

#endif
