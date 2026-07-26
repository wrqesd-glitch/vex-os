#ifndef VEX_INIT_DESKTOP_REGISTRY_H
#define VEX_INIT_DESKTOP_REGISTRY_H

#include "desktop_catalog.h"

typedef unsigned int u32;

void desktop_registry_init(void);
u32 desktop_registry_is_installed(shell_view_t view);
u32 desktop_registry_is_pinned(shell_view_t view);
void desktop_registry_install(shell_view_t view);
void desktop_registry_pin(shell_view_t view);
void desktop_registry_begin_install(shell_view_t view);
void desktop_registry_complete_install(shell_view_t view);
void desktop_registry_begin_launch(shell_view_t view);
void desktop_registry_sync_window(shell_view_t view, u32 open, u32 active);
desktop_app_lifecycle_t desktop_registry_lifecycle(shell_view_t view);
const char* desktop_registry_lifecycle_label(shell_view_t view);
u32 desktop_registry_desktop_entry_count(void);
shell_view_t desktop_registry_desktop_entry_view(u32 desktop_index);
u32 desktop_registry_start_entry_count(void);
shell_view_t desktop_registry_start_entry_view(u32 start_index);
u32 desktop_registry_taskbar_entry_count(void);
shell_view_t desktop_registry_taskbar_entry_view(u32 taskbar_index);
u32 desktop_registry_taskbar_index_for_view(shell_view_t view);
u32 desktop_registry_installed_count(void);
u32 desktop_registry_pinned_count(void);

#endif
