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
    COLOR_BG = 0x0010181Fu,
    COLOR_PANEL = 0x0018232Du,
    COLOR_PANEL_ALT = 0x00111A29u,
    COLOR_ACCENT = 0x003D85EDu,
    COLOR_TEXT = 0x00F1F6FFu,
    COLOR_TEXT_SOFT = 0x008FA7CCu,
    COLOR_TEXT_DIM = 0x00B8CAE8u,
    COLOR_OK = 0x0041B76Du,
    COLOR_WARN = 0x00C88B2Du
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
    static const unsigned char dot[7] = {0,0,0,0,0,12,12};
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
    static const unsigned char K[7] = {17,18,20,24,20,18,17};
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

    switch (c) {
    case ' ': return blank;
    case '-': return dash;
    case '.': return dot;
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
    case 'K': return K;
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
    case 'W': return W;
    case 'X': return X;
    case 'Y': return Y;
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

static void append_volatile_string(char* dst, u32 dst_size, const volatile char* src) {
    u32 index = 0u;
    while (index + 1u < dst_size && dst[index] != 0) {
        ++index;
    }
    while (*src != 0 && index + 1u < dst_size) {
        dst[index++] = (char)*src++;
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

static u32 verified_package_count(void) {
    u32 count = 0u;
    for (u32 index = 0u; index < g_boot->boot_file_count; ++index) {
        const volatile vex_boot_file_entry_t* entry = &g_boot->boot_files[index];
        if ((entry->flags & VEX_BOOT_FILE_PACKAGE) != 0u &&
            (entry->flags & VEX_BOOT_FILE_VERIFIED) != 0u) {
            ++count;
        }
    }
    return count;
}

static const volatile vex_boot_file_entry_t* file_at(u32 index) {
    if (index >= g_boot->boot_file_count) {
        return 0;
    }
    return &g_boot->boot_files[index];
}

static void draw_entry_line(u32 x, u32 y, u32 index, const volatile vex_boot_file_entry_t* entry) {
    char text[96];
    text[0] = 0;
    if (entry == 0) {
        append_string(text, sizeof(text), "EMPTY");
        draw_text(x, y, text, 1u, COLOR_TEXT_DIM);
        return;
    }

    if ((entry->flags & VEX_BOOT_FILE_DIRECTORY) != 0u) {
        append_string(text, sizeof(text), "DIR ");
    } else if ((entry->flags & VEX_BOOT_FILE_PACKAGE) != 0u) {
        append_string(text, sizeof(text), "PKG ");
    } else {
        append_string(text, sizeof(text), "FILE ");
    }
    append_u32(text, sizeof(text), index);
    append_string(text, sizeof(text), ": ");
    append_volatile_string(text, sizeof(text), entry->path);
    draw_text(x, y, text, 1u, COLOR_TEXT);

    text[0] = 0;
    append_string(text, sizeof(text), "SIZE ");
    append_u32(text, sizeof(text), (u32)(entry->size / 1024u));
    append_string(text, sizeof(text), "KB");
    if ((entry->flags & VEX_BOOT_FILE_VERIFIED) != 0u) {
        append_string(text, sizeof(text), " VERIFIED");
    }
    draw_text(x + 530u, y, text, 1u, (entry->flags & VEX_BOOT_FILE_VERIFIED) != 0u ? COLOR_OK : COLOR_TEXT_SOFT);
}

static void write_mailbox(void) {
    g_mailbox->magic = VEX_COMPOSITOR_MAILBOX_MAGIC;
    g_mailbox->abi_version = 1u;
    g_mailbox->sequence += 1u;
    g_mailbox->packet.abi_version = 1u;
    g_mailbox->packet.opcode = VEX_COMPOSITOR_PRESENT_SURFACE;
    g_mailbox->packet.payload_bytes = sizeof(vex_compositor_packet_t);
    g_mailbox->packet.sequence = g_mailbox->sequence;
    g_mailbox->packet.window.window_id = 3u;
    g_mailbox->packet.window.kind = VEX_WINDOW_SERVICES;
    g_mailbox->packet.surface.width = g_boot->framebuffer.width;
    g_mailbox->packet.surface.height = g_boot->framebuffer.height;
    g_mailbox->packet.surface.stride = g_boot->framebuffer.pixels_per_scanline;
    g_mailbox->packet.surface.buffer_count = 3u;
    g_mailbox->packet.surface.present_index = 0u;
}

void _start(void) {
    write_mailbox();
    fill_rect(0u, 0u, width(), height(), COLOR_BG);
    fill_rect(56u, 52u, width() - 112u, height() - 104u, COLOR_PANEL);
    fill_rect(56u, 52u, width() - 112u, 8u, COLOR_ACCENT);
    fill_rect(78u, 86u, width() - 156u, 98u, COLOR_PANEL_ALT);
    fill_rect(78u, 210u, width() - 156u, height() - 288u, COLOR_PANEL_ALT);

    draw_text(92u, 96u, "EXPLORER", 2u, COLOR_TEXT);
    draw_text(92u, 132u, "BOOT VOLUME CATALOG", 1u, COLOR_TEXT_SOFT);
    draw_text(92u, 158u, "SOURCE:", 1u, COLOR_TEXT_DIM);
    {
        char volume_name[16];
        volume_name[0] = 0;
        copy_string(volume_name, sizeof(volume_name), g_boot->boot_volume_name);
        draw_text(172u, 158u, volume_name, 1u, COLOR_TEXT);
    }
    draw_text(274u, 158u, "FILES:", 1u, COLOR_TEXT_DIM);
    {
        char count_text[16];
        count_text[0] = 0;
        append_u32(count_text, sizeof(count_text), g_boot->boot_file_count);
        draw_text(340u, 158u, count_text, 1u, COLOR_TEXT);
        count_text[0] = 0;
        append_u32(count_text, sizeof(count_text), verified_package_count());
        draw_text(432u, 158u, "VERIFIED:", 1u, COLOR_TEXT_DIM);
        draw_text(534u, 158u, count_text, 1u, COLOR_OK);
    }
    fill_rect(92u, 154u, 62u, 22u, COLOR_OK);
    draw_text(104u, 161u, "READY", 1u, COLOR_TEXT);
    fill_rect(168u, 154u, 70u, 22u, COLOR_WARN);
    draw_text(180u, 161u, "RO", 1u, COLOR_TEXT);

    draw_text(92u, 222u, "INDEX", 1u, COLOR_TEXT_DIM);
    draw_text(148u, 222u, "TYPE PATH", 1u, COLOR_TEXT_DIM);
    draw_text(612u, 222u, "STATE", 1u, COLOR_TEXT_DIM);

    for (u32 row = 0u; row < 8u; ++row) {
        draw_entry_line(92u, 250u + row * 28u, row, file_at(row));
    }

    draw_text(92u, height() - 58u, "REAL UEFI FILE ENUMERATION EXPOSED BY BOOTINFO REVISION 2", 1u, COLOR_TEXT_SOFT);
    draw_text(92u, height() - 32u, "PACKAGES ARE DISCOVERED FROM ACTUAL BOOT VOLUME CONTENTS", 1u, COLOR_TEXT_SOFT);

    for (;;) {
        write_mailbox();
        __asm__ volatile ("pause");
    }
}
