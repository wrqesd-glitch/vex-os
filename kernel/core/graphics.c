#include "../include/vex/kernel.h"

enum {
    VEX_MAX_GRAPHICS_SURFACES = 8u,
    VEX_MAX_GRAPHICS_FENCES = 16u
};

static vex_graphics_surface_t g_surfaces[VEX_MAX_GRAPHICS_SURFACES];
static vex_graphics_fence_t g_fences[VEX_MAX_GRAPHICS_FENCES];
static u32 g_surface_count;
static u32 g_fence_count;
static u32 g_surface_handle_seed;
static u32 g_fence_handle_seed;
static vex_framebuffer_info_t g_boot_framebuffer;

static void zero_bytes(void* dst, u64 size) {
    u8* out = (u8*)dst;
    for (u64 index = 0u; index < size; ++index) {
        out[index] = 0u;
    }
}

static u64 align_up_4k(u64 value) {
    return (value + 4095ull) & ~4095ull;
}

static vex_graphics_surface_t* surface_from_handle(u32 handle) {
    for (u32 index = 0u; index < g_surface_count; ++index) {
        if (g_surfaces[index].handle == handle) {
            return &g_surfaces[index];
        }
    }
    return 0;
}

static vex_graphics_fence_t* fence_from_handle(u32 handle) {
    for (u32 index = 0u; index < g_fence_count; ++index) {
        if (g_fences[index].handle == handle) {
            return &g_fences[index];
        }
    }
    return 0;
}

static u32 owns_graphics_object(u32 owner_pid, const vex_process_t* process) {
    return process != 0 && process->pid == owner_pid;
}

void graphics_init(const vex_boot_info_t* boot_info) {
    g_surface_count = 0u;
    g_fence_count = 0u;
    g_surface_handle_seed = 1u;
    g_fence_handle_seed = 1u;
    zero_bytes(&g_boot_framebuffer, sizeof(g_boot_framebuffer));
    if (boot_info != 0) {
        g_boot_framebuffer = boot_info->framebuffer;
    }
    zero_bytes(g_surfaces, sizeof(g_surfaces));
    zero_bytes(g_fences, sizeof(g_fences));
}

u64 graphics_create_fence(vex_process_t* process) {
    vex_graphics_fence_t* fence;
    if (process == 0 || (process->capability_mask & VEX_CAP_GRAPHICS) == 0u || g_fence_count >= VEX_MAX_GRAPHICS_FENCES) {
        return 0u;
    }

    fence = &g_fences[g_fence_count++];
    zero_bytes(fence, sizeof(*fence));
    fence->handle = g_fence_handle_seed++;
    fence->owner_pid = process->pid;
    return fence->handle;
}

u64 graphics_create_surface(
    const vex_boot_info_t* boot_info,
    vex_process_t* process,
    vex_address_space_t* space,
    u32 width,
    u32 height,
    u32 format,
    u32 buffer_count,
    u64 requested_mapping_base
) {
    vex_graphics_surface_t* surface;
    const u64 bytes_per_pixel = 4u;
    u64 bytes_per_buffer;
    u64 allocation_size;
    u64 allocation_base;
    u64 mapping_base;
    u64 page_count;
    u32 fence_handle;

    if (process == 0 || space == 0 || (process->capability_mask & VEX_CAP_GRAPHICS) == 0u) {
        return 0u;
    }
    if (g_surface_count >= VEX_MAX_GRAPHICS_SURFACES) {
        return 0u;
    }
    if (width == 0u || height == 0u) {
        if (boot_info == 0 && (width == 0u || height == 0u)) {
            return 0u;
        }
        if (width == 0u) {
            width = boot_info != 0 ? boot_info->framebuffer.width : g_boot_framebuffer.width;
        }
        if (height == 0u) {
            height = boot_info != 0 ? boot_info->framebuffer.height : g_boot_framebuffer.height;
        }
    }
    if (width == 0u || height == 0u) {
        return 0u;
    }
    if (buffer_count < 2u) {
        buffer_count = 2u;
    }
    if (buffer_count > 3u) {
        buffer_count = 3u;
    }
    if (format != VEX_SURFACE_FORMAT_XRGB8888) {
        return 0u;
    }

    bytes_per_buffer = (u64)width * (u64)height * bytes_per_pixel;
    allocation_size = align_up_4k(bytes_per_buffer * (u64)buffer_count);
    page_count = allocation_size / 4096u;
    allocation_base = memory_alloc_pages(page_count);
    if (allocation_base == 0u) {
        return 0u;
    }
    zero_bytes((void*)(usize)allocation_base, allocation_size);

    mapping_base = requested_mapping_base != 0u
        ? requested_mapping_base
        : (VEX_USER_SURFACE_BASE + (u64)g_surface_count * VEX_USER_SURFACE_STRIDE);

    if (address_space_map_user_range(space, mapping_base, allocation_base, allocation_size, 1u, 0u) != 0) {
        return 0u;
    }

    fence_handle = (u32)graphics_create_fence(process);
    if (fence_handle == 0u) {
        return 0u;
    }

    surface = &g_surfaces[g_surface_count++];
    zero_bytes(surface, sizeof(*surface));
    surface->handle = g_surface_handle_seed++;
    surface->owner_pid = process->pid;
    surface->width = width;
    surface->height = height;
    surface->stride = width;
    surface->format = format;
    surface->buffer_count = buffer_count;
    surface->present_index = 0u;
    surface->present_fence_handle = fence_handle;
    surface->bytes_per_buffer = bytes_per_buffer;
    surface->allocation_base = allocation_base;
    surface->allocation_size = allocation_size;
    surface->mapping_base = mapping_base;
    surface->last_present_sequence = 0u;
    return surface->handle;
}

u64 graphics_query_surface(u32 handle, u32 field, const vex_process_t* process) {
    const vex_graphics_surface_t* surface = surface_from_handle(handle);
    if (surface == 0 || owns_graphics_object(surface->owner_pid, process) == 0u) {
        return 0u;
    }

    switch (field) {
    case VEX_SURFACE_QUERY_WIDTH: return surface->width;
    case VEX_SURFACE_QUERY_HEIGHT: return surface->height;
    case VEX_SURFACE_QUERY_STRIDE: return surface->stride;
    case VEX_SURFACE_QUERY_FORMAT: return surface->format;
    case VEX_SURFACE_QUERY_BUFFER_COUNT: return surface->buffer_count;
    case VEX_SURFACE_QUERY_MAPPING_BASE: return surface->mapping_base;
    case VEX_SURFACE_QUERY_BUFFER_BYTES: return surface->bytes_per_buffer;
    case VEX_SURFACE_QUERY_PRESENT_INDEX: return surface->present_index;
    case VEX_SURFACE_QUERY_FENCE_HANDLE: return surface->present_fence_handle;
    case VEX_SURFACE_QUERY_LAST_SEQUENCE: return surface->last_present_sequence;
    default:
        return 0u;
    }
}

u64 graphics_query_fence(u32 handle, u32 field, const vex_process_t* process) {
    const vex_graphics_fence_t* fence = fence_from_handle(handle);
    if (fence == 0 || owns_graphics_object(fence->owner_pid, process) == 0u) {
        return 0u;
    }

    switch (field) {
    case VEX_FENCE_QUERY_SIGNALED: return fence->signaled;
    case VEX_FENCE_QUERY_COMPLETED_VALUE: return fence->completed_value;
    default:
        return 0u;
    }
}

u64 graphics_signal_fence(u32 handle, u64 value, const vex_process_t* process) {
    vex_graphics_fence_t* fence = fence_from_handle(handle);
    if (fence == 0 || owns_graphics_object(fence->owner_pid, process) == 0u) {
        return 0u;
    }
    if (value >= fence->completed_value) {
        fence->completed_value = value;
        fence->signaled = 1u;
    }
    return fence->completed_value;
}

u64 graphics_wait_fence(u32 handle, u64 target_value, const vex_process_t* process) {
    vex_graphics_fence_t* fence = fence_from_handle(handle);
    if (fence == 0 || owns_graphics_object(fence->owner_pid, process) == 0u) {
        return 0u;
    }
    return fence->completed_value >= target_value ? 1u : 0u;
}

u64 graphics_share_surface(
    u32 handle,
    vex_process_t* target_process,
    vex_address_space_t* target_space,
    u64 requested_mapping_base
) {
    vex_graphics_surface_t* surface = surface_from_handle(handle);
    u64 mapping_base;

    if (surface == 0 || target_process == 0 || target_space == 0) {
        return 0u;
    }
    if ((target_process->capability_mask & VEX_CAP_GRAPHICS) == 0u) {
        return 0u;
    }

    mapping_base = requested_mapping_base != 0u
        ? requested_mapping_base
        : (VEX_USER_SHARED_SURFACE_BASE + (u64)surface->handle * VEX_USER_SHARED_SURFACE_STRIDE);

    if (address_space_map_user_range(
            target_space,
            mapping_base,
            surface->allocation_base,
            surface->allocation_size,
            1u,
            0u
        ) != 0) {
        return 0u;
    }
    return mapping_base;
}

u64 graphics_present_surface(u32 handle, u32 buffer_index, u64 sequence, const vex_process_t* process) {
    vex_graphics_surface_t* surface = surface_from_handle(handle);
    if (surface == 0 || owns_graphics_object(surface->owner_pid, process) == 0u) {
        return 0u;
    }
    if (buffer_index >= surface->buffer_count) {
        return 0u;
    }
    surface->present_index = buffer_index;
    surface->last_present_sequence = sequence;
    (void)graphics_signal_fence(surface->present_fence_handle, sequence, process);
    return sequence;
}

u32 graphics_surface_count(void) {
    return g_surface_count;
}

u32 graphics_fence_count(void) {
    return g_fence_count;
}

const vex_graphics_surface_t* graphics_surface_get(u32 index) {
    if (index >= g_surface_count) {
        return 0;
    }
    return &g_surfaces[index];
}

const vex_graphics_fence_t* graphics_fence_get(u32 index) {
    if (index >= g_fence_count) {
        return 0;
    }
    return &g_fences[index];
}
