#ifndef VEX_INIT_DESKTOP_VIEWS_H
#define VEX_INIT_DESKTOP_VIEWS_H

#include "desktop_animation.h"
#include "desktop_layout.h"
#include "desktop_rtc.h"

typedef unsigned long long u64;

typedef struct desktop_boot_metrics {
    u64 rsdp_address;
    u64 framebuffer_base;
    u64 framebuffer_size;
    u32 framebuffer_width;
    u32 framebuffer_height;
    u32 framebuffer_pitch;
    u32 abi_revision;
    u64 memory_map_size;
    u64 memory_descriptor_size;
} desktop_boot_metrics_t;

typedef struct desktop_view_theme {
    u32 color_accent;
    u32 color_window_border;
    u32 color_window;
    u32 color_window_alt;
    u32 color_window_title;
    u32 color_window_title_active;
    u32 color_text;
    u32 color_text_dim;
    u32 color_text_soft;
    u32 color_selection;
    u32 color_panel;
    u32 color_menu_active;
    u32 color_ready;
    u32 color_warning;
    u32 color_pass;
    u32 color_glow;
} desktop_view_theme_t;

typedef void (*desktop_fill_rect_fn)(u32 x, u32 y, u32 width, u32 height, u32 color);
typedef void (*desktop_draw_frame_fn)(u32 x, u32 y, u32 width, u32 height, u32 border, u32 fill);
typedef void (*desktop_draw_shadow_fn)(u32 x, u32 y, u32 width, u32 height);
typedef void (*desktop_draw_glow_rect_fn)(u32 x, u32 y, u32 width, u32 height, u32 color);
typedef void (*desktop_draw_text_fn)(u32 x, u32 y, const char* text, u32 scale, u32 color);
typedef void (*desktop_draw_metric_fn)(u32 x, u32 y, const char* label, u64 value, u32 hex_mode);
typedef void (*desktop_draw_badge_fn)(u32 x, u32 y, const char* text, u32 fill);
typedef u32 (*desktop_mix_color_fn)(u32 a, u32 b, u32 numerator, u32 denominator);

typedef struct desktop_view_ops {
    desktop_fill_rect_fn fill_rect;
    desktop_draw_frame_fn draw_frame;
    desktop_draw_shadow_fn draw_shadow;
    desktop_draw_glow_rect_fn draw_glow_rect;
    desktop_draw_text_fn draw_text;
    desktop_draw_metric_fn draw_metric;
    desktop_draw_badge_fn draw_badge;
    desktop_mix_color_fn mix_color;
} desktop_view_ops_t;

void desktop_views_render_active_windows(
    const shell_state_t* state,
    const shell_window_t* windows,
    const u32* order,
    u32 window_count,
    const desktop_boot_metrics_t* boot,
    const desktop_animation_state_t* animation,
    const desktop_view_theme_t* theme,
    const desktop_view_ops_t* ops
);
void desktop_views_render_window(
    const shell_state_t* state,
    const shell_window_t* windows,
    u32 slot,
    const desktop_boot_metrics_t* boot,
    const desktop_animation_state_t* animation,
    const desktop_view_theme_t* theme,
    const desktop_view_ops_t* ops
);

#endif
