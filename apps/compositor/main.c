#define FB_BASE 0x50000000ull
#define BOOTINFO_BASE 0x50400000ull
#define MAILBOX_BASE 0x70002000ull

#include "../init/vex_boot_info.h"
#include "../init/vex_gpu_proto.h"
#include "../init/vex_ui_proto.h"

typedef unsigned long long u64;

typedef struct compositor_route {
    u32 active;
    u32 width;
    u32 height;
    u32 stride;
    u32 buffer_count;
    u64 mapping_base;
    u64 mailbox_base;
    u64 bytes_per_buffer;
} compositor_route_t;

static volatile vex_boot_info_t* const g_boot = (volatile vex_boot_info_t*)BOOTINFO_BASE;
static volatile u32* const g_pixels = (volatile u32*)FB_BASE;
static volatile vex_compositor_scene_mailbox_t* const g_scene = (volatile vex_compositor_scene_mailbox_t*)MAILBOX_BASE;
static volatile vex_gpu_mailbox_t* g_gpu = 0;
static compositor_route_t g_routes[VEX_COMPOSITOR_SCENE_SLOTS];

enum {
    COLOR_BG = 0x00101824u,
    COLOR_PANEL = 0x00172131u,
    COLOR_PANEL_ALT = 0x00111A28u,
    COLOR_ACCENT = 0x00466DFFu,
    COLOR_TEXT = 0x00F1F6FFu,
    COLOR_TEXT_DIM = 0x0093A8C8u,
    COLOR_OK = 0x0041B76Du,
    COLOR_WARN = 0x00C88B2Du,
    COLOR_IDLE = 0x00243A55u,
    COLOR_GPU = 0x003ED38Au,
    GLYPH_W = 5u,
    GLYPH_H = 7u
};

static u32 width(void) { return g_boot->framebuffer.width; }
static u32 height(void) { return g_boot->framebuffer.height; }
static u32 pitch(void) { return g_boot->framebuffer.pixels_per_scanline; }

static void copy_string(char* dst, u32 dst_size, const volatile char* src) {
    u32 index = 0u;
    if (dst_size == 0u) {
        return;
    }
    while (src[index] != 0 && index + 1u < dst_size) {
        dst[index] = (char)src[index];
        ++index;
    }
    dst[index] = 0;
}

static void write_string(volatile char* dst, u32 dst_size, const char* src) {
    u32 index = 0u;
    if (dst_size == 0u) {
        return;
    }
    while (src[index] != 0 && index + 1u < dst_size) {
        dst[index] = src[index];
        ++index;
    }
    dst[index] = 0;
}

static void put_pixel(u32 x, u32 y, u32 color) {
    if (x >= width() || y >= height()) {
        return;
    }
    g_pixels[(u64)y * pitch() + x] = color;
}

static void fill_rect(u32 x, u32 y, u32 w, u32 h, u32 color) {
    const u32 max_x = x + w > width() ? width() : x + w;
    const u32 max_y = y + h > height() ? height() : y + h;
    for (u32 yy = y; yy < max_y; ++yy) {
        for (u32 xx = x; xx < max_x; ++xx) {
            put_pixel(xx, yy, color);
        }
    }
}

static const unsigned char* glyph(char c) {
    static const unsigned char blank[7] = {0,0,0,0,0,0,0};
    static const unsigned char dash[7] = {0,0,0,14,0,0,0};
    static const unsigned char colon[7] = {0,12,12,0,12,12,0};
    static const unsigned char slash[7] = {1,2,4,4,8,16,16};
    static const unsigned char d0[7] = {14,17,19,21,25,17,14};
    static const unsigned char d1[7] = {4,12,4,4,4,4,14};
    static const unsigned char d2[7] = {14,17,1,2,4,8,31};
    static const unsigned char d3[7] = {30,1,1,14,1,1,30};
    static const unsigned char d4[7] = {2,6,10,18,31,2,2};
    static const unsigned char d5[7] = {31,16,16,30,1,1,30};
    static const unsigned char d6[7] = {6,8,16,30,17,17,14};
    static const unsigned char d7[7] = {31,1,2,4,8,8,8};
    static const unsigned char d8[7] = {14,17,17,14,17,17,14};
    static const unsigned char d9[7] = {14,17,17,15,1,2,28};
    static const unsigned char A[7] = {14,17,17,31,17,17,17};
    static const unsigned char B[7] = {30,17,17,30,17,17,30};
    static const unsigned char C[7] = {14,17,16,16,16,17,14};
    static const unsigned char D[7] = {28,18,17,17,17,18,28};
    static const unsigned char E[7] = {31,16,16,30,16,16,31};
    static const unsigned char F[7] = {31,16,16,30,16,16,16};
    static const unsigned char G[7] = {14,17,16,23,17,17,14};
    static const unsigned char H[7] = {17,17,17,31,17,17,17};
    static const unsigned char I[7] = {14,4,4,4,4,4,14};
    static const unsigned char L[7] = {16,16,16,16,16,16,31};
    static const unsigned char M[7] = {17,27,21,21,17,17,17};
    static const unsigned char N[7] = {17,25,21,19,17,17,17};
    static const unsigned char O[7] = {14,17,17,17,17,17,14};
    static const unsigned char P[7] = {30,17,17,30,16,16,16};
    static const unsigned char R[7] = {30,17,17,30,20,18,17};
    static const unsigned char S[7] = {15,16,16,14,1,1,30};
    static const unsigned char T[7] = {31,4,4,4,4,4,4};
    static const unsigned char U[7] = {17,17,17,17,17,17,14};
    static const unsigned char V[7] = {17,17,17,17,17,10,4};
    static const unsigned char W[7] = {17,17,17,21,21,21,10};
    static const unsigned char X[7] = {17,17,10,4,10,17,17};
    static const unsigned char Y[7] = {17,17,10,4,4,4,4};
    static const unsigned char Z[7] = {31,1,2,4,8,16,31};

    switch (c) {
    case ' ': return blank;
    case '-': return dash;
    case ':': return colon;
    case '/': return slash;
    case '0': return d0; case '1': return d1; case '2': return d2; case '3': return d3; case '4': return d4;
    case '5': return d5; case '6': return d6; case '7': return d7; case '8': return d8; case '9': return d9;
    case 'A': return A; case 'B': return B; case 'C': return C; case 'D': return D; case 'E': return E;
    case 'F': return F; case 'G': return G; case 'H': return H; case 'I': return I; case 'L': return L;
    case 'M': return M; case 'N': return N; case 'O': return O; case 'P': return P; case 'R': return R;
    case 'S': return S; case 'T': return T; case 'U': return U; case 'V': return V; case 'W': return W;
    case 'X': return X; case 'Y': return Y; case 'Z': return Z;
    default: return blank;
    }
}

static void draw_char(u32 x, u32 y, char c, u32 scale, u32 color) {
    const unsigned char* data = glyph(c);
    for (u32 gy = 0u; gy < GLYPH_H; ++gy) {
        for (u32 gx = 0u; gx < GLYPH_W; ++gx) {
            if ((data[gy] & (1u << (GLYPH_W - 1u - gx))) == 0u) {
                continue;
            }
            fill_rect(x + gx * scale, y + gy * scale, scale, scale, color);
        }
    }
}

static void draw_text(u32 x, u32 y, const char* text, u32 scale, u32 color) {
    u32 cursor = x;
    while (*text != 0) {
        draw_char(cursor, y, *text, scale, color);
        cursor += (GLYPH_W + 2u) * scale;
        ++text;
    }
}

static void append_string(char* dst, u32 dst_size, const char* src) {
    u32 index = 0u;
    while (index + 1u < dst_size && dst[index] != 0) {
        ++index;
    }
    while (*src != 0 && index + 1u < dst_size) {
        dst[index++] = *src++;
    }
    dst[index] = 0;
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
        char ch[2];
        ch[0] = digits[count - 1u];
        ch[1] = 0;
        append_string(dst, dst_size, ch);
        --count;
    }
}

static const char* fallback_title(u32 slot) {
    switch (slot) {
    case 0u: return "DIAGNOSTICS";
    case 1u: return "TEST CENTER";
    case 2u: return "SERVICES";
    case 3u: return "TERMINAL";
    default: return "APP";
    }
}

static void init_routes(void) {
    if (g_boot->compositor_gpu_mailbox_base != 0u) {
        g_gpu = (volatile vex_gpu_mailbox_t*)(u64)g_boot->compositor_gpu_mailbox_base;
    }
    for (u32 index = 0u; index < VEX_COMPOSITOR_SCENE_SLOTS; ++index) {
        const volatile vex_shared_surface_info_t* info = &g_boot->shared_surfaces[index];
        g_routes[index].active = info->compositor_mapping_base != 0u;
        g_routes[index].width = info->width;
        g_routes[index].height = info->height;
        g_routes[index].stride = info->stride;
        g_routes[index].buffer_count = info->buffer_count;
        g_routes[index].mapping_base = info->compositor_mapping_base;
        g_routes[index].mailbox_base = info->compositor_mailbox_base;
        g_routes[index].bytes_per_buffer = info->bytes_per_buffer;
    }
}

static void write_gpu_request(void) {
    if (g_gpu == 0) {
        return;
    }
    g_gpu->magic = VEX_GPU_MAILBOX_MAGIC;
    g_gpu->abi_version = VEX_GPU_ABI_VERSION;
    g_gpu->request_sequence += 1u;
    g_gpu->request.desired_backend = VEX_GPU_BACKEND_BOOT_FB;
    g_gpu->request.requested_features =
        VEX_GPU_FEATURE_TRIPLE_BUFFER |
        VEX_GPU_FEATURE_PRESENT_FENCE |
        VEX_GPU_FEATURE_SHARED_SURFACE_IMPORT |
        VEX_GPU_FEATURE_SCENE_IMPORT |
        VEX_GPU_FEATURE_COMPOSITOR_LINK |
        VEX_GPU_FEATURE_EXPLICIT_SYNC_STUB;
    g_gpu->request.scene_sequence = g_scene->frame_sequence;
}

static u32 sample_surface_color(const compositor_route_t* route, u32 present_index) {
    const volatile u32* surface;
    u32 sample_x;
    u32 sample_y;
    if (route->active == 0u || route->mapping_base == 0u || route->width == 0u || route->height == 0u) {
        return COLOR_IDLE;
    }
    if (present_index >= route->buffer_count) {
        present_index = 0u;
    }
    surface = (const volatile u32*)(u64)(route->mapping_base + (u64)present_index * route->bytes_per_buffer);
    sample_x = route->width / 2u;
    sample_y = route->height / 2u;
    return surface[(u64)sample_y * route->stride + sample_x];
}

static void write_scene(void) {
    g_scene->magic = VEX_COMPOSITOR_SCENE_MAGIC;
    g_scene->abi_version = VEX_COMPOSITOR_ABI_VERSION;
    g_scene->frame_sequence += 1u;
    g_scene->entry_count = VEX_COMPOSITOR_SCENE_SLOTS;

    for (u32 index = 0u; index < VEX_COMPOSITOR_SCENE_SLOTS; ++index) {
        volatile vex_compositor_scene_entry_t* entry = &g_scene->entries[index];
        const compositor_route_t* route = &g_routes[index];

        entry->slot = index + 1u;
        entry->visible = 0u;
        entry->sequence = 0u;
        entry->acknowledged_sequence = 0u;
        entry->window_id = index + 1u;
        entry->kind = 0u;
        entry->present_index = 0u;
        entry->buffer_count = route->buffer_count;
        entry->width = route->width;
        entry->height = route->height;
        entry->stride = route->stride;
        write_string(entry->title, sizeof(entry->title), fallback_title(index));

        if (route->active == 0u || route->mailbox_base == 0u) {
            continue;
        }

        {
            volatile vex_compositor_mailbox_t* mailbox = (volatile vex_compositor_mailbox_t*)(u64)route->mailbox_base;
            if (mailbox->magic != VEX_COMPOSITOR_MAILBOX_MAGIC ||
                mailbox->abi_version != VEX_COMPOSITOR_ABI_VERSION) {
                continue;
            }

            entry->visible = 1u;
            entry->sequence = mailbox->sequence;
            entry->acknowledged_sequence = mailbox->sequence;
            entry->window_id = mailbox->packet.window.window_id;
            entry->kind = mailbox->packet.window.kind;
            entry->present_index = mailbox->packet.surface.present_index;
            if (entry->present_index >= route->buffer_count) {
                entry->present_index = 0u;
            }
            if (mailbox->packet.surface.width != 0u) {
                entry->width = mailbox->packet.surface.width;
            }
            if (mailbox->packet.surface.height != 0u) {
                entry->height = mailbox->packet.surface.height;
            }
            if (mailbox->packet.surface.stride != 0u) {
                entry->stride = mailbox->packet.surface.stride;
            }
            if (mailbox->packet.surface.buffer_count != 0u) {
                entry->buffer_count = mailbox->packet.surface.buffer_count;
            }
            if (mailbox->packet.window.title[0] != 0) {
                copy_string((char*)entry->title, sizeof(entry->title), mailbox->packet.window.title);
            }
            mailbox->acknowledged_sequence = mailbox->sequence;
        }
    }
}

static void draw_scene_status(void) {
    char line[96];
    fill_rect(0u, 0u, width(), height(), COLOR_BG);
    fill_rect(72u, 52u, width() - 144u, height() - 104u, COLOR_PANEL);
    fill_rect(72u, 52u, width() - 144u, 8u, COLOR_ACCENT);
    fill_rect(96u, 96u, width() - 192u, 72u, COLOR_PANEL_ALT);

    draw_text(118u, 108u, "COMPOSITOR SERVICE", 2u, COLOR_TEXT);
    draw_text(118u, 140u, "SCENE MAILBOX ONLINE", 1u, COLOR_TEXT_DIM);
    if (g_gpu != 0 &&
        g_gpu->magic == VEX_GPU_MAILBOX_MAGIC &&
        g_gpu->abi_version == VEX_GPU_ABI_VERSION) {
        line[0] = 0;
        append_string(line, sizeof(line), "GPU ");
        append_u32(line, sizeof(line), g_gpu->status.selected_backend);
        append_string(line, sizeof(line), " HB ");
        append_u32(line, sizeof(line), g_gpu->status.heartbeat);
        append_string(line, sizeof(line), " VIS ");
        append_u32(line, sizeof(line), g_gpu->status.visible_window_count);
        append_string(line, sizeof(line), " FENCE ");
        append_u32(line, sizeof(line), g_gpu->status.last_fence_value);
        draw_text(118u, 164u, line, 1u, COLOR_GPU);
    } else {
        draw_text(118u, 164u, "GPU LINK OFFLINE", 1u, COLOR_WARN);
    }

    for (u32 index = 0u; index < VEX_COMPOSITOR_SCENE_SLOTS; ++index) {
        const volatile vex_compositor_scene_entry_t* entry = &g_scene->entries[index];
        const compositor_route_t* route = &g_routes[index];
        const u32 y = 204u + index * 120u;
        const u32 swatch = sample_surface_color(route, entry->present_index);
        char title[48];

        fill_rect(104u, y, width() - 208u, 94u, entry->visible != 0u ? COLOR_PANEL_ALT : COLOR_IDLE);
        fill_rect(124u, y + 18u, 22u, 22u, entry->visible != 0u ? COLOR_OK : COLOR_WARN);
        fill_rect(164u, y + 18u, 56u, 22u, swatch);
        copy_string(title, sizeof(title), entry->title);
        draw_text(238u, y + 20u, title, 1u, COLOR_TEXT);

        line[0] = 0;
        append_string(line, sizeof(line), "SEQ ");
        append_u32(line, sizeof(line), entry->sequence);
        append_string(line, sizeof(line), " ACK ");
        append_u32(line, sizeof(line), entry->acknowledged_sequence);
        draw_text(238u, y + 44u, line, 1u, COLOR_TEXT_DIM);

        line[0] = 0;
        append_string(line, sizeof(line), "BUF ");
        append_u32(line, sizeof(line), entry->present_index);
        append_string(line, sizeof(line), "/");
        append_u32(line, sizeof(line), entry->buffer_count);
        append_string(line, sizeof(line), " ");
        append_u32(line, sizeof(line), entry->width);
        append_string(line, sizeof(line), "X");
        append_u32(line, sizeof(line), entry->height);
        draw_text(238u, y + 68u, line, 1u, COLOR_TEXT_DIM);
    }
}

void _start(void) {
    init_routes();
    for (;;) {
        write_scene();
        write_gpu_request();
        draw_scene_status();
        __asm__ volatile ("pause");
    }
}
