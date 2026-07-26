#include "desktop_catalog.h"
#include "desktop_domain.h"
#include "desktop_registry.h"

u32 view_is_installed(shell_view_t view) {
    return desktop_registry_is_installed(view);
}

u32 view_is_installable(shell_view_t view) {
    return desktop_domain_is_installable(view);
}

u32 view_is_pinned(shell_view_t view) {
    return desktop_registry_is_pinned(view);
}

desktop_app_lifecycle_t view_lifecycle(shell_view_t view) {
    return desktop_registry_lifecycle(view);
}

const char* view_lifecycle_label(shell_view_t view) {
    return desktop_registry_lifecycle_label(view);
}

const char* view_label(shell_view_t view) {
    return desktop_domain_descriptor(view)->label;
}

const char* view_subtitle(shell_view_t view) {
    return desktop_domain_descriptor(view)->subtitle;
}

void install_view(shell_view_t view) {
    desktop_registry_install(view);
}

void pin_view(shell_view_t view) {
    desktop_registry_pin(view);
}

u32 desktop_entry_count(void) {
    return desktop_registry_desktop_entry_count();
}

shell_view_t desktop_entry_view(u32 desktop_index) {
    return desktop_registry_desktop_entry_view(desktop_index);
}

u32 start_entry_count(void) {
    return desktop_registry_start_entry_count();
}

shell_view_t start_entry_view(u32 start_index) {
    return desktop_registry_start_entry_view(start_index);
}

u32 taskbar_entry_count(void) {
    return desktop_registry_taskbar_entry_count();
}

shell_view_t taskbar_entry_view(u32 taskbar_index) {
    return desktop_registry_taskbar_entry_view(taskbar_index);
}

u32 taskbar_index_for_view(shell_view_t view) {
    return desktop_registry_taskbar_index_for_view(view);
}

u32 hub_card_count(void) {
    return 5u;
}

shell_view_t hub_card_view(u32 card_index) {
    switch (card_index) {
    case 0u: return VIEW_DIAGNOSTICS;
    case 1u: return VIEW_TESTS;
    case 2u: return VIEW_SERVICES;
    case 3u: return VIEW_TERMINAL;
    default: return VIEW_CONSOLE;
    }
}

u32 installed_view_count(void) {
    return desktop_registry_installed_count();
}

u32 pinned_view_count(void) {
    return desktop_registry_pinned_count();
}
