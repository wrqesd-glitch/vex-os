#include "../../include/vex/kernel.h"

typedef struct __attribute__((packed)) vex_idt_entry {
    u16 offset_low;
    u16 selector;
    u8 ist;
    u8 type_attributes;
    u16 offset_mid;
    u32 offset_high;
    u32 reserved;
} vex_idt_entry_t;

typedef struct __attribute__((packed)) vex_idt_descriptor {
    u16 limit;
    u64 base;
} vex_idt_descriptor_t;

typedef struct vex_fault_frame {
    u64 vector;
    u64 error_code;
    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;
} vex_fault_frame_t;

extern void vex_isr_ts(void);
extern void vex_isr_np(void);
extern void vex_isr_ss(void);
extern void vex_isr_gp(void);
extern void vex_isr_pf(void);
extern void vex_isr_ud(void);
extern void vex_isr_df(void);

static vex_idt_entry_t g_idt[256];

static void serial_write_text(const char* text) {
    while (*text != '\0') {
        serial_write_char(*text++);
    }
}

static void serial_write_u64_value(u64 value) {
    char digits[32];
    u32 count = 0;
    if (value == 0u) {
        serial_write_char('0');
        return;
    }
    while (value != 0u) {
        digits[count++] = (char)('0' + (value % 10u));
        value /= 10u;
    }
    while (count > 0u) {
        serial_write_char(digits[--count]);
    }
}

static void serial_write_hex_value(u64 value) {
    static const char hex[] = "0123456789ABCDEF";
    serial_write_text("0x");
    for (u32 shift = 16u; shift > 0u; --shift) {
        const u32 nibble = (u32)((value >> ((shift - 1u) * 4u)) & 0xFu);
        serial_write_char(hex[nibble]);
    }
}

static void set_gate(u8 vector, void (*handler)(void)) {
    const u64 address = (u64)(usize)handler;
    g_idt[vector].offset_low = (u16)(address & 0xFFFFu);
    g_idt[vector].selector = 0x08u;
    g_idt[vector].ist = 0u;
    g_idt[vector].type_attributes = 0x8Eu;
    g_idt[vector].offset_mid = (u16)((address >> 16) & 0xFFFFu);
    g_idt[vector].offset_high = (u32)((address >> 32) & 0xFFFFFFFFu);
    g_idt[vector].reserved = 0u;
}

static void load_idt(const vex_idt_descriptor_t* descriptor) {
    __asm__ volatile ("lidt (%0)" : : "r"(descriptor));
}

void interrupts_init(void) {
    for (u32 i = 0; i < 256u; ++i) {
        g_idt[i].offset_low = 0u;
        g_idt[i].selector = 0u;
        g_idt[i].ist = 0u;
        g_idt[i].type_attributes = 0u;
        g_idt[i].offset_mid = 0u;
        g_idt[i].offset_high = 0u;
        g_idt[i].reserved = 0u;
    }

    set_gate(6u, vex_isr_ud);
    set_gate(8u, vex_isr_df);
    set_gate(10u, vex_isr_ts);
    set_gate(11u, vex_isr_np);
    set_gate(12u, vex_isr_ss);
    set_gate(13u, vex_isr_gp);
    set_gate(14u, vex_isr_pf);

    vex_idt_descriptor_t descriptor = {
        .limit = (u16)(sizeof(g_idt) - 1u),
        .base = (u64)(usize)g_idt
    };
    load_idt(&descriptor);
}

void vex_interrupt_fault(const vex_fault_frame_t* frame) {
    serial_write_text("\nvex:fault");
    serial_write_text(" vector=");
    serial_write_u64_value(frame->vector);
    serial_write_text(" error=");
    serial_write_hex_value(frame->error_code);
    serial_write_text(" rip=");
    serial_write_hex_value(frame->rip);
    serial_write_text(" cs=");
    serial_write_hex_value(frame->cs);
    serial_write_text(" rsp=");
    serial_write_hex_value(frame->rsp);
    serial_write_text(" ss=");
    serial_write_hex_value(frame->ss);
    serial_write_text("\n");
}
