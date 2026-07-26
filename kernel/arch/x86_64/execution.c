#include "../../include/vex/kernel.h"

typedef struct __attribute__((packed)) vex_gdt_descriptor {
    u16 limit;
    u64 base;
} vex_gdt_descriptor_t;

typedef struct __attribute__((packed)) vex_tss {
    u32 reserved0;
    u64 rsp0;
    u64 rsp1;
    u64 rsp2;
    u64 reserved1;
    u64 ist1;
    u64 ist2;
    u64 ist3;
    u64 ist4;
    u64 ist5;
    u64 ist6;
    u64 ist7;
    u64 reserved2;
    u16 reserved3;
    u16 io_map_base;
} vex_tss_t;

static u64 g_gdt[8];
static vex_tss_t g_tss;
static vex_execution_state_t g_execution_state;
static u8 g_fault_stack[4096] __attribute__((aligned(16)));
static u8 g_transition_stack[4096] __attribute__((aligned(16)));
static u64 g_kernel_fault_stack_top;

enum {
    IA32_EFER = 0xC0000080u,
    IA32_STAR = 0xC0000081u,
    IA32_LSTAR = 0xC0000082u,
    IA32_FMASK = 0xC0000084u,
    IA32_EFER_SCE = 1u
};

static u64 make_code_data_descriptor(u32 access, u32 flags) {
    return ((u64)access << 40) | ((u64)flags << 52);
}

static u64 read_msr(u32 msr) {
    u32 low;
    u32 high;
    __asm__ volatile ("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((u64)high << 32) | low;
}

static void write_msr(u32 msr, u64 value) {
    const u32 low = (u32)(value & 0xFFFFFFFFu);
    const u32 high = (u32)(value >> 32);
    __asm__ volatile ("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

static void load_gdt(const vex_gdt_descriptor_t* descriptor) {
    __asm__ volatile ("lgdt (%0)" : : "r"(descriptor));
}

static void load_task_register(u16 selector) {
    __asm__ volatile ("ltr %0" : : "r"(selector));
}

static void load_data_segments(u16 selector) {
    __asm__ volatile (
        "mov %0, %%ds\n\t"
        "mov %0, %%es\n\t"
        "mov %0, %%ss\n\t"
        :
        : "r"(selector)
        : "memory"
    );
}

static void write_tss_descriptor(u16 selector, u64 base, u32 limit) {
    const u32 slot = selector / 8u;
    const u64 low =
        (limit & 0xFFFFu) |
        ((base & 0xFFFFFFu) << 16) |
        ((u64)0x89u << 40) |
        (((u64)limit >> 16) & 0xFu) << 48 |
        ((base >> 24) & 0xFFu) << 56;
    const u64 high = base >> 32;
    g_gdt[slot] = low;
    g_gdt[slot + 1u] = high;
}

void execution_init(void) {
    const u16 kernel_code = 0x08u;
    const u16 kernel_data = 0x10u;
    const u16 user_data = 0x18u;
    const u16 user_code = 0x20u;
    const u16 tss_selector = 0x28u;

    for (u32 i = 0; i < 8u; ++i) {
        g_gdt[i] = 0;
    }

    g_gdt[1] = make_code_data_descriptor(0x9Au, 0x2u);
    g_gdt[2] = make_code_data_descriptor(0x92u, 0x0u);
    g_gdt[3] = make_code_data_descriptor(0xF2u, 0x0u);
    g_gdt[4] = make_code_data_descriptor(0xFAu, 0x2u);

    g_kernel_fault_stack_top = (u64)(usize)&g_fault_stack[4096];

    g_tss.rsp0 = g_kernel_fault_stack_top;
    g_tss.io_map_base = sizeof(vex_tss_t);
    write_tss_descriptor(tss_selector, (u64)(usize)&g_tss, sizeof(vex_tss_t) - 1u);

    vex_gdt_descriptor_t descriptor = {
        .limit = (u16)(sizeof(g_gdt) - 1u),
        .base = (u64)(usize)g_gdt
    };

    load_gdt(&descriptor);
    load_data_segments(kernel_data);
    load_task_register(tss_selector);

    g_execution_state.kernel_code_selector = kernel_code;
    g_execution_state.kernel_data_selector = kernel_data;
    g_execution_state.user_code_selector = user_code | 0x3u;
    g_execution_state.user_data_selector = user_data | 0x3u;
    g_execution_state.tss_selector = tss_selector;
    g_execution_state.tss_base = (u64)(usize)&g_tss;
    g_execution_state.rsp0 = g_kernel_fault_stack_top;
    g_execution_state.syscall_entry = (u64)(usize)vex_syscall_entry;
    g_execution_state.syscall_stack_top = (u64)(usize)&g_transition_stack[4096];
    g_execution_state.gdt_loaded = 1u;

    write_msr(IA32_EFER, read_msr(IA32_EFER) | IA32_EFER_SCE);
    write_msr(
        IA32_STAR,
        ((u64)kernel_code << 32) | ((u64)(user_code | 0x3u) << 48)
    );
    write_msr(IA32_LSTAR, g_execution_state.syscall_entry);
    write_msr(IA32_FMASK, 0x200u);
    g_execution_state.syscall_ready = 1u;
}

const vex_execution_state_t* execution_state(void) {
    return &g_execution_state;
}

int execution_prepare_user_context(
    const vex_process_t* process,
    u64 user_entrypoint,
    u64 user_stack_pointer,
    vex_user_context_t* out_context
) {
    if (process == 0 || out_context == 0 || g_execution_state.gdt_loaded == 0u || g_execution_state.rsp0 == 0u) {
        return -1;
    }

    out_context->rip = user_entrypoint;
    out_context->rsp = user_stack_pointer;
    out_context->rflags = 0x3202u;
    out_context->cr3_phys = process->address_space_root;
    out_context->cs = g_execution_state.user_code_selector;
    out_context->ss = g_execution_state.user_data_selector;

    return 0;
}

void execution_enter_user(const vex_user_context_t* context) {
    const u64 user_ss = context->ss;
    const u64 user_cs = context->cs;
    const u64 transition_stack_top = (u64)(usize)&g_transition_stack[4096];
    __asm__ volatile (
        "mov %[kstack], %%rsp\n\t"
        "mov %[ssw], %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "pushq %[ss]\n\t"
        "pushq %[rsp]\n\t"
        "pushq %[rflags]\n\t"
        "pushq %[cs]\n\t"
        "pushq %[rip]\n\t"
        "mov %[cr3], %%cr3\n\t"
        "iretq\n\t"
        :
        : [cr3] "r"(context->cr3_phys),
          [kstack] "r"(transition_stack_top),
          [ssw] "r"((u16)context->ss),
          [ss] "r"(user_ss),
          [rsp] "r"(context->rsp),
          [rflags] "r"(context->rflags),
          [cs] "r"(user_cs),
          [rip] "r"(context->rip)
        : "memory", "rax"
    );
    for (;;) {
        __asm__ volatile ("hlt");
    }
}
