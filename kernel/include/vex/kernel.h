#ifndef VEX_KERNEL_H
#define VEX_KERNEL_H

#include "boot_info.h"
#include "types.h"

#define VEX_ABI_VERSION 1u
#define VEX_CAP_LOG            0x1ull
#define VEX_CAP_IPC            0x2ull
#define VEX_CAP_SERVICE_LOCATE 0x4ull
#define VEX_CAP_PACKAGE_EXEC   0x8ull
#define VEX_CAP_GRAPHICS       0x10ull
#define VEX_CAP_GPU            0x20ull
#define VEX_USER_FRAMEBUFFER_BASE 0x50000000ull
#define VEX_USER_BOOTINFO_BASE 0x50400000ull
#define VEX_USER_SURFACE_BASE 0x50800000ull
#define VEX_USER_MAILBOX_BASE 0x70002000ull
#define VEX_USER_SURFACE_STRIDE 0x02000000ull
#define VEX_USER_SHARED_SURFACE_BASE 0x70000000ull
#define VEX_USER_SHARED_MAILBOX_BASE 0x0000000900000000ull
#define VEX_USER_SHARED_SURFACE_STRIDE 0x02000000ull
#define VEX_USER_COMPOSITOR_SHARED_SURFACE_BASE 0x0000000A00000000ull
#define VEX_USER_COMPOSITOR_SHARED_MAILBOX_BASE 0x0000000B00000000ull
#define VEX_USER_INIT_COMPOSITOR_MAILBOX_BASE 0x0000000C00000000ull
#define VEX_USER_GPU_SHARED_SURFACE_BASE 0x0000000D00000000ull
#define VEX_USER_GPU_SHARED_MAILBOX_BASE 0x0000000E00000000ull
#define VEX_USER_COMPOSITOR_GPU_MAILBOX_BASE 0x0000000F00000000ull
#define VEX_USER_INIT_GPU_MAILBOX_BASE 0x0000001000000000ull
#define VEX_USER_GPU_SCENE_MAILBOX_BASE 0x0000001100000000ull

typedef struct vex_process {
    u32 pid;
    u64 capability_mask;
    u64 address_space_root;
    u64 user_entrypoint;
    u64 user_stack_pointer;
    u32 default_surface_handle;
    u32 default_fence_handle;
    u64 default_surface_va;
    u64 default_mailbox_va;
    u64 default_mailbox_phys;
} vex_process_t;

typedef struct vex_thread {
    u32 tid;
    u32 priority;
    u32 time_slice_ticks;
    u32 remaining_ticks;
    struct vex_thread* next;
} vex_thread_t;

typedef struct vex_channel_message {
    u32 id;
    u32 length;
    char payload[48];
} vex_channel_message_t;

typedef struct vex_channel {
    u32 head;
    u32 tail;
    u32 count;
    vex_channel_message_t ring[16];
} vex_channel_t;

typedef struct vex_manifest_view {
    char name[32];
    char version[16];
    char entrypoint[64];
    u32 abi_version;
    u64 capability_mask;
} vex_manifest_view_t;

typedef struct vex_service {
    u32 handle;
    char name[32];
    vex_process_t* owner;
    vex_channel_t* bootstrap_channel;
} vex_service_t;

typedef struct vex_page_table {
    u64 entries[512];
} vex_page_table_t;

typedef struct vex_address_space {
    u64 cr3_phys;
    vex_page_table_t* pml4;
    u64 mapped_pages;
    u64 kernel_window_base;
    u64 kernel_window_size;
    u64 user_window_base;
    u64 user_window_size;
} vex_address_space_t;

typedef struct vex_user_context {
    u64 rip;
    u64 rsp;
    u64 rflags;
    u64 cr3_phys;
    u16 cs;
    u16 ss;
} vex_user_context_t;

typedef struct vex_execution_state {
    u16 kernel_code_selector;
    u16 kernel_data_selector;
    u16 user_code_selector;
    u16 user_data_selector;
    u16 tss_selector;
    u64 tss_base;
    u64 rsp0;
    u64 syscall_entry;
    u64 syscall_stack_top;
    u32 gdt_loaded;
    u32 syscall_ready;
} vex_execution_state_t;

typedef struct vex_acpi_state {
    u64 rsdp_address;
    u64 rsdt_address;
    u64 xsdt_address;
    u64 madt_address;
    u64 hpet_address;
    u64 hpet_mmio_base;
    u32 lapic_address;
    u32 cpu_count;
    u32 ioapic_count;
    u32 hpet_present;
} vex_acpi_state_t;

typedef struct vex_timer_state {
    u64 counter_base;
    u64 counter_period_femtoseconds;
    u32 source_kind;
    u32 available;
} vex_timer_state_t;

typedef struct vex_desktop_snapshot {
    u64 rsdp_address;
    u64 framebuffer_base;
    u64 framebuffer_size;
    u64 kernel_cr3;
    u64 kernel_mapped_pages;
    u64 memory_regions;
    u64 usable_pages;
    u64 allocated_pages;
    u64 xsdt_address;
    u64 madt_address;
    u64 cpu_count;
    u64 ioapic_count;
    u64 timer_kind;
    u64 timer_base;
    u64 timer_period_femtoseconds;
    u64 timer_ready;
    u64 tss_base;
    u64 user_cs;
    u64 user_ss;
    u64 service_count;
    u64 process_count;
    u64 init_cr3;
    u64 init_entry_va;
    u64 init_stack_va;
    u64 user_rip;
    u64 user_rsp;
    u64 init_caps;
    u32 framebuffer_width;
    u32 framebuffer_height;
    u32 framebuffer_pitch;
    u32 gdt_loaded;
    u32 selftest_passed;
    char init_domain[32];
    char init_entrypoint[64];
} vex_desktop_snapshot_t;

typedef struct vex_graphics_surface {
    u32 handle;
    u32 owner_pid;
    u32 width;
    u32 height;
    u32 stride;
    u32 format;
    u32 buffer_count;
    u32 present_index;
    u32 present_fence_handle;
    u32 reserved0;
    u64 bytes_per_buffer;
    u64 allocation_base;
    u64 allocation_size;
    u64 mapping_base;
    u64 last_present_sequence;
} vex_graphics_surface_t;

typedef struct vex_graphics_fence {
    u32 handle;
    u32 owner_pid;
    u32 signaled;
    u32 reserved0;
    u64 completed_value;
} vex_graphics_fence_t;

enum {
    VEX_SURFACE_FORMAT_XRGB8888 = 1u
};

enum {
    VEX_SURFACE_QUERY_WIDTH = 0u,
    VEX_SURFACE_QUERY_HEIGHT = 1u,
    VEX_SURFACE_QUERY_STRIDE = 2u,
    VEX_SURFACE_QUERY_FORMAT = 3u,
    VEX_SURFACE_QUERY_BUFFER_COUNT = 4u,
    VEX_SURFACE_QUERY_MAPPING_BASE = 5u,
    VEX_SURFACE_QUERY_BUFFER_BYTES = 6u,
    VEX_SURFACE_QUERY_PRESENT_INDEX = 7u,
    VEX_SURFACE_QUERY_FENCE_HANDLE = 8u,
    VEX_SURFACE_QUERY_LAST_SEQUENCE = 9u
};

enum {
    VEX_FENCE_QUERY_SIGNALED = 0u,
    VEX_FENCE_QUERY_COMPLETED_VALUE = 1u
};

typedef enum vex_key_event {
    VEX_KEY_NONE = 0,
    VEX_KEY_UP,
    VEX_KEY_DOWN,
    VEX_KEY_LEFT,
    VEX_KEY_RIGHT,
    VEX_KEY_TAB,
    VEX_KEY_ENTER,
    VEX_KEY_ESCAPE
} vex_key_event_t;

void serial_init(void);
void serial_write_char(char value);

void console_write(const char* text);
void console_write_line(const char* text);
void console_write_hex(u64 value);
void console_write_u64(u64 value);

void framebuffer_fill_banner(const vex_framebuffer_info_t* framebuffer);
void framebuffer_console_init(const vex_framebuffer_info_t* framebuffer);
void framebuffer_console_write_char(char value);
u32 framebuffer_console_columns(void);
u32 framebuffer_console_rows(void);
u32 framebuffer_console_is_ready(void);
void framebuffer_console_clear(void);
void framebuffer_console_draw_text(u32 col, u32 row, const char* text, u32 color);
void framebuffer_console_fill_cells(u32 col, u32 row, u32 width, u32 height, u32 color);

void memory_init(const vex_boot_info_t* boot_info);
u64 memory_usable_pages(void);
u64 memory_region_count(void);
u64 memory_alloc_page(void);
u64 memory_alloc_pages(u64 count);
u64 memory_allocated_pages(void);

void scheduler_init(void);
vex_thread_t* scheduler_create_thread(u32 tid, u32 priority, u32 slice_ticks);
void scheduler_enqueue(vex_thread_t* thread);
vex_thread_t* scheduler_tick(void);
u32 scheduler_ready_count(void);

void ipc_init(vex_channel_t* channel);
int ipc_send(vex_channel_t* channel, const vex_process_t* process, const vex_channel_message_t* message);
int ipc_receive(vex_channel_t* channel, vex_channel_message_t* out_message);

u64 syscall_dispatch(u32 abi_version, u32 syscall_id, u64 arg0, u64 arg1, u64 arg2, const vex_process_t* process);
u64 syscall_enter_from_user(u32 abi_version, u32 syscall_id, u64 arg0, u64 arg1, u64 arg2, u64 user_cr3);

int manifest_parse_package_image(const vex_package_image_info_t* image, vex_manifest_view_t* out_manifest);
int manifest_parse_init(const vex_boot_info_t* boot_info, vex_manifest_view_t* out_manifest);

void address_space_init(const vex_boot_info_t* boot_info);
const vex_address_space_t* address_space_kernel(void);
int address_space_create_process(
    u64 user_entrypoint,
    u64 image_phys_base,
    u64 image_size,
    vex_address_space_t* out_space
);
int address_space_map_user_range(
    vex_address_space_t* space,
    u64 virtual_base,
    u64 physical_base,
    u64 size,
    u32 writable,
    u32 executable
);
int address_space_translate(
    const vex_address_space_t* space,
    u64 virtual_address,
    u64* out_physical,
    u64* out_flags
);
u64 address_space_page_count(const vex_address_space_t* space);

void execution_init(void);
const vex_execution_state_t* execution_state(void);
int execution_prepare_user_context(
    const vex_process_t* process,
    u64 user_entrypoint,
    u64 user_stack_pointer,
    vex_user_context_t* out_context
);
void execution_enter_user(const vex_user_context_t* context) __attribute__((noreturn));
void vex_syscall_entry(void);
void interrupts_init(void);

void acpi_init(const vex_boot_info_t* boot_info);
const vex_acpi_state_t* acpi_state(void);

void timer_init(void);
const vex_timer_state_t* timer_state(void);
void ps2_keyboard_init(void);
vex_key_event_t ps2_keyboard_poll(void);

void graphics_init(const vex_boot_info_t* boot_info);
u64 graphics_create_surface(
    const vex_boot_info_t* boot_info,
    vex_process_t* process,
    vex_address_space_t* space,
    u32 width,
    u32 height,
    u32 format,
    u32 buffer_count,
    u64 requested_mapping_base
);
u64 graphics_create_fence(vex_process_t* process);
u64 graphics_query_surface(u32 handle, u32 field, const vex_process_t* process);
u64 graphics_query_fence(u32 handle, u32 field, const vex_process_t* process);
u64 graphics_present_surface(u32 handle, u32 buffer_index, u64 sequence, const vex_process_t* process);
u64 graphics_signal_fence(u32 handle, u64 value, const vex_process_t* process);
u64 graphics_wait_fence(u32 handle, u64 target_value, const vex_process_t* process);
u64 graphics_share_surface(
    u32 handle,
    vex_process_t* target_process,
    vex_address_space_t* target_space,
    u64 requested_mapping_base
);
u32 graphics_surface_count(void);
u32 graphics_fence_count(void);
const vex_graphics_surface_t* graphics_surface_get(u32 index);
const vex_graphics_fence_t* graphics_fence_get(u32 index);

void process_init(void);
vex_process_t* process_create(u32 pid, u64 capability_mask);
vex_address_space_t* process_address_space(vex_process_t* process);
vex_process_t* process_find_by_cr3(u64 cr3_phys);
u32 process_count(void);
int bootstrap_package_domain(
    const vex_boot_info_t* boot_info,
    const vex_package_image_info_t* image,
    u32 pid,
    vex_manifest_view_t* out_manifest
);
int bootstrap_init_domain(const vex_boot_info_t* boot_info, vex_manifest_view_t* out_manifest);
u32 service_count(void);
const vex_service_t* service_get(u32 index);
const vex_user_context_t* process_user_context(u32 index);

void desktop_init(const vex_desktop_snapshot_t* snapshot);
void desktop_handle_key(vex_key_event_t key);

#endif
