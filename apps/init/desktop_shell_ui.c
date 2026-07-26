#include "desktop_shell_ui.h"
#include "desktop_domain.h"
#include "desktop_layout.h"

static void draw_taskbar(
    u32 desktop_width,
    u32 desktop_height,
    const desktop_animation_state_t* animation,
    const desktop_shell_ui_theme_t* theme,
    const desktop_shell_ui_ops_t* ops
) {
    const u32 top = desktop_height - 48u;
    ops->fill_rect(0u, top, desktop_width, 48u, theme->color_taskbar);
    ops->fill_rect(0u, top, desktop_width, 2u, theme->color_taskbar_edge);
    ops->fill_rect(0u, top + 2u, desktop_width, 1u, ops->mix_color(theme->color_taskbar_edge, theme->color_taskbar, 1u, 4u));
    ops->fill_rect(0u, top - 14u, desktop_width, 14u, 0x00070C14u);
    ops->fill_rect(0u, top - 1u, desktop_width, 1u, 0x00040A14u);
    (void)animation;
}

static void draw_taskbar_button(
    u32 desktop_height,
    u32 x,
    u32 width,
    u32 selected,
    u32 hovered,
    u32 open,
    u32 active,
    u32 minimized,
    const char* label,
    const char* badge_label,
    u32 accent_color,
    u32 start_button,
    const desktop_animation_state_t* animation,
    const desktop_shell_ui_theme_t* theme,
    const desktop_shell_ui_ops_t* ops
) {
    const u32 top = desktop_height - 42u;
    const u32 animated_hover = hovered != 0u
        ? ops->mix_color(theme->color_panel_alt, theme->color_taskbar_hilite, 2u + animation->pulse_fast / 6u, 6u)
        : theme->color_panel_alt;
    const u32 base_fill = start_button != 0u ? theme->color_start : theme->color_panel_alt;
    const u32 selected_fill = start_button != 0u ? theme->color_start_active : theme->color_taskbar_hilite;
    u32 fill = selected != 0u ? selected_fill : (hovered != 0u ? animated_hover : base_fill);
    u32 border = hovered != 0u ? theme->color_accent : theme->color_window_border;
    u32 label_color = theme->color_text;

    if (active != 0u && start_button == 0u) {
        fill = ops->mix_color(theme->color_taskbar_hilite, accent_color, 1u + animation->pulse_fast / 7u, 4u);
        border = theme->color_accent;
    } else if (minimized != 0u) {
        fill = ops->mix_color(base_fill, accent_color, 1u, 5u);
        label_color = theme->color_text_dim;
    }

    ops->draw_frame(x, top, width, 34u, border, fill);
    ops->fill_rect(x + 1u, top + 1u, width - 2u, 6u, ops->mix_color(fill, theme->color_text, 1u, 12u));
    if (start_button == 0u) {
        ops->fill_rect(x + 2u, top + 2u, 4u, 30u, ops->mix_color(accent_color, theme->color_glow, 1u, 6u));
        ops->fill_rect(x + 7u, top + 2u, 1u, 30u, ops->mix_color(accent_color, theme->color_panel_alt, 1u, 2u));
    }
    if (open != 0u) {
        const u32 indicator_color = active != 0u
            ? theme->color_text
            : (minimized != 0u ? theme->color_text_dim : theme->color_accent);
        const u32 indicator_width = active != 0u ? width - 16u : (minimized != 0u ? width - 28u : width - 22u);
        const u32 indicator_x = active != 0u ? x + 8u : (minimized != 0u ? x + 14u : x + 11u);
        ops->fill_rect(indicator_x, top + 28u, indicator_width, minimized != 0u ? 2u : 3u, indicator_color);
    }
    if (selected != 0u || hovered != 0u) {
        ops->fill_rect(x + 6u, top + 4u, width - 12u, 1u, ops->mix_color(fill, theme->color_glow, 1u + animation->pulse_fast / 5u, 8u));
    }
    ops->draw_text(x + (start_button != 0u ? 10u : 14u), top + 11u, label, 1u, label_color);
    if (start_button == 0u && badge_label != 0 && badge_label[0] != 0) {
        ops->fill_rect(x + width - 32u, top + 8u, 22u, 10u, ops->mix_color(accent_color, theme->color_panel_alt, 3u, 4u));
        ops->draw_text(x + width - 29u, top + 10u, badge_label, 1u, theme->color_text);
    }
}

static void draw_clock(
    u32 desktop_width,
    u32 desktop_height,
    const shell_state_t* state,
    const desktop_rtc_state_t* clock,
    const desktop_animation_state_t* animation,
    const desktop_shell_ui_theme_t* theme,
    const desktop_shell_ui_ops_t* ops
) {
    const u32 y = desktop_height - 30u;
    ops->draw_frame(
        desktop_width - 324u,
        desktop_height - 40u,
        308u,
        24u,
        ops->mix_color(theme->color_window_border, theme->color_accent, 1u, 4u),
        ops->mix_color(theme->color_taskbar, theme->color_panel_alt, 1u + animation->pulse_slow / 10u, 6u)
    );
    if (state->launch_busy != 0u) {
        const desktop_domain_descriptor_t* descriptor = desktop_domain_descriptor(state->launch_view);
        ops->draw_text(
            desktop_width - 304u,
            y,
            state->launch_action == 1u ? "INSTALLING" : "OPENING",
            1u,
            theme->color_text
        );
        ops->draw_text(desktop_width - 224u, y, descriptor->label, 1u, theme->color_text_soft);
        ops->fill_rect(desktop_width - 136u, desktop_height - 33u, state->launch_progress, 4u, descriptor->accent_color);
    } else if (state->active_view != VIEW_HUB && view_is_installed(state->active_view) != 0u) {
        ops->draw_text(desktop_width - 304u, y, view_lifecycle_label(state->active_view), 1u, theme->color_text);
        ops->draw_text(desktop_width - 224u, y, view_label(state->active_view), 1u, theme->color_text_soft);
    }
    ops->draw_text(desktop_width - 150u, y, clock->date_text, 1u, theme->color_text_soft);
    ops->draw_text(desktop_width - 70u, y, clock->time_text, 1u, theme->color_text);
}

static void draw_start_menu(
    const shell_state_t* state,
    u32 desktop_height,
    const desktop_animation_state_t* animation,
    const desktop_shell_ui_theme_t* theme,
    const desktop_shell_ui_ops_t* ops
) {
    const u32 x = 10u;
    const u32 y = desktop_height - 406u;
    const u32 menu_height = state->start_render_height;
    (void)animation;
    if (menu_height < 24u) {
        return;
    }
    ops->draw_frame(x, y + (344u - menu_height), 280u, menu_height, theme->color_window_border, theme->color_menu);
    if (menu_height > 4u) {
        const u32 inner_height = menu_height > 2u ? menu_height - 2u : 0u;
        ops->fill_rect(x + 1u, y + (344u - menu_height) + 1u, 62u, inner_height, theme->color_menu_header);
    }
    if (menu_height > 54u) {
        ops->fill_rect(x + 63u, y + (344u - menu_height) + 1u, 216u, 52u, ops->mix_color(theme->color_menu_header, theme->color_start_active, 1u, 2u));
    }
    ops->draw_text(x + 82u, y + 18u, "START", 2u, theme->color_text);
    ops->draw_text(x + 82u, y + 60u, "CATALOG APPS", 1u, theme->color_text_soft);
    for (u32 index = 0; index < start_entry_count(); ++index) {
        const u32 row_y = y + 76u + index * 38u;
        if (row_y + 28u > y + menu_height) {
            break;
        }
        const ui_rect_t row_rect = start_item_rect(index, desktop_height);
        const u32 active = state->focus == FOCUS_START && state->start_index == index;
        const u32 hovered = point_in_rect(state->cursor_x, state->cursor_y, &row_rect);
        ops->draw_frame(
            x + 12u,
            row_y,
            256u,
            28u,
            active != 0u ? theme->color_selection : (hovered != 0u ? theme->color_accent : theme->color_panel_alt),
            active != 0u
                ? theme->color_menu_active
                : (hovered != 0u ? ops->mix_color(theme->color_panel_alt, theme->color_menu_active, 1u, 2u) : theme->color_panel_alt)
        );
        if (index + 1u < start_entry_count()) {
            const desktop_domain_descriptor_t* descriptor = desktop_domain_descriptor(start_entry_view(index));
            ops->fill_rect(x + 18u, row_y + 7u, 4u, 14u, descriptor->accent_color);
            ops->draw_text(x + 24u, row_y + 9u, descriptor->label, 1u, theme->color_text);
            ops->draw_text(x + 146u, row_y + 9u, descriptor->manifest_version, 1u, theme->color_text_dim);
            ops->draw_text(x + 202u, row_y + 9u, view_lifecycle_label(start_entry_view(index)), 1u, theme->color_text_soft);
        } else {
            ops->draw_text(x + 24u, row_y + 9u, "SHUTDOWN", 1u, theme->color_text);
        }
    }
}

void desktop_shell_ui_render(
    const shell_state_t* state,
    const shell_window_t* windows,
    u32 window_count,
    u32 desktop_width,
    u32 desktop_height,
    const desktop_rtc_state_t* clock,
    const desktop_animation_state_t* animation,
    const desktop_shell_ui_theme_t* theme,
    const desktop_shell_ui_ops_t* ops
) {
    draw_taskbar(desktop_width, desktop_height, animation, theme, ops);
    for (u32 index = 0; index < taskbar_entry_count(); ++index) {
        const ui_rect_t rect = taskbar_button_rect(index, desktop_height);
        const shell_view_t view = taskbar_entry_view(index);
        const u32 keyboard_selected = state->focus == FOCUS_TASKBAR && state->taskbar_index == index;
        const u32 hovered = point_in_rect(state->cursor_x, state->cursor_y, &rect);
        const u32 active = index == 0u ? state->start_open : (windows[(u32)view].open != 0u && state->active_view == view);
        const u32 minimized = index == 0u ? 0u : windows[(u32)view].minimized != 0u;
        const u32 open = index == 0u ? state->start_open : (windows[(u32)view].open != 0u || minimized != 0u);
        const u32 selected = keyboard_selected != 0u || active != 0u;
        const desktop_domain_descriptor_t* descriptor = desktop_domain_descriptor(view);
        if (index == 0u) {
            draw_taskbar_button(desktop_height, rect.x, rect.width, selected, hovered, open, active, 0u, "START", "", theme->color_start_active, 1u, animation, theme, ops);
            continue;
        }
        switch (view) {
        case VIEW_HUB:
            draw_taskbar_button(desktop_height, rect.x, rect.width, selected, hovered, open, active, minimized, "HUB", descriptor->badge_label, descriptor->accent_color, 0u, animation, theme, ops);
            break;
        case VIEW_DIAGNOSTICS:
            draw_taskbar_button(desktop_height, rect.x, rect.width, selected, hovered, open, active, minimized, "DIAG", descriptor->badge_label, descriptor->accent_color, 0u, animation, theme, ops);
            break;
        case VIEW_TESTS:
            draw_taskbar_button(desktop_height, rect.x, rect.width, selected, hovered, open, active, minimized, "TESTS", descriptor->badge_label, descriptor->accent_color, 0u, animation, theme, ops);
            break;
        case VIEW_SERVICES:
            draw_taskbar_button(desktop_height, rect.x, rect.width, selected, hovered, open, active, minimized, "FILES", descriptor->badge_label, descriptor->accent_color, 0u, animation, theme, ops);
            break;
        default:
            draw_taskbar_button(desktop_height, rect.x, rect.width, selected, hovered, open, active, minimized, "TERM", descriptor->badge_label, descriptor->accent_color, 0u, animation, theme, ops);
            break;
        }
    }
    draw_clock(desktop_width, desktop_height, state, clock, animation, theme, ops);
    if (state->start_visible != 0u) {
        draw_start_menu(state, desktop_height, animation, theme, ops);
    }
    (void)window_count;
}
