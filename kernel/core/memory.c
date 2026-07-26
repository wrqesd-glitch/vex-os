#include "../include/vex/kernel.h"

typedef struct uefi_memory_descriptor {
    u32 type;
    u32 padding;
    u64 physical_start;
    u64 virtual_start;
    u64 page_count;
    u64 attributes;
} uefi_memory_descriptor_t;

static u64 g_usable_pages;
static u64 g_region_count;
static u64 g_allocated_pages;

typedef struct memory_region_cursor {
    u64 next_page;
    u64 last_page;
} memory_region_cursor_t;

static memory_region_cursor_t g_regions[32];
static u32 g_region_slots_used;

typedef struct reserved_range {
    u64 base;
    u64 end;
} reserved_range_t;

static reserved_range_t g_reserved_ranges[16];
static u32 g_reserved_slots_used;

extern char __kernel_start[];
extern char __kernel_end[];

static const u32 UEFI_CONVENTIONAL_MEMORY = 7u;
static const u32 UEFI_BOOT_SERVICES_CODE = 3u;
static const u32 UEFI_BOOT_SERVICES_DATA = 4u;
static const u32 UEFI_LOADER_CODE = 1u;
static const u32 UEFI_LOADER_DATA = 2u;

static int region_is_reclaimable(u32 type) {
    return type == UEFI_CONVENTIONAL_MEMORY ||
           type == UEFI_BOOT_SERVICES_CODE ||
           type == UEFI_BOOT_SERVICES_DATA ||
           type == UEFI_LOADER_CODE ||
           type == UEFI_LOADER_DATA;
}

static u64 align_down(u64 value) {
    return value & ~4095ull;
}

static u64 align_up(u64 value) {
    return (value + 4095ull) & ~4095ull;
}

static void reserve_range(u64 base, u64 size) {
    if (size == 0u || g_reserved_slots_used >= 16u) {
        return;
    }

    g_reserved_ranges[g_reserved_slots_used].base = align_down(base);
    g_reserved_ranges[g_reserved_slots_used].end = align_up(base + size);
    ++g_reserved_slots_used;
}

void memory_init(const vex_boot_info_t* boot_info) {
    g_usable_pages = 0;
    g_region_count = 0;
    g_allocated_pages = 0;
    g_region_slots_used = 0;
    g_reserved_slots_used = 0;

    reserve_range((u64)(usize)__kernel_start, (u64)(usize)__kernel_end - (u64)(usize)__kernel_start);
    reserve_range((u64)(usize)boot_info, sizeof(vex_boot_info_t));
    reserve_range(boot_info->memory_map, boot_info->memory_map_size);
    reserve_range(boot_info->init_image.package_base, boot_info->init_image.package_size);
    for (u32 index = 0; index < boot_info->app_image_count && index < VEX_MAX_APP_IMAGES; ++index) {
        if (boot_info->app_images[index].verified == 0u) {
            continue;
        }
        reserve_range(boot_info->app_images[index].package_base, boot_info->app_images[index].package_size);
    }

    const u8* cursor = (const u8*)(usize)boot_info->memory_map;
    const u8* end = cursor + boot_info->memory_map_size;

    while (cursor < end) {
        const uefi_memory_descriptor_t* desc = (const uefi_memory_descriptor_t*)cursor;
        if (region_is_reclaimable(desc->type)) {
            g_usable_pages += desc->page_count;
            if (desc->page_count > 0 && g_region_slots_used < 32u) {
                g_regions[g_region_slots_used].next_page = desc->physical_start;
                g_regions[g_region_slots_used].last_page = desc->physical_start + desc->page_count * 4096u;
                ++g_region_slots_used;
            }
        }
        ++g_region_count;
        cursor += boot_info->memory_descriptor_size;
    }
}

u64 memory_usable_pages(void) {
    return g_usable_pages;
}

u64 memory_region_count(void) {
    return g_region_count;
}

u64 memory_alloc_page(void) {
    return memory_alloc_pages(1u);
}

u64 memory_alloc_pages(u64 count) {
    const u64 span = count * 4096u;
    for (u32 i = 0; i < g_region_slots_used; ++i) {
        u64 candidate = align_up(g_regions[i].next_page);
        while (candidate + span <= g_regions[i].last_page) {
            if (candidate < 0x100000u) {
                candidate = 0x100000u;
            }

            u32 overlap_found = 0u;
            for (u32 reserved = 0; reserved < g_reserved_slots_used; ++reserved) {
                const u64 reserved_base = g_reserved_ranges[reserved].base;
                const u64 reserved_end = g_reserved_ranges[reserved].end;
                if (candidate < reserved_end && candidate + span > reserved_base) {
                    candidate = align_up(reserved_end);
                    overlap_found = 1u;
                    break;
                }
            }

            if (overlap_found != 0u) {
                continue;
            }

            g_regions[i].next_page = candidate + span;
            g_allocated_pages += count;
            return candidate;
        }
    }
    return 0u;
}

u64 memory_allocated_pages(void) {
    return g_allocated_pages;
}
