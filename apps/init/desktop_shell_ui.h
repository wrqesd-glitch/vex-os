#ifndef VEX_INIT_DESKTOP_SHELL_UI_H
#define VEX_INIT_DESKTOP_SHELL_UI_H

#include "desktop_animation.h"
#include "desktop_rtc.h"
#include "desktop_session.h"

typedef struct desktop_shell_ui_theme {
    u32 color_taskbar;
    u32 color_taskbar_edge;
    u32 color_taskbar_hilite;
    u32 color_start;
    u32 color_start_active;
    u32 color_panel_alt;
    u32 color_window_border;
    u32 color_accent;
    u32 color_text;
    u32 color_text_dim;
    u32 color_text_soft;
    u32 color_menu;
    u32 color_menu_header;
    u32 color_menu_active;
    u32 color_selection;
    u32 color_warning;
    u32 color_glow;
} desktop_shell_ui_theme_t;

typedef void (*desktop_shell_fill_rect_fn)(u32 x, u32 y, u32 width, u32 height, u32 color);
typedef void (*desktop_shell_draw_frame_fn)(u32 x, u32 y, u32 width, u32 height, u32 border, u32 fill);
typedef void (*desktop_shell_draw_shadow_fn)(u32 x, u32 y, u32 width, u32 height);
typedef void (*desktop_shell_draw_text_fn)(u32 x, u32 y, const char* text, u32 scale, u32 color);
typedef u32 (*desktop_shell_mix_color_fn)(u32 a, u32 b, u32 numerator, u32 denominator);

typedef struct desktop_shell_ui_ops {
    desktop_shell_fill_rect_fn fill_rect;
    desktop_shell_draw_frame_fn draw_frame;
    desktop_shell_draw_shadow_fn draw_shadow;
    desktop_shell_draw_text_fn draw_text;
    desktop_shell_mix_color_fn mix_color;
} desktop_shell_ui_ops_t;

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
);

#endif
