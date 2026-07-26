#define FB_BASE 0x50000000ull
#define BOOTINFO_BASE 0x50400000ull
#define MAILBOX_BASE 0x70002000ull

#include "../init/vex_boot_info.h"
#include "../init/vex_ui_proto.h"

static volatile vex_boot_info_t* const g_boot = (volatile vex_boot_info_t*)BOOTINFO_BASE;
static volatile u32* const g_pixels = (volatile u32*)FB_BASE;
static volatile vex_compositor_mailbox_t* const g_mailbox = (volatile vex_compositor_mailbox_t*)MAILBOX_BASE;

enum {
    COLOR_BG = 0x00111924,
    COLOR_TITLE = 0x00274989,
    COLOR_PANEL = 0x00141C29,
    COLOR_ACCENT = 0x00356BE3,
    COLOR_ROW = 0x00223D6B,
    COLOR_TEXT = 0x00F1F6FF,
    COLOR_TEXT_DIM = 0x008FA7CC,
    GLYPH_W = 5u,
    GLYPH_H = 7u
};

static u32 fb_width(void) {
    return g_boot->framebuffer.width;
}

static u32 fb_height(void) {
    return g_boot->framebuffer.height;
}

static u32 fb_pitch(void) {
    return g_boot->framebuffer.pixels_per_scanline;
}

static void put_pixel(u32 x, u32 y, u32 color) {
    if (x >= fb_width() || y >= fb_height()) {
        return;
    }
    g_pixels[(u64)y * fb_pitch() + x] = color;
}

static void fill_rect(u32 x, u32 y, u32 width, u32 height, u32 color) {
    const u32 max_x = x + width > fb_width() ? fb_width() : x + width;
    const u32 max_y = y + height > fb_height() ? fb_height() : y + height;
    for (u32 yy = y; yy < max_y; ++yy) {
        for (u32 xx = x; xx < max_x; ++xx) {
            put_pixel(xx, yy, color);
        }
    }
}

static const unsigned char* glyph(char c) {
    static const unsigned char blank[7] = {0, 0, 0, 0, 0, 0, 0};
    static const unsigned char dash[7] = {0, 0, 0, 14, 0, 0, 0};
    static const unsigned char colon[7] = {0, 12, 12, 0, 12, 12, 0};
    static const unsigned char slash[7] = {1, 2, 4, 4, 8, 16, 16};
    static const unsigned char d0[7] = {14, 17, 19, 21, 25, 17, 14};
    static const unsigned char d1[7] = {4, 12, 4, 4, 4, 4, 14};
    static const unsigned char d2[7] = {14, 17, 1, 2, 4, 8, 31};
    static const unsigned char d3[7] = {30, 1, 1, 14, 1, 1, 30};
    static const unsigned char d4[7] = {2, 6, 10, 18, 31, 2, 2};
    static const unsigned char d5[7] = {31, 16, 16, 30, 1, 1, 30};
    static const unsigned char d6[7] = {6, 8, 16, 30, 17, 17, 14};
    static const unsigned char d7[7] = {31, 1, 2, 4, 8, 8, 8};
    static const unsigned char d8[7] = {14, 17, 17, 14, 17, 17, 14};
    static const unsigned char d9[7] = {14, 17, 17, 15, 1, 2, 28};
    static const unsigned char A[7] = {14, 17, 17, 31, 17, 17, 17};
    static const unsigned char B[7] = {30, 17, 17, 30, 17, 17, 30};
    static const unsigned char C[7] = {14, 17, 16, 16, 16, 17, 14};
    static const unsigned char D[7] = {28, 18, 17, 17, 17, 18, 28};
    static const unsigned char E[7] = {31, 16, 16, 30, 16, 16, 31};
    static const unsigned char F[7] = {31, 16, 16, 30, 16, 16, 16};
    static const unsigned char G[7] = {14, 17, 16, 23, 17, 17, 14};
    static const unsigned char H[7] = {17, 17, 17, 31, 17, 17, 17};
    static const unsigned char I[7] = {14, 4, 4, 4, 4, 4, 14};
    static const unsigned char J[7] = {1, 1, 1, 1, 17, 17, 14};
    static const unsigned char K[7] = {17, 18, 20, 24, 20, 18, 17};
    static const unsigned char L[7] = {16, 16, 16, 16, 16, 16, 31};
    static const unsigned char M[7] = {17, 27, 21, 21, 17, 17, 17};
    static const unsigned char N[7] = {17, 25, 21, 19, 17, 17, 17};
    static const unsigned char O[7] = {14, 17, 17, 17, 17, 17, 14};
    static const unsigned char P[7] = {30, 17, 17, 30, 16, 16, 16};
    static const unsigned char Q[7] = {14, 17, 17, 17, 21, 18, 13};
    static const unsigned char R[7] = {30, 17, 17, 30, 20, 18, 17};
    static const unsigned char S[7] = {15, 16, 16, 14, 1, 1, 30};
    static const unsigned char T[7] = {31, 4, 4, 4, 4, 4, 4};
    static const unsigned char U[7] = {17, 17, 17, 17, 17, 17, 14};
    static const unsigned char V[7] = {17, 17, 17, 17, 17, 10, 4};
    static const unsigned char W[7] = {17, 17, 17, 21, 21, 21, 10};
    static const unsigned char X[7] = {17, 17, 10, 4, 10, 17, 17};
    static const unsigned char Y[7] = {17, 17, 10, 4, 4, 4, 4};
    static const unsigned char Z[7] = {31, 1, 2, 4, 8, 16, 31};

    switch (c) {
    case ' ': return blank;
    case '-': return dash;
    case ':': return colon;
    case '/': return slash;
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

static void draw_text(u32 x, u32 y, const char* text, u32 color) {
    u32 cursor = x;
    while (*text != 0) {
        const unsigned char* data = glyph(*text++);
        for (u32 row = 0; row < GLYPH_H; ++row) {
            for (u32 col = 0; col < GLYPH_W; ++col) {
                if ((data[row] & (1u << (GLYPH_W - 1u - col))) != 0u) {
                    fill_rect(cursor + col * 2u, y + row * 2u, 2u, 2u, color);
                }
            }
        }
        cursor += 14u;
    }
}

static void format_u32(u32 value, char* out) {
    char digits[16];
    u32 count = 0u;
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

static void draw_metric(u32 x, u32 y, const char* label, u32 value) {
    char buffer[16];
    format_u32(value, buffer);
    draw_text(x, y, label, COLOR_TEXT_DIM);
    draw_text(x + 210u, y, buffer, COLOR_TEXT);
}

static void write_mailbox(void) {
    g_mailbox->magic = VEX_COMPOSITOR_MAILBOX_MAGIC;
    g_mailbox->abi_version = 1u;
    g_mailbox->sequence += 1u;
    g_mailbox->packet.abi_version = 1u;
    g_mailbox->packet.opcode = VEX_COMPOSITOR_PRESENT_SURFACE;
    g_mailbox->packet.payload_bytes = sizeof(vex_compositor_packet_t);
    g_mailbox->packet.sequence = g_mailbox->sequence;
    g_mailbox->packet.window.window_id = 1u;
    g_mailbox->packet.window.kind = VEX_WINDOW_DIAGNOSTICS;
    g_mailbox->packet.surface.width = g_boot->framebuffer.width;
    g_mailbox->packet.surface.height = g_boot->framebuffer.height;
    g_mailbox->packet.surface.stride = g_boot->framebuffer.pixels_per_scanline;
    g_mailbox->packet.surface.buffer_count = 3u;
    g_mailbox->packet.surface.present_index = 0u;
}

void _start(void) {
    write_mailbox();
    fill_rect(0u, 0u, fb_width(), fb_height(), COLOR_BG);
    fill_rect(64u, 64u, fb_width() - 128u, 52u, COLOR_TITLE);
    fill_rect(64u, 132u, fb_width() - 128u, fb_height() - 196u, COLOR_PANEL);

    draw_text(92u, 80u, "DIAGNOSTICS DOMAIN", COLOR_TEXT);

    fill_rect(92u, 156u, 420u, 40u, COLOR_ACCENT);
    draw_text(112u, 168u, "BOOT CHANNEL ONLINE", COLOR_TEXT);

    fill_rect(92u, 220u, 520u, 236u, COLOR_ROW);
    draw_metric(116u, 244u, "BOOT REV", g_boot->revision);
    draw_metric(116u, 274u, "APP IMAGES", g_boot->app_image_count);
    draw_metric(116u, 304u, "FB WIDTH", g_boot->framebuffer.width);
    draw_metric(116u, 334u, "FB HEIGHT", g_boot->framebuffer.height);
    draw_metric(116u, 364u, "DIAG VERIFIED", g_boot->app_image_count > 0u ? g_boot->app_images[0].verified : 0u);
    draw_metric(116u, 394u, "PAYLOAD KB", g_boot->app_image_count > 0u ? (u32)(g_boot->app_images[0].payload_size / 1024u) : 0u);

    draw_text(92u, fb_height() - 84u, "USERSPACE PACKAGE ACTIVE", COLOR_TEXT);
    draw_text(92u, fb_height() - 56u, "MICROKERNEL BOOT IMAGE DATA CONSUMED DIRECTLY", COLOR_TEXT_DIM);

    for (;;) {
        write_mailbox();
        __asm__ volatile ("pause");
    }
}
