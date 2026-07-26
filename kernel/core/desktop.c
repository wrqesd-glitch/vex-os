#include "../include/vex/kernel.h"

typedef enum vex_desktop_view {
    VEX_VIEW_DIAGNOSTICS = 0,
    VEX_VIEW_TESTS = 1,
    VEX_VIEW_SERVICES = 2
} vex_desktop_view_t;

typedef struct vex_desktop_state {
    vex_desktop_snapshot_t snapshot;
    u32 selected_icon;
    u32 opened_view;
    u32 has_window;
    u32 ready;
} vex_desktop_state_t;

static vex_desktop_state_t g_desktop;

enum {
    COLOR_BG = 0x000B1220,
    COLOR_PANEL = 0x00111C31,
    COLOR_PANEL_ALT = 0x00162038,
    COLOR_TEXT = 0x00E6F0FF,
    COLOR_TEXT_DIM = 0x009DB7D7,
    COLOR_ACCENT = 0x002F6FED,
    COLOR_SELECTED = 0x00355FA5,
    COLOR_ACTIVE = 0x00213A66,
    SIDEBAR_WIDTH = 24
};

static void format_u64(u64 value, char* out, u32 capacity) {
    char digits[32];
    u32 count = 0;
    if (capacity == 0u) {
        return;
    }
    if (value == 0u) {
        if (capacity > 1u) {
            out[0] = '0';
            out[1] = '\0';
        } else {
            out[0] = '\0';
        }
        return;
    }
    while (value > 0u && count < sizeof(digits)) {
        digits[count++] = (char)('0' + (value % 10u));
        value /= 10u;
    }
    u32 out_index = 0;
    while (count > 0u && out_index + 1u < capacity) {
        out[out_index++] = digits[--count];
    }
    out[out_index] = '\0';
}

static void copy_bytes(void* dst, const void* src, usize size) {
    u8* out = (u8*)dst;
    const u8* in = (const u8*)src;
    for (usize index = 0; index < size; ++index) {
        out[index] = in[index];
    }
}

static void format_hex(u64 value, char* out, u32 capacity) {
    static const char hex[] = "0123456789ABCDEF";
    if (capacity < 19u) {
        if (capacity > 0u) {
            out[0] = '\0';
        }
        return;
    }
    out[0] = '0';
    out[1] = 'x';
    for (u32 i = 0; i < 16u; ++i) {
        const u32 shift = (15u - i) * 4u;
        out[2u + i] = hex[(value >> shift) & 0xFu];
    }
    out[18] = '\0';
}

static void draw_label_value(u32 row, const char* label, const char* value) {
    framebuffer_console_draw_text(SIDEBAR_WIDTH + 4u, row, label, COLOR_TEXT_DIM);
    framebuffer_console_draw_text(SIDEBAR_WIDTH + 22u, row, value, COLOR_TEXT);
}

static void draw_hex_line(u32 row, const char* label, u64 value) {
    char buffer[24];
    format_hex(value, buffer, sizeof(buffer));
    draw_label_value(row, label, buffer);
}

static void draw_dec_line(u32 row, const char* label, u64 value) {
    char buffer[24];
    format_u64(value, buffer, sizeof(buffer));
    draw_label_value(row, label, buffer);
}

static void draw_header(void) {
    const u32 cols = framebuffer_console_columns();
    framebuffer_console_fill_cells(0u, 0u, cols, 3u, COLOR_ACCENT);
    framebuffer_console_draw_text(2u, 1u, "Vex OS desktop", COLOR_TEXT);
    framebuffer_console_draw_text(24u, 1u, "desktop / arrows tab enter esc", COLOR_TEXT);
}

static void draw_sidebar(void) {
    static const char* items[3] = {
        "Diagnostics",
        "Tests",
        "Services"
    };

    framebuffer_console_fill_cells(1u, 4u, SIDEBAR_WIDTH - 2u, 22u, COLOR_PANEL);
    framebuffer_console_draw_text(3u, 5u, "desktop icons", COLOR_TEXT_DIM);
    for (u32 index = 0; index < 3u; ++index) {
        const u32 row = 8u + index * 4u;
        const u32 active = g_desktop.selected_icon == index;
        framebuffer_console_fill_cells(3u, row - 1u, SIDEBAR_WIDTH - 6u, 3u, active ? COLOR_SELECTED : COLOR_PANEL_ALT);
        framebuffer_console_draw_text(5u, row, items[index], COLOR_TEXT);
    }
}

static void draw_window_frame(const char* title) {
    const u32 cols = framebuffer_console_columns();
    const u32 rows = framebuffer_console_rows();
    const u32 left = SIDEBAR_WIDTH + 1u;
    const u32 top = 4u;
    const u32 width = cols - left - 1u;
    const u32 height = rows - top - 3u;
    framebuffer_console_fill_cells(left, top, width, height, COLOR_PANEL);
    framebuffer_console_fill_cells(left, top, width, 3u, COLOR_ACTIVE);
    framebuffer_console_draw_text(left + 2u, top + 1u, title, COLOR_TEXT);
}

static void render_diagnostics(void) {
    draw_window_frame("Diagnostics");
    draw_hex_line(9u, "RSDP", g_desktop.snapshot.rsdp_address);
    draw_hex_line(11u, "FB base", g_desktop.snapshot.framebuffer_base);
    draw_dec_line(13u, "FB width", g_desktop.snapshot.framebuffer_width);
    draw_dec_line(15u, "FB height", g_desktop.snapshot.framebuffer_height);
    draw_dec_line(17u, "FB pitch", g_desktop.snapshot.framebuffer_pitch);
    draw_dec_line(19u, "Memory regions", g_desktop.snapshot.memory_regions);
    draw_dec_line(21u, "Usable pages", g_desktop.snapshot.usable_pages);
    draw_dec_line(23u, "Allocated pages", g_desktop.snapshot.allocated_pages);
    draw_hex_line(25u, "Kernel CR3", g_desktop.snapshot.kernel_cr3);
    draw_dec_line(27u, "Mapped pages", g_desktop.snapshot.kernel_mapped_pages);
    draw_hex_line(29u, "XSDT", g_desktop.snapshot.xsdt_address);
    draw_hex_line(31u, "MADT", g_desktop.snapshot.madt_address);
}

static void render_tests(void) {
    draw_window_frame("Tests");
    framebuffer_console_draw_text(SIDEBAR_WIDTH + 4u, 9u, "boot-smoke", COLOR_TEXT_DIM);
    framebuffer_console_draw_text(SIDEBAR_WIDTH + 24u, 9u, "PASS", COLOR_TEXT);
    framebuffer_console_draw_text(SIDEBAR_WIDTH + 4u, 12u, "kernel selftest", COLOR_TEXT_DIM);
    framebuffer_console_draw_text(SIDEBAR_WIDTH + 24u, 12u,
        g_desktop.snapshot.selftest_passed != 0u ? "PASS" : "FAIL", COLOR_TEXT);
    draw_dec_line(16u, "Timer kind", g_desktop.snapshot.timer_kind);
    draw_hex_line(18u, "Timer base", g_desktop.snapshot.timer_base);
    draw_dec_line(20u, "Timer ready", g_desktop.snapshot.timer_ready);
    draw_dec_line(22u, "CPU count", g_desktop.snapshot.cpu_count);
    draw_dec_line(24u, "IOAPIC count", g_desktop.snapshot.ioapic_count);
    draw_dec_line(26u, "GDT loaded", g_desktop.snapshot.gdt_loaded);
    draw_hex_line(28u, "TSS base", g_desktop.snapshot.tss_base);
    draw_hex_line(30u, "User CS", g_desktop.snapshot.user_cs);
    draw_hex_line(32u, "User SS", g_desktop.snapshot.user_ss);
}

static void render_services(void) {
    draw_window_frame("Services");
    framebuffer_console_draw_text(SIDEBAR_WIDTH + 4u, 9u, "init domain", COLOR_TEXT_DIM);
    framebuffer_console_draw_text(SIDEBAR_WIDTH + 22u, 9u, g_desktop.snapshot.init_domain, COLOR_TEXT);
    framebuffer_console_draw_text(SIDEBAR_WIDTH + 4u, 12u, "entrypoint", COLOR_TEXT_DIM);
    framebuffer_console_draw_text(SIDEBAR_WIDTH + 22u, 12u, g_desktop.snapshot.init_entrypoint, COLOR_TEXT);
    draw_hex_line(16u, "Capabilities", g_desktop.snapshot.init_caps);
    draw_dec_line(18u, "Services", g_desktop.snapshot.service_count);
    draw_dec_line(20u, "Processes", g_desktop.snapshot.process_count);
    draw_hex_line(22u, "Init CR3", g_desktop.snapshot.init_cr3);
    draw_hex_line(24u, "Entry VA", g_desktop.snapshot.init_entry_va);
    draw_hex_line(26u, "Stack VA", g_desktop.snapshot.init_stack_va);
    draw_hex_line(28u, "User RIP", g_desktop.snapshot.user_rip);
    draw_hex_line(30u, "User RSP", g_desktop.snapshot.user_rsp);
}

static void render_desktop(void) {
    if (g_desktop.ready == 0u) {
        return;
    }

    framebuffer_console_clear();
    draw_header();
    draw_sidebar();
    if (g_desktop.has_window == 0u) {
        draw_window_frame("Desktop");
        framebuffer_console_draw_text(SIDEBAR_WIDTH + 4u, 10u, "Select an icon and press Enter.", COLOR_TEXT);
        framebuffer_console_draw_text(SIDEBAR_WIDTH + 4u, 13u, "Diagnostics  : platform, memory, ACPI, VM.", COLOR_TEXT_DIM);
        framebuffer_console_draw_text(SIDEBAR_WIDTH + 4u, 15u, "Tests        : boot-smoke and kernel selftest state.", COLOR_TEXT_DIM);
        framebuffer_console_draw_text(SIDEBAR_WIDTH + 4u, 17u, "Services     : init domain and service graph.", COLOR_TEXT_DIM);
        return;
    }

    switch ((vex_desktop_view_t)g_desktop.opened_view) {
    case VEX_VIEW_DIAGNOSTICS:
        render_diagnostics();
        break;
    case VEX_VIEW_TESTS:
        render_tests();
        break;
    case VEX_VIEW_SERVICES:
        render_services();
        break;
    }
}

void desktop_init(const vex_desktop_snapshot_t* snapshot) {
    if (framebuffer_console_is_ready() == 0u) {
        g_desktop.ready = 0u;
        return;
    }

    copy_bytes(&g_desktop.snapshot, snapshot, sizeof(g_desktop.snapshot));
    g_desktop.selected_icon = 0u;
    g_desktop.opened_view = 0u;
    g_desktop.has_window = 1u;
    g_desktop.ready = 1u;
    render_desktop();
}

void desktop_handle_key(vex_key_event_t key) {
    if (g_desktop.ready == 0u || key == VEX_KEY_NONE) {
        return;
    }

    switch (key) {
    case VEX_KEY_UP:
        if (g_desktop.selected_icon > 0u) {
            g_desktop.selected_icon -= 1u;
        }
        break;
    case VEX_KEY_DOWN:
        if (g_desktop.selected_icon < 2u) {
            g_desktop.selected_icon += 1u;
        }
        break;
    case VEX_KEY_LEFT:
    case VEX_KEY_ESCAPE:
        g_desktop.has_window = 0u;
        break;
    case VEX_KEY_RIGHT:
    case VEX_KEY_ENTER:
        g_desktop.opened_view = g_desktop.selected_icon;
        g_desktop.has_window = 1u;
        break;
    case VEX_KEY_TAB:
        g_desktop.selected_icon = (g_desktop.selected_icon + 1u) % 3u;
        break;
    case VEX_KEY_NONE:
        break;
    }

    render_desktop();
}
