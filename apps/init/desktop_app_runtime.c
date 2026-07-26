/* To add a new view:
 *   1. add value to shell_view_t enum in desktop_session.h
 *   2. add descriptor to g_domains[] in desktop_domain.c
 *   3. add state fields + update_<view>() in desktop_app_runtime.c
 *   4. add render_*() in desktop_views.c
 *   5. add key handling in desktop_app_runtime_handle_key()
 *   6. register in desktop_catalog.c and desktop_registry.c
 */

#include "desktop_app_runtime.h"
#include "vex_gpu_proto.h"

enum {
    APP_STATE_COUNT = 6u,
    APP_REFRESH_INTERVAL = 32u,
    TEST_CHECK_COUNT = 6u,
    TEST_ITEM_COUNT = 10u,
    TEST_VISIBLE_ROWS = 6u,
    TEST_PANIC_ARM_TICKS = 96u
};

static desktop_app_state_t g_app_states[APP_STATE_COUNT];
static u32 g_next_session_id = 1u;
static const vex_boot_info_t* g_boot_info = 0;

static void update_terminal_lines(desktop_app_state_t* state);

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

static void append_string(char* dst, u32 dst_size, const char* src) {
    u32 index = 0u;
    while (index + 1u < dst_size && dst[index] != 0) {
        ++index;
    }
    if (index + 1u >= dst_size) {
        return;
    }
    copy_string(dst + index, dst_size - index, src);
}

static void append_u32(char* dst, u32 dst_size, u32 value) {
    char digits[16];
    u32 count = 0u;

    if (value == 0u) {
        append_string(dst, dst_size, "0");
        return;
    }
    while (value > 0u && count < sizeof(digits)) {
        digits[count++] = (char)('0' + (value % 10u));
        value /= 10u;
    }
    while (count > 0u) {
        char text[2];
        text[0] = digits[count - 1u];
        text[1] = 0;
        append_string(dst, dst_size, text);
        --count;
    }
}

static u32 string_equals(const char* left, const char* right) {
    u32 index = 0u;
    while (left[index] != 0 || right[index] != 0) {
        if (left[index] != right[index]) {
            return 0u;
        }
        ++index;
    }
    return 1u;
}

static u32 string_starts_with(const char* text, const char* prefix) {
    u32 index = 0u;
    while (prefix[index] != 0) {
        if (text[index] != prefix[index]) {
            return 0u;
        }
        ++index;
    }
    return 1u;
}

static u32 string_length(const char* text) {
    u32 length = 0u;
    while (text[length] != 0) {
        ++length;
    }
    return length;
}

static int string_compare(const char* left, const char* right) {
    u32 index = 0u;
    while (left[index] != 0 && right[index] != 0) {
        if (left[index] != right[index]) {
            return (int)(u8)left[index] - (int)(u8)right[index];
        }
        ++index;
    }
    if (left[index] == 0 && right[index] == 0) {
        return 0;
    }
    return left[index] == 0 ? -1 : 1;
}

static void clear_state(desktop_app_state_t* state) {
    state->running = 0u;
    state->session_id = 0u;
    state->launch_count = 0u;
    state->uptime_ticks = 0u;
    state->heartbeat = 0u;
    state->meter_primary = 0u;
    state->meter_secondary = 0u;
    state->meter_tertiary = 0u;
    state->refresh_divider = 0u;
    state->terminal_cursor = 0u;
    state->terminal_history_count = 0u;
    state->browser_cursor = 0u;
    state->diagnostics_page = 0u;
    state->test_cursor = 0u;
    state->test_run_count = 0u;
    state->test_pass_count = 0u;
    state->test_armed = 0u;
    state->test_armed_ticks = 0u;
    state->test_demo_active = 0u;
    state->test_demo_ticks = 0u;
    state->gpu_suspend_requested = 0u;
    state->gpu_suspend_sequence = 0u;
    state->gpu_suspend_applied_sequence = 0u;
    state->headline[0] = 0;
    state->detail[0] = 0;
    state->terminal_input[0] = 0;
    state->browser_prefix[0] = 0;
    state->explorer_selected_index = 0u;
    state->explorer_left_selected = 0u;
    state->explorer_focus_left = 0u;
    state->explorer_current_path[0] = 0u;
    for (u32 index = 0; index < 6u; ++index) {
        state->lines[index][0] = 0;
    }
    for (u32 index = 0; index < TEST_VISIBLE_ROWS; ++index) {
        state->test_list[index][0] = 0;
    }
    for (u32 index = 0; index < 2u; ++index) {
        state->test_result[index][0] = 0;
    }
    for (u32 index = 0; index < 8u; ++index) {
        state->terminal_history[index][0] = 0;
    }
}

static void set_line(desktop_app_state_t* state, u32 index, const char* text) {
    if (index >= 6u) {
        return;
    }
    copy_string(state->lines[index], sizeof(state->lines[index]), text);
}

static void start_state(desktop_app_state_t* state, const desktop_domain_descriptor_t* descriptor) {
    state->running = 1u;
    state->session_id = g_next_session_id++;
    state->launch_count += 1u;
    state->uptime_ticks = 0u;
    state->heartbeat = 0u;
    state->meter_primary = 0u;
    state->meter_secondary = 0u;
    state->meter_tertiary = 0u;
    state->refresh_divider = 0u;
    state->terminal_cursor = 0u;
    state->terminal_history_count = 0u;
    state->browser_cursor = 0u;
    state->diagnostics_page = 0u;
    state->test_cursor = 0u;
    state->test_run_count = 0u;
    state->test_pass_count = 0u;
    state->test_armed = 0u;
    state->test_armed_ticks = 0u;
    state->test_demo_active = 0u;
    state->test_demo_ticks = 0u;
    state->gpu_suspend_requested = 0u;
    state->gpu_suspend_sequence = 0u;
    state->gpu_suspend_applied_sequence = 0u;
    state->terminal_input[0] = 0;
    state->browser_prefix[0] = 0;
    state->explorer_selected_index = 0u;
    state->explorer_left_selected = 0u;
    state->explorer_focus_left = 0u;
    copy_string(state->explorer_current_path, sizeof(state->explorer_current_path), "/");
    copy_string(state->headline, sizeof(state->headline), descriptor->label);
    copy_string(state->detail, sizeof(state->detail), descriptor->entrypoint);
    for (u32 index = 0; index < 6u; ++index) {
        state->lines[index][0] = 0;
    }
    for (u32 index = 0; index < TEST_VISIBLE_ROWS; ++index) {
        state->test_list[index][0] = 0;
    }
    for (u32 index = 0; index < 2u; ++index) {
        state->test_result[index][0] = 0;
    }
    for (u32 index = 0; index < 8u; ++index) {
        state->terminal_history[index][0] = 0;
    }
}

static void push_terminal_history(desktop_app_state_t* state, const char* text) {
    if (state->terminal_history_count < 8u) {
        copy_string(state->terminal_history[state->terminal_history_count], sizeof(state->terminal_history[0]), text);
        state->terminal_history_count += 1u;
        return;
    }
    for (u32 index = 1u; index < 8u; ++index) {
        copy_string(state->terminal_history[index - 1u], sizeof(state->terminal_history[0]), state->terminal_history[index]);
    }
    copy_string(state->terminal_history[7], sizeof(state->terminal_history[0]), text);
}

static u32 boot_file_count(const vex_boot_info_t* boot_info) {
    return boot_info != 0 ? boot_info->boot_file_count : 0u;
}

static const vex_boot_file_entry_t* boot_file_at(const vex_boot_info_t* boot_info, u32 index) {
    if (boot_info == 0 || index >= boot_info->boot_file_count) {
        return 0;
    }
    return &boot_info->boot_files[index];
}

static u32 verified_package_count(const vex_boot_info_t* boot_info) {
    u32 count = 0u;
    const u32 total = boot_file_count(boot_info);
    for (u32 index = 0u; index < total; ++index) {
        const vex_boot_file_entry_t* entry = boot_file_at(boot_info, index);
        if (entry != 0 &&
            (entry->flags & VEX_BOOT_FILE_PACKAGE) != 0u &&
            (entry->flags & VEX_BOOT_FILE_VERIFIED) != 0u) {
            ++count;
        }
    }
    return count;
}

static u32 app_domain_count(const vex_boot_info_t* boot_info) {
    return boot_info != 0 ? boot_info->app_image_count + 1u : 0u;
}

static u32 shared_surface_count(const vex_boot_info_t* boot_info) {
    u32 count = 0u;
    if (boot_info == 0) {
        return 0u;
    }
    for (u32 index = 0u; index < VEX_MAX_SHARED_SURFACES; ++index) {
        const vex_shared_surface_info_t* surface = &boot_info->shared_surfaces[index];
        if (surface->surface_handle != 0u &&
            surface->shared_mapping_base != 0u &&
            surface->buffer_count >= 3u &&
            surface->bytes_per_buffer != 0u) {
            ++count;
        }
    }
    return count;
}

static u32 verified_app_image_count(const vex_boot_info_t* boot_info) {
    u32 count = 0u;
    if (boot_info == 0) {
        return 0u;
    }
    for (u32 index = 0u; index < boot_info->app_image_count && index < VEX_MAX_APP_IMAGES; ++index) {
        count += boot_info->app_images[index].verified != 0u ? 1u : 0u;
    }
    return count;
}

static u32 graphics_ready(const vex_boot_info_t* boot_info) {
    if (boot_info == 0) {
        return 0u;
    }
    return boot_info->framebuffer.base != 0u &&
           boot_info->framebuffer.width != 0u &&
           boot_info->framebuffer.height != 0u &&
           boot_info->framebuffer.pixels_per_scanline >= boot_info->framebuffer.width;
}

static u32 boot_revision_ready(const vex_boot_info_t* boot_info) {
    return boot_info != 0 && boot_info->revision >= 4u;
}

static u32 compositor_route_ready(const vex_boot_info_t* boot_info) {
    if (boot_info == 0) {
        return 0u;
    }
    return boot_info->compositor_scene_mailbox_base != 0u &&
           boot_info->init_gpu_mailbox_base != 0u &&
           boot_info->compositor_gpu_mailbox_base != 0u;
}

static u32 current_prefix_depth(const char* prefix) {
    u32 depth = 0u;
    u32 index = 0u;
    if (prefix[0] == 0) {
        return 0u;
    }
    depth = 1u;
    while (prefix[index] != 0) {
        if (prefix[index] == '/') {
            ++depth;
        }
        ++index;
    }
    return depth;
}

static const char* path_leaf_name(const char* path) {
    const char* leaf = path;
    while (*path != 0) {
        if (*path == '/') {
            leaf = path + 1;
        }
        ++path;
    }
    return leaf;
}

static u32 explorer_child_match(
    const vex_boot_file_entry_t* entry,
    const char* dir_path
) {
    u32 dir_length;
    if (entry == 0 || entry->depth == 0u) {
        return 0u;
    }
    if (dir_path[0] == '/' && dir_path[1] == 0) {
        return entry->depth == 1u;
    }
    dir_length = string_length(dir_path);
    if (string_starts_with(entry->path, dir_path) == 0u) {
        return 0u;
    }
    if (entry->path[dir_length] != '/') {
        return 0u;
    }
    return entry->depth == current_prefix_depth(dir_path) + 1u;
}

static u32 explorer_child_count(
    const vex_boot_info_t* boot_info,
    const char* dir_path
) {
    u32 count = 0u;
    const u32 total = boot_file_count(boot_info);
    for (u32 index = 0u; index < total; ++index) {
        const vex_boot_file_entry_t* entry = boot_file_at(boot_info, index);
        if (explorer_child_match(entry, dir_path) != 0u) {
            ++count;
        }
    }
    return count;
}

static const vex_boot_file_entry_t* explorer_child_at(
    const vex_boot_info_t* boot_info,
    const char* dir_path,
    u32 wanted
) {
    u32 current = 0u;
    const u32 total = boot_file_count(boot_info);
    for (u32 index = 0u; index < total; ++index) {
        const vex_boot_file_entry_t* entry = boot_file_at(boot_info, index);
        if (explorer_child_match(entry, dir_path) == 0u) {
            continue;
        }
        if (current == wanted) {
            return entry;
        }
        ++current;
    }
    return 0;
}

static void explorer_root_dir_list(
    const vex_boot_info_t* boot_info,
    char roots[][VEX_EXPLORER_PATH_MAX],
    u32* count
) {
    u32 n = 0u;
    const u32 total = boot_file_count(boot_info);
    if (count == 0) {
        return;
    }
    for (u32 index = 0u; index < total && n < VEX_EXPLORER_ROOT_MAX; ++index) {
        const vex_boot_file_entry_t* entry = boot_file_at(boot_info, index);
        if (entry == 0 || entry->depth != 1u) {
            continue;
        }
        if ((entry->flags & VEX_BOOT_FILE_DIRECTORY) == 0u) {
            continue;
        }
        copy_string(roots[n], VEX_EXPLORER_PATH_MAX, entry->path);
        ++n;
    }
    for (u32 i = 0u; i + 1u < n; ++i) {
        for (u32 j = 0u; j + 1u < n - i; ++j) {
            if (string_compare(roots[j], roots[j + 1u]) > 0) {
                char tmp[VEX_EXPLORER_PATH_MAX];
                copy_string(tmp, sizeof(tmp), roots[j]);
                copy_string(roots[j], VEX_EXPLORER_PATH_MAX, roots[j + 1u]);
                copy_string(roots[j + 1u], VEX_EXPLORER_PATH_MAX, tmp);
            }
        }
    }
    *count = n;
}

static const char* explorer_icon(const vex_boot_file_entry_t* entry) {
    if (entry == 0) {
        return "[?]";
    }
    if ((entry->flags & VEX_BOOT_FILE_DIRECTORY) != 0u) {
        return "[D]";
    }
    if ((entry->flags & VEX_BOOT_FILE_PACKAGE) != 0u) {
        return "[P]";
    }
    return "[F]";
}

static const char* explorer_type_name(const vex_boot_file_entry_t* entry) {
    if (entry == 0) {
        return "NONE";
    }
    if ((entry->flags & VEX_BOOT_FILE_DIRECTORY) != 0u) {
        return "DIR";
    }
    if ((entry->flags & VEX_BOOT_FILE_PACKAGE) != 0u) {
        return "PKG";
    }
    return "FILE";
}

u32 explorer_visible_count(const vex_boot_info_t* boot_info, const desktop_app_state_t* state) {
    if (state == 0) {
        return 0u;
    }
    return explorer_child_count(boot_info, state->explorer_current_path);
}

const vex_boot_file_entry_t* explorer_entry_at(const vex_boot_info_t* boot_info, const desktop_app_state_t* state, u32 index) {
    if (state == 0) {
        return 0;
    }
    return explorer_child_at(boot_info, state->explorer_current_path, index);
}

void explorer_root_dirs(const vex_boot_info_t* boot_info, char roots[][VEX_EXPLORER_PATH_MAX], u32* count) {
    explorer_root_dir_list(boot_info, roots, count);
}

const vex_boot_info_t* desktop_app_runtime_boot_info(void) {
    return g_boot_info;
}

static void clamp_explorer_cursor(desktop_app_state_t* state, const vex_boot_info_t* boot_info) {
    const u32 child_visible = explorer_child_count(boot_info, state->explorer_current_path);
    if (child_visible == 0u) {
        state->explorer_selected_index = 0u;
    } else if (state->explorer_selected_index >= child_visible) {
        state->explorer_selected_index = child_visible - 1u;
    }
    {
        char roots[VEX_EXPLORER_ROOT_MAX][VEX_EXPLORER_PATH_MAX];
        u32 root_count = 0u;
        explorer_root_dir_list(boot_info, roots, &root_count);
        if (root_count == 0u) {
            state->explorer_left_selected = 0u;
        } else if (state->explorer_left_selected >= root_count) {
            state->explorer_left_selected = root_count - 1u;
        }
    }
}

static void update_diagnostics(
    desktop_app_state_t* state,
    const desktop_domain_descriptor_t* descriptor,
    const vex_boot_info_t* boot_info,
    const desktop_animation_state_t* animation
) {
    char line[VEX_LINE_MAX];
    copy_string(state->headline, sizeof(state->headline), "BOOT TELEMETRY");
    copy_string(
        state->detail,
        sizeof(state->detail),
        state->diagnostics_page == 0u ? "overview page" : "package channel page"
    );
    state->meter_primary = boot_info->framebuffer.width;
    state->meter_secondary = boot_file_count(boot_info);
    state->meter_tertiary = animation->pulse_fast + state->diagnostics_page * 10u;

    if (state->diagnostics_page == 0u) {
        copy_string(line, sizeof(line), "SESSION ");
        append_u32(line, sizeof(line), state->session_id);
        append_string(line, sizeof(line), " ABI ");
        append_u32(line, sizeof(line), boot_info->revision);
        set_line(state, 0u, line);

        copy_string(line, sizeof(line), "FB ");
        append_u32(line, sizeof(line), boot_info->framebuffer.width);
        append_string(line, sizeof(line), "x");
        append_u32(line, sizeof(line), boot_info->framebuffer.height);
        append_string(line, sizeof(line), " PITCH ");
        append_u32(line, sizeof(line), boot_info->framebuffer.pixels_per_scanline);
        set_line(state, 1u, line);

        copy_string(line, sizeof(line), "RSDP ");
        append_u32(line, sizeof(line), (u32)(boot_info->rsdp_address & 0xFFFFu));
        append_string(line, sizeof(line), " MMAP ");
        append_u32(line, sizeof(line), (u32)boot_info->memory_map_size);
        set_line(state, 2u, line);

        copy_string(line, sizeof(line), "FILES ");
        append_u32(line, sizeof(line), boot_file_count(boot_info));
        append_string(line, sizeof(line), " PKG ");
        append_u32(line, sizeof(line), verified_package_count(boot_info));
        set_line(state, 3u, line);

        copy_string(line, sizeof(line), "VOL ");
        append_string(line, sizeof(line), boot_info->boot_volume_name);
        append_string(line, sizeof(line), " DOM ");
        append_u32(line, sizeof(line), app_domain_count(boot_info));
        set_line(state, 4u, line);

        set_line(state, 5u, graphics_ready(boot_info) != 0u ? "FRAMEBUFFER READY" : "FRAMEBUFFER INVALID");
        return;
    }

    copy_string(line, sizeof(line), "INIT VERIFIED ");
    append_u32(line, sizeof(line), boot_info->init_image.verified);
    set_line(state, 0u, line);

    copy_string(line, sizeof(line), "APP DOMAINS ");
    append_u32(line, sizeof(line), boot_info->app_image_count);
    append_string(line, sizeof(line), " READY");
    set_line(state, 1u, line);

    copy_string(line, sizeof(line), "PKG VERIFIED ");
    append_u32(line, sizeof(line), verified_package_count(boot_info));
    append_string(line, sizeof(line), " OF ");
    append_u32(line, sizeof(line), boot_file_count(boot_info));
    set_line(state, 2u, line);

    copy_string(line, sizeof(line), "ENTRY ");
    append_string(line, sizeof(line), descriptor->entrypoint);
    set_line(state, 3u, line);

    copy_string(line, sizeof(line), "CAPS ");
    append_string(line, sizeof(line), descriptor->capability_line);
    set_line(state, 4u, line);

    set_line(state, 5u, descriptor->package_verified != 0u ? "PACKAGE VERIFIED" : "PACKAGE UNVERIFIED");
}

static void update_tests(
    desktop_app_state_t* state,
    const desktop_domain_descriptor_t* descriptor,
    const vex_boot_info_t* boot_info,
    const desktop_animation_state_t* animation
) {
    char line[VEX_LINE_MAX];
    u32 pass_count = 0u;
    const u32 check_boot = boot_revision_ready(boot_info);
    const u32 check_graphics = graphics_ready(boot_info);
    const u32 check_packages = verified_package_count(boot_info) >= 6u;
    const u32 check_domains = verified_app_image_count(boot_info) >= 6u;
    const u32 check_surfaces = shared_surface_count(boot_info) >= 4u;
    const u32 check_routes = compositor_route_ready(boot_info);
    volatile vex_gpu_mailbox_t* gpu = 0;
    u32 gpu_ready = 0u;
    u32 gpu_completed = 0u;

    if (boot_info != 0 && boot_info->init_gpu_mailbox_base != 0u) {
        gpu = (volatile vex_gpu_mailbox_t*)(u64)boot_info->init_gpu_mailbox_base;
        if (gpu->magic == VEX_GPU_MAILBOX_MAGIC && gpu->abi_version == VEX_GPU_ABI_VERSION) {
            gpu_ready = gpu->status.ready;
            gpu_completed = gpu->control.completed_sequence;
            if (state->gpu_suspend_applied_sequence != state->gpu_suspend_sequence) {
                gpu->control.command = state->gpu_suspend_requested != 0u ? VEX_GPU_CONTROL_SUSPEND : VEX_GPU_CONTROL_RESUME;
                gpu->control.flags = 0u;
                gpu->control.sequence = state->gpu_suspend_sequence;
                state->gpu_suspend_applied_sequence = state->gpu_suspend_sequence;
            }
        }
    }

    pass_count += check_boot;
    pass_count += check_graphics;
    pass_count += check_packages;
    pass_count += check_domains;
    pass_count += check_surfaces;
    pass_count += check_routes;
    state->test_pass_count = pass_count;

    if (state->test_armed != 0u) {
        state->test_armed_ticks += 1u;
        if (state->test_armed_ticks > TEST_PANIC_ARM_TICKS) {
            state->test_armed = 0u;
            state->test_armed_ticks = 0u;
        }
    }
    if (state->test_demo_active != 0u) {
        state->test_demo_ticks += 1u;
    } else {
        state->test_demo_ticks = 0u;
    }

    copy_string(state->headline, sizeof(state->headline), "TEST CENTER");
    copy_string(state->detail, sizeof(state->detail), "validation + microkernel demos");
    state->meter_primary = (pass_count * 100u) / TEST_CHECK_COUNT;
    state->meter_secondary = state->test_run_count;
    state->meter_tertiary = animation->pulse_fast + state->test_cursor * 3u;

    if (state->test_cursor >= TEST_ITEM_COUNT) {
        state->test_cursor = TEST_ITEM_COUNT - 1u;
    }

    u32 top = 0u;
    if (TEST_ITEM_COUNT > TEST_VISIBLE_ROWS) {
        if (state->test_cursor >= TEST_VISIBLE_ROWS) {
            top = state->test_cursor - (TEST_VISIBLE_ROWS - 1u);
        }
        if (top + TEST_VISIBLE_ROWS > TEST_ITEM_COUNT) {
            top = TEST_ITEM_COUNT - TEST_VISIBLE_ROWS;
        }
    }

    for (u32 row = 0u; row < TEST_VISIBLE_ROWS; ++row) {
        const u32 index = top + row;
        if (index >= TEST_ITEM_COUNT) {
            state->test_list[row][0] = 0;
            continue;
        }
        line[0] = 0;
        append_string(line, sizeof(line), index == state->test_cursor ? "> " : "  ");
        if (index == 0u) {
            append_string(line, sizeof(line), "BOOT ABI        ");
            append_string(line, sizeof(line), check_boot != 0u ? "PASS" : "FAIL");
        } else if (index == 1u) {
            append_string(line, sizeof(line), "FRAMEBUFFER     ");
            append_string(line, sizeof(line), check_graphics != 0u ? "PASS" : "FAIL");
        } else if (index == 2u) {
            append_string(line, sizeof(line), "PACKAGE VERIFY  ");
            append_string(line, sizeof(line), check_packages != 0u ? "PASS" : "FAIL");
        } else if (index == 3u) {
            append_string(line, sizeof(line), "DOMAIN MAP      ");
            append_string(line, sizeof(line), check_domains != 0u ? "PASS" : "FAIL");
        } else if (index == 4u) {
            append_string(line, sizeof(line), "SURFACES        ");
            append_string(line, sizeof(line), check_surfaces != 0u ? "PASS " : "FAIL ");
            append_u32(line, sizeof(line), shared_surface_count(boot_info));
            append_string(line, sizeof(line), "/4");
        } else if (index == 5u) {
            append_string(line, sizeof(line), "GPU ROUTE       ");
            append_string(line, sizeof(line), check_routes != 0u ? "PASS" : "FAIL");
        } else if (index == 6u) {
            append_string(line, sizeof(line), "KILL GPU DRIVER ");
            if (gpu == 0) {
                append_string(line, sizeof(line), "N/A");
            } else {
                append_string(line, sizeof(line), gpu_ready != 0u ? "READY" : "SUSP");
            }
        } else if (index == 7u) {
            append_string(line, sizeof(line), "REVIVE GPU DRV  ");
            if (gpu == 0) {
                append_string(line, sizeof(line), "N/A");
            } else {
                append_string(line, sizeof(line), gpu_ready != 0u ? "READY" : "SUSP");
            }
        } else if (index == 8u) {
            append_string(line, sizeof(line), "CHAOS RASTER    ");
            append_string(line, sizeof(line), state->test_demo_active != 0u ? "RUN " : "IDLE");
        } else {
            append_string(line, sizeof(line), "KERNEL PANIC    ");
            append_string(line, sizeof(line), state->test_armed != 0u ? "ARM " : "SAFE");
        }
        copy_string(state->test_list[row], sizeof(state->test_list[0]), line);
    }

    state->test_result[0][0] = 0;
    state->test_result[1][0] = 0;
    if (state->test_cursor == 6u) {
        copy_string(state->test_result[0], sizeof(state->test_result[0]), "ENTER SUSPENDS GPU SERVICE (USERSPACE DRIVER)");
        copy_string(state->test_result[1], sizeof(state->test_result[1]), "DESKTOP SHOULD STAY UP: DRIVER IS NOT THE KERNEL");
    } else if (state->test_cursor == 7u) {
        copy_string(state->test_result[0], sizeof(state->test_result[0]), "ENTER RESUMES GPU SERVICE");
        copy_string(state->test_result[1], sizeof(state->test_result[1]), "CONTROL SEQ ");
        append_u32(state->test_result[1], sizeof(state->test_result[1]), state->gpu_suspend_sequence);
        append_string(state->test_result[1], sizeof(state->test_result[1]), " ACK ");
        append_u32(state->test_result[1], sizeof(state->test_result[1]), gpu_completed);
    } else if (state->test_cursor == 8u) {
        copy_string(state->test_result[0], sizeof(state->test_result[0]), "ENTER TOGGLES RASTER CHAOS DEMO");
        copy_string(state->test_result[1], sizeof(state->test_result[1]), "RENDER LOAD + GLITCH ANIMATION (NO INPUT ROUTING)");
    } else if (state->test_cursor == 9u) {
        if (state->test_armed == 0u) {
            copy_string(state->test_result[0], sizeof(state->test_result[0]), "ENTER ARMS PANIC (UD2)");
            copy_string(state->test_result[1], sizeof(state->test_result[1]), "SECOND ENTER TRIGGERS KERNEL FAULT HANDLER");
        } else {
            copy_string(state->test_result[0], sizeof(state->test_result[0]), "PANIC ARMED");
            copy_string(state->test_result[1], sizeof(state->test_result[1]), "ENTER NOW TO EXECUTE (AUTO DISARM IN ");
            append_u32(state->test_result[1], sizeof(state->test_result[1]), (TEST_PANIC_ARM_TICKS - state->test_armed_ticks) / 32u);
            append_string(state->test_result[1], sizeof(state->test_result[1]), ")");
        }
    } else {
        copy_string(state->test_result[0], sizeof(state->test_result[0]), "UP/DOWN SELECT, ENTER RUNS");
        copy_string(state->test_result[1], sizeof(state->test_result[1]), "PASS ");
        append_u32(state->test_result[1], sizeof(state->test_result[1]), state->test_pass_count);
        append_string(state->test_result[1], sizeof(state->test_result[1]), "/");
        append_u32(state->test_result[1], sizeof(state->test_result[1]), TEST_CHECK_COUNT);
        append_string(state->test_result[1], sizeof(state->test_result[1]), " RUNS ");
        append_u32(state->test_result[1], sizeof(state->test_result[1]), state->test_run_count);
    }

    (void)descriptor;
}

static void update_explorer(
    desktop_app_state_t* state,
    const desktop_domain_descriptor_t* descriptor,
    const vex_boot_info_t* boot_info
) {
    char line[VEX_LINE_MAX];
    char roots[VEX_EXPLORER_ROOT_MAX][VEX_EXPLORER_PATH_MAX];
    u32 root_count = 0u;
    const vex_boot_file_entry_t* selected;
    const u32 visible_count = explorer_child_count(boot_info, state->explorer_current_path);

    copy_string(state->browser_prefix, sizeof(state->browser_prefix), state->explorer_current_path);
    state->browser_cursor = state->explorer_selected_index;
    clamp_explorer_cursor(state, boot_info);

    if (state->explorer_current_path[0] == 0) {
        const vex_boot_file_entry_t* first = explorer_child_at(boot_info, "/", 0u);
        if (first != 0) {
            copy_string(state->explorer_current_path, sizeof(state->explorer_current_path), first->path);
        } else {
            copy_string(state->explorer_current_path, sizeof(state->explorer_current_path), "/");
        }
        copy_string(state->browser_prefix, sizeof(state->browser_prefix), state->explorer_current_path);
    }

    explorer_root_dir_list(boot_info, roots, &root_count);

    copy_string(state->headline, sizeof(state->headline), "BOOT EXPLORER");
    copy_string(state->detail, sizeof(state->detail), "ADDR ");
    append_string(state->detail, sizeof(state->detail), state->explorer_current_path);
    state->meter_primary = boot_file_count(boot_info);
    state->meter_secondary = visible_count;
    state->meter_tertiary = state->explorer_selected_index + 1u;

    line[0] = 0;
    append_string(line, sizeof(line), state->explorer_focus_left != 0u ? "TREE* " : "TREE  ");
    if (root_count == 0u) {
        append_string(line, sizeof(line), "(no root dirs)");
    } else {
        for (u32 index = 0u; index < root_count && index < 4u; ++index) {
            append_string(line, sizeof(line), index == state->explorer_left_selected ? "[" : " ");
            append_string(line, sizeof(line), path_leaf_name(roots[index]));
            append_string(line, sizeof(line), index == state->explorer_left_selected ? "]" : " ");
        }
        if (root_count > 4u) {
            append_string(line, sizeof(line), " ...");
        }
    }
    set_line(state, 0u, line);

    copy_string(line, sizeof(line), "RIGHT FILES: NAME | SIZE | TYPE");
    set_line(state, 1u, line);

    for (u32 row = 0u; row < 4u; ++row) {
        const vex_boot_file_entry_t* entry = explorer_child_at(
            boot_info, state->explorer_current_path, state->explorer_selected_index + row
        );
        if (entry == 0) {
            state->lines[2u + row][0] = 0;
            continue;
        }
        line[0] = 0;
        append_string(line, sizeof(line), row == 0u ? "> " : "  ");
        append_string(line, sizeof(line), explorer_icon(entry));
        append_string(line, sizeof(line), " ");
        append_string(line, sizeof(line), path_leaf_name(entry->path));
        append_string(line, sizeof(line), " | ");
        append_u32(line, sizeof(line), (u32)(entry->size / 1024u));
        append_string(line, sizeof(line), "KB | ");
        append_string(line, sizeof(line), explorer_type_name(entry));
        set_line(state, 2u + row, line);
    }

    line[0] = 0;
    selected = explorer_child_at(boot_info, state->explorer_current_path, state->explorer_selected_index);
    if (selected != 0) {
        append_string(line, sizeof(line), path_leaf_name(selected->path));
        append_string(line, sizeof(line), " ");
        append_u32(line, sizeof(line), (u32)(selected->size / 1024u));
        append_string(line, sizeof(line), "KB");
        if ((selected->flags & VEX_BOOT_FILE_VERIFIED) != 0u) {
            append_string(line, sizeof(line), " VERIFIED");
        }
        if (state->explorer_focus_left == 0u && (selected->flags & VEX_BOOT_FILE_DIRECTORY) != 0u) {
            append_string(line, sizeof(line), " [dir]");
        }
    } else {
        append_string(line, sizeof(line), "NO ENTRY");
    }
    set_line(state, 5u, line);
    (void)descriptor;
}

static void update_terminal_lines(desktop_app_state_t* state) {
    char line[VEX_LINE_MAX];
    const u32 recent = state->terminal_history_count < 4u ? state->terminal_history_count : 4u;
    const u32 start = state->terminal_history_count > 4u ? state->terminal_history_count - 4u : 0u;

    copy_string(state->headline, sizeof(state->headline), "SESSION CONSOLE");
    copy_string(state->detail, sizeof(state->detail), "help ls status vol pkg time clear ping");

    for (u32 index = 0u; index < 4u; ++index) {
        if (index < recent) {
            copy_string(state->lines[index], sizeof(state->lines[index]), state->terminal_history[start + index]);
        } else {
            state->lines[index][0] = 0;
        }
    }

    copy_string(line, sizeof(line), "> ");
    append_string(line, sizeof(line), state->terminal_input);
    if ((state->heartbeat & 1u) == 0u) {
        append_string(line, sizeof(line), "_");
    }
    set_line(state, 4u, line);

    copy_string(line, sizeof(line), "history=");
    append_u32(line, sizeof(line), state->terminal_history_count);
    append_string(line, sizeof(line), " session=");
    append_u32(line, sizeof(line), state->session_id);
    set_line(state, 5u, line);
}

static void terminal_command_ls(desktop_app_state_t* state, const vex_boot_info_t* boot_info) {
    u32 emitted = 0u;
    for (u32 index = 0u; index < boot_file_count(boot_info) && emitted < 3u; ++index) {
        char line[VEX_LINE_MAX];
        const vex_boot_file_entry_t* entry = boot_file_at(boot_info, index);
        if (entry == 0 || entry->depth == 0u) {
            continue;
        }
        line[0] = 0;
        append_string(line, sizeof(line), (entry->flags & VEX_BOOT_FILE_DIRECTORY) != 0u ? "dir " : "file ");
        append_string(line, sizeof(line), entry->path);
        push_terminal_history(state, line);
        ++emitted;
    }
    if (emitted == 0u) {
        push_terminal_history(state, "no files");
    }
}

static void execute_terminal_command(desktop_app_state_t* state, const vex_boot_info_t* boot_info) {
    char line[VEX_LINE_MAX];

    copy_string(line, sizeof(line), "> ");
    append_string(line, sizeof(line), state->terminal_input);
    push_terminal_history(state, line);

    if (state->terminal_input[0] == 0) {
        push_terminal_history(state, "idle");
    } else if (string_equals(state->terminal_input, "help") != 0u) {
        push_terminal_history(state, "help ls vol pkg surface route time status clear ping");
    } else if (string_equals(state->terminal_input, "clear") != 0u) {
        state->terminal_history_count = 0u;
    } else if (string_equals(state->terminal_input, "status") != 0u) {
        line[0] = 0;
        append_string(line, sizeof(line), "domains ");
        append_u32(line, sizeof(line), app_domain_count(boot_info));
        append_string(line, sizeof(line), " files ");
        append_u32(line, sizeof(line), boot_file_count(boot_info));
        push_terminal_history(state, line);
    } else if (string_equals(state->terminal_input, "vol") != 0u) {
        line[0] = 0;
        append_string(line, sizeof(line), "volume ");
        append_string(line, sizeof(line), boot_info->boot_volume_name);
        append_string(line, sizeof(line), " files ");
        append_u32(line, sizeof(line), boot_file_count(boot_info));
        push_terminal_history(state, line);
    } else if (string_equals(state->terminal_input, "pkg") != 0u) {
        line[0] = 0;
        append_string(line, sizeof(line), "verified packages ");
        append_u32(line, sizeof(line), verified_package_count(boot_info));
        push_terminal_history(state, line);
    } else if (string_equals(state->terminal_input, "surface") != 0u) {
        line[0] = 0;
        append_string(line, sizeof(line), "shared surfaces ");
        append_u32(line, sizeof(line), shared_surface_count(boot_info));
        append_string(line, sizeof(line), " triple-buffered");
        push_terminal_history(state, line);
    } else if (string_equals(state->terminal_input, "route") != 0u) {
        push_terminal_history(state, compositor_route_ready(boot_info) != 0u ? "gpu route online" : "gpu route unavailable");
    } else if (string_equals(state->terminal_input, "time") != 0u) {
        push_terminal_history(state, "clock routed to desktop taskbar");
    } else if (string_equals(state->terminal_input, "ls") != 0u) {
        terminal_command_ls(state, boot_info);
    } else if (string_equals(state->terminal_input, "ping") != 0u) {
        push_terminal_history(state, "pong");
    } else {
        push_terminal_history(state, "unknown command");
    }

    state->terminal_input[0] = 0;
    state->terminal_cursor = 0u;
}

void desktop_app_runtime_init(void) {
    g_next_session_id = 1u;
    for (u32 index = 0u; index < APP_STATE_COUNT; ++index) {
        clear_state(&g_app_states[index]);
    }
}

void desktop_app_runtime_sync_view(
    shell_view_t view,
    u32 engaged,
    const desktop_domain_descriptor_t* descriptor
) {
    desktop_app_state_t* state;

    if ((u32)view >= APP_STATE_COUNT || view == VIEW_HUB || descriptor == 0) {
        return;
    }

    state = &g_app_states[(u32)view];
    if (engaged != 0u) {
        if (state->running == 0u) {
            start_state(state, descriptor);
            if (view == VIEW_TERMINAL) {
                push_terminal_history(state, "terminal online");
                update_terminal_lines(state);
            }
            if (view == VIEW_CONSOLE) {
                push_terminal_history(state, "console online");
                update_terminal_lines(state);
            }
        }
        return;
    }

    if (state->running != 0u) {
        state->running = 0u;
        state->uptime_ticks = 0u;
        state->heartbeat = 0u;
        state->refresh_divider = 0u;
    }
}

u32 desktop_app_runtime_tick(
    const vex_boot_info_t* boot_info,
    const desktop_animation_state_t* animation
) {
    u32 dirty = 0u;

    if (boot_info != 0) {
        g_boot_info = boot_info;
    }

    for (u32 index = 0u; index < APP_STATE_COUNT; ++index) {
        desktop_app_state_t* state = &g_app_states[index];
        const shell_view_t view = (shell_view_t)index;
        const desktop_domain_descriptor_t* descriptor = desktop_domain_descriptor(view);
        if (state->running == 0u || descriptor == 0 || boot_info == 0) {
            continue;
        }

        state->refresh_divider += 1u;
        if (state->refresh_divider < APP_REFRESH_INTERVAL) {
            continue;
        }
        state->refresh_divider = 0u;
        state->uptime_ticks += 1u;
        state->heartbeat += 1u;

        switch (view) {
        case VIEW_DIAGNOSTICS:
            update_diagnostics(state, descriptor, boot_info, animation);
            break;
        case VIEW_TESTS:
            update_tests(state, descriptor, boot_info, animation);
            break;
        case VIEW_SERVICES:
            update_explorer(state, descriptor, boot_info);
            break;
        case VIEW_TERMINAL:
            state->meter_primary = state->launch_count;
            state->meter_secondary = boot_file_count(boot_info);
            state->meter_tertiary = verified_package_count(boot_info);
            update_terminal_lines(state);
            break;
        case VIEW_CONSOLE:
            state->meter_primary = state->launch_count;
            state->meter_secondary = boot_file_count(boot_info);
            state->meter_tertiary = verified_package_count(boot_info);
            update_terminal_lines(state);
            break;
        default:
            break;
        }
        dirty = 1u;
    }

    return dirty;
}

u32 desktop_app_runtime_handle_key(shell_view_t view, ui_key_t key, const vex_boot_info_t* boot_info) {
    desktop_app_state_t* state;

    if ((u32)view >= APP_STATE_COUNT) {
        return 0u;
    }
    state = &g_app_states[(u32)view];
    if (state->running == 0u) {
        return 0u;
    }

    if (view == VIEW_TERMINAL || view == VIEW_CONSOLE) {
        if (key == KEY_BACKSPACE) {
            if (state->terminal_cursor > 0u) {
                state->terminal_cursor -= 1u;
                state->terminal_input[state->terminal_cursor] = 0;
                update_terminal_lines(state);
                return 1u;
            }
            return 0u;
        }
        if (key == KEY_ENTER && boot_info != 0) {
            execute_terminal_command(state, boot_info);
            update_terminal_lines(state);
            return 1u;
        }
        return 0u;
    }

    if (view == VIEW_SERVICES) {
        char roots[VEX_EXPLORER_ROOT_MAX][VEX_EXPLORER_PATH_MAX];
        u32 root_count = 0u;
        explorer_root_dir_list(boot_info, roots, &root_count);
        clamp_explorer_cursor(state, boot_info);

        if (key == KEY_TAB) {
            state->explorer_focus_left = state->explorer_focus_left != 0u ? 0u : 1u;
            return 1u;
        }
        if (key == KEY_UP) {
            if (state->explorer_focus_left != 0u) {
                if (state->explorer_left_selected > 0u) {
                    state->explorer_left_selected -= 1u;
                }
            } else if (state->explorer_selected_index > 0u) {
                state->explorer_selected_index -= 1u;
            }
            return 1u;
        }
        if (key == KEY_DOWN) {
            if (state->explorer_focus_left != 0u) {
                if (state->explorer_left_selected + 1u < root_count) {
                    state->explorer_left_selected += 1u;
                }
            } else {
                const u32 visible = explorer_child_count(boot_info, state->explorer_current_path);
                if (state->explorer_selected_index + 1u < visible) {
                    state->explorer_selected_index += 1u;
                }
            }
            return 1u;
        }
        if (key == KEY_LEFT) {
            if (state->explorer_focus_left != 0u) {
    copy_string(state->explorer_current_path, VEX_EXPLORER_PATH_MAX, "/");
                state->explorer_selected_index = 0u;
            } else {
                state->explorer_focus_left = 1u;
            }
            clamp_explorer_cursor(state, boot_info);
            return 1u;
        }
        if (key == KEY_RIGHT) {
            if (state->explorer_focus_left != 0u) {
                if (root_count > 0u && state->explorer_left_selected < root_count) {
                    copy_string(state->explorer_current_path, sizeof(state->explorer_current_path), roots[state->explorer_left_selected]);
                    state->explorer_selected_index = 0u;
                    state->explorer_focus_left = 0u;
                }
            } else {
                const vex_boot_file_entry_t* selected = explorer_child_at(
                    boot_info, state->explorer_current_path, state->explorer_selected_index
                );
                if (selected != 0 && (selected->flags & VEX_BOOT_FILE_DIRECTORY) != 0u) {
                    copy_string(state->explorer_current_path, sizeof(state->explorer_current_path), selected->path);
                    state->explorer_selected_index = 0u;
                }
            }
            clamp_explorer_cursor(state, boot_info);
            return 1u;
        }
        if (key == KEY_ENTER && boot_info != 0) {
            if (state->explorer_focus_left != 0u) {
                if (root_count > 0u && state->explorer_left_selected < root_count) {
                    copy_string(state->explorer_current_path, sizeof(state->explorer_current_path), roots[state->explorer_left_selected]);
                    state->explorer_selected_index = 0u;
                    state->explorer_focus_left = 0u;
                }
            } else {
                const vex_boot_file_entry_t* selected = explorer_child_at(
                    boot_info, state->explorer_current_path, state->explorer_selected_index
                );
                if (selected != 0 && (selected->flags & VEX_BOOT_FILE_DIRECTORY) != 0u) {
                    copy_string(state->explorer_current_path, sizeof(state->explorer_current_path), selected->path);
                    state->explorer_selected_index = 0u;
                }
            }
            clamp_explorer_cursor(state, boot_info);
            return 1u;
        }
    }

    if (view == VIEW_DIAGNOSTICS) {
        if (key == KEY_LEFT && state->diagnostics_page > 0u) {
            state->diagnostics_page -= 1u;
            return 1u;
        }
        if (key == KEY_RIGHT && state->diagnostics_page < 1u) {
            state->diagnostics_page += 1u;
            return 1u;
        }
    }

    if (view == VIEW_TESTS) {
        if (key == KEY_UP && state->test_cursor > 0u) {
            state->test_cursor -= 1u;
            state->test_armed = 0u;
            state->test_armed_ticks = 0u;
            return 1u;
        }
        if (key == KEY_DOWN && state->test_cursor + 1u < TEST_ITEM_COUNT) {
            state->test_cursor += 1u;
            state->test_armed = 0u;
            state->test_armed_ticks = 0u;
            return 1u;
        }
        if (key == KEY_ENTER) {
            state->test_run_count += 1u;
            if (state->test_cursor == 6u) {
                state->gpu_suspend_requested = 1u;
                state->gpu_suspend_sequence += 1u;
                return 1u;
            }
            if (state->test_cursor == 7u) {
                state->gpu_suspend_requested = 0u;
                state->gpu_suspend_sequence += 1u;
                return 1u;
            }
            if (state->test_cursor == 8u) {
                state->test_demo_active = state->test_demo_active != 0u ? 0u : 1u;
                state->test_demo_ticks = 0u;
                return 1u;
            }
            if (state->test_cursor == 9u) {
                if (state->test_armed == 0u) {
                    state->test_armed = 1u;
                    state->test_armed_ticks = 0u;
                    return 1u;
                }
                __asm__ volatile ("ud2");
            }
            return 1u;
        }
    }
    return 0u;
}

u32 desktop_app_runtime_handle_char(shell_view_t view, char ch) {
    desktop_app_state_t* state;

    if ((view != VIEW_TERMINAL && view != VIEW_CONSOLE) || (u32)view >= APP_STATE_COUNT || ch == 0) {
        return 0u;
    }
    state = &g_app_states[(u32)view];
    if (state->running == 0u || state->terminal_cursor + 1u >= sizeof(state->terminal_input)) {
        return 0u;
    }
    state->terminal_input[state->terminal_cursor++] = ch;
    state->terminal_input[state->terminal_cursor] = 0;
    update_terminal_lines(state);
    return 1u;
}

const desktop_app_state_t* desktop_app_runtime_state(shell_view_t view) {
    if ((u32)view >= APP_STATE_COUNT) {
        return 0;
    }
    return &g_app_states[(u32)view];
}
