#include "desktop_views.h"
#include "desktop_app_runtime.h"
#include "desktop_catalog.h"
#include "desktop_domain.h"
#include "vex_boot_info.h"

static const char* explorer_leaf(const char* path) {
    const char* leaf = path;
    if (path == 0) {
        return "";
    }
    while (*path != 0) {
        if (*path == '/') {
            leaf = path + 1;
        }
        ++path;
    }
    return leaf;
}

static const char* explorer_icon_label(u32 flags) {
    if ((flags & VEX_BOOT_FILE_DIRECTORY) != 0u) {
        return "[D]";
    }
    if ((flags & VEX_BOOT_FILE_PACKAGE) != 0u) {
        return "[P]";
    }
    return "[F]";
}

static void explorer_format_u32(char* dst, u32 size, u32 value) {
    char digits[16];
    u32 count = 0u;
    u32 index = 0u;
    if (dst == 0 || size == 0u) {
        return;
    }
    if (value == 0u) {
        dst[0] = '0';
        dst[1] = 0;
        return;
    }
    while (value > 0u && count < sizeof(digits)) {
        digits[count++] = (char)('0' + (value % 10u));
        value /= 10u;
    }
    while (count > 0u && index + 1u < size) {
        dst[index++] = digits[--count];
    }
    dst[index] = 0;
}

static void explorer_append(char* dst, u32 size, const char* src) {
    u32 index = 0u;
    if (dst == 0 || src == 0) {
        return;
    }
    while (index + 1u < size && dst[index] != 0) {
        ++index;
    }
    while (index + 1u < size && *src != 0) {
        dst[index++] = *src++;
    }
    dst[index] = 0;
}

static void draw_text_rows(
    u32 x,
    u32 y,
    const desktop_app_state_t* runtime,
    u32 line_count,
    u32 color,
    u32 dim_color,
    const desktop_view_ops_t* ops
) {
    for (u32 index = 0u; index < line_count; ++index) {
        if (runtime->lines[index][0] == 0) {
            continue;
        }
        ops->draw_text(x, y + index * 22u, runtime->lines[index], 1u, index < 4u ? color : dim_color);
    }
}

static void draw_window_chrome(
    const shell_state_t* state,
    const shell_window_t* windows,
    u32 slot,
    const char* title,
    const desktop_domain_descriptor_t* descriptor,
    const desktop_animation_state_t* animation,
    const desktop_view_theme_t* theme,
    const desktop_view_ops_t* ops
) {
    const ui_rect_t window = window_rect_for_slot(windows, slot);
    const u32 active = state->focus == FOCUS_WINDOW && (u32)state->active_view == slot ? 1u : 0u;
    const u32 accent = descriptor != 0 ? descriptor->accent_color : theme->color_accent;
    ui_rect_t minimize_rect = window;
    ui_rect_t close_rect = window;
    minimize_rect.x = window.x + window.width - 60u;
    minimize_rect.y = window.y + 10u;
    minimize_rect.width = 14u;
    minimize_rect.height = 14u;
    close_rect.x = window.x + window.width - 38u;
    close_rect.y = window.y + 10u;
    close_rect.width = 14u;
    close_rect.height = 14u;
    const u32 hover_min = point_in_rect(state->cursor_x, state->cursor_y, &minimize_rect);
    const u32 hover_close = point_in_rect(state->cursor_x, state->cursor_y, &close_rect);

    ops->draw_shadow(window.x, window.y, window.width, window.height);
    ops->draw_frame(
        window.x,
        window.y,
        window.width,
        window.height,
        active != 0u ? accent : theme->color_window_border,
        theme->color_window
    );
    ops->fill_rect(
        window.x + 1u,
        window.y + 1u,
        window.width - 2u,
        36u,
        active != 0u
            ? ops->mix_color(theme->color_window_title_active, accent, 1u, 4u)
            : ops->mix_color(theme->color_window_title, accent, 1u, 8u)
    );
    ops->fill_rect(
        window.x + 1u,
        window.y + 37u,
        window.width - 2u,
        10u,
        ops->mix_color(theme->color_window_alt, accent, 1u, 4u)
    );
    ops->fill_rect(window.x + 18u, window.y + 39u, 68u, 2u, ops->mix_color(theme->color_window_alt, accent, 1u, 3u));
    ops->fill_rect(window.x + 1u, window.y + 1u, 12u, 36u, ops->mix_color(accent, theme->color_window, 3u, 4u));
    ops->fill_rect(window.x + 1u, window.y + 1u, window.width - 2u, 5u, ops->mix_color(accent, theme->color_text, 1u, 12u));
    ops->fill_rect(window.x + window.width - 82u, window.y + 10u, 14u, 14u, theme->color_text_soft);
    ops->fill_rect(
        window.x + window.width - 60u,
        window.y + 10u,
        14u,
        14u,
        hover_min != 0u ? accent : theme->color_text_dim
    );
    ops->fill_rect(
        window.x + window.width - 38u,
        window.y + 10u,
        14u,
        14u,
        hover_close != 0u ? 0x00FF7D7Du : 0x00D86666u
    );
    ops->draw_text(window.x + 20u, window.y + 11u, title, 2u, theme->color_text);
    if (descriptor != 0 && descriptor->badge_label != 0 && descriptor->badge_label[0] != 0) {
        ops->draw_badge(window.x + window.width - 176u, window.y + 9u, descriptor->badge_label, ops->mix_color(accent, theme->color_window, 3u, 5u));
    }
    (void)animation;
}

static void draw_window_panel(
    u32 x,
    u32 y,
    u32 width,
    u32 height,
    const char* title,
    u32 accent,
    const desktop_view_theme_t* theme,
    const desktop_view_ops_t* ops
) {
    ops->draw_frame(x, y, width, height, theme->color_window_border, theme->color_window_alt);
    ops->fill_rect(x + 1u, y + 1u, width - 2u, 4u, ops->mix_color(accent, theme->color_window_alt, 3u, 5u));
    ops->draw_text(x + 12u, y + 10u, title, 1u, theme->color_text);
}

static const char* domain_mode_label(const desktop_domain_descriptor_t* descriptor) {
    if (descriptor->package_present != 0u && descriptor->package_verified != 0u) {
        return "PACKAGE DOMAIN";
    }
    if (descriptor->builtin != 0u) {
        return "SHELL DOMAIN";
    }
    return "DESKTOP VIEW";
}

static void render_hub(
    const shell_state_t* state,
    const shell_window_t* windows,
    u32 slot,
    const desktop_animation_state_t* animation,
    const desktop_view_theme_t* theme,
    const desktop_view_ops_t* ops
) {
    const ui_rect_t window = window_rect_for_slot(windows, slot);
    const desktop_domain_descriptor_t* hub = desktop_domain_descriptor(VIEW_HUB);
    const shell_view_t selected_view = hub_card_view(state->hub_index);
    const desktop_domain_descriptor_t* selected_descriptor = desktop_domain_descriptor(selected_view);
    const u32 selected_installed = view_is_installed(selected_view);
    const u32 selected_installable = view_is_installable(selected_view);
    const u32 selected_launching = state->launch_busy != 0u && state->launch_view == selected_view;
    draw_window_chrome(state, windows, slot, "HUB", hub, animation, theme, ops);
    ops->draw_frame(window.x + 16u, window.y + 54u, 180u, window.height - 70u, theme->color_window_border, theme->color_panel);
    ops->draw_frame(
        window.x + 208u,
        window.y + 54u,
        window.width - 224u,
        window.height - 70u,
        theme->color_window_alt,
        theme->color_window_alt
    );
    ops->fill_rect(window.x + 16u, window.y + 54u, 180u, 8u, ops->mix_color(hub->accent_color, theme->color_panel, 3u, 4u));
    ops->fill_rect(window.x + 208u, window.y + 54u, window.width - 224u, 8u, ops->mix_color(hub->accent_color, theme->color_window_alt, 1u, 5u));
    ops->draw_text(window.x + 32u, window.y + 72u, "SYSTEM HUB", 1u, theme->color_text_soft);
    ops->draw_text(window.x + 32u, window.y + 102u, "CATALOG", 1u, theme->color_text);
    ops->draw_text(window.x + 32u, window.y + 124u, "INSTALL", 1u, theme->color_text);
    ops->draw_text(window.x + 32u, window.y + 146u, "DESKTOP", 1u, theme->color_text);
    ops->draw_text(window.x + 32u, window.y + 168u, "LAUNCH", 1u, theme->color_text);
    ops->draw_text(window.x + 32u, window.y + 212u, "SESSION", 1u, theme->color_text_dim);
    ops->draw_text(window.x + 32u, window.y + 234u, "USERSPACE DESKTOP", 1u, theme->color_text);
    ops->draw_text(window.x + 32u, window.y + 256u, "DOMAIN RUNTIME", 1u, theme->color_text_soft);
    ops->draw_text(window.x + 32u, window.y + 310u, "PACKAGE ABI", 1u, theme->color_text_dim);
    ops->draw_text(window.x + 32u, window.y + 332u, "V1 VERIFIED LOAD", 1u, theme->color_text);
    ops->draw_text(window.x + 32u, window.y + 354u, "CAPABILITY ROUTED", 1u, theme->color_text_soft);
    ops->draw_text(window.x + 32u, window.y + 396u, "INSTALLED", 1u, theme->color_text_dim);
    ops->draw_metric(window.x + 108u, window.y + 396u, "", installed_view_count(), 0u);
    ops->draw_text(window.x + 32u, window.y + 418u, "TASKBAR", 1u, theme->color_text_dim);
    ops->draw_metric(window.x + 108u, window.y + 418u, "", taskbar_entry_count() > 0u ? taskbar_entry_count() - 1u : 0u, 0u);
    ops->draw_text(window.x + 32u, window.y + 456u, "SELECTED", 1u, theme->color_text_dim);
    ops->draw_text(window.x + 32u, window.y + 478u, selected_descriptor->label, 1u, theme->color_text);
    ops->draw_text(window.x + 32u, window.y + 500u, selected_descriptor->manifest_version, 1u, theme->color_text_dim);
    ops->draw_text(window.x + 32u, window.y + 522u, selected_descriptor->capability_line, 1u, theme->color_text_soft);
    ops->draw_frame(
        window.x + 28u,
        window.y + 548u,
        136u,
        28u,
        selected_installed != 0u ? theme->color_ready : selected_descriptor->accent_color,
        selected_installed != 0u ? theme->color_window_alt : ops->mix_color(theme->color_menu_active, selected_descriptor->accent_color, 1u, 4u)
    );
    ops->draw_text(
        window.x + 42u,
        window.y + 558u,
        selected_launching != 0u
            ? (state->launch_action == 1u ? "INSTALLING" : "OPENING")
            : (selected_installed != 0u ? "OPEN" : (selected_installable != 0u ? "INSTALL" : "UNAVAILABLE")),
        1u,
        theme->color_text
    );
    ops->draw_text(window.x + 32u, window.y + 592u, "ARROWS MOVE", 1u, theme->color_text_dim);
    ops->draw_text(window.x + 32u, window.y + 614u, "ENTER ACTS", 1u, theme->color_text_dim);
    ops->draw_text(window.x + 228u, window.y + 72u, "AVAILABLE PACKAGES", 1u, theme->color_text_soft);
    ops->draw_text(window.x + 228u, window.y + 94u, "CLICK OR PRESS ENTER TO INSTALL OR OPEN", 1u, theme->color_text_dim);

    for (u32 index = 0; index < hub_card_count(); ++index) {
        const u32 row = index / 2u;
        const u32 col = index % 2u;
        const ui_rect_t rect = explorer_card_rect(state, row, col);
        const shell_view_t view = hub_card_view(index);
        const desktop_domain_descriptor_t* descriptor = desktop_domain_descriptor(view);
        const u32 installed = view_is_installed(view);
        const u32 installable = view_is_installable(view);
        const u32 hovered = point_in_rect(state->cursor_x, state->cursor_y, &rect);
        const u32 launching = state->launch_busy != 0u && state->launch_view == view;
        const u32 selected = state->focus == FOCUS_WINDOW && state->active_view == VIEW_HUB && state->hub_index == index;
        draw_window_panel(rect.x, rect.y, rect.width, rect.height, view_label(view), descriptor->accent_color, theme, ops);
        ops->fill_rect(rect.x + 1u, rect.y + 1u, rect.width - 2u, 18u, ops->mix_color(descriptor->accent_color, theme->color_window_alt, 2u, 5u));
        if (hovered != 0u || selected != 0u) {
            ops->draw_glow_rect(
                rect.x,
                rect.y,
                rect.width,
                rect.height,
                selected != 0u ? theme->color_selection : descriptor->accent_color
            );
            ops->fill_rect(rect.x + 8u, rect.y + 8u, rect.width - 16u, 2u, ops->mix_color(theme->color_window_alt, theme->color_glow, 1u + animation->pulse_fast / 5u, 7u));
        }
        if (descriptor->badge_label != 0 && descriptor->badge_label[0] != 0) {
            ops->draw_badge(rect.x + rect.width - 84u, rect.y + 26u, descriptor->badge_label, ops->mix_color(descriptor->accent_color, theme->color_window_alt, 3u, 4u));
        }
        ops->draw_text(rect.x + 18u, rect.y + 32u, installed != 0u ? "STATE" : "PACKAGE", 1u, theme->color_text_dim);
        ops->draw_text(
            rect.x + 18u,
            rect.y + 54u,
            launching != 0u ? view_lifecycle_label(view) : view_lifecycle_label(view),
            1u,
            view_lifecycle(view) == DESKTOP_APP_ACTIVE ||
            view_lifecycle(view) == DESKTOP_APP_OPEN ||
            view_lifecycle(view) == DESKTOP_APP_PINNED
                ? theme->color_ready
                : (descriptor->package_verified != 0u ? theme->color_text_soft : theme->color_warning)
        );
        ops->draw_text(rect.x + 18u, rect.y + 76u, descriptor->manifest_version, 1u, theme->color_text_dim);
        ops->draw_text(rect.x + 88u, rect.y + 76u, installed != 0u ? "DESKTOP" : "CATALOG", 1u, installed != 0u ? theme->color_ready : theme->color_text_dim);
        ops->draw_frame(
            rect.x + 18u,
            rect.y + 100u,
            108u,
            24u,
            installed != 0u ? theme->color_ready : descriptor->accent_color,
            installed != 0u ? theme->color_window_alt : ops->mix_color(theme->color_menu_active, descriptor->accent_color, 1u, 4u)
        );
        ops->draw_text(
            rect.x + 22u,
            rect.y + 108u,
            launching != 0u
                ? (state->launch_action == 1u ? "DEPLOYING" : "OPENING")
                : (installed != 0u ? "OPEN" : (installable != 0u ? "INSTALL" : "UNAVAILABLE")),
            1u,
            theme->color_text
        );
        ops->draw_text(rect.x + 18u, rect.y + 132u, descriptor->capability_line, 1u, theme->color_text_soft);
        ops->draw_text(rect.x + 18u, rect.y + 154u, selected != 0u ? "ENTER TO ACTIVATE" : domain_mode_label(descriptor), 1u, selected != 0u ? theme->color_text : theme->color_text_dim);
        ops->draw_text(rect.x + 18u, rect.y + 176u, descriptor->entrypoint, 1u, theme->color_text_dim);
        if (launching != 0u) {
            ops->fill_rect(rect.x + 18u, rect.y + 198u, state->launch_progress, 4u, descriptor->accent_color);
        }
    }
}

static void render_diagnostics(
    const shell_state_t* state,
    const shell_window_t* windows,
    u32 slot,
    const desktop_boot_metrics_t* boot,
    const desktop_animation_state_t* animation,
    const desktop_view_theme_t* theme,
    const desktop_view_ops_t* ops
) {
    const ui_rect_t window = window_rect_for_slot(windows, slot);
    const desktop_domain_descriptor_t* diagnostics = desktop_domain_descriptor(VIEW_DIAGNOSTICS);
    const desktop_app_state_t* runtime = desktop_app_runtime_state(VIEW_DIAGNOSTICS);
    draw_window_chrome(state, windows, slot, "SYSTEM DIAGNOSTICS", diagnostics, animation, theme, ops);
    draw_window_panel(window.x + 20u, window.y + 60u, window.width - 40u, 174u, "LIVE SNAPSHOT", diagnostics->accent_color, theme, ops);
    draw_window_panel(window.x + 20u, window.y + 250u, window.width - 40u, 188u, runtime->diagnostics_page == 0u ? "BOOT OVERVIEW" : "PACKAGE CHANNEL", diagnostics->accent_color, theme, ops);
    draw_window_panel(window.x + 20u, window.y + 454u, window.width - 40u, window.height - 474u, "RAW MACHINE METRICS", diagnostics->accent_color, theme, ops);
    ops->draw_text(window.x + 42u, window.y + 86u, runtime->headline, 1u, theme->color_text);
    ops->draw_text(window.x + 42u, window.y + 108u, runtime->detail, 1u, theme->color_text_soft);
    draw_text_rows(window.x + 42u, window.y + 136u, runtime, 3u, theme->color_text, theme->color_text_soft, ops);
    ops->draw_badge(window.x + window.width - 194u, window.y + 82u, runtime->diagnostics_page == 0u ? "PAGE 1" : "PAGE 2", diagnostics->accent_color);
    ops->draw_text(window.x + window.width - 188u, window.y + 118u, "LEFT/RIGHT SWITCH", 1u, theme->color_text_dim);
    draw_text_rows(window.x + 42u, window.y + 278u, runtime, 6u, theme->color_text, theme->color_text_soft, ops);
    ops->draw_metric(window.x + 42u, window.y + 482u, "RSDP", boot->rsdp_address, 1u);
    ops->draw_metric(window.x + 42u, window.y + 504u, "FB BASE", boot->framebuffer_base, 1u);
    ops->draw_metric(window.x + 42u, window.y + 526u, "FB SIZE", boot->framebuffer_size, 0u);
    ops->draw_metric(window.x + 42u, window.y + 548u, "FB WIDTH", boot->framebuffer_width, 0u);
    ops->draw_metric(window.x + 42u, window.y + 570u, "FB HEIGHT", boot->framebuffer_height, 0u);
    ops->draw_metric(window.x + 42u, window.y + 592u, "FB PITCH", boot->framebuffer_pitch, 0u);
    ops->draw_metric(window.x + 42u, window.y + 614u, "BOOT FILES", boot->memory_map_size, 0u);
    ops->draw_badge(window.x + window.width - 156u, window.y + 478u, diagnostics->package_verified != 0u ? "VERIFIED" : "UNVERIFIED", diagnostics->package_verified != 0u ? theme->color_ready : theme->color_warning);
}

static void render_tests(
    const shell_state_t* state,
    const shell_window_t* windows,
    u32 slot,
    const desktop_animation_state_t* animation,
    const desktop_view_theme_t* theme,
    const desktop_view_ops_t* ops
) {
    const ui_rect_t window = window_rect_for_slot(windows, slot);
    const desktop_domain_descriptor_t* tests = desktop_domain_descriptor(VIEW_TESTS);
    const desktop_app_state_t* runtime = desktop_app_runtime_state(VIEW_TESTS);
    draw_window_chrome(state, windows, slot, "TEST CENTER", tests, animation, theme, ops);
    draw_window_panel(window.x + 20u, window.y + 60u, window.width - 40u, 166u, "ACTIVE SUITE", tests->accent_color, theme, ops);
    draw_window_panel(window.x + 20u, window.y + 242u, window.width - 40u, 192u, "ASSERTION MATRIX", tests->accent_color, theme, ops);
    draw_window_panel(window.x + 20u, window.y + 450u, window.width - 40u, window.height - 470u, "RESULT CHANNEL", tests->accent_color, theme, ops);
    ops->draw_text(window.x + 42u, window.y + 86u, runtime->headline, 1u, theme->color_text);
    ops->draw_text(window.x + 42u, window.y + 108u, runtime->detail, 1u, theme->color_text_soft);
    ops->draw_badge(window.x + 42u, window.y + 136u, "ENTER RUNS", tests->accent_color);
    ops->draw_badge(window.x + 132u, window.y + 136u, "UP DOWN SELECT", theme->color_ready);
    ops->draw_metric(window.x + 42u, window.y + 174u, "PASS", runtime->test_pass_count, 0u);
    ops->draw_metric(window.x + 192u, window.y + 174u, "RUNS", runtime->test_run_count, 0u);
    for (u32 row = 0u; row < 6u; ++row) {
        if (runtime->test_list[row][0] == 0) {
            continue;
        }
        ops->draw_text(window.x + 42u, window.y + 270u + row * 22u, runtime->test_list[row], 1u, theme->color_text);
    }
    ops->draw_text(window.x + 42u, window.y + 474u, runtime->test_result[0], 1u, theme->color_text_soft);
    ops->draw_text(window.x + 42u, window.y + 498u, runtime->test_result[1], 1u, theme->color_text);
    ops->draw_text(window.x + window.width - 186u, window.y + 498u, tests->manifest_version, 1u, theme->color_text_dim);

    if (runtime->test_demo_active != 0u) {
        const u32 origin_x = window.x + 32u;
        const u32 origin_y = window.y + 520u;
        const u32 area_w = window.width - 64u;
        const u32 area_h = window.height - 548u;
        for (u32 i = 0u; i < 14u; ++i) {
            const u32 seed = runtime->test_demo_ticks * 17u + animation->pulse_fast * 11u + i * 37u;
            const u32 w = 24u + (seed % 84u);
            const u32 h = 6u + ((seed / 3u) % 22u);
            const u32 x = origin_x + (seed % (area_w > w ? (area_w - w) : 1u));
            const u32 y = origin_y + ((seed / 7u) % (area_h > h ? (area_h - h) : 1u));
            const u32 color = ops->mix_color(tests->accent_color, theme->color_glow, 1u + (seed % 3u), 4u);
            ops->fill_rect(x, y, w, h, ops->mix_color(color, theme->color_panel, 1u, 3u));
            ops->draw_glow_rect(x - 2u, y - 2u, w + 4u, h + 4u, ops->mix_color(color, theme->color_glow, 1u, 2u));
        }
    }
}

static void render_services(
    const shell_state_t* state,
    const shell_window_t* windows,
    u32 slot,
    const desktop_animation_state_t* animation,
    const desktop_view_theme_t* theme,
    const desktop_view_ops_t* ops
) {
    const ui_rect_t window = window_rect_for_slot(windows, slot);
    const desktop_domain_descriptor_t* services = desktop_domain_descriptor(VIEW_SERVICES);
    const desktop_app_state_t* runtime = desktop_app_runtime_state(VIEW_SERVICES);
    const vex_boot_info_t* boot = desktop_app_runtime_boot_info();
    const u32 accent = services != 0 ? services->accent_color : theme->color_accent;
    const u32 focus_left = runtime->explorer_focus_left;
    const u32 addr_x = window.x + 18u;
    const u32 addr_y = window.y + 54u;
    const u32 addr_w = window.width > 36u ? window.width - 36u : 100u;
    const u32 addr_h = 26u;
    const u32 list_top = window.y + 88u;
    const u32 status_h = 24u;
    const u32 list_bottom = window.y + window.height - status_h - 6u;
    const u32 list_h = list_bottom > list_top ? list_bottom - list_top : 60u;
    const u32 left_x = window.x + 18u;
    const u32 left_w = 180u;
    const u32 right_x = window.x + 208u;
    const u32 right_w = window.width > (right_x - window.x + 18u) ? window.width - (right_x - window.x) - 18u : 200u;
    const u32 row_h = 20u;
    const u32 stat_x = window.x + 18u;
    const u32 stat_y = window.y + window.height - status_h - 4u;
    const u32 stat_w = window.width > 36u ? window.width - 36u : 100u;
    const u32 root_base_y = list_top + 34u;
    const u32 row_base = list_top + 30u;
    char roots[VEX_EXPLORER_ROOT_MAX][96];
    u32 root_count = 0u;
    u32 visible = 0u;
    const char* path = runtime->explorer_current_path[0] != 0 ? runtime->explorer_current_path : runtime->browser_prefix;

    (void)animation;

    draw_window_chrome(state, windows, slot, "EXPLORER", services, animation, theme, ops);

    ops->draw_frame(addr_x, addr_y, addr_w, addr_h, theme->color_window_border, theme->color_window_alt);
    ops->fill_rect(addr_x + 1u, addr_y + 1u, addr_w - 2u, 4u, ops->mix_color(accent, theme->color_window_alt, 3u, 5u));
    ops->draw_text(addr_x + 12u, addr_y + 9u, "ADDRESS", 1u, theme->color_text_dim);
    ops->draw_text(addr_x + 84u, addr_y + 9u, path[0] != 0 ? path : "/", 1u, theme->color_text);

    if (boot != 0) {
        explorer_root_dirs(boot, roots, &root_count);
    }
    if (boot != 0) {
        visible = explorer_visible_count(boot, runtime);
    }

    ops->draw_frame(left_x, list_top, left_w, list_h, focus_left != 0u ? accent : theme->color_window_border, theme->color_window_alt);
    ops->fill_rect(left_x + 1u, list_top + 1u, left_w - 2u, 6u, ops->mix_color(accent, theme->color_window_alt, 3u, 5u));
    ops->draw_text(left_x + 10u, list_top + 13u, "FOLDERS", 1u, theme->color_text_soft);
    if (focus_left != 0u) {
        ops->draw_glow_rect(left_x, list_top, left_w, list_h, accent);
        ops->fill_rect(left_x + 2u, list_top + 24u, 3u, list_h - 28u, accent);
    }
    for (u32 i = 0u; i < root_count; ++i) {
        const u32 ry = root_base_y + i * row_h;
        if (ry + row_h > list_bottom) {
            break;
        }
        if (i == runtime->explorer_left_selected) {
            ops->fill_rect(left_x + 6u, ry, left_w - 12u, row_h - 2u, ops->mix_color(theme->color_selection, accent, 1u, 4u));
            ops->draw_text(left_x + 14u, ry + 4u, explorer_leaf(roots[i]), 1u, theme->color_text);
        } else {
            ops->draw_text(left_x + 14u, ry + 4u, explorer_leaf(roots[i]), 1u, theme->color_text_soft);
        }
    }

    ops->draw_frame(right_x, list_top, right_w, list_h, focus_left == 0u ? accent : theme->color_window_border, theme->color_window_alt);
    ops->fill_rect(right_x + 1u, list_top + 1u, right_w - 2u, 6u, ops->mix_color(accent, theme->color_window_alt, 3u, 5u));
    ops->draw_text(right_x + 12u, list_top + 13u, "TYPE", 1u, theme->color_text_dim);
    ops->draw_text(right_x + 54u, list_top + 13u, "NAME", 1u, theme->color_text_dim);
    ops->draw_text(right_x + right_w - 70u, list_top + 13u, "SIZE", 1u, theme->color_text_dim);
    ops->fill_rect(right_x + 1u, list_top + 22u, right_w - 2u, 1u, ops->mix_color(theme->color_window_border, accent, 1u, 6u));
    for (u32 i = 0u; i < visible; ++i) {
        const vex_boot_file_entry_t* entry = explorer_entry_at(boot, runtime, i);
        const u32 ry = row_base + i * row_h;
        const u32 selected = (i == runtime->explorer_selected_index) ? 1u : 0u;
        if (ry + row_h > list_bottom) {
            break;
        }
        if (selected != 0u) {
            ops->fill_rect(right_x + 4u, ry, right_w - 8u, row_h - 2u, ops->mix_color(theme->color_selection, accent, 1u, 4u));
        }
        if (entry == 0) {
            continue;
        }
        ops->draw_text(right_x + 12u, ry + 3u, explorer_icon_label(entry->flags), 1u, selected != 0u ? theme->color_text : theme->color_text_soft);
        ops->draw_text(right_x + 54u, ry + 3u, explorer_leaf(entry->path), 1u, selected != 0u ? theme->color_text : theme->color_text);
        if ((entry->flags & VEX_BOOT_FILE_DIRECTORY) != 0u) {
            ops->draw_text(right_x + right_w - 70u, ry + 3u, "DIR", 1u, theme->color_text_dim);
        } else if (selected != 0u) {
            char sz[16];
            explorer_format_u32(sz, sizeof(sz), (u32)(entry->size / 1024u));
            ops->draw_text(right_x + right_w - 70u, ry + 3u, sz, 1u, theme->color_text);
        } else {
            char sz[16];
            explorer_format_u32(sz, sizeof(sz), (u32)(entry->size / 1024u));
            ops->draw_text(right_x + right_w - 70u, ry + 3u, sz, 1u, theme->color_text_dim);
        }
    }

    ops->draw_frame(stat_x, stat_y, stat_w, status_h, theme->color_window_border, theme->color_window_alt);
    ops->fill_rect(stat_x + 1u, stat_y + 1u, stat_w - 2u, 4u, ops->mix_color(accent, theme->color_window_alt, 3u, 5u));
    {
        char stat[48];
        stat[0] = 0;
        explorer_format_u32(stat, sizeof(stat), visible);
        explorer_append(stat, sizeof(stat), " objects");
        ops->draw_text(stat_x + 12u, stat_y + 9u, stat, 1u, theme->color_text_soft);
    }
}

static void render_terminal(
    const shell_state_t* state,
    const shell_window_t* windows,
    u32 slot,
    const desktop_animation_state_t* animation,
    const desktop_view_theme_t* theme,
    const desktop_view_ops_t* ops
) {
    const ui_rect_t window = window_rect_for_slot(windows, slot);
    const desktop_domain_descriptor_t* terminal = desktop_domain_descriptor(VIEW_TERMINAL);
    const desktop_app_state_t* runtime = desktop_app_runtime_state(VIEW_TERMINAL);
    draw_window_chrome(state, windows, slot, "TERMINAL", terminal, animation, theme, ops);
    draw_window_panel(window.x + 18u, window.y + 56u, window.width - 36u, 92u, "SESSION", terminal->accent_color, theme, ops);
    draw_window_panel(window.x + 18u, window.y + 164u, window.width - 36u, window.height - 182u, "CONSOLE", terminal->accent_color, theme, ops);
    ops->draw_text(window.x + 40u, window.y + 82u, runtime->headline, 1u, theme->color_text);
    ops->draw_text(window.x + 40u, window.y + 104u, runtime->detail, 1u, theme->color_text_soft);
    ops->draw_text(window.x + 40u, window.y + 126u, "TYPE COMMANDS DIRECTLY INTO THIS WINDOW", 1u, theme->color_text_dim);
    draw_text_rows(window.x + 40u, window.y + 192u, runtime, 4u, theme->color_text, theme->color_text_soft, ops);
    ops->fill_rect(window.x + 34u, window.y + window.height - 78u, window.width - 68u, 28u, theme->color_panel);
    ops->draw_text(window.x + 46u, window.y + window.height - 70u, runtime->lines[4], 1u, theme->color_text);
    ops->draw_text(window.x + 46u, window.y + window.height - 42u, runtime->lines[5], 1u, theme->color_text_soft);
}

void desktop_views_render_window(
    const shell_state_t* state,
    const shell_window_t* windows,
    u32 slot,
    const desktop_boot_metrics_t* boot,
    const desktop_animation_state_t* animation,
    const desktop_view_theme_t* theme,
    const desktop_view_ops_t* ops
) {
    switch (windows[slot].kind) {
    case VIEW_HUB:
        render_hub(state, windows, slot, animation, theme, ops);
        break;
    case VIEW_DIAGNOSTICS:
        render_diagnostics(state, windows, slot, boot, animation, theme, ops);
        break;
    case VIEW_TESTS:
        render_tests(state, windows, slot, animation, theme, ops);
        break;
    case VIEW_SERVICES:
        render_services(state, windows, slot, animation, theme, ops);
        break;
    case VIEW_TERMINAL:
        render_terminal(state, windows, slot, animation, theme, ops);
        break;
    case VIEW_CONSOLE:
        render_terminal(state, windows, slot, animation, theme, ops);
        break;
    }
}

void desktop_views_render_active_windows(
    const shell_state_t* state,
    const shell_window_t* windows,
    const u32* order,
    u32 window_count,
    const desktop_boot_metrics_t* boot,
    const desktop_animation_state_t* animation,
    const desktop_view_theme_t* theme,
    const desktop_view_ops_t* ops
) {
    for (u32 index = 0; index < window_count; ++index) {
        const u32 slot = order[index];
        if (windows[slot].render_visible == 0u) {
            continue;
        }
        desktop_views_render_window(state, windows, slot, boot, animation, theme, ops);
    }
}
