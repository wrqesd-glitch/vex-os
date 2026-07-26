#include "desktop_stage.h"
#include "desktop_domain.h"

enum {
    WALLPAPER_WIDTH = 1280u,
    WALLPAPER_HEIGHT = 800u
};

extern unsigned char _binary_apps_init_wallpaper_bin_start[];
extern unsigned char _binary_apps_init_wallpaper_bin_end[];

void desktop_stage_draw_wallpaper(
    u32 desktop_width,
    u32 desktop_height,
    u32 surface_pitch,
    u32* surface_pixels,
    const desktop_animation_state_t* animation,
    const desktop_stage_theme_t* theme,
    const desktop_stage_ops_t* ops
) {
    const u64 wallpaper_bytes = (u64)(_binary_apps_init_wallpaper_bin_end - _binary_apps_init_wallpaper_bin_start);
    const u32* wallpaper = (const u32*)_binary_apps_init_wallpaper_bin_start;

    if (desktop_width == WALLPAPER_WIDTH &&
        desktop_height == WALLPAPER_HEIGHT &&
        wallpaper_bytes >= (u64)WALLPAPER_WIDTH * WALLPAPER_HEIGHT * sizeof(u32)) {
        for (u32 y = 0; y < WALLPAPER_HEIGHT; ++y) {
            ops->copy_row(
                &surface_pixels[(u64)y * surface_pitch],
                &wallpaper[(u64)y * WALLPAPER_WIDTH],
                WALLPAPER_WIDTH
            );
        }
    } else {
        ops->fill_rect(0u, 0u, desktop_width, desktop_height, theme->fallback_wallpaper);
    }
    (void)animation;
}

static void draw_desktop_icon(
    u32 x,
    u32 y,
    u32 selected,
    u32 hovered,
    u32 kind,
    u32 accent_color,
    const char* label,
    const char* subtitle,
    const char* badge_label,
    const desktop_stage_theme_t* theme,
    const desktop_stage_ops_t* ops
) {
    const u32 active = selected != 0u || hovered != 0u;
    const u32 border = selected != 0u ? theme->color_selection : (hovered != 0u ? theme->color_accent : theme->color_window_border);
    const u32 accent_fill = ops->mix_color(theme->color_icon_bg, accent_color, hovered != 0u ? 1u : 1u, hovered != 0u ? 2u : 5u);
    const u32 fill = hovered != 0u
        ? ops->mix_color(accent_fill, theme->color_icon_active, 1u, 2u)
        : (selected != 0u ? ops->mix_color(accent_fill, theme->color_icon_active, 1u, 3u) : accent_fill);
    const u32 glyph = hovered != 0u || selected != 0u
        ? theme->color_text
        : ops->mix_color(theme->color_text, accent_color, 1u, 4u);
    if (active != 0u) {
        ops->draw_frame(x, y, 96u, 118u, border, fill);
        ops->fill_rect(x + 1u, y + 1u, 94u, 8u, ops->mix_color(accent_color, theme->color_glow, 1u, 5u));
        ops->fill_rect(x + 1u, y + 84u, 94u, 33u, ops->mix_color(fill, theme->color_black, 1u, 3u));
        ops->fill_rect(x + 1u, y + 83u, 94u, 1u, ops->mix_color(accent_color, theme->color_glow, 1u, 7u));
    }
    if (kind == 0u) {
        ops->fill_rect(x + 20u, y + 18u, 10u, 38u, glyph);
        ops->fill_rect(x + 36u, y + 10u, 10u, 46u, glyph);
        ops->fill_rect(x + 52u, y + 24u, 10u, 32u, glyph);
        ops->fill_rect(x + 68u, y + 18u, 10u, 38u, glyph);
    } else if (kind == 1u) {
        ops->draw_frame(x + 24u, y + 18u, 46u, 42u, glyph, theme->color_black);
        ops->fill_rect(x + 34u, y + 40u, 8u, 8u, glyph);
        ops->fill_rect(x + 42u, y + 32u, 8u, 8u, glyph);
        ops->fill_rect(x + 50u, y + 26u, 8u, 8u, glyph);
        ops->fill_rect(x + 58u, y + 18u, 8u, 8u, glyph);
    } else if (kind == 2u) {
        ops->draw_frame(x + 22u, y + 18u, 50u, 12u, glyph, theme->color_black);
        ops->draw_frame(x + 22u, y + 36u, 50u, 12u, glyph, theme->color_black);
        ops->draw_frame(x + 22u, y + 54u, 50u, 12u, glyph, theme->color_black);
    } else if (kind == 3u) {
        ops->draw_frame(x + 22u, y + 16u, 48u, 38u, glyph, theme->color_black);
        ops->fill_rect(x + 18u, y + 54u, 56u, 6u, glyph);
        ops->fill_rect(x + 42u, y + 60u, 8u, 8u, glyph);
    } else {
        ops->fill_rect(x + 24u, y + 18u, 42u, 44u, glyph);
        ops->fill_rect(x + 66u, y + 18u, 8u, 12u, active != 0u ? theme->color_icon_bg : theme->fallback_wallpaper);
    }
    if (badge_label != 0 && badge_label[0] != 0) {
        if (active != 0u) {
            ops->fill_rect(x + 56u, y + 62u, 28u, 12u, ops->mix_color(accent_color, theme->color_black, 3u, 5u));
        }
        ops->draw_text(x + 62u, y + 66u, badge_label, 1u, theme->color_text);
    }
    ops->draw_text(x + 10u, y + 90u, label, 1u, theme->color_text);
    ops->draw_text(x + 10u, y + 104u, subtitle, 1u, ops->mix_color(theme->color_text_soft, accent_color, 1u, 6u));
    if (selected != 0u || hovered != 0u) {
        ops->fill_rect(x + 10u, y + 75u, 18u, 3u, hovered != 0u ? theme->color_accent : theme->color_selection);
    }
}

void desktop_stage_draw_icons(
    const shell_state_t* state,
    const desktop_animation_state_t* animation,
    const desktop_stage_theme_t* theme,
    const desktop_stage_ops_t* ops
) {
    const u32 icon_count = desktop_entry_count();
    for (u32 index = 0; index < icon_count; ++index) {
        const u32 hover_lift = state->cursor_x != 0u ? animation->pulse_fast / 9u : 0u;
        const u32 y = 80u + index * 124u;
        const ui_rect_t rect = desktop_icon_rect(index);
        const u32 selected = state->focus == FOCUS_DESKTOP && state->desktop_index == index;
        const u32 hovered = point_in_rect(state->cursor_x, state->cursor_y, &rect);
        const shell_view_t view = desktop_entry_view(index);
        const desktop_domain_descriptor_t* descriptor = desktop_domain_descriptor(view);
        draw_desktop_icon(
            22u + (hovered != 0u ? 1u : 0u),
            y - (hovered != 0u ? hover_lift : (selected != 0u ? 1u : 0u)),
            selected,
            hovered,
            (u32)view,
            descriptor->accent_color,
            descriptor->label,
            descriptor->subtitle,
            descriptor->badge_label,
            theme,
            ops
        );
        if (selected != 0u || hovered != 0u) {
            ops->fill_rect(
                28u,
                y + 114u + (hovered != 0u ? 0u : 1u),
                80u,
                2u,
                hovered != 0u
                    ? ops->mix_color(theme->color_black, theme->color_glow, 1u + animation->pulse_fast / 5u, 10u)
                    : ops->mix_color(theme->color_black, theme->color_selection, 1u + animation->pulse_slow / 9u, 12u)
            );
        }
    }
}

void desktop_stage_draw_cursor(
    const desktop_stage_cursor_t* cursor,
    const desktop_animation_state_t* animation,
    const desktop_stage_theme_t* theme,
    const desktop_stage_ops_t* ops
) {
    static const unsigned short shape[14] = {
        0x0001, 0x0003, 0x0007, 0x000F,
        0x001F, 0x0037, 0x0063, 0x00C1,
        0x0181, 0x0301, 0x0601, 0x0403,
        0x0006, 0x0004
    };
    for (u32 row = 0; row < 14u; ++row) {
        for (u32 col = 0; col < 10u; ++col) {
            if ((shape[row] & (1u << col)) == 0u) {
                continue;
            }
            const u32 px = cursor->x + col;
            const u32 py = cursor->y + row;
            if (cursor->hot != 0u && px + 1u < cursor->desktop_width && py + 1u < cursor->desktop_height) {
                ops->put_pixel(
                    px + 1u,
                    py + 1u,
                    animation->cursor_pulse > 5u ? theme->color_glow : theme->color_accent
                );
            }
            ops->put_pixel(px, py, theme->color_text);
            if (px > 0u && py > 0u) {
                ops->put_pixel(px - 1u, py, theme->color_black);
                ops->put_pixel(px, py - 1u, theme->color_black);
            }
        }
    }
}
