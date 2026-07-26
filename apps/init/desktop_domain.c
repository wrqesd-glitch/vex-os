#include "desktop_domain.h"

static desktop_domain_descriptor_t g_domains[6] = {
    {
        .view = VIEW_HUB,
        .package_slot = 0xFFFFFFFFu,
        .package_present = 1u,
        .package_verified = 1u,
        .installed = 1u,
        .builtin = 1u,
        .abi_version = 1u,
        .capability_mask = 0x1Fu,
        .accent_color = 0x00466DFFu,
        .label = "HUB",
        .subtitle = "CATALOG",
        .badge_label = "CORE",
        .manifest_name = "init-shell",
        .manifest_version = "0.1.0",
        .entrypoint = "svc/init-shell",
        .status_line = "READY",
        .capability_line = "LOG IPC EXEC GFX"
    },
    {
        .view = VIEW_DIAGNOSTICS,
        .package_slot = 0u,
        .package_present = 0u,
        .package_verified = 0u,
        .installed = 0u,
        .builtin = 0u,
        .abi_version = 0u,
        .capability_mask = 0u,
        .accent_color = 0x003ED38Au,
        .label = "DIAGNOSTICS",
        .subtitle = "BOOT",
        .badge_label = "BOOT"
    },
    {
        .view = VIEW_TESTS,
        .package_slot = 0xFFFFFFFFu,
        .package_present = 1u,
        .package_verified = 1u,
        .installed = 0u,
        .builtin = 1u,
        .abi_version = 1u,
        .capability_mask = 0x13u,
        .accent_color = 0x00F0B24Eu,
        .label = "TEST CENTER",
        .subtitle = "QA",
        .badge_label = "QA",
        .manifest_name = "tests-shell",
        .manifest_version = "0.1.0",
        .entrypoint = "svc/test-center",
        .status_line = "INSTALLABLE",
        .capability_line = "LOG IPC GFX"
    },
    {
        .view = VIEW_SERVICES,
        .package_slot = 0xFFFFFFFFu,
        .package_present = 1u,
        .package_verified = 1u,
        .installed = 1u,
        .builtin = 1u,
        .abi_version = 1u,
        .capability_mask = 0x17u,
        .accent_color = 0x00FF8752u,
        .label = "EXPLORER",
        .subtitle = "FILES",
        .badge_label = "FS",
        .manifest_name = "explorer-console",
        .manifest_version = "0.1.0",
        .entrypoint = "app/explorer-console",
        .status_line = "READY",
        .capability_line = "LOG IPC LOC GFX"
    },
    {
        .view = VIEW_TERMINAL,
        .package_slot = 0xFFFFFFFFu,
        .package_present = 1u,
        .package_verified = 1u,
        .installed = 0u,
        .builtin = 1u,
        .abi_version = 1u,
        .capability_mask = 0x13u,
        .accent_color = 0x009973FFu,
        .label = "TERMINAL",
        .subtitle = "SHELL",
        .badge_label = "TTY",
        .manifest_name = "terminal-shell",
        .manifest_version = "0.1.0",
        .entrypoint = "svc/terminal",
        .status_line = "INSTALLABLE",
        .capability_line = "LOG IPC GFX"
    },
    {
        .view = VIEW_CONSOLE,
        .package_slot = 0xFFFFFFFFu,
        .package_present = 1u,
        .package_verified = 1u,
        .installed = 1u,
        .builtin = 1u,
        .abi_version = 1u,
        .capability_mask = 0x3u,
        .accent_color = 0x008888FFu,
        .label = "CONSOLE",
        .subtitle = "TERMINAL",
        .badge_label = "",
        .manifest_name = "console",
        .manifest_version = "0.1.0",
        .entrypoint = "app/console",
        .status_line = "READY",
        .capability_line = "LOG IPC"
    }
};

static void copy_string(char* dst, u32 dst_size, const char* src) {
    u32 index = 0u;
    if (dst == 0 || dst_size == 0u) {
        return;
    }
    if (src == 0) {
        dst[0] = 0;
        return;
    }
    while (src[index] != 0 && index + 1u < dst_size) {
        dst[index] = src[index];
        ++index;
    }
    dst[index] = 0;
}

static u32 match_at(const char* text, u64 length, u64 offset, const char* pattern) {
    u64 index = 0u;
    while (pattern[index] != 0) {
        if (offset + index >= length || text[offset + index] != pattern[index]) {
            return 0u;
        }
        ++index;
    }
    return 1u;
}

static int copy_json_string_value(const char* key, const char* text, u64 length, char* out, u32 out_size) {
    for (u64 index = 0; index < length; ++index) {
        if (match_at(text, length, index, key) == 0u) {
            continue;
        }
        u64 cursor = index;
        while (cursor < length && text[cursor] != ':') {
            ++cursor;
        }
        if (cursor >= length) {
            return -1;
        }
        ++cursor;
        while (cursor < length && (text[cursor] == ' ' || text[cursor] == '\n' || text[cursor] == '\r' || text[cursor] == '\t')) {
            ++cursor;
        }
        if (cursor >= length || text[cursor] != '"') {
            return -1;
        }
        ++cursor;
        u32 out_index = 0u;
        while (cursor < length && text[cursor] != '"') {
            if (out_index + 1u >= out_size) {
                return -1;
            }
            out[out_index++] = text[cursor++];
        }
        if (cursor >= length) {
            return -1;
        }
        out[out_index] = 0;
        return 0;
    }
    return -1;
}

static int parse_u32_value(const char* key, const char* text, u64 length, u32* out_value) {
    for (u64 index = 0; index < length; ++index) {
        if (match_at(text, length, index, key) == 0u) {
            continue;
        }
        u64 cursor = index;
        u32 value = 0u;
        u32 found = 0u;
        while (cursor < length && text[cursor] != ':') {
            ++cursor;
        }
        if (cursor >= length) {
            return -1;
        }
        ++cursor;
        while (cursor < length && (text[cursor] == ' ' || text[cursor] == '\n' || text[cursor] == '\r' || text[cursor] == '\t')) {
            ++cursor;
        }
        while (cursor < length && text[cursor] >= '0' && text[cursor] <= '9') {
            found = 1u;
            value = value * 10u + (u32)(text[cursor] - '0');
            ++cursor;
        }
        if (found == 0u) {
            return -1;
        }
        *out_value = value;
        return 0;
    }
    return -1;
}

static u32 json_array_contains(const char* key, const char* text, u64 length, const char* item) {
    for (u64 index = 0; index < length; ++index) {
        if (match_at(text, length, index, key) == 0u) {
            continue;
        }
        for (u64 cursor = index; cursor < length && text[cursor] != ']'; ++cursor) {
            if (match_at(text, length, cursor, item) != 0u) {
                return 1u;
            }
        }
        return 0u;
    }
    return 0u;
}

static void format_capabilities(u64 capability_mask, char* out, u32 out_size) {
    u32 used = 0u;
    out[0] = 0;
    if ((capability_mask & 0x1u) != 0u && used + 4u < out_size) {
        copy_string(out + used, out_size - used, "LOG");
        used += 3u;
    }
    if ((capability_mask & 0x2u) != 0u && used + 5u < out_size) {
        if (used != 0u) {
            out[used++] = ' ';
        }
        copy_string(out + used, out_size - used, "IPC");
        used += 3u;
    }
    if ((capability_mask & 0x4u) != 0u && used + 5u < out_size) {
        if (used != 0u) {
            out[used++] = ' ';
        }
        copy_string(out + used, out_size - used, "LOC");
        used += 3u;
    }
    if ((capability_mask & 0x8u) != 0u && used + 6u < out_size) {
        if (used != 0u) {
            out[used++] = ' ';
        }
        copy_string(out + used, out_size - used, "EXEC");
        used += 4u;
    }
    if ((capability_mask & 0x10u) != 0u && used + 5u < out_size) {
        if (used != 0u) {
            out[used++] = ' ';
        }
        copy_string(out + used, out_size - used, "GFX");
        used += 3u;
    }
    if ((capability_mask & 0x20u) != 0u && used + 5u < out_size) {
        if (used != 0u) {
            out[used++] = ' ';
        }
        copy_string(out + used, out_size - used, "GPU");
        used += 3u;
    }
    out[used] = 0;
}

static void refresh_status_line(desktop_domain_descriptor_t* descriptor) {
    if (descriptor->installed == 0u) {
        copy_string(descriptor->status_line, sizeof(descriptor->status_line), "INSTALLABLE");
    } else if (descriptor->package_present == 0u) {
        copy_string(descriptor->status_line, sizeof(descriptor->status_line), "BUILTIN");
    } else if (descriptor->package_verified == 0u) {
        copy_string(descriptor->status_line, sizeof(descriptor->status_line), "UNVERIFIED");
    } else {
        copy_string(descriptor->status_line, sizeof(descriptor->status_line), "READY");
    }
    format_capabilities(descriptor->capability_mask, descriptor->capability_line, sizeof(descriptor->capability_line));
}

static void parse_package_manifest(const vex_package_image_info_t* image, desktop_domain_descriptor_t* descriptor) {
    const char* manifest = (const char*)(u64)image->manifest_base;
    const u64 length = image->manifest_size;

    if (manifest == 0 || length == 0u) {
        return;
    }
    (void)copy_json_string_value("\"name\"", manifest, length, descriptor->manifest_name, sizeof(descriptor->manifest_name));
    (void)copy_json_string_value("\"version\"", manifest, length, descriptor->manifest_version, sizeof(descriptor->manifest_version));
    (void)copy_json_string_value("\"entrypoint\"", manifest, length, descriptor->entrypoint, sizeof(descriptor->entrypoint));
    (void)parse_u32_value("\"abi_version\"", manifest, length, &descriptor->abi_version);
    descriptor->capability_mask = 0u;
    if (json_array_contains("\"permissions\"", manifest, length, "\"log\"") != 0u) {
        descriptor->capability_mask |= 0x1u;
    }
    if (json_array_contains("\"permissions\"", manifest, length, "\"ipc\"") != 0u) {
        descriptor->capability_mask |= 0x2u;
    }
    if (json_array_contains("\"required_capabilities\"", manifest, length, "\"service.locate\"") != 0u) {
        descriptor->capability_mask |= 0x4u;
    }
    if (json_array_contains("\"required_capabilities\"", manifest, length, "\"channel.open\"") != 0u) {
        descriptor->capability_mask |= 0x8u;
    }
    if (json_array_contains("\"permissions\"", manifest, length, "\"graphics\"") != 0u) {
        descriptor->capability_mask |= 0x10u;
    }
    if (json_array_contains("\"required_capabilities\"", manifest, length, "\"gpu.device\"") != 0u) {
        descriptor->capability_mask |= 0x20u;
    }
}

static void apply_package_slot(
    const vex_boot_info_t* boot_info,
    u32 app_slot,
    shell_view_t view,
    const char* fallback_name,
    const char* fallback_version,
    const char* fallback_entrypoint
) {
    desktop_domain_descriptor_t* descriptor;
    const vex_package_image_info_t* image;

    if (boot_info->app_image_count <= app_slot) {
        return;
    }

    descriptor = &g_domains[(u32)view];
    image = &boot_info->app_images[app_slot];
    descriptor->package_slot = app_slot;
    descriptor->package_present = image->package_base != 0u && image->package_size != 0u;
    descriptor->package_verified = image->verified;
    parse_package_manifest(image, descriptor);
    if (descriptor->manifest_name[0] == 0) {
        copy_string(descriptor->manifest_name, sizeof(descriptor->manifest_name), fallback_name);
    }
    if (descriptor->manifest_version[0] == 0) {
        copy_string(descriptor->manifest_version, sizeof(descriptor->manifest_version), fallback_version);
    }
    if (descriptor->entrypoint[0] == 0) {
        copy_string(descriptor->entrypoint, sizeof(descriptor->entrypoint), fallback_entrypoint);
    }
    refresh_status_line(descriptor);
}

void desktop_domain_init(const vex_boot_info_t* boot_info) {
    for (u32 index = 0; index < 6u; ++index) {
        refresh_status_line(&g_domains[index]);
    }
    if (boot_info == 0u) {
        return;
    }

    apply_package_slot(boot_info, 0u, VIEW_DIAGNOSTICS, "diagnostics", "0.1.0", "app/diagnostics");
    refresh_status_line(&g_domains[(u32)VIEW_DIAGNOSTICS]);

    apply_package_slot(boot_info, 1u, VIEW_TESTS, "test-center", "0.1.0", "app/test-center");
    apply_package_slot(boot_info, 2u, VIEW_SERVICES, "explorer-console", "0.1.0", "app/explorer-console");
    apply_package_slot(boot_info, 3u, VIEW_TERMINAL, "terminal-console", "0.1.0", "app/terminal-console");
}

const desktop_domain_descriptor_t* desktop_domain_descriptor(shell_view_t view) {
    return &g_domains[(u32)view];
}

u32 desktop_domain_is_installed(shell_view_t view) {
    return g_domains[(u32)view].installed;
}

u32 desktop_domain_is_installable(shell_view_t view) {
    const desktop_domain_descriptor_t* descriptor = &g_domains[(u32)view];
    if (descriptor->installed != 0u) {
        return 0u;
    }
    if (descriptor->builtin != 0u) {
        return 1u;
    }
    return descriptor->package_present != 0u &&
           descriptor->package_verified != 0u &&
           descriptor->abi_version == 1u &&
           descriptor->entrypoint[0] != 0;
}

void desktop_domain_install(shell_view_t view) {
    g_domains[(u32)view].installed = 1u;
    refresh_status_line(&g_domains[(u32)view]);
}

u32 desktop_domain_desktop_entry_count(void) {
    u32 count = 0u;
    for (u32 slot = 0; slot < 6u; ++slot) {
        if (g_domains[slot].installed != 0u) {
            ++count;
        }
    }
    return count;
}

shell_view_t desktop_domain_desktop_entry_view(u32 desktop_index) {
    u32 current = 0u;
    for (u32 slot = 0; slot < 6u; ++slot) {
        if (g_domains[slot].installed == 0u) {
            continue;
        }
        if (current == desktop_index) {
            return (shell_view_t)slot;
        }
        ++current;
    }
    return VIEW_HUB;
}

u32 desktop_domain_start_entry_count(void) {
    return desktop_domain_desktop_entry_count() + 1u;
}

shell_view_t desktop_domain_start_entry_view(u32 start_index) {
    return desktop_domain_desktop_entry_view(start_index);
}

u32 desktop_domain_taskbar_entry_count(void) {
    return desktop_domain_desktop_entry_count() + 1u;
}

shell_view_t desktop_domain_taskbar_entry_view(u32 taskbar_index) {
    if (taskbar_index == 0u) {
        return VIEW_HUB;
    }
    return desktop_domain_desktop_entry_view(taskbar_index - 1u);
}

u32 desktop_domain_taskbar_index_for_view(shell_view_t view) {
    for (u32 index = 1u; index < desktop_domain_taskbar_entry_count(); ++index) {
        if (desktop_domain_taskbar_entry_view(index) == view) {
            return index;
        }
    }
    return 0u;
}
