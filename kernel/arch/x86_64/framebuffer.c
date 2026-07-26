#include "../../include/vex/kernel.h"

typedef struct vex_fb_console_state {
    volatile u32* pixels;
    u32 width;
    u32 height;
    u32 pitch;
    u32 cursor_x;
    u32 cursor_y;
    u32 cols;
    u32 rows;
    u32 ready;
} vex_fb_console_state_t;

static vex_fb_console_state_t g_console;

enum {
    GLYPH_WIDTH = 5,
    GLYPH_HEIGHT = 7,
    CELL_WIDTH = 8,
    CELL_HEIGHT = 10,
    LEFT_MARGIN = 12,
    TOP_MARGIN = 56,
    BG_COLOR = 0x000B1220,
    FG_COLOR = 0x00D8E6FF,
    DIM_COLOR = 0x00203B66
};

static void write_pixel(volatile u32* base, u32 pitch, u32 x, u32 y, u32 color) {
    base[(usize)y * (usize)pitch + (usize)x] = color;
}

static void clear_console_state(void) {
    volatile u8* bytes = (volatile u8*)(void*)&g_console;
    for (usize index = 0; index < sizeof(g_console); ++index) {
        bytes[index] = 0u;
    }
}

static void fill_rect(u32 x, u32 y, u32 width, u32 height, u32 color) {
    if (g_console.ready == 0u) {
        return;
    }

    if (x >= g_console.width || y >= g_console.height) {
        return;
    }

    const u32 max_x = x + width > g_console.width ? g_console.width : x + width;
    const u32 max_y = y + height > g_console.height ? g_console.height : y + height;
    for (u32 row = y; row < max_y; ++row) {
        for (u32 col = x; col < max_x; ++col) {
            write_pixel(g_console.pixels, g_console.pitch, col, row, color);
        }
    }
}

static void clear_cell(u32 col, u32 row) {
    fill_rect(
        LEFT_MARGIN + col * CELL_WIDTH,
        TOP_MARGIN + row * CELL_HEIGHT,
        CELL_WIDTH,
        CELL_HEIGHT,
        BG_COLOR
    );
}

static void draw_glyph_at(u32 origin_x, u32 origin_y, const u8* glyph, u32 color, u32 clear_background) {
    if (clear_background != 0u) {
        fill_rect(origin_x, origin_y, CELL_WIDTH, CELL_HEIGHT, BG_COLOR);
    }
    for (u32 glyph_y = 0; glyph_y < GLYPH_HEIGHT; ++glyph_y) {
        const u8 bits = glyph[glyph_y];
        for (u32 glyph_x = 0; glyph_x < GLYPH_WIDTH; ++glyph_x) {
            const u8 mask = (u8)(1u << (GLYPH_WIDTH - 1u - glyph_x));
            if ((bits & mask) != 0u) {
                write_pixel(g_console.pixels, g_console.pitch, origin_x + glyph_x, origin_y + glyph_y, color);
            }
        }
    }
}

static const u8* glyph_for_char(char value) {
    static const u8 glyph_space[GLYPH_HEIGHT] = {0, 0, 0, 0, 0, 0, 0};
    static const u8 glyph_dash[GLYPH_HEIGHT] = {0, 0, 0, 0x0E, 0, 0, 0};
    static const u8 glyph_dot[GLYPH_HEIGHT] = {0, 0, 0, 0, 0, 0x0C, 0x0C};
    static const u8 glyph_colon[GLYPH_HEIGHT] = {0, 0x0C, 0x0C, 0, 0x0C, 0x0C, 0};
    static const u8 glyph_slash[GLYPH_HEIGHT] = {0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x10};
    static const u8 glyph_equal[GLYPH_HEIGHT] = {0, 0x1E, 0, 0x1E, 0, 0, 0};
    static const u8 glyph_underscore[GLYPH_HEIGHT] = {0, 0, 0, 0, 0, 0, 0x1F};
    static const u8 glyph_0[GLYPH_HEIGHT] = {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E};
    static const u8 glyph_1[GLYPH_HEIGHT] = {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E};
    static const u8 glyph_2[GLYPH_HEIGHT] = {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F};
    static const u8 glyph_3[GLYPH_HEIGHT] = {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E};
    static const u8 glyph_4[GLYPH_HEIGHT] = {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02};
    static const u8 glyph_5[GLYPH_HEIGHT] = {0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E};
    static const u8 glyph_6[GLYPH_HEIGHT] = {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E};
    static const u8 glyph_7[GLYPH_HEIGHT] = {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08};
    static const u8 glyph_8[GLYPH_HEIGHT] = {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E};
    static const u8 glyph_9[GLYPH_HEIGHT] = {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x1C};
    static const u8 glyph_A[GLYPH_HEIGHT] = {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
    static const u8 glyph_B[GLYPH_HEIGHT] = {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E};
    static const u8 glyph_C[GLYPH_HEIGHT] = {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E};
    static const u8 glyph_D[GLYPH_HEIGHT] = {0x1C, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1C};
    static const u8 glyph_E[GLYPH_HEIGHT] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F};
    static const u8 glyph_F[GLYPH_HEIGHT] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10};
    static const u8 glyph_G[GLYPH_HEIGHT] = {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E};
    static const u8 glyph_H[GLYPH_HEIGHT] = {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
    static const u8 glyph_I[GLYPH_HEIGHT] = {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E};
    static const u8 glyph_K[GLYPH_HEIGHT] = {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11};
    static const u8 glyph_L[GLYPH_HEIGHT] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F};
    static const u8 glyph_M[GLYPH_HEIGHT] = {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11};
    static const u8 glyph_N[GLYPH_HEIGHT] = {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11};
    static const u8 glyph_O[GLYPH_HEIGHT] = {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
    static const u8 glyph_P[GLYPH_HEIGHT] = {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10};
    static const u8 glyph_R[GLYPH_HEIGHT] = {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11};
    static const u8 glyph_S[GLYPH_HEIGHT] = {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E};
    static const u8 glyph_T[GLYPH_HEIGHT] = {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
    static const u8 glyph_U[GLYPH_HEIGHT] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
    static const u8 glyph_V[GLYPH_HEIGHT] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04};
    static const u8 glyph_W[GLYPH_HEIGHT] = {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A};
    static const u8 glyph_X[GLYPH_HEIGHT] = {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11};
    static const u8 glyph_Y[GLYPH_HEIGHT] = {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04};
    static const u8 glyph_a[GLYPH_HEIGHT] = {0, 0, 0x0E, 0x01, 0x0F, 0x11, 0x0F};
    static const u8 glyph_b[GLYPH_HEIGHT] = {0x10, 0x10, 0x16, 0x19, 0x11, 0x19, 0x16};
    static const u8 glyph_c[GLYPH_HEIGHT] = {0, 0, 0x0E, 0x10, 0x10, 0x10, 0x0E};
    static const u8 glyph_d[GLYPH_HEIGHT] = {0x01, 0x01, 0x0D, 0x13, 0x11, 0x13, 0x0D};
    static const u8 glyph_e[GLYPH_HEIGHT] = {0, 0, 0x0E, 0x11, 0x1F, 0x10, 0x0E};
    static const u8 glyph_f[GLYPH_HEIGHT] = {0x06, 0x08, 0x08, 0x1E, 0x08, 0x08, 0x08};
    static const u8 glyph_g[GLYPH_HEIGHT] = {0, 0, 0x0D, 0x13, 0x13, 0x0D, 0x01};
    static const u8 glyph_h[GLYPH_HEIGHT] = {0x10, 0x10, 0x16, 0x19, 0x11, 0x11, 0x11};
    static const u8 glyph_i[GLYPH_HEIGHT] = {0x04, 0, 0x0C, 0x04, 0x04, 0x04, 0x0E};
    static const u8 glyph_k[GLYPH_HEIGHT] = {0x10, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11};
    static const u8 glyph_l[GLYPH_HEIGHT] = {0x0C, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E};
    static const u8 glyph_m[GLYPH_HEIGHT] = {0, 0, 0x1A, 0x15, 0x15, 0x15, 0x15};
    static const u8 glyph_n[GLYPH_HEIGHT] = {0, 0, 0x16, 0x19, 0x11, 0x11, 0x11};
    static const u8 glyph_o[GLYPH_HEIGHT] = {0, 0, 0x0E, 0x11, 0x11, 0x11, 0x0E};
    static const u8 glyph_p[GLYPH_HEIGHT] = {0, 0, 0x16, 0x19, 0x19, 0x16, 0x10};
    static const u8 glyph_r[GLYPH_HEIGHT] = {0, 0, 0x16, 0x19, 0x10, 0x10, 0x10};
    static const u8 glyph_s[GLYPH_HEIGHT] = {0, 0, 0x0F, 0x10, 0x0E, 0x01, 0x1E};
    static const u8 glyph_t[GLYPH_HEIGHT] = {0x08, 0x08, 0x1E, 0x08, 0x08, 0x08, 0x06};
    static const u8 glyph_u[GLYPH_HEIGHT] = {0, 0, 0x11, 0x11, 0x11, 0x13, 0x0D};
    static const u8 glyph_v[GLYPH_HEIGHT] = {0, 0, 0x11, 0x11, 0x11, 0x0A, 0x04};
    static const u8 glyph_w[GLYPH_HEIGHT] = {0, 0, 0x11, 0x11, 0x15, 0x15, 0x0A};
    static const u8 glyph_x[GLYPH_HEIGHT] = {0, 0, 0x11, 0x0A, 0x04, 0x0A, 0x11};
    static const u8 glyph_y[GLYPH_HEIGHT] = {0, 0, 0x11, 0x13, 0x0D, 0x01, 0x0E};

    switch (value) {
    case ' ': return glyph_space;
    case '-': return glyph_dash;
    case '.': return glyph_dot;
    case ':': return glyph_colon;
    case '/': return glyph_slash;
    case '=': return glyph_equal;
    case '_': return glyph_underscore;
    case '0': return glyph_0;
    case '1': return glyph_1;
    case '2': return glyph_2;
    case '3': return glyph_3;
    case '4': return glyph_4;
    case '5': return glyph_5;
    case '6': return glyph_6;
    case '7': return glyph_7;
    case '8': return glyph_8;
    case '9': return glyph_9;
    case 'A': return glyph_A;
    case 'B': return glyph_B;
    case 'C': return glyph_C;
    case 'D': return glyph_D;
    case 'E': return glyph_E;
    case 'F': return glyph_F;
    case 'G': return glyph_G;
    case 'H': return glyph_H;
    case 'I': return glyph_I;
    case 'K': return glyph_K;
    case 'L': return glyph_L;
    case 'M': return glyph_M;
    case 'N': return glyph_N;
    case 'O': return glyph_O;
    case 'P': return glyph_P;
    case 'R': return glyph_R;
    case 'S': return glyph_S;
    case 'T': return glyph_T;
    case 'U': return glyph_U;
    case 'V': return glyph_V;
    case 'W': return glyph_W;
    case 'X': return glyph_X;
    case 'Y': return glyph_Y;
    case 'a': return glyph_a;
    case 'b': return glyph_b;
    case 'c': return glyph_c;
    case 'd': return glyph_d;
    case 'e': return glyph_e;
    case 'f': return glyph_f;
    case 'g': return glyph_g;
    case 'h': return glyph_h;
    case 'i': return glyph_i;
    case 'k': return glyph_k;
    case 'l': return glyph_l;
    case 'm': return glyph_m;
    case 'n': return glyph_n;
    case 'o': return glyph_o;
    case 'p': return glyph_p;
    case 'r': return glyph_r;
    case 's': return glyph_s;
    case 't': return glyph_t;
    case 'u': return glyph_u;
    case 'v': return glyph_v;
    case 'w': return glyph_w;
    case 'x': return glyph_x;
    case 'y': return glyph_y;
    default: return glyph_space;
    }
}

static void scroll_console(void) {
    const u32 start_y = TOP_MARGIN;
    const u32 end_y = TOP_MARGIN + g_console.rows * CELL_HEIGHT;

    for (u32 y = start_y; y + CELL_HEIGHT < end_y; ++y) {
        volatile u32* dst = g_console.pixels + (usize)y * g_console.pitch + LEFT_MARGIN;
        volatile u32* src = g_console.pixels + (usize)(y + CELL_HEIGHT) * g_console.pitch + LEFT_MARGIN;
        for (u32 x = 0; x < g_console.cols * CELL_WIDTH; ++x) {
            dst[x] = src[x];
        }
    }

    fill_rect(LEFT_MARGIN, end_y - CELL_HEIGHT, g_console.cols * CELL_WIDTH, CELL_HEIGHT, BG_COLOR);
}

static void newline(void) {
    g_console.cursor_x = 0u;
    if (g_console.cursor_y + 1u < g_console.rows) {
        g_console.cursor_y += 1u;
        return;
    }
    scroll_console();
}

static void draw_glyph(u32 col, u32 row, const u8* glyph, u32 color) {
    const u32 origin_x = LEFT_MARGIN + col * CELL_WIDTH;
    const u32 origin_y = TOP_MARGIN + row * CELL_HEIGHT;
    clear_cell(col, row);
    draw_glyph_at(origin_x, origin_y, glyph, color, 0u);
}

void framebuffer_fill_banner(const vex_framebuffer_info_t* framebuffer) {
    if (framebuffer->base == 0 || framebuffer->width == 0 || framebuffer->height == 0) {
        return;
    }

    volatile u32* pixels = (volatile u32*)(usize)framebuffer->base;
    for (u32 y = 0; y < framebuffer->height; ++y) {
        for (u32 x = 0; x < framebuffer->width; ++x) {
            write_pixel(pixels, framebuffer->pixels_per_scanline, x, y, BG_COLOR);
        }
    }

    const u32 max_y = framebuffer->height < 48u ? framebuffer->height : 48u;
    for (u32 y = 0; y < max_y; ++y) {
        for (u32 x = 0; x < framebuffer->width; ++x) {
            const u32 color = (y < 8u || y > (max_y - 8u)) ? 0x001C3A6Bu : 0x00264E8Fu;
            write_pixel(pixels, framebuffer->pixels_per_scanline, x, y, color);
        }
    }
}

void framebuffer_console_init(const vex_framebuffer_info_t* framebuffer) {
    clear_console_state();
    if (framebuffer->base == 0 || framebuffer->width < LEFT_MARGIN + CELL_WIDTH ||
        framebuffer->height < TOP_MARGIN + CELL_HEIGHT) {
        return;
    }

    g_console.pixels = (volatile u32*)(usize)framebuffer->base;
    g_console.width = framebuffer->width;
    g_console.height = framebuffer->height;
    g_console.pitch = framebuffer->pixels_per_scanline;
    g_console.cols = (framebuffer->width - LEFT_MARGIN * 2u) / CELL_WIDTH;
    g_console.rows = (framebuffer->height - TOP_MARGIN - 12u) / CELL_HEIGHT;
    g_console.ready = g_console.cols > 0u && g_console.rows > 0u;
    if (g_console.ready == 0u) {
        return;
    }

    fill_rect(LEFT_MARGIN - 6u, TOP_MARGIN - 8u, g_console.cols * CELL_WIDTH + 12u, g_console.rows * CELL_HEIGHT + 8u, DIM_COLOR);
    fill_rect(LEFT_MARGIN - 4u, TOP_MARGIN - 6u, g_console.cols * CELL_WIDTH + 8u, g_console.rows * CELL_HEIGHT + 4u, BG_COLOR);
    draw_glyph(0u, 0u, glyph_for_char('V'), FG_COLOR);
    draw_glyph(1u, 0u, glyph_for_char('e'), FG_COLOR);
    draw_glyph(2u, 0u, glyph_for_char('x'), FG_COLOR);
    draw_glyph(3u, 0u, glyph_for_char(' '), FG_COLOR);
    draw_glyph(4u, 0u, glyph_for_char('O'), FG_COLOR);
    draw_glyph(5u, 0u, glyph_for_char('S'), FG_COLOR);
    draw_glyph(6u, 0u, glyph_for_char(' '), FG_COLOR);
    draw_glyph(7u, 0u, glyph_for_char('b'), FG_COLOR);
    draw_glyph(8u, 0u, glyph_for_char('r'), FG_COLOR);
    draw_glyph(9u, 0u, glyph_for_char('i'), FG_COLOR);
    draw_glyph(10u, 0u, glyph_for_char('n'), FG_COLOR);
    draw_glyph(11u, 0u, glyph_for_char('g'), FG_COLOR);
    draw_glyph(12u, 0u, glyph_for_char('u'), FG_COLOR);
    draw_glyph(13u, 0u, glyph_for_char('p'), FG_COLOR);
    g_console.cursor_x = 0u;
    g_console.cursor_y = 2u;
}

void framebuffer_console_write_char(char value) {
    if (g_console.ready == 0u) {
        return;
    }

    if (value == '\r') {
        return;
    }
    if (value == '\n') {
        newline();
        return;
    }

    if (g_console.cursor_x >= g_console.cols) {
        newline();
    }

    draw_glyph(g_console.cursor_x, g_console.cursor_y, glyph_for_char(value), FG_COLOR);
    g_console.cursor_x += 1u;
}

u32 framebuffer_console_columns(void) {
    return g_console.cols;
}

u32 framebuffer_console_rows(void) {
    return g_console.rows;
}

u32 framebuffer_console_is_ready(void) {
    return g_console.ready;
}

void framebuffer_console_clear(void) {
    if (g_console.ready == 0u) {
        return;
    }
    fill_rect(LEFT_MARGIN - 4u, TOP_MARGIN - 6u, g_console.cols * CELL_WIDTH + 8u, g_console.rows * CELL_HEIGHT + 4u, BG_COLOR);
    g_console.cursor_x = 0u;
    g_console.cursor_y = 0u;
}

void framebuffer_console_draw_text(u32 col, u32 row, const char* text, u32 color) {
    if (g_console.ready == 0u || row >= g_console.rows) {
        return;
    }

    u32 current_col = col;
    while (*text != '\0' && current_col < g_console.cols) {
        const u32 origin_x = LEFT_MARGIN + current_col * CELL_WIDTH;
        const u32 origin_y = TOP_MARGIN + row * CELL_HEIGHT;
        draw_glyph_at(origin_x, origin_y, glyph_for_char(*text++), color, 1u);
        current_col += 1u;
    }
}

void framebuffer_console_fill_cells(u32 col, u32 row, u32 width, u32 height, u32 color) {
    if (g_console.ready == 0u || col >= g_console.cols || row >= g_console.rows) {
        return;
    }

    const u32 max_cols = col + width > g_console.cols ? g_console.cols - col : width;
    const u32 max_rows = row + height > g_console.rows ? g_console.rows - row : height;
    fill_rect(
        LEFT_MARGIN + col * CELL_WIDTH,
        TOP_MARGIN + row * CELL_HEIGHT,
        max_cols * CELL_WIDTH,
        max_rows * CELL_HEIGHT,
        color
    );
}
