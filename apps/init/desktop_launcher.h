#ifndef VEX_INIT_DESKTOP_LAUNCHER_H
#define VEX_INIT_DESKTOP_LAUNCHER_H

#include "desktop_catalog.h"

typedef u32 (*launcher_view_predicate_t)(shell_view_t view);
typedef void (*launcher_view_handler_t)(shell_view_t view);

void launcher_open_desktop_entry(u32 desktop_index, launcher_view_handler_t open_handler);
void launcher_open_start_entry(u32 start_index, launcher_view_handler_t open_handler);
void launcher_activate_hub_card(
    u32 card_index,
    launcher_view_predicate_t installed_predicate,
    launcher_view_handler_t install_handler,
    launcher_view_handler_t open_handler
);

#endif
