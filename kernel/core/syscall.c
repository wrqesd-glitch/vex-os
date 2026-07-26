#include "../include/vex/kernel.h"

enum {
    VEX_SYS_GET_ABI = 0,
    VEX_SYS_LOG = 1,
    VEX_SYS_IPC_PING = 2,
    VEX_SYS_GFX_CREATE_SURFACE = 16,
    VEX_SYS_GFX_SURFACE_QUERY = 17,
    VEX_SYS_GFX_PRESENT = 18,
    VEX_SYS_GFX_CREATE_FENCE = 19,
    VEX_SYS_GFX_FENCE_QUERY = 20,
    VEX_SYS_GFX_SIGNAL_FENCE = 21,
    VEX_SYS_GFX_WAIT_FENCE = 22
};

u64 syscall_dispatch(u32 abi_version, u32 syscall_id, u64 arg0, u64 arg1, u64 arg2, const vex_process_t* process) {
    (void)arg1;
    (void)arg2;

    if (abi_version != VEX_ABI_VERSION) {
        return 0xFFFF0001u;
    }

    switch (syscall_id) {
        case VEX_SYS_GET_ABI:
            return VEX_ABI_VERSION;
        case VEX_SYS_LOG:
            if ((process->capability_mask & VEX_CAP_LOG) == 0u) {
                return 0xFFFF0002u;
            }
            console_write((const char*)(usize)arg0);
            return 0;
        case VEX_SYS_IPC_PING:
            if ((process->capability_mask & VEX_CAP_IPC) == 0u) {
                return 0xFFFF0003u;
            }
            return arg0 ^ 0x564558u;
        case VEX_SYS_GFX_CREATE_SURFACE:
            if ((process->capability_mask & VEX_CAP_GRAPHICS) == 0u) {
                return 0xFFFF0010u;
            }
            {
                vex_address_space_t* space = process_address_space((vex_process_t*)process);
                const u32 width = (u32)(arg0 & 0xFFFFFFFFu);
                const u32 height = (u32)((arg0 >> 32u) & 0xFFFFFFFFu);
                const u32 format = (u32)(arg1 & 0xFFFFFFFFu);
                const u32 buffer_count = (u32)((arg1 >> 32u) & 0xFFFFFFFFu);
                if (space == 0) {
                    return 0xFFFF0011u;
                }
                return graphics_create_surface(
                    0,
                    (vex_process_t*)process,
                    space,
                    width,
                    height,
                    format,
                    buffer_count,
                    arg2
                );
            }
        case VEX_SYS_GFX_SURFACE_QUERY:
            if ((process->capability_mask & VEX_CAP_GRAPHICS) == 0u) {
                return 0xFFFF0010u;
            }
            return graphics_query_surface((u32)arg0, (u32)arg1, process);
        case VEX_SYS_GFX_PRESENT:
            if ((process->capability_mask & VEX_CAP_GRAPHICS) == 0u) {
                return 0xFFFF0010u;
            }
            return graphics_present_surface((u32)arg0, (u32)arg1, arg2, process);
        case VEX_SYS_GFX_CREATE_FENCE:
            if ((process->capability_mask & VEX_CAP_GRAPHICS) == 0u) {
                return 0xFFFF0010u;
            }
            return graphics_create_fence((vex_process_t*)process);
        case VEX_SYS_GFX_FENCE_QUERY:
            if ((process->capability_mask & VEX_CAP_GRAPHICS) == 0u) {
                return 0xFFFF0010u;
            }
            return graphics_query_fence((u32)arg0, (u32)arg1, process);
        case VEX_SYS_GFX_SIGNAL_FENCE:
            if ((process->capability_mask & VEX_CAP_GRAPHICS) == 0u) {
                return 0xFFFF0010u;
            }
            return graphics_signal_fence((u32)arg0, arg1, process);
        case VEX_SYS_GFX_WAIT_FENCE:
            if ((process->capability_mask & VEX_CAP_GRAPHICS) == 0u) {
                return 0xFFFF0010u;
            }
            return graphics_wait_fence((u32)arg0, arg1, process);
        default:
            return 0xFFFF00FFu;
    }
}

u64 syscall_enter_from_user(u32 abi_version, u32 syscall_id, u64 arg0, u64 arg1, u64 arg2, u64 user_cr3) {
    const vex_process_t* process = process_find_by_cr3(user_cr3);
    if (process == 0) {
        return 0xFFFF0004u;
    }
    return syscall_dispatch(abi_version, syscall_id, arg0, arg1, arg2, process);
}
