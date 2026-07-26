#define FB_BASE 0x50000000ull
#define BOOTINFO_BASE 0x50400000ull
#define MAILBOX_BASE 0x70002000ull

#include "../init/vex_boot_info.h"
#include "../init/vex_ui_proto.h"

typedef unsigned long long u64;

static volatile vex_boot_info_t* const g_boot = (volatile vex_boot_info_t*)BOOTINFO_BASE;
static volatile u32* const g_pixels = (volatile u32*)FB_BASE;
static volatile vex_compositor_mailbox_t* const g_mailbox = (volatile vex_compositor_mailbox_t*)MAILBOX_BASE;

enum {
    COLOR_BG = 0x0009121Fu,
    COLOR_PANEL = 0x0011192Au,
    COLOR_ACCENT = 0x009973FFu,
    COLOR_TEXT = 0x00F1F6FFu,
    COLOR_TEXT_SOFT = 0x008FA7CCu,
    COLOR_OK = 0x003D85EDu
};

static u32 width(void) { return g_boot->framebuffer.width; }
static u32 height(void) { return g_boot->framebuffer.height; }
static u32 pitch(void) { return g_boot->framebuffer.pixels_per_scanline; }

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
    static const unsigned char greater[7] = {16,8,4,2,4,8,16};
    static const unsigned char d0[7] = {14,17,19,21,25,17,14};
    static const unsigned char d1[7] = {4,12,4,4,4,4,14};
    static const unsigned char A[7] = {14,17,17,31,17,17,17};
    static const unsigned char C[7] = {14,17,16,16,16,17,14};
    static const unsigned char D[7] = {28,18,17,17,17,18,28};
    static const unsigned char E[7] = {31,16,16,30,16,16,31};
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
    switch (c) {
    case ' ': return blank;
    case '-': return dash;
    case '>': return greater;
    case '0': return d0;
    case '1': return d1;
    case 'A': return A;
    case 'C': return C;
    case 'D': return D;
    case 'E': return E;
    case 'H': return H;
    case 'I': return I;
    case 'L': return L;
    case 'M': return M;
    case 'N': return N;
    case 'O': return O;
    case 'P': return P;
    case 'R': return R;
    case 'S': return S;
    case 'T': return T;
    case 'U': return U;
    case 'V': return V;
    default: return blank;
    }
}

static void draw_char(u32 x, u32 y, char c, u32 scale, u32 color) {
    const unsigned char* data = glyph(c);
    for (u32 gy = 0; gy < 7u; ++gy) {
        for (u32 gx = 0; gx < 5u; ++gx) {
            if ((data[gy] & (1u << (4u - gx))) == 0u) {
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
        cursor += (5u + 2u) * scale;
        ++text;
    }
}

static void write_mailbox(void) {
    g_mailbox->magic = VEX_COMPOSITOR_MAILBOX_MAGIC;
    g_mailbox->abi_version = 1u;
    g_mailbox->sequence += 1u;
    g_mailbox->packet.abi_version = 1u;
    g_mailbox->packet.opcode = VEX_COMPOSITOR_PRESENT_SURFACE;
    g_mailbox->packet.payload_bytes = sizeof(vex_compositor_packet_t);
    g_mailbox->packet.sequence = g_mailbox->sequence;
    g_mailbox->packet.window.window_id = 4u;
    g_mailbox->packet.window.kind = VEX_WINDOW_TERMINAL;
    g_mailbox->packet.surface.width = g_boot->framebuffer.width;
    g_mailbox->packet.surface.height = g_boot->framebuffer.height;
    g_mailbox->packet.surface.stride = g_boot->framebuffer.pixels_per_scanline;
    g_mailbox->packet.surface.buffer_count = 3u;
    g_mailbox->packet.surface.present_index = 0u;
}

void _start(void) {
    write_mailbox();
    fill_rect(0u, 0u, width(), height(), COLOR_BG);
    fill_rect(88u, 72u, width() - 176u, height() - 144u, COLOR_PANEL);
    fill_rect(88u, 72u, width() - 176u, 6u, COLOR_ACCENT);
    draw_text(118u, 108u, "TERMINAL", 2u, COLOR_TEXT);
    draw_text(118u, 154u, "> OPEN DOMAIN", 1u, COLOR_ACCENT);
    draw_text(118u, 182u, "> ABI V1 READY", 1u, COLOR_TEXT);
    draw_text(118u, 210u, "> IPC CAP SET", 1u, COLOR_TEXT);
    draw_text(118u, 238u, "> USERSPACE CONSOLE", 1u, COLOR_TEXT_SOFT);
    fill_rect(118u, 286u, 132u, 22u, COLOR_OK);
    draw_text(132u, 293u, "READY", 1u, COLOR_TEXT);
    for (;;) {
        write_mailbox();
        __asm__ volatile ("pause");
    }
}
