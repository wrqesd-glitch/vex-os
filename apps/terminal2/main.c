#include "../init/vex_boot_info.h"
#include "../init/vex_gpu_proto.h"
#include "../init/vex_ui_proto.h"

typedef unsigned long long u64;
typedef unsigned int u32;

#define FB_BASE 0x50000000ull
#define BOOTINFO_BASE 0x50400000ull
#define MAILBOX_BASE 0x70002000ull

#define TERM_MAX_ROWS 24
#define TERM_MAX_COLS 80
#define TERM_HISTORY_MAX 64
#define TERM_PROMPT "VexOS> "
#define TERM_PROMPT_LEN 8

static volatile vex_boot_info_t* const g_boot = (volatile vex_boot_info_t*)BOOTINFO_BASE;
static volatile u32* const g_pixels = (volatile u32*)FB_BASE;
static volatile vex_compositor_mailbox_t* const g_mailbox = (volatile vex_compositor_mailbox_t*)MAILBOX_BASE;

enum {
    COLOR_BG = 0x0009121Fu,
    COLOR_TEXT = 0x00F1F6FFu,
    COLOR_PROMPT = 0x009973FFu,
    COLOR_ERROR = 0x00FF6666u,
    COLOR_HEADER = 0x00CCCCCCu,
    COLOR_SELECT = 0x003D85EDu
};

typedef struct term_line {
    char data[TERM_MAX_COLS + 1];
    u32 len;
} term_line_t;

typedef struct term_state {
    term_line_t lines[TERM_MAX_ROWS];
    u32 cursor_row;
    u32 cursor_col;
    u32 scroll_top;
    char history[TERM_HISTORY_MAX][TERM_MAX_COLS + 1];
    u32 history_count;
    u32 history_index;
    u32 input_len;
    char input[TERM_MAX_COLS + 1];
    u32 dirty;
    u32 fb_width;
    u32 fb_height;
    u32 cell_w;
    u32 cell_h;
} term_state_t;

static term_state_t g_term;
static u32 g_sequence;

static void copy_string(char* dst, u32 dst_size, const char* src) {
    u32 i = 0u;
    if (dst_size == 0u) return;
    while (src[i] != 0 && i + 1u < dst_size) { dst[i] = src[i]; ++i; }
    dst[i] = 0;
}

static u32 width(void) { return g_boot->framebuffer.width; }
static u32 height(void) { return g_boot->framebuffer.height; }
static u32 pitch(void) { return g_boot->framebuffer.pixels_per_scanline; }

static void put_pixel(u32 x, u32 y, u32 color) {
    if (x >= width() || y >= height()) return;
    g_pixels[(u64)y * pitch() + x] = color;
}

static void fill_rect(u32 x, u32 y, u32 w, u32 h, u32 color) {
    const u32 max_x = x + w > width() ? width() : x + w;
    const u32 max_y = y + h > height() ? height() : y + h;
    for (u32 yy = y; yy < max_y; ++yy) {
        for (u32 xx = x; xx < max_x; ++xx) { put_pixel(xx, yy, color); }
    }
}

static const unsigned char glyph_a5x7[128][7] = {
    [' '] = {0,0,0,0,0,0,0},
    ['!'] = {4,4,4,4,0,4,0},
    ['"'] = {10,10,10,0,0,0,0},
    ['#'] = {10,10,31,10,31,10,0},
    ['$'] = {12,18,20,8,18,12,0},
    ['%'] = {24,4,8,16,18,2,0},
    ['&'] = {8,20,16,22,18,14,0},
    ['\''] = {8,8,8,0,0,0,0},
    ['('] = {2,4,8,8,8,4,2},
    [')'] = {8,4,2,2,2,4,8},
    ['*'] = {0,4,21,14,21,4,0},
    ['+'] = {0,4,4,31,4,4,0},
    [','] = {0,0,0,0,4,4,8},
    ['-'] = {0,0,0,31,0,0,0},
    ['.'] = {0,0,0,0,6,6,6},
    ['/'] = {2,2,4,8,8,16,16},
    ['0'] = {14,27,25,29,19,31,14},
    ['1'] = {4,12,4,4,4,4,14},
    ['2'] = {14,27,1,2,4,8,31},
    ['3'] = {14,27,1,6,1,27,14},
    ['4'] = {2,6,10,18,31,2,2},
    ['5'] = {31,16,30,1,1,27,14},
    ['6'] = {6,8,16,30,27,27,14},
    ['7'] = {31,25,2,4,4,4,4},
    ['8'] = {14,27,27,14,27,27,14},
    ['9'] = {14,27,27,15,1,2,12},
    [':'] = {0,6,6,0,6,6,0},
    [';'] = {0,6,6,0,6,4,8},
    ['<'] = {2,4,8,16,8,4,2},
    ['='] = {0,0,31,0,31,0,0},
    ['>'] = {8,4,2,1,2,4,8},
    ['?'] = {14,27,1,2,4,0,4},
    ['@'] = {14,27,25,25,19,16,14},
    ['A'] = {14,27,27,31,27,27,27},
    ['B'] = {30,27,27,30,27,27,30},
    ['C'] = {14,27,16,16,16,27,14},
    ['D'] = {28,26,27,27,27,26,28},
    ['E'] = {31,16,16,30,16,16,31},
    ['F'] = {31,16,16,30,16,16,16},
    ['G'] = {14,27,16,23,27,27,14},
    ['H'] = {27,27,27,31,27,27,27},
    ['I'] = {14,4,4,4,4,4,14},
    ['J'] = {7,1,1,1,25,25,12},
    ['K'] = {27,18,20,24,20,18,19},
    ['L'] = {16,16,16,16,16,16,31},
    ['M'] = {27,27,31,31,27,27,27},
    ['N'] = {27,27,19,15,23,27,27},
    ['O'] = {14,27,27,27,27,27,14},
    ['P'] = {30,27,27,30,16,16,16},
    ['Q'] = {14,27,27,27,29,17,14},
    ['R'] = {30,27,27,30,20,18,19},
    ['S'] = {14,27,16,14,1,27,14},
    ['T'] = {31,4,4,4,4,4,4},
    ['U'] = {27,27,27,27,27,27,14},
    ['V'] = {27,27,27,10,10,4,4},
    ['W'] = {27,27,27,31,31,10,10},
    ['X'] = {27,27,10,4,10,27,27},
    ['Y'] = {27,10,4,4,4,4,4},
    ['Z'] = {31,1,2,4,8,16,31},
    ['['] = {14,8,8,8,8,8,14},
    ['\\'] = {16,8,4,2,2,4,8},
    [']'] = {14,2,2,2,2,2,14},
    ['^'] = {4,10,17,0,0,0,0},
    ['_'] = {0,0,0,0,0,31,0},
    ['`'] = {16,8,4,0,0,0,0},
    ['a'] = {0,0,14,1,15,27,14},
    ['b'] = {16,16,30,27,27,27,30},
    ['c'] = {0,0,14,27,16,27,14},
    ['d'] = {1,1,15,27,27,27,15},
    ['e'] = {0,0,14,27,31,16,14},
    ['f'] = {2,4,4,14,4,4,4},
    ['g'] = {0,15,27,27,15,1,14},
    ['h'] = {16,16,30,27,27,27,27},
    ['i'] = {4,0,12,4,4,4,14},
    ['j'] = {2,0,6,2,2,18,12},
    ['k'] = {16,18,20,24,20,18,19},
    ['l'] = {12,4,4,4,4,4,14},
    ['m'] = {0,0,26,31,31,27,27},
    ['n'] = {0,0,30,27,27,27,27},
    ['o'] = {0,0,14,27,27,27,14},
    ['p'] = {0,0,30,27,27,30,16},
    ['q'] = {0,0,15,27,27,15,1},
    ['r'] = {0,0,22,24,16,16,16},
    ['s'] = {0,0,14,16,14,1,14},
    ['t'] = {4,4,14,4,4,4,2},
    ['u'] = {0,0,27,27,27,27,14},
    ['v'] = {0,0,27,27,10,10,4},
    ['w'] = {0,0,27,27,31,10,10},
    ['x'] = {0,0,27,10,4,10,27},
    ['y'] = {0,0,27,27,15,1,14},
    ['z'] = {0,0,31,2,4,8,31},
    ['{'] = {2,4,4,8,4,4,2},
    ['|'] = {4,4,4,0,4,4,4},
    ['}'] = {8,4,4,2,4,4,8},
    ['~'] = {0,0,8,15,18,0,0},
};

static void draw_char(u32 x, u32 y, char c, u32 color) {
    const unsigned char* g = glyph_a5x7[(unsigned char)c];
    for (u32 row = 0; row < 7; ++row) {
        for (u32 col = 0; col < 5; ++col) {
            if (g[row] & (1u << (4 - col))) {
                put_pixel(x + col, y + row, color);
            }
        }
    }
}

static void draw_text(u32 x, u32 y, const char* text, u32 color) {
    u32 i = 0;
    while (text[i] != 0) { draw_char(x + i * 5u, y, text[i], color); ++i; }
}

static void scrolled_draw_text(u32 x, u32 y, const char* text, u32 color, u32 window_height, u32 start_y) {
    if (y >= start_y + window_height) return;
    if (y + 7 <= start_y) return;
    draw_text(x, y, text, color);
}

static void term_push_line(const char* text) {
    u32 len = 0;
    while (text[len] != 0 && len < TERM_MAX_COLS) ++len;
    if (len == 0) return;

    if (g_term.cursor_row >= TERM_MAX_ROWS) {
        for (u32 i = 1; i < TERM_MAX_ROWS; ++i) {
            for (u32 j = 0; j < TERM_MAX_COLS; ++j) g_term.lines[i - 1].data[j] = g_term.lines[i].data[j];
            g_term.lines[i - 1].len = g_term.lines[i].len;
        }
        g_term.lines[TERM_MAX_ROWS - 1].len = len;
        for (u32 i = 0; i < len && i < TERM_MAX_COLS; ++i) g_term.lines[TERM_MAX_ROWS - 1].data[i] = text[i];
        g_term.cursor_row = TERM_MAX_ROWS - 1;
    } else {
        g_term.lines[g_term.cursor_row].len = len;
        for (u32 i = 0; i < len && i < TERM_MAX_COLS; ++i) g_term.lines[g_term.cursor_row].data[i] = text[i];
        ++g_term.cursor_row;
    }
    if (g_term.cursor_col >= TERM_MAX_COLS) {
        g_term.cursor_col = 0u;
        if (g_term.cursor_row < TERM_MAX_ROWS) ++g_term.cursor_row;
    }
    g_term.dirty = 1u;
}

static void term_clear(void) {
    for (u32 i = 0; i < TERM_MAX_ROWS; ++i) g_term.lines[i].len = 0u;
    g_term.cursor_row = 0u;
    g_term.cursor_col = 0u;
    g_term.dirty = 1u;
}

static void term_draw_frame(void) {
    const u32 fb_w = g_term.fb_width;
    const u32 fb_h = g_term.fb_height;
    fill_rect(0u, 0u, fb_w, fb_h, COLOR_BG);

    u32 draw_width = fb_w;
    u32 draw_height = fb_h - 48u;
    u32 start_x = 0u;
    u32 start_y = 0u;
    if (draw_width > (u32)TERM_MAX_COLS * 7u) {
        start_x = (draw_width - (u32)TERM_MAX_COLS * 7u) / 2u;
        draw_width = (u32)TERM_MAX_COLS * 7u;
    }
    start_y = 0u;

    for (u32 i = g_term.scroll_top; i < TERM_MAX_ROWS && start_y + (i - g_term.scroll_top) * 9u + 7u < fb_h; ++i) {
        if (g_term.lines[i].len == 0u) continue;
        char tmp[TERM_MAX_COLS + 1];
        u32 copy_len = g_term.lines[i].len < TERM_MAX_COLS ? g_term.lines[i].len : TERM_MAX_COLS;
        for (u32 j = 0; j < copy_len; ++j) tmp[j] = g_term.lines[i].data[j];
        tmp[copy_len] = 0;
        scrolled_draw_text(start_x, start_y + (i - g_term.scroll_top) * 9u, tmp, COLOR_TEXT, draw_height, start_y);
    }

    char prompt[TERM_MAX_COLS + 1];
    u32 pos = 0;
    for (u32 i = 0; i < TERM_PROMPT_LEN && pos < TERM_MAX_COLS; ++i, ++pos) prompt[pos] = TERM_PROMPT[i];
    for (u32 i = 0; i < g_term.input_len && pos < TERM_MAX_COLS; ++i, ++pos) prompt[pos] = g_term.input[i];
    prompt[pos] = 0;

    u32 cursor_visual_row = (g_term.cursor_row > g_term.scroll_top ? g_term.cursor_row : g_term.scroll_top);
    if (cursor_visual_row >= TERM_MAX_ROWS) cursor_visual_row = TERM_MAX_ROWS - 1;
    scrolled_draw_text(start_x, start_y + (cursor_visual_row - g_term.scroll_top) * 9u, prompt, COLOR_PROMPT, draw_height, start_y);
    g_term.dirty = 0u;
}

static void term_append_history(const char* text) {
    u32 len = 0;
    while (text[len] != 0 && len < TERM_MAX_COLS) ++len;
    if (len == 0) return;
    if (g_term.history_count >= TERM_HISTORY_MAX) {
        for (u32 i = 1; i < TERM_HISTORY_MAX; ++i) copy_string(g_term.history[i - 1], TERM_MAX_COLS + 1, g_term.history[i]);
        g_term.history_count = TERM_HISTORY_MAX - 1;
    }
    copy_string(g_term.history[g_term.history_count], TERM_MAX_COLS + 1, text);
    ++g_term.history_count;
}

static void write_mailbox(void) {
    g_mailbox->magic = VEX_COMPOSITOR_MAILBOX_MAGIC;
    g_mailbox->abi_version = 1u;
    g_sequence += 1u;
    g_mailbox->sequence = g_sequence;
    g_mailbox->packet.abi_version = 1u;
    g_mailbox->packet.opcode = VEX_COMPOSITOR_PRESENT_SURFACE;
    g_mailbox->packet.payload_bytes = sizeof(vex_compositor_packet_t);
    g_mailbox->packet.sequence = g_sequence;
    g_mailbox->packet.window.window_id = 6u;
    g_mailbox->packet.window.kind = VEX_WINDOW_TERMINAL;
    g_mailbox->packet.surface.width = g_boot->framebuffer.width;
    g_mailbox->packet.surface.height = g_boot->framebuffer.height;
    g_mailbox->packet.surface.stride = g_boot->framebuffer.pixels_per_scanline;
    g_mailbox->packet.surface.buffer_count = 3u;
    g_mailbox->packet.surface.present_index = 0u;
}

static void term_draw_manual_colon(void) {
    u32 draw_width = g_term.fb_width;
    if (draw_width > (u32)TERM_MAX_COLS * 7u) draw_width = (u32)TERM_MAX_COLS * 7u;
    u32 start_y = 0u;
    u32 line = g_term.cursor_row > g_term.scroll_top ? g_term.cursor_row : g_term.scroll_top;
    if (line >= TERM_MAX_ROWS) line = TERM_MAX_ROWS - 1;
    const u32 x = 0u;
    const u32 y = start_y + (line - g_term.scroll_top) * 9u + 0u;
    draw_char(x + TERM_PROMPT_LEN * 5u, y, ':', COLOR_PROMPT);
}

void _start(void) {
    g_term.cursor_row = 0u;
    g_term.cursor_col = 0u;
    g_term.scroll_top = 0u;
    g_term.history_count = 0u;
    g_term.history_index = 0u;
    g_term.input_len = 0u;
    g_term.input[0] = 0;
    g_term.dirty = 1u;
    g_term.fb_width = g_boot->framebuffer.width;
    g_term.fb_height = g_boot->framebuffer.height;
    g_term.cell_w = 5u;
    g_term.cell_h = 9u;
    g_sequence = 0u;

    term_push_line("VexOS console v1.0");
    term_push_line("type: help / ls / cd / cat / clear");
    term_push_line("");
    term_draw_manual_colon();

    write_mailbox();
    for (;;) {
        if (g_term.dirty != 0u) term_draw_frame();
        write_mailbox();
        __asm__ volatile ("pause");
    }
}
