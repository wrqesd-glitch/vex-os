#define FB_BASE 0x50000000ull
#define BOOTINFO_BASE 0x50400000ull

#include "desktop_catalog.h"
#include "desktop_animation.h"
#include "desktop_app_runtime.h"
#include "desktop_domain.h"
#include "desktop_input.h"
#include "desktop_launcher.h"
#include "desktop_layout.h"
#include "desktop_pointer.h"
#include "desktop_ps2.h"
#include "desktop_registry.h"
#include "desktop_rtc.h"
#include "desktop_runtime.h"
#include "desktop_shell_ui.h"
#include "desktop_session.h"
#include "desktop_stage.h"
#include "desktop_views.h"
#include "vex_boot_info.h"
#include "vex_ui_proto.h"

typedef unsigned short u16;

enum {
    COLOR_WALLPAPER_GLOW = 0x0057B4FF,
    COLOR_TASKBAR = 0x000D1118,
    COLOR_TASKBAR_EDGE = 0x004A8DFF,
    COLOR_TASKBAR_HILITE = 0x001A2A42,
    COLOR_START = 0x001D4F97,
    COLOR_START_ACTIVE = 0x003C88F7,
    COLOR_PANEL = 0x00101822,
    COLOR_PANEL_ALT = 0x00162131,
    COLOR_WINDOW = 0x00121924,
    COLOR_WINDOW_ALT = 0x00172434,
    COLOR_WINDOW_BORDER = 0x00395078,
    COLOR_WINDOW_TITLE = 0x0019274B,
    COLOR_WINDOW_TITLE_ACTIVE = 0x002E5FB3,
    COLOR_MENU = 0x000F1622,
    COLOR_MENU_HEADER = 0x001A315A,
    COLOR_MENU_ACTIVE = 0x002A568F,
    COLOR_ICON_BG = 0x0014202F,
    COLOR_ICON_ACTIVE = 0x0026467A,
    COLOR_SELECTION = 0x004B93FF,
    COLOR_TEXT = 0x00F1F6FF,
    COLOR_TEXT_DIM = 0x00C1CFE5,
    COLOR_TEXT_SOFT = 0x0093A8C7,
    COLOR_PASS = 0x0041B76D,
    COLOR_READY = 0x004289F2,
    COLOR_WARNING = 0x00C88B2D,
    COLOR_ACCENT = 0x005494FF,
    COLOR_BLACK = 0x00000000,
    GLYPH_W = 5u,
    GLYPH_H = 7u,
    SWAPCHAIN_BUFFER_COUNT = 3u,
    SWAPCHAIN_MAX_WIDTH = 1280u,
    SWAPCHAIN_MAX_HEIGHT = 800u,
    SWAPCHAIN_MAX_PIXELS = SWAPCHAIN_MAX_WIDTH * SWAPCHAIN_MAX_HEIGHT,
    COMPOSITOR_WINDOW_COUNT = 5u,
    COMPOSITOR_SURFACE_PAD = 12u,
    COMPOSITOR_SURFACE_MAX_WIDTH = 1152u,
    COMPOSITOR_SURFACE_MAX_HEIGHT = 736u,
    COMPOSITOR_SURFACE_MAX_PIXELS = COMPOSITOR_SURFACE_MAX_WIDTH * COMPOSITOR_SURFACE_MAX_HEIGHT
};

typedef struct desktop_compositor_surface {
    u32 active;
    u32 width;
    u32 height;
    u32 pitch;
    u32 origin_x;
    u32 origin_y;
    u64 present_sequence;
    vex_compositor_packet_t packet;
} desktop_compositor_surface_t;

typedef struct desktop_remote_surface_route {
    u32 available;
    u32 width;
    u32 height;
    u32 stride;
    u32 buffer_count;
    u32 present_index;
    u32 surface_handle;
    u32 fence_handle;
    u64 shared_mapping_base;
    u64 shared_mailbox_base;
    u64 bytes_per_buffer;
} desktop_remote_surface_route_t;

static volatile vex_boot_info_t* const g_boot = (volatile vex_boot_info_t*)BOOTINFO_BASE;
static volatile u32* const g_pixels = (volatile u32*)FB_BASE;
static shell_state_t g_shell = {
    .active_view = VIEW_HUB,
    .launch_view = VIEW_HUB,
    .focus = FOCUS_DESKTOP,
    .start_open = 0u,
    .start_visible = 0u,
    .start_animating = 0u,
    .start_animation_frame = 0u,
    .start_render_height = 0u,
    .launch_busy = 0u,
    .launch_action = 0u,
    .launch_progress = 0u,
    .window_open = 0u,
    .desktop_index = 0u,
    .hub_index = 0u,
    .taskbar_index = 1u,
    .start_index = 0u,
    .cursor_x = 240u,
    .cursor_y = 180u,
    .mouse_left_down = 0u,
    .window_x = 180u,
    .window_y = 86u,
    .window_width = 0u,
    .window_height = 0u,
    .dragging_window = 0u,
    .drag_offset_x = 0u,
    .drag_offset_y = 0u,
    .dirty = 1u
};
static u32 g_extended_prefix;
static u8 g_mouse_packet[3];
static u32 g_mouse_index;
static shell_window_t g_windows[5];
static u32 g_window_order[5] = {0u, 1u, 2u, 3u, 4u};
static u32 g_swapchain_storage[SWAPCHAIN_BUFFER_COUNT][SWAPCHAIN_MAX_PIXELS];
static u32 g_window_surface_storage[COMPOSITOR_WINDOW_COUNT][COMPOSITOR_SURFACE_MAX_PIXELS];
static desktop_compositor_surface_t g_window_surfaces[COMPOSITOR_WINDOW_COUNT];
static u32* g_draw_pixels;
static u32 g_draw_pitch;
static u32 g_draw_width;
static u32 g_draw_height;
static u32 g_draw_origin_x;
static u32 g_draw_origin_y;
static u32 g_front_buffer_index;
static u32 g_ready_buffer_index = 1u;
static u32 g_draw_buffer_index = 2u;
static desktop_rtc_state_t g_clock;
static desktop_animation_state_t g_animation;
static desktop_runtime_state_t g_runtime;
static u32 g_session_engaged[5];
static desktop_remote_surface_route_t g_remote_surfaces[5];

static u32 fb_width(void);
static u32 fb_height(void);
static void draw_text_scaled(u32 x, u32 y, const char* text, u32 scale, u32 color);
static void open_view(shell_view_t view);
static void request_open_view(shell_view_t view);
static void activate_desktop_entry(u32 index);
static void activate_start_entry(u32 index);
static void set_render_target(u32* pixels, u32 pitch, u32 width, u32 height, u32 origin_x, u32 origin_y);
static void init_remote_surface_routes(void);

static const volatile vex_compositor_scene_entry_t* compositor_scene_entry_for_slot(u32 slot) {
    const volatile vex_compositor_scene_mailbox_t* scene;
    const u32 scene_index = slot > 0u ? slot - 1u : 0u;
    if (slot == 0u || slot >= 5u || g_boot->compositor_scene_mailbox_base == 0u) {
        return 0;
    }
    scene = (const volatile vex_compositor_scene_mailbox_t*)(u64)g_boot->compositor_scene_mailbox_base;
    if (scene->magic != VEX_COMPOSITOR_SCENE_MAGIC ||
        scene->abi_version != VEX_COMPOSITOR_ABI_VERSION ||
        scene_index >= scene->entry_count ||
        scene_index >= VEX_COMPOSITOR_SCENE_SLOTS) {
        return 0;
    }
    return &scene->entries[scene_index];
}

static void sync_registry_sessions(void) {
    for (u32 slot = 0; slot < 5u; ++slot) {
        const u32 open = g_windows[slot].open != 0u || g_windows[slot].minimized != 0u;
        const u32 active = g_windows[slot].open != 0u && (u32)g_shell.active_view == slot;
        desktop_registry_sync_window((shell_view_t)slot, open, active);
    }
}

static void init_remote_surface_routes(void) {
    for (u32 slot = 0u; slot < 5u; ++slot) {
        g_remote_surfaces[slot].available = 0u;
    }
    for (u32 index = 0u; index < VEX_MAX_SHARED_SURFACES; ++index) {
        const volatile vex_shared_surface_info_t* info = &g_boot->shared_surfaces[index];
        const u32 slot = index + 1u;
        if (slot >= 5u || info->shared_mapping_base == 0u || info->width == 0u || info->height == 0u) {
            continue;
        }
        g_remote_surfaces[slot].available = 1u;
        g_remote_surfaces[slot].width = info->width;
        g_remote_surfaces[slot].height = info->height;
        g_remote_surfaces[slot].stride = info->stride;
        g_remote_surfaces[slot].buffer_count = info->buffer_count;
        g_remote_surfaces[slot].present_index = info->present_index;
        g_remote_surfaces[slot].surface_handle = info->surface_handle;
        g_remote_surfaces[slot].fence_handle = info->fence_handle;
        g_remote_surfaces[slot].shared_mapping_base = info->shared_mapping_base;
        g_remote_surfaces[slot].shared_mailbox_base = info->shared_mailbox_base;
        g_remote_surfaces[slot].bytes_per_buffer = info->bytes_per_buffer;
    }
}

static desktop_boot_metrics_t current_boot_metrics(void) {
    desktop_boot_metrics_t boot = {
        .rsdp_address = g_boot->rsdp_address,
        .framebuffer_base = g_boot->framebuffer.base,
        .framebuffer_size = g_boot->framebuffer.size,
        .framebuffer_width = g_boot->framebuffer.width,
        .framebuffer_height = g_boot->framebuffer.height,
        .framebuffer_pitch = g_boot->framebuffer.pixels_per_scanline,
        .abi_revision = g_boot->revision,
        .memory_map_size = g_boot->memory_map_size,
        .memory_descriptor_size = g_boot->memory_descriptor_size
    };
    return boot;
}

static void sync_app_sessions(void) {
    for (u32 slot = 0; slot < 5u; ++slot) {
        const u32 engaged = g_windows[slot].open != 0u || g_windows[slot].minimized != 0u;
        if (engaged == g_session_engaged[slot]) {
            continue;
        }
        g_session_engaged[slot] = engaged;
        desktop_app_runtime_sync_view((shell_view_t)slot, engaged, desktop_domain_descriptor((shell_view_t)slot));
        g_shell.dirty = 1u;
    }
}

static void install_view_and_mark_dirty(shell_view_t view) {
    install_view(view);
    g_shell.dirty = 1u;
}

static void request_install_view(shell_view_t view) {
    desktop_session_set_start_open(&g_shell, 0u);
    if (view_is_installable(view) == 0u) {
        g_shell.dirty = 1u;
        return;
    }
    (void)desktop_runtime_request_install(&g_runtime, &g_shell, view, view_is_installed);
}

static void normalize_shell_state(void) {
    const u32 desktop_count = desktop_entry_count();
    const u32 start_count = start_entry_count();

    if (desktop_count == 0u) {
        g_shell.desktop_index = 0u;
    } else if (g_shell.desktop_index >= desktop_count) {
        g_shell.desktop_index = desktop_count - 1u;
    }

    if (start_count == 0u) {
        g_shell.start_index = 0u;
    } else if (g_shell.start_index >= start_count) {
        g_shell.start_index = start_count - 1u;
    }

    if (g_shell.hub_index >= hub_card_count()) {
        g_shell.hub_index = 0u;
    }

    if ((u32)g_shell.active_view >= 5u || view_is_installed(g_shell.active_view) == 0u) {
        g_shell.active_view = VIEW_HUB;
    }
    if (g_shell.taskbar_index >= taskbar_entry_count()) {
        g_shell.taskbar_index = 0u;
    }
}

static u32 any_open_windows_wrapper(void) {
    return desktop_session_any_open_windows(g_windows, 5u);
}

static u32 rgb(u32 r, u32 g, u32 b) {
    return (r << 16) | (g << 8) | b;
}

static u32 mix_color(u32 a, u32 b, u32 numerator, u32 denominator) {
    const u32 ar = (a >> 16) & 0xFFu;
    const u32 ag = (a >> 8) & 0xFFu;
    const u32 ab = a & 0xFFu;
    const u32 br = (b >> 16) & 0xFFu;
    const u32 bg = (b >> 8) & 0xFFu;
    const u32 bb = b & 0xFFu;
    const u32 rr = (ar * (denominator - numerator) + br * numerator) / denominator;
    const u32 rg = (ag * (denominator - numerator) + bg * numerator) / denominator;
    const u32 rb = (ab * (denominator - numerator) + bb * numerator) / denominator;
    return rgb(rr, rg, rb);
}

static u32 fb_width(void) {
    return g_boot->framebuffer.width;
}

static u32 fb_height(void) {
    return g_boot->framebuffer.height;
}

static u32 fb_pitch(void) {
    return g_boot->framebuffer.pixels_per_scanline;
}

static u32 surface_pitch(void) {
    return g_draw_pitch;
}

static u32 surface_width(void) {
    return g_draw_width;
}

static u32 surface_height(void) {
    return g_draw_height;
}

static void set_render_target(u32* pixels, u32 pitch, u32 width, u32 height, u32 origin_x, u32 origin_y) {
    g_draw_pixels = pixels;
    g_draw_pitch = pitch;
    g_draw_width = width;
    g_draw_height = height;
    g_draw_origin_x = origin_x;
    g_draw_origin_y = origin_y;
}

static void put_pixel(u32 x, u32 y, u32 color) {
    if (x >= surface_width() || y >= surface_height()) {
        return;
    }
    g_draw_pixels[(u64)(y + g_draw_origin_y) * surface_pitch() + (x + g_draw_origin_x)] = color;
}

static void fill_rect(u32 x, u32 y, u32 width, u32 height, u32 color) {
    const u32 max_x = x + width > surface_width() ? surface_width() : x + width;
    const u32 max_y = y + height > surface_height() ? surface_height() : y + height;
    for (u32 yy = y; yy < max_y; ++yy) {
        for (u32 xx = x; xx < max_x; ++xx) {
            put_pixel(xx, yy, color);
        }
    }
}

static void copy_pixel_row(u32* dst, const u32* src, u32 count) {
    for (u32 index = 0; index < count; ++index) {
        dst[index] = src[index];
    }
}

static void draw_hline(u32 x, u32 y, u32 width, u32 color) {
    fill_rect(x, y, width, 1u, color);
}

static void draw_vline(u32 x, u32 y, u32 height, u32 color) {
    fill_rect(x, y, 1u, height, color);
}

static void draw_frame(u32 x, u32 y, u32 width, u32 height, u32 border, u32 fill) {
    if (width < 2u || height < 2u) {
        return;
    }
    fill_rect(x, y, width, height, fill);
    draw_hline(x, y, width, border);
    draw_hline(x, y + height - 1u, width, border);
    draw_vline(x, y, height, border);
    draw_vline(x + width - 1u, y, height, border);
    if (width > 4u && height > 4u) {
        const u32 inner_hi = mix_color(fill, COLOR_TEXT, 1u, 10u);
        const u32 inner_lo = mix_color(fill, COLOR_BLACK, 1u, 3u);
        draw_hline(x + 1u, y + 1u, width - 2u, inner_hi);
        draw_vline(x + 1u, y + 1u, height - 2u, inner_hi);
        draw_hline(x + 1u, y + height - 2u, width - 2u, inner_lo);
        draw_vline(x + width - 2u, y + 1u, height - 2u, inner_lo);
    }
}

static void draw_shadow(u32 x, u32 y, u32 width, u32 height) {
    fill_rect(x + 4u, y + 4u, width, height, 0x00060A12);
    fill_rect(x + 8u, y + 8u, width, height, 0x00040910);
    fill_rect(x + 12u, y + 12u, width, height, 0x0002060C);
}

static void draw_glow_rect(u32 x, u32 y, u32 width, u32 height, u32 color) {
    fill_rect(x, y, width, 1u, color);
    fill_rect(x, y + height - 1u, width, 1u, color);
    fill_rect(x, y, 1u, height, color);
    fill_rect(x + width - 1u, y, 1u, height, color);
    if (width > 4u && height > 4u) {
        const u32 outer = mix_color(color, COLOR_BLACK, 1u, 4u);
        fill_rect(x + 1u, y + 1u, width - 2u, 1u, outer);
        fill_rect(x + 1u, y + height - 2u, width - 2u, 1u, outer);
        fill_rect(x + 1u, y + 1u, 1u, height - 2u, outer);
        fill_rect(x + width - 2u, y + 1u, 1u, height - 2u, outer);
    }
}

static void swapchain_init(void) {
    g_front_buffer_index = 0u;
    g_ready_buffer_index = 1u;
    g_draw_buffer_index = 2u;
    set_render_target(g_swapchain_storage[g_draw_buffer_index], fb_pitch(), fb_width(), fb_height(), 0u, 0u);
    for (u32 slot = 0u; slot < COMPOSITOR_WINDOW_COUNT; ++slot) {
        g_window_surfaces[slot].active = 0u;
        g_window_surfaces[slot].present_sequence = 0u;
    }
}

static void begin_frame(void) {
    set_render_target(g_swapchain_storage[g_draw_buffer_index], fb_pitch(), fb_width(), fb_height(), 0u, 0u);
}

static void present_frame(void) {
    volatile u32* const front_pixels = g_pixels;
    const u32 visible_pitch = fb_pitch();
    const u32 visible_height = fb_height();
    const u32* const ready_pixels = g_swapchain_storage[g_draw_buffer_index];

    for (u32 y = 0; y < visible_height; ++y) {
        volatile u32* const dst = &front_pixels[(u64)y * visible_pitch];
        const u32* const src = &ready_pixels[(u64)y * g_draw_pitch];
        for (u32 x = 0; x < visible_pitch; ++x) {
            dst[x] = src[x];
        }
    }

    const u32 previous_front = g_front_buffer_index;
    g_front_buffer_index = g_draw_buffer_index;
    g_draw_buffer_index = g_ready_buffer_index;
    g_ready_buffer_index = previous_front;
}

static const u8* glyph(char c) {
    static const u8 blank[7] = {0, 0, 0, 0, 0, 0, 0};
    static const u8 dash[7] = {0, 0, 0, 14, 0, 0, 0};
    static const u8 dot[7] = {0, 0, 0, 0, 0, 12, 12};
    static const u8 colon[7] = {0, 12, 12, 0, 12, 12, 0};
    static const u8 slash[7] = {1, 2, 4, 4, 8, 16, 16};
    static const u8 greater[7] = {16, 8, 4, 2, 4, 8, 16};
    static const u8 d0[7] = {14, 17, 19, 21, 25, 17, 14};
    static const u8 d1[7] = {4, 12, 4, 4, 4, 4, 14};
    static const u8 d2[7] = {14, 17, 1, 2, 4, 8, 31};
    static const u8 d3[7] = {30, 1, 1, 14, 1, 1, 30};
    static const u8 d4[7] = {2, 6, 10, 18, 31, 2, 2};
    static const u8 d5[7] = {31, 16, 16, 30, 1, 1, 30};
    static const u8 d6[7] = {6, 8, 16, 30, 17, 17, 14};
    static const u8 d7[7] = {31, 1, 2, 4, 8, 8, 8};
    static const u8 d8[7] = {14, 17, 17, 14, 17, 17, 14};
    static const u8 d9[7] = {14, 17, 17, 15, 1, 2, 28};
    static const u8 A[7] = {14, 17, 17, 31, 17, 17, 17};
    static const u8 B[7] = {30, 17, 17, 30, 17, 17, 30};
    static const u8 C[7] = {14, 17, 16, 16, 16, 17, 14};
    static const u8 D[7] = {28, 18, 17, 17, 17, 18, 28};
    static const u8 E[7] = {31, 16, 16, 30, 16, 16, 31};
    static const u8 F[7] = {31, 16, 16, 30, 16, 16, 16};
    static const u8 G[7] = {14, 17, 16, 23, 17, 17, 14};
    static const u8 H[7] = {17, 17, 17, 31, 17, 17, 17};
    static const u8 I[7] = {14, 4, 4, 4, 4, 4, 14};
    static const u8 J[7] = {1, 1, 1, 1, 17, 17, 14};
    static const u8 K[7] = {17, 18, 20, 24, 20, 18, 17};
    static const u8 L[7] = {16, 16, 16, 16, 16, 16, 31};
    static const u8 M[7] = {17, 27, 21, 21, 17, 17, 17};
    static const u8 N[7] = {17, 25, 21, 19, 17, 17, 17};
    static const u8 O[7] = {14, 17, 17, 17, 17, 17, 14};
    static const u8 P[7] = {30, 17, 17, 30, 16, 16, 16};
    static const u8 Q[7] = {14, 17, 17, 17, 21, 18, 13};
    static const u8 R[7] = {30, 17, 17, 30, 20, 18, 17};
    static const u8 S[7] = {15, 16, 16, 14, 1, 1, 30};
    static const u8 T[7] = {31, 4, 4, 4, 4, 4, 4};
    static const u8 U[7] = {17, 17, 17, 17, 17, 17, 14};
    static const u8 V[7] = {17, 17, 17, 17, 17, 10, 4};
    static const u8 W[7] = {17, 17, 17, 21, 21, 21, 10};
    static const u8 X[7] = {17, 17, 10, 4, 10, 17, 17};
    static const u8 Y[7] = {17, 17, 10, 4, 4, 4, 4};
    static const u8 Z[7] = {31, 1, 2, 4, 8, 16, 31};

    switch (c) {
    case ' ': return blank;
    case '-': return dash;
    case '.': return dot;
    case ':': return colon;
    case '/': return slash;
    case '>': return greater;
    case '0': return d0;
    case '1': return d1;
    case '2': return d2;
    case '3': return d3;
    case '4': return d4;
    case '5': return d5;
    case '6': return d6;
    case '7': return d7;
    case '8': return d8;
    case '9': return d9;
    case 'A': return A;
    case 'B': return B;
    case 'C': return C;
    case 'D': return D;
    case 'E': return E;
    case 'F': return F;
    case 'G': return G;
    case 'H': return H;
    case 'I': return I;
    case 'J': return J;
    case 'K': return K;
    case 'L': return L;
    case 'M': return M;
    case 'N': return N;
    case 'O': return O;
    case 'P': return P;
    case 'Q': return Q;
    case 'R': return R;
    case 'S': return S;
    case 'T': return T;
    case 'U': return U;
    case 'V': return V;
    case 'W': return W;
    case 'X': return X;
    case 'Y': return Y;
    case 'Z': return Z;
    default: return blank;
    }
}

static void draw_char_scaled(u32 x, u32 y, char c, u32 scale, u32 color) {
    const u8* data = glyph(c);
    for (u32 gy = 0; gy < GLYPH_H; ++gy) {
        for (u32 gx = 0; gx < GLYPH_W; ++gx) {
            if ((data[gy] & (1u << (GLYPH_W - 1u - gx))) == 0u) {
                continue;
            }
            fill_rect(x + gx * scale, y + gy * scale, scale, scale, color);
        }
    }
}

static void draw_text_scaled(u32 x, u32 y, const char* text, u32 scale, u32 color) {
    u32 cursor = x;
    while (*text != 0) {
        draw_char_scaled(cursor, y, *text, scale, color);
        cursor += (GLYPH_W + 2u) * scale;
        ++text;
    }
}

static void format_u64(u64 value, char* out) {
    char digits[32];
    u32 count = 0;
    if (value == 0u) {
        out[0] = '0';
        out[1] = 0;
        return;
    }
    while (value > 0u) {
        digits[count++] = (char)('0' + (value % 10u));
        value /= 10u;
    }
    for (u32 i = 0; i < count; ++i) {
        out[i] = digits[count - 1u - i];
    }
    out[count] = 0;
}

static void format_hex(u64 value, char* out) {
    static const char hex[] = "0123456789ABCDEF";
    out[0] = '0';
    out[1] = 'X';
    for (u32 i = 0; i < 16u; ++i) {
        out[2u + i] = hex[(value >> ((15u - i) * 4u)) & 0xFu];
    }
    out[18] = 0;
}

static void draw_metric_line(u32 x, u32 y, const char* label, u64 value, u32 hex_mode) {
    char buffer[24];
    if (hex_mode != 0u) {
        format_hex(value, buffer);
    } else {
        format_u64(value, buffer);
    }
    draw_text_scaled(x, y, label, 1u, COLOR_TEXT_DIM);
    draw_text_scaled(x + 144u, y, buffer, 1u, COLOR_TEXT);
}

static void draw_badge(u32 x, u32 y, const char* text, u32 fill) {
    draw_frame(x, y, 74u, 22u, fill, fill);
    draw_text_scaled(x + 8u, y + 7u, text, 1u, COLOR_TEXT);
}

static void init_window_layout(void) {
    if (g_shell.window_width != 0u && g_shell.window_height != 0u) {
        return;
    }
    desktop_session_init_windows(&g_shell, g_windows, g_window_order, 5u, fb_width(), fb_height());
}

static u32 desktop_icon_at(u32 x, u32 y) {
    const u32 count = desktop_entry_count();
    for (u32 index = 0; index < count; ++index) {
        const ui_rect_t rect = desktop_icon_rect(index);
        if (point_in_rect(x, y, &rect) != 0u) {
            return index + 1u;
        }
    }
    return 0u;
}

static u32 taskbar_button_at(u32 x, u32 y) {
    const u32 count = taskbar_entry_count();
    for (u32 index = 0; index < count; ++index) {
        const ui_rect_t rect = taskbar_button_rect(index, fb_height());
        if (point_in_rect(x, y, &rect) != 0u) {
            return index + 1u;
        }
    }
    return 0u;
}

static u32 start_item_at(u32 x, u32 y) {
    if (g_shell.start_visible == 0u) {
        return 0u;
    }
    const u32 count = start_entry_count();
    for (u32 index = 0; index < count; ++index) {
        const ui_rect_t rect = start_item_rect(index, fb_height());
        if (point_in_rect(x, y, &rect) != 0u) {
            return index + 1u;
        }
    }
    return 0u;
}

static u32 explorer_card_at(u32 x, u32 y) {
    if (g_shell.window_open == 0u || g_shell.active_view != VIEW_HUB) {
        return 0u;
    }
    for (u32 row = 0; row < 2u; ++row) {
        for (u32 col = 0; col < 2u; ++col) {
            const ui_rect_t rect = explorer_card_rect(&g_shell, row, col);
            if (point_in_rect(x, y, &rect) != 0u) {
                return row * 2u + col + 1u;
            }
        }
    }
    return 0u;
}

static u32 start_menu_at(u32 x, u32 y) {
    ui_rect_t menu = start_menu_rect(fb_height());
    if (g_shell.start_visible == 0u) {
        return 0u;
    }
    menu.y += 344u - g_shell.start_render_height;
    menu.height = g_shell.start_render_height;
    return point_in_rect(x, y, &menu);
}

static u32 window_close_at(u32 x, u32 y) {
    const ui_rect_t rect = window_close_rect(&g_shell);
    return point_in_rect(x, y, &rect);
}

static u32 window_minimize_at(u32 x, u32 y) {
    const ui_rect_t rect = window_minimize_rect(&g_shell);
    return point_in_rect(x, y, &rect);
}

static u32 window_title_at(u32 x, u32 y) {
    const ui_rect_t rect = window_title_rect(&g_shell);
    return point_in_rect(x, y, &rect);
}

static u32 window_slot_at(u32 x, u32 y) {
    for (u32 i = 0; i < 5u; ++i) {
        const u32 slot = g_window_order[4u - i];
        if (g_windows[slot].render_visible == 0u) {
            continue;
        }
        {
            const ui_rect_t window = window_rect_for_slot(g_windows, slot);
            if (point_in_rect(x, y, &window) != 0u) {
                return slot + 1u;
            }
        }
    }
    return 0u;
}

static void activate_hub_card(u32 index) {
    g_shell.hub_index = index;
    g_shell.focus = FOCUS_WINDOW;
    g_shell.dirty = 1u;
    launcher_activate_hub_card(index, view_is_installed, request_install_view, request_open_view);
}

static void activate_window_slot(u32 slot) {
    desktop_session_activate_window_slot(&g_shell, g_windows, g_window_order, 5u, slot);
}

static desktop_pointer_routes_t pointer_routes(void) {
    desktop_pointer_routes_t routes;
    routes.desktop_icon_hit = desktop_icon_at;
    routes.taskbar_hit = taskbar_button_at;
    routes.start_item_hit = start_item_at;
    routes.hub_card_hit = explorer_card_at;
    routes.window_slot_hit = window_slot_at;
    routes.start_menu_hit = start_menu_at;
    routes.window_close_hit = window_close_at;
    routes.window_minimize_hit = window_minimize_at;
    routes.window_title_hit = window_title_at;
    routes.open_view = request_open_view;
    routes.activate_desktop_entry = activate_desktop_entry;
    routes.activate_start_entry = activate_start_entry;
    routes.activate_hub_card = activate_hub_card;
    routes.activate_window_slot = activate_window_slot;
    routes.framebuffer_width = fb_width;
    routes.framebuffer_height = fb_height;
    return routes;
}

static void open_view(shell_view_t view) {
    if (view_is_installed(view) == 0u) {
        return;
    }
    normalize_shell_state();
    desktop_session_set_start_open(&g_shell, 0u);
    desktop_session_open_view(&g_shell, g_windows, g_window_order, 5u, view);
    g_shell.taskbar_index = taskbar_index_for_view(view);
    g_shell.dirty = 1u;
}

static void request_open_view(shell_view_t view) {
    desktop_session_set_start_open(&g_shell, 0u);
    (void)desktop_runtime_request_open(&g_runtime, &g_shell, view, view_is_installed);
}

static void handle_tab(void) {
    normalize_shell_state();
    desktop_input_handle_tab(&g_shell, any_open_windows_wrapper);
}

static void handle_arrows(ui_key_t key) {
    normalize_shell_state();
    if (g_shell.focus == FOCUS_WINDOW && g_shell.active_view == VIEW_HUB) {
        const u32 columns = 2u;
        const u32 rows = 2u;
        u32 row = g_shell.hub_index / columns;
        u32 col = g_shell.hub_index % columns;
        if (key == KEY_LEFT && col > 0u) {
            col -= 1u;
        } else if (key == KEY_RIGHT && col + 1u < columns) {
            col += 1u;
        } else if (key == KEY_UP && row > 0u) {
            row -= 1u;
        } else if (key == KEY_DOWN && row + 1u < rows) {
            row += 1u;
        }
        g_shell.hub_index = row * columns + col;
        if (g_shell.hub_index >= hub_card_count()) {
            g_shell.hub_index = hub_card_count() - 1u;
        }
        g_shell.dirty = 1u;
        return;
    }
    desktop_input_handle_arrows(&g_shell, key);
}

static void handle_enter(void) {
    normalize_shell_state();
    if (g_shell.focus == FOCUS_WINDOW && g_shell.active_view == VIEW_HUB) {
        activate_hub_card(g_shell.hub_index);
        return;
    }
    desktop_input_handle_enter(&g_shell, g_windows, g_window_order, 5u, request_open_view, desktop_session_close_all_windows);
}

static void handle_escape(void) {
    normalize_shell_state();
    desktop_input_handle_escape(&g_shell, g_windows, 5u, any_open_windows_wrapper);
}

static void activate_desktop_entry(u32 index) {
    desktop_input_activate_desktop_entry(&g_shell, index, request_open_view);
}

static void activate_start_entry(u32 index) {
    desktop_input_activate_start_entry(&g_shell, g_windows, 5u, index, request_open_view, desktop_session_close_all_windows);
}

static void handle_key(ui_key_t key) {
    if (key == KEY_NONE) {
        return;
    }
    if (g_shell.focus == FOCUS_WINDOW &&
        desktop_app_runtime_handle_key(g_shell.active_view, key, (const vex_boot_info_t*)g_boot) != 0u) {
        g_shell.dirty = 1u;
        return;
    }
    switch (key) {
    case KEY_TAB:
        handle_tab();
        break;
    case KEY_UP:
    case KEY_DOWN:
    case KEY_LEFT:
    case KEY_RIGHT:
        handle_arrows(key);
        break;
    case KEY_ENTER:
        handle_enter();
        break;
    case KEY_ESCAPE:
        handle_escape();
        break;
    case KEY_BACKSPACE:
        break;
    case KEY_NONE:
        break;
    }
}

static void handle_text_char(char ch) {
    if (ch == 0) {
        return;
    }
    if (g_shell.focus == FOCUS_WINDOW &&
        desktop_app_runtime_handle_char(g_shell.active_view, ch) != 0u) {
        g_shell.dirty = 1u;
    }
}

static void handle_mouse_packet(void) {
    const desktop_pointer_routes_t routes = pointer_routes();
    desktop_pointer_handle_packet(&g_shell, g_windows, g_window_order, 5u, g_mouse_packet, &routes);
}

static void update_input(void) {
    while ((ps2_read_status() & PS2_STATUS_OUTPUT_FULL) != 0u) {
        const u8 status = ps2_read_status();
        const u8 data = ps2_read_data();
        const u32 keyboard_prefix_before = g_extended_prefix;
        if ((status & PS2_STATUS_AUX_FULL) != 0u) {
            if (g_mouse_index == 0u && (data & 0x08u) == 0u) {
                continue;
            }
            g_mouse_packet[g_mouse_index++] = data;
            if (g_mouse_index == 3u) {
                g_mouse_index = 0u;
                handle_mouse_packet();
            }
            continue;
        }
        if (keyboard_prefix_before == 0u) {
            handle_text_char(ps2_decode_keyboard_char(data, &g_extended_prefix));
            g_extended_prefix = keyboard_prefix_before;
        }
        handle_key(ps2_decode_keyboard_scancode(data, &g_extended_prefix));
    }
}

static void clear_surface_pixels(u32* pixels, u32 pitch, u32 width, u32 height, u32 color) {
    for (u32 y = 0u; y < height; ++y) {
        for (u32 x = 0u; x < width; ++x) {
            pixels[(u64)y * pitch + x] = color;
        }
    }
}

static void blit_surface_to_desktop(
    const u32* src,
    u32 src_pitch,
    u32 src_width,
    u32 src_height,
    u32 dst_x,
    u32 dst_y
) {
    for (u32 y = 0u; y < src_height; ++y) {
        const u32 desktop_y = dst_y + y;
        if (desktop_y >= fb_height()) {
            break;
        }
        if (dst_x >= fb_width()) {
            break;
        }
        u32 copy_width = src_width;
        if (dst_x + copy_width > fb_width()) {
            copy_width = fb_width() - dst_x;
        }
        copy_pixel_row(
            &g_draw_pixels[(u64)desktop_y * g_draw_pitch + dst_x],
            &src[(u64)y * src_pitch],
            copy_width
        );
    }
}

static void overlay_remote_window_content(u32 slot) {
    const desktop_remote_surface_route_t* route = &g_remote_surfaces[slot];
    const volatile vex_compositor_scene_entry_t* scene_entry = compositor_scene_entry_for_slot(slot);
    const shell_window_t* window = &g_windows[slot];
    const u32 client_x = COMPOSITOR_SURFACE_PAD + 18u;
    const u32 client_y = COMPOSITOR_SURFACE_PAD + 56u;
    const u32 client_width = window->render_width > 36u ? window->render_width - 36u : 0u;
    const u32 client_height = window->render_height > 74u ? window->render_height - 74u : 0u;
    u32 copy_width = route->width < client_width ? route->width : client_width;
    u32 copy_height = route->height < client_height ? route->height : client_height;
    u32 buffer_index = route->present_index < route->buffer_count ? route->present_index : 0u;
    const volatile u32* src;

    if (route->available == 0u || copy_width == 0u || copy_height == 0u) {
        return;
    }
    if (scene_entry != 0 && scene_entry->visible != 0u) {
        if (scene_entry->width < copy_width) {
            copy_width = scene_entry->width;
        }
        if (scene_entry->height < copy_height) {
            copy_height = scene_entry->height;
        }
        buffer_index = scene_entry->present_index;
        if (buffer_index >= route->buffer_count) {
            buffer_index = 0u;
        }
    }
    src = (const volatile u32*)(u64)(route->shared_mapping_base + (u64)buffer_index * route->bytes_per_buffer);

    fill_rect(client_x - 2u, client_y - 2u, copy_width + 4u, copy_height + 4u, 0x00121A28u);
    for (u32 y = 0u; y < copy_height; ++y) {
        for (u32 x = 0u; x < copy_width; ++x) {
            put_pixel(client_x + x, client_y + y, src[(u64)y * route->stride + x]);
        }
    }
}

static void render_window_surface(
    u32 slot,
    const desktop_boot_metrics_t* boot,
    const desktop_view_theme_t* theme,
    const desktop_view_ops_t* ops
) {
    shell_state_t local_shell = g_shell;
    shell_window_t local_windows[5];
    const shell_window_t* window = &g_windows[slot];
    desktop_compositor_surface_t* surface = &g_window_surfaces[slot];
    const u32 origin_x = window->render_x > COMPOSITOR_SURFACE_PAD ? window->render_x - COMPOSITOR_SURFACE_PAD : 0u;
    const u32 origin_y = window->render_y > COMPOSITOR_SURFACE_PAD ? window->render_y - COMPOSITOR_SURFACE_PAD : 0u;
    u32 width = window->render_width + COMPOSITOR_SURFACE_PAD * 2u;
    u32 height = window->render_height + COMPOSITOR_SURFACE_PAD * 2u;

    if (width > COMPOSITOR_SURFACE_MAX_WIDTH) {
        width = COMPOSITOR_SURFACE_MAX_WIDTH;
    }
    if (height > COMPOSITOR_SURFACE_MAX_HEIGHT) {
        height = COMPOSITOR_SURFACE_MAX_HEIGHT;
    }

    for (u32 index = 0u; index < 5u; ++index) {
        local_windows[index] = g_windows[index];
    }
    local_windows[slot].x = COMPOSITOR_SURFACE_PAD;
    local_windows[slot].y = COMPOSITOR_SURFACE_PAD;
    local_windows[slot].render_x = COMPOSITOR_SURFACE_PAD;
    local_windows[slot].render_y = COMPOSITOR_SURFACE_PAD;

    local_shell.window_x = COMPOSITOR_SURFACE_PAD;
    local_shell.window_y = COMPOSITOR_SURFACE_PAD;
    local_shell.window_width = window->render_width;
    local_shell.window_height = window->render_height;
    local_shell.cursor_x = g_shell.cursor_x >= origin_x ? g_shell.cursor_x - origin_x : 0xFFFFFFFFu;
    local_shell.cursor_y = g_shell.cursor_y >= origin_y ? g_shell.cursor_y - origin_y : 0xFFFFFFFFu;

    clear_surface_pixels(g_window_surface_storage[slot], width, width, height, 0x00000000u);
    set_render_target(g_window_surface_storage[slot], width, width, height, 0u, 0u);
    desktop_views_render_window(&local_shell, local_windows, slot, boot, &g_animation, theme, ops);
    if (slot != (u32)VIEW_HUB) {
        overlay_remote_window_content(slot);
    }

    surface->active = 1u;
    surface->width = width;
    surface->height = height;
    surface->pitch = width;
    surface->origin_x = origin_x;
    surface->origin_y = origin_y;
    surface->present_sequence += 1u;
    surface->packet.abi_version = 1u;
    surface->packet.opcode = VEX_COMPOSITOR_PRESENT_SURFACE;
    surface->packet.flags = 0u;
    surface->packet.payload_bytes = sizeof(vex_compositor_packet_t);
    surface->packet.sequence = (u32)surface->present_sequence;
    surface->packet.window.window_id = slot + 1u;
    surface->packet.window.kind = window->kind;
    surface->packet.window.x = window->render_x;
    surface->packet.window.y = window->render_y;
    surface->packet.window.width = window->render_width;
    surface->packet.window.height = window->render_height;
    surface->packet.surface.width = width;
    surface->packet.surface.height = height;
    surface->packet.surface.stride = width;
    surface->packet.surface.buffer_count = 1u;
    surface->packet.surface.present_index = 0u;
    surface->packet.surface.surface_handle = 0u;
    surface->packet.surface.fence_handle = 0u;
    surface->packet.surface.mapping_base = (u64)(void*)g_window_surface_storage[slot];
    surface->packet.surface.bytes_per_buffer = (u64)width * height * sizeof(u32);
}

static void render_composited_windows(void) {
    const desktop_boot_metrics_t boot = current_boot_metrics();
    const desktop_view_theme_t theme = {
        .color_accent = COLOR_ACCENT,
        .color_window_border = COLOR_WINDOW_BORDER,
        .color_window = COLOR_WINDOW,
        .color_window_alt = COLOR_WINDOW_ALT,
        .color_window_title = COLOR_WINDOW_TITLE,
        .color_window_title_active = COLOR_WINDOW_TITLE_ACTIVE,
        .color_text = COLOR_TEXT,
        .color_text_dim = COLOR_TEXT_DIM,
        .color_text_soft = COLOR_TEXT_SOFT,
        .color_selection = COLOR_SELECTION,
        .color_panel = COLOR_PANEL,
        .color_menu_active = COLOR_MENU_ACTIVE,
        .color_ready = COLOR_READY,
        .color_warning = COLOR_WARNING,
        .color_pass = COLOR_PASS,
        .color_glow = COLOR_WALLPAPER_GLOW
    };
    const desktop_view_ops_t ops = {
        .fill_rect = fill_rect,
        .draw_frame = draw_frame,
        .draw_shadow = draw_shadow,
        .draw_glow_rect = draw_glow_rect,
        .draw_text = draw_text_scaled,
        .draw_metric = draw_metric_line,
        .draw_badge = draw_badge,
        .mix_color = mix_color
    };

    set_render_target(g_swapchain_storage[g_draw_buffer_index], fb_pitch(), fb_width(), fb_height(), 0u, 0u);
    for (u32 index = 0u; index < 5u; ++index) {
        const u32 slot = g_window_order[index];
        if (g_windows[slot].render_visible == 0u) {
            g_window_surfaces[slot].active = 0u;
            continue;
        }
        render_window_surface(slot, &boot, &theme, &ops);
        set_render_target(g_swapchain_storage[g_draw_buffer_index], fb_pitch(), fb_width(), fb_height(), 0u, 0u);
        blit_surface_to_desktop(
            g_window_surface_storage[slot],
            g_window_surfaces[slot].pitch,
            g_window_surfaces[slot].width,
            g_window_surfaces[slot].height,
            g_window_surfaces[slot].origin_x,
            g_window_surfaces[slot].origin_y
        );
    }
}

static void render_desktop(void) {
    normalize_shell_state();
    sync_registry_sessions();
    {
        const desktop_stage_theme_t stage_theme = {
            .color_selection = COLOR_SELECTION,
            .color_accent = COLOR_ACCENT,
            .color_window_border = COLOR_WINDOW_BORDER,
            .color_icon_bg = COLOR_ICON_BG,
            .color_icon_active = COLOR_ICON_ACTIVE,
            .color_glow = COLOR_WALLPAPER_GLOW,
            .color_text = COLOR_TEXT,
            .color_text_soft = COLOR_TEXT_SOFT,
            .color_black = COLOR_BLACK,
            .fallback_wallpaper = 0x00081018u
        };
        const desktop_stage_ops_t stage_ops = {
            .fill_rect = fill_rect,
            .put_pixel = put_pixel,
            .copy_row = copy_pixel_row,
            .draw_frame = draw_frame,
            .draw_text = draw_text_scaled,
            .mix_color = mix_color
        };
        desktop_stage_draw_wallpaper(fb_width(), fb_height(), surface_pitch(), g_draw_pixels, &g_animation, &stage_theme, &stage_ops);
        desktop_stage_draw_icons(&g_shell, &g_animation, &stage_theme, &stage_ops);
    }

    render_composited_windows();
    {
        const desktop_shell_ui_theme_t shell_theme = {
            .color_taskbar = COLOR_TASKBAR,
            .color_taskbar_edge = COLOR_TASKBAR_EDGE,
            .color_taskbar_hilite = COLOR_TASKBAR_HILITE,
            .color_start = COLOR_START,
            .color_start_active = COLOR_START_ACTIVE,
            .color_panel_alt = COLOR_PANEL_ALT,
            .color_window_border = COLOR_WINDOW_BORDER,
            .color_accent = COLOR_ACCENT,
            .color_text = COLOR_TEXT,
            .color_text_dim = COLOR_TEXT_DIM,
            .color_text_soft = COLOR_TEXT_SOFT,
            .color_menu = COLOR_MENU,
            .color_menu_header = COLOR_MENU_HEADER,
            .color_menu_active = COLOR_MENU_ACTIVE,
            .color_selection = COLOR_SELECTION,
            .color_warning = COLOR_WARNING,
            .color_glow = COLOR_WALLPAPER_GLOW
        };
        const desktop_shell_ui_ops_t shell_ops = {
            .fill_rect = fill_rect,
            .draw_frame = draw_frame,
            .draw_shadow = draw_shadow,
            .draw_text = draw_text_scaled,
            .mix_color = mix_color
        };
        desktop_shell_ui_render(&g_shell, g_windows, 5u, fb_width(), fb_height(), &g_clock, &g_animation, &shell_theme, &shell_ops);
    }
    {
        const desktop_pointer_routes_t routes = pointer_routes();
        const desktop_stage_theme_t stage_theme = {
            .color_selection = COLOR_SELECTION,
            .color_accent = COLOR_ACCENT,
            .color_window_border = COLOR_WINDOW_BORDER,
            .color_icon_bg = COLOR_ICON_BG,
            .color_icon_active = COLOR_ICON_ACTIVE,
            .color_glow = COLOR_WALLPAPER_GLOW,
            .color_text = COLOR_TEXT,
            .color_text_soft = COLOR_TEXT_SOFT,
            .color_black = COLOR_BLACK,
            .fallback_wallpaper = 0x00081018u
        };
        const desktop_stage_ops_t stage_ops = {
            .fill_rect = fill_rect,
            .put_pixel = put_pixel,
            .copy_row = copy_pixel_row,
            .draw_frame = draw_frame,
            .draw_text = draw_text_scaled,
            .mix_color = mix_color
        };
        const desktop_stage_cursor_t cursor = {
            .x = g_shell.cursor_x,
            .y = g_shell.cursor_y,
            .hot = desktop_pointer_is_hot(&g_shell, g_windows, 5u, &routes),
            .desktop_width = fb_width(),
            .desktop_height = fb_height()
        };
        desktop_stage_draw_cursor(&cursor, &g_animation, &stage_theme, &stage_ops);
    }
}

void _start(void) {
    g_extended_prefix = 0u;
    desktop_domain_init((const vex_boot_info_t*)g_boot);
    init_remote_surface_routes();
    desktop_registry_init();
    desktop_animation_init(&g_animation);
    desktop_app_runtime_init();
    desktop_runtime_init(&g_runtime, &g_shell);
    desktop_rtc_init(&g_clock);
    init_window_layout();
    swapchain_init();
    begin_frame();
    render_desktop();
    present_frame();
    g_shell.dirty = 0u;
    (void)ps2_mouse_init();
    g_mouse_index = 0u;
    if (desktop_rtc_update(&g_clock) != 0u) {
        g_shell.dirty = 1u;
    }
    for (;;) {
        update_input();
        if (desktop_session_tick(&g_shell, g_windows, 5u) != 0u) {
            g_shell.dirty = 1u;
        }
        if (desktop_runtime_tick(&g_runtime, &g_shell, install_view_and_mark_dirty, open_view) != 0u) {
            g_shell.dirty = 1u;
        }
        sync_registry_sessions();
        sync_app_sessions();
        if (desktop_app_runtime_tick((const vex_boot_info_t*)g_boot, &g_animation) != 0u) {
            g_shell.dirty = 1u;
        }
        if (desktop_animation_tick(&g_animation) != 0u) {
            g_shell.dirty = 1u;
        }
        if (desktop_rtc_update(&g_clock) != 0u) {
            g_shell.dirty = 1u;
        }
        if (g_shell.dirty != 0u) {
            begin_frame();
            render_desktop();
            present_frame();
            g_shell.dirty = 0u;
        }
        __asm__ volatile ("pause");
    }
}
