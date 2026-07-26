#ifndef VEX_INIT_DESKTOP_DOMAIN_H
#define VEX_INIT_DESKTOP_DOMAIN_H

#include "desktop_catalog.h"
#include "vex_boot_info.h"

typedef struct desktop_domain_descriptor {
    shell_view_t view;
    u32 package_slot;
    u32 package_present;
    u32 package_verified;
    u32 installed;
    u32 builtin;
    u32 abi_version;
    u64 capability_mask;
    u32 accent_color;
    const char* label;
    const char* subtitle;
    const char* badge_label;
    char manifest_name[32];
    char manifest_version[16];
    char entrypoint[64];
    char status_line[24];
    char capability_line[24];
} desktop_domain_descriptor_t;

void desktop_domain_init(const vex_boot_info_t* boot_info);
const desktop_domain_descriptor_t* desktop_domain_descriptor(shell_view_t view);
u32 desktop_domain_is_installed(shell_view_t view);
u32 desktop_domain_is_installable(shell_view_t view);
void desktop_domain_install(shell_view_t view);
u32 desktop_domain_desktop_entry_count(void);
shell_view_t desktop_domain_desktop_entry_view(u32 desktop_index);
u32 desktop_domain_start_entry_count(void);
shell_view_t desktop_domain_start_entry_view(u32 start_index);
u32 desktop_domain_taskbar_entry_count(void);
shell_view_t desktop_domain_taskbar_entry_view(u32 taskbar_index);
u32 desktop_domain_taskbar_index_for_view(shell_view_t view);

#endif
