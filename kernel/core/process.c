#include "../include/vex/kernel.h"

static vex_process_t g_processes[8];
static vex_service_t g_services[8];
static vex_channel_t g_service_channels[8];
static vex_user_context_t g_user_contexts[8];
static vex_address_space_t g_process_spaces[8];
static u32 g_process_slots_used;
static u32 g_service_slots_used;

static void copy_bytes(void* dst, const void* src, u64 size) {
    u8* out = (u8*)dst;
    const u8* in = (const u8*)src;
    for (u64 index = 0; index < size; ++index) {
        out[index] = in[index];
    }
}

static void zero_bytes(void* dst, u64 size) {
    u8* out = (u8*)dst;
    for (u64 index = 0; index < size; ++index) {
        out[index] = 0u;
    }
}

static u64 page_index_for_level(u64 virtual_address, u32 shift) {
    return (virtual_address >> shift) & 0x1FFu;
}

static void copy_string(char* dst, u32 capacity, const char* src) {
    u32 index = 0;
    while (index + 1u < capacity && src[index] != '\0') {
        dst[index] = src[index];
        ++index;
    }
    dst[index] = '\0';
}

void process_init(void) {
    g_process_slots_used = 0;
    g_service_slots_used = 0;
    zero_bytes(g_process_spaces, sizeof(g_process_spaces));
}

vex_process_t* process_create(u32 pid, u64 capability_mask) {
    if (g_process_slots_used >= 8u) {
        return 0;
    }

    vex_process_t* process = &g_processes[g_process_slots_used++];
    process->pid = pid;
    process->capability_mask = capability_mask;
    process->address_space_root = 0;
    process->user_entrypoint = 0;
    process->user_stack_pointer = 0;
    process->default_surface_handle = 0u;
    process->default_fence_handle = 0u;
    process->default_surface_va = 0u;
    process->default_mailbox_va = 0u;
    process->default_mailbox_phys = 0u;
    return process;
}

vex_address_space_t* process_address_space(vex_process_t* process) {
    if (process == 0 || process < g_processes || process >= g_processes + 8u) {
        return 0;
    }
    return &g_process_spaces[(u32)(process - g_processes)];
}

vex_process_t* process_find_by_cr3(u64 cr3_phys) {
    for (u32 index = 0; index < g_process_slots_used; ++index) {
        if (g_processes[index].address_space_root == cr3_phys) {
            return &g_processes[index];
        }
    }
    return 0;
}

u32 process_count(void) {
    return g_process_slots_used;
}

u32 service_count(void) {
    return g_service_slots_used;
}

const vex_service_t* service_get(u32 index) {
    if (index >= g_service_slots_used) {
        return 0;
    }
    return &g_services[index];
}

const vex_user_context_t* process_user_context(u32 index) {
    if (index >= g_process_slots_used) {
        return 0;
    }
    return &g_user_contexts[index];
}

int bootstrap_package_domain(
    const vex_boot_info_t* boot_info,
    const vex_package_image_info_t* image,
    u32 pid,
    vex_manifest_view_t* out_manifest
) {
    if (boot_info == 0 || image == 0 || out_manifest == 0) {
        return -1;
    }
    if (manifest_parse_package_image(image, out_manifest) != 0) {
        return -1;
    }

    vex_process_t* process = process_create(pid, out_manifest->capability_mask);
    u32 process_index;
    if (process == 0) {
        return -2;
    }
    process_index = (u32)(process - g_processes);

    vex_address_space_t process_space;
    const u64 user_entrypoint = 0x40000000u;
    const u64 payload_pages = (image->payload_size + 4095u) / 4096u;
    const u64 payload_image_base = memory_alloc_pages(payload_pages);
    if (payload_image_base == 0u) {
        return -3;
    }
    zero_bytes((void*)(usize)payload_image_base, payload_pages * 4096u);
    copy_bytes((void*)(usize)payload_image_base, (const void*)(usize)image->payload_base, image->payload_size);

    if (address_space_create_process(user_entrypoint, payload_image_base, image->payload_size, &process_space) != 0) {
        return -4;
    }

    u64 translated_phys = 0;
    u64 translated_flags = 0;
    if (address_space_translate(&process_space, user_entrypoint, &translated_phys, &translated_flags) != 0) {
        return -5;
    }
    if (translated_phys != payload_image_base || (translated_flags & (0x004u | 0x002u)) != (0x004u | 0x002u)) {
        return -6;
    }
    if ((translated_flags & (1ull << 63)) != 0u) {
        return -19;
    }

    {
        const u64 pt_present = 0x001u;
        const u64 pt_user = 0x004u;
        const u64 pt_nx = 1ull << 63;
        const vex_page_table_t* pml4 = process_space.pml4;
        const u64 pml4_entry = pml4->entries[page_index_for_level(user_entrypoint, 39u)];
        if ((pml4_entry & (pt_present | pt_user)) != (pt_present | pt_user) || (pml4_entry & pt_nx) != 0u) {
            return -20;
        }
        const vex_page_table_t* pdpt = (const vex_page_table_t*)(usize)(pml4_entry & ~0xFFFu);
        const u64 pdpt_entry = pdpt->entries[page_index_for_level(user_entrypoint, 30u)];
        if ((pdpt_entry & (pt_present | pt_user)) != (pt_present | pt_user) || (pdpt_entry & pt_nx) != 0u) {
            return -21;
        }
        const vex_page_table_t* pd = (const vex_page_table_t*)(usize)(pdpt_entry & ~0xFFFu);
        const u64 pd_entry = pd->entries[page_index_for_level(user_entrypoint, 21u)];
        if ((pd_entry & (pt_present | pt_user)) != (pt_present | pt_user) || (pd_entry & pt_nx) != 0u) {
            return -22;
        }
        const vex_page_table_t* pt = (const vex_page_table_t*)(usize)(pd_entry & ~0xFFFu);
        const u64 pt_entry = pt->entries[page_index_for_level(user_entrypoint, 12u)];
        if ((pt_entry & (pt_present | pt_user)) != (pt_present | pt_user) || (pt_entry & pt_nx) != 0u) {
            return -23;
        }
    }

    const u64 user_stack_page = memory_alloc_page();
    if (user_stack_page == 0u) {
        return -7;
    }
    const u64 user_stack_base = 0x70000000u;
    if (address_space_map_user_range(&process_space, user_stack_base, user_stack_page, 4096u, 1u, 0u) != 0) {
        return -8;
    }

    u64 stack_phys = 0;
    u64 stack_flags = 0;
    if (address_space_translate(&process_space, user_stack_base, &stack_phys, &stack_flags) != 0) {
        return -9;
    }
    if (stack_phys != user_stack_page || (stack_flags & (0x004u | 0x002u)) != (0x004u | 0x002u)) {
        return -10;
    }

    {
        const u64 framebuffer_base = boot_info->framebuffer.base & ~0xFFFull;
        const u64 framebuffer_bias = boot_info->framebuffer.base - framebuffer_base;
        const u64 framebuffer_span = (boot_info->framebuffer.size + framebuffer_bias + 0xFFFull) & ~0xFFFull;
        if (address_space_map_user_range(
                &process_space,
                VEX_USER_FRAMEBUFFER_BASE - framebuffer_bias,
                framebuffer_base,
                framebuffer_span,
                1u,
                0u
            ) != 0) {
            return -11;
        }
        if (address_space_translate(&process_space, VEX_USER_FRAMEBUFFER_BASE, &translated_phys, &translated_flags) != 0 ||
            translated_phys != boot_info->framebuffer.base ||
            (translated_flags & (0x004u | 0x002u)) != (0x004u | 0x002u)) {
            return -12;
        }
    }

    {
        const u64 boot_info_base = ((u64)(usize)boot_info) & ~0xFFFull;
        const u64 boot_info_bias = (u64)(usize)boot_info - boot_info_base;
        const u64 boot_info_span = (sizeof(vex_boot_info_t) + boot_info_bias + 0xFFFull) & ~0xFFFull;
        if (address_space_map_user_range(
                &process_space,
                VEX_USER_BOOTINFO_BASE - boot_info_bias,
                boot_info_base,
                boot_info_span,
                0u,
                0u
            ) != 0) {
            return -13;
        }
        if (address_space_translate(&process_space, VEX_USER_BOOTINFO_BASE, &translated_phys, &translated_flags) != 0 ||
            translated_phys != (u64)(usize)boot_info ||
            (translated_flags & 0x004u) != 0x004u ||
            (translated_flags & 0x002u) != 0u) {
            return -14;
        }
    }

    g_process_spaces[process_index] = process_space;
    process->address_space_root = process_space.cr3_phys;
    process->user_entrypoint = user_entrypoint;
    /*
     * Userspace entry lands in a compiled C function (`_start`) via `iretq`,
     * not via a real `call`. SysV x86_64 code expects `%rsp % 16 == 8` on
     * function entry because a caller would have pushed a return address.
     * Bias the initial stack by 8 bytes so SSE stack spills stay aligned.
     */
    process->user_stack_pointer = user_stack_base + 4096u - 8u;

    if ((process->capability_mask & VEX_CAP_GRAPHICS) != 0u) {
        const u64 surface_handle = graphics_create_surface(
            boot_info,
            process,
            &g_process_spaces[process_index],
            boot_info->framebuffer.width,
            boot_info->framebuffer.height,
            VEX_SURFACE_FORMAT_XRGB8888,
            3u,
            VEX_USER_SURFACE_BASE
        );
        if (surface_handle == 0u) {
            return -24;
        }
        process->default_surface_handle = (u32)surface_handle;
        process->default_surface_va = graphics_query_surface((u32)surface_handle, VEX_SURFACE_QUERY_MAPPING_BASE, process);
        process->default_fence_handle = (u32)graphics_query_surface((u32)surface_handle, VEX_SURFACE_QUERY_FENCE_HANDLE, process);
        if (process->pid != 3u) {
            process->default_mailbox_phys = memory_alloc_page();
            if (process->default_mailbox_phys == 0u) {
                return -25;
            }
            zero_bytes((void*)(usize)process->default_mailbox_phys, 4096u);
            process->default_mailbox_va = VEX_USER_MAILBOX_BASE;
            if (address_space_map_user_range(
                    &g_process_spaces[process_index],
                    process->default_mailbox_va,
                    process->default_mailbox_phys,
                    4096u,
                    1u,
                    0u
                ) != 0) {
                return -26;
            }
        }
    }

    if (execution_prepare_user_context(
            process,
            process->user_entrypoint,
            process->user_stack_pointer,
            &g_user_contexts[process_index]
        ) != 0) {
        return -15;
    }

    vex_thread_t* thread = scheduler_create_thread(pid, 2u, 3u);
    if (thread == 0) {
        return -16;
    }
    scheduler_enqueue(thread);

    if (g_service_slots_used >= 8u) {
        return -17;
    }

    vex_service_t* service = &g_services[g_service_slots_used];
    vex_channel_t* channel = &g_service_channels[g_service_slots_used];
    ipc_init(channel);

    service->handle = g_service_slots_used + 1u;
    service->owner = process;
    service->bootstrap_channel = channel;
    copy_string(service->name, sizeof(service->name), out_manifest->name);
    ++g_service_slots_used;

    vex_process_t kernel_supervisor = { .pid = 1u, .capability_mask = VEX_CAP_LOG | VEX_CAP_IPC | VEX_CAP_SERVICE_LOCATE };
    vex_channel_message_t bootstrap = { .id = 1u, .length = 15u, .payload = "bootstrap-ready" };
    if (ipc_send(channel, &kernel_supervisor, &bootstrap) != 0) {
        return -18;
    }

    return 0;
}

int bootstrap_init_domain(const vex_boot_info_t* boot_info, vex_manifest_view_t* out_manifest) {
    if (boot_info == 0) {
        return -1;
    }
    return bootstrap_package_domain(boot_info, &boot_info->init_image, 3u, out_manifest);
}
