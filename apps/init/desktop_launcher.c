#include "desktop_launcher.h"

void launcher_open_desktop_entry(u32 desktop_index, launcher_view_handler_t open_handler) {
    if (open_handler == 0) {
        return;
    }
    open_handler(desktop_entry_view(desktop_index));
}

void launcher_open_start_entry(u32 start_index, launcher_view_handler_t open_handler) {
    if (open_handler == 0) {
        return;
    }
    open_handler(start_entry_view(start_index));
}

void launcher_activate_hub_card(
    u32 card_index,
    launcher_view_predicate_t installed_predicate,
    launcher_view_handler_t install_handler,
    launcher_view_handler_t open_handler
) {
    const shell_view_t view = hub_card_view(card_index);
    if (installed_predicate != 0 && installed_predicate(view) != 0u) {
        if (open_handler != 0) {
            open_handler(view);
        }
        return;
    }
    if (install_handler != 0) {
        install_handler(view);
    }
}
