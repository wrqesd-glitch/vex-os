#ifndef VEX_INIT_DESKTOP_STAGE_H
#define VEX_INIT_DESKTOP_STAGE_H

#include "desktop_animation.h"
#include "desktop_layout.h"

typedef unsigned long long u64;

typedef void (*desktop_stage_fill_rect_fn)(u32 x, u32 y, u32 width, u32 height, u32 color);
typedef void (*desktop_stage_put_pixel_fn)(u32 x, u32 y, u32 color);
typedef void (*desktop_stage_copy_row_fn)(u32* dst, const u32* src, u32 count);
typedef void (*desktop_stage_draw_frame_fn)(u32 x, u32 y, u32 width, u32 height, u32 border, u32 fill);
typedef void (*desktop_stage_draw_text_fn)(u32 x, u32 y, const char* text, u32 scale, u32 color);
typedef u32 (*desktop_stage_mix_color_fn)(u32 a, u32 b, u32 numerator, u32 denominator);

typedef struct desktop_stage_theme {
    u32 color_selection;
    u32 color_accent;
    u32 color_window_border;
    u32 color_icon_bg;
    u32 color_icon_active;
    u32 color_glow;
    u32 color_text;
    u32 color_text_soft;
    u32 color_black;
    u32 fallback_wallpaper;
} desktop_stage_theme_t;

typedef struct desktop_stage_ops {
    desktop_stage_fill_rect_fn fill_rect;
    desktop_stage_put_pixel_fn put_pixel;
    desktop_stage_copy_row_fn copy_row;
    desktop_stage_draw_frame_fn draw_frame;
    desktop_stage_draw_text_fn draw_text;
    desktop_stage_mix_color_fn mix_color;
} desktop_stage_ops_t;

typedef struct desktop_stage_cursor {
    u32 x;
    u32 y;
    u32 hot;
    u32 desktop_width;
    u32 desktop_height;
} desktop_stage_cursor_t;

void desktop_stage_draw_wallpaper(
    u32 desktop_width,
    u32 desktop_height,
    u32 surface_pitch,
    u32* surface_pixels,
    const desktop_animation_state_t* animation,
    const desktop_stage_theme_t* theme,
    const desktop_stage_ops_t* ops
);
void desktop_stage_draw_icons(
    const shell_state_t* state,
    const desktop_animation_state_t* animation,
    const desktop_stage_theme_t* theme,
    const desktop_stage_ops_t* ops
);
void desktop_stage_draw_cursor(
    const desktop_stage_cursor_t* cursor,
    const desktop_animation_state_t* animation,
    const desktop_stage_theme_t* theme,
    const desktop_stage_ops_t* ops
);

#endif
