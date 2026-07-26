#include "../include/vex/kernel.h"

enum {
    PAGE_SIZE = 4096u,
    PT_PRESENT = 0x001u,
    PT_WRITABLE = 0x002u,
    PT_USER = 0x004u,
    PT_NX = 1ull << 63
};

static vex_address_space_t g_kernel_space;
static u64 g_kernel_phys_base;
static u64 g_kernel_phys_size;
static u64 g_boot_info_phys_base;
static u64 g_boot_info_phys_size;
static u64 g_package_phys_base[VEX_MAX_APP_IMAGES + 1u];
static u64 g_package_phys_size[VEX_MAX_APP_IMAGES + 1u];
static u32 g_package_window_count;

extern char __kernel_start[];
extern char __kernel_end[];

static void zero_page(void* base) {
    volatile u8* bytes = (volatile u8*)base;
    for (u64 i = 0; i < PAGE_SIZE; ++i) {
        bytes[i] = 0;
    }
}

static u64 align_down(u64 value) {
    return value & ~(u64)(PAGE_SIZE - 1u);
}

static u64 align_up(u64 value) {
    return (value + (PAGE_SIZE - 1u)) & ~(u64)(PAGE_SIZE - 1u);
}

static u64 page_index_for_level(u64 virtual_address, u32 shift) {
    return (virtual_address >> shift) & 0x1FFu;
}

static vex_page_table_t* ensure_child_table(vex_page_table_t* parent, u64 index, u64 extra_flags) {
    u64 entry = parent->entries[index];
    if ((entry & PT_PRESENT) != 0u) {
        if ((extra_flags & PT_USER) != 0u && (entry & PT_USER) == 0u) {
            parent->entries[index] = entry | PT_USER;
            entry = parent->entries[index];
        }
        return (vex_page_table_t*)(usize)(entry & ~0xFFFu);
    }

    u64 page = memory_alloc_page();
    if (page == 0u) {
        return 0;
    }

    vex_page_table_t* table = (vex_page_table_t*)(usize)page;
    zero_page(table);
    parent->entries[index] = page | PT_PRESENT | PT_WRITABLE | extra_flags;
    return table;
}

static int map_page(vex_address_space_t* space, u64 virtual_address, u64 physical_address, u64 flags) {
    const u64 pml4_index = page_index_for_level(virtual_address, 39u);
    const u64 pdpt_index = page_index_for_level(virtual_address, 30u);
    const u64 pd_index = page_index_for_level(virtual_address, 21u);
    const u64 pt_index = page_index_for_level(virtual_address, 12u);

    vex_page_table_t* pdpt = ensure_child_table(space->pml4, pml4_index, flags & PT_USER);
    if (pdpt == 0) {
        return -1;
    }
    vex_page_table_t* pd = ensure_child_table(pdpt, pdpt_index, flags & PT_USER);
    if (pd == 0) {
        return -1;
    }
    vex_page_table_t* pt = ensure_child_table(pd, pd_index, flags & PT_USER);
    if (pt == 0) {
        return -1;
    }

    if ((pt->entries[pt_index] & PT_PRESENT) != 0u) {
        const u64 existing_page = pt->entries[pt_index] & ~0xFFFu;
        if (existing_page == align_down(physical_address)) {
            return 0;
        }
        return -1;
    }

    pt->entries[pt_index] = align_down(physical_address) | flags | PT_PRESENT;
    ++space->mapped_pages;
    return 0;
}

static int map_range(
    vex_address_space_t* space,
    u64 virtual_base,
    u64 physical_base,
    u64 size,
    u64 flags
) {
    u64 mapped = 0;
    const u64 page_count = align_up(size) / PAGE_SIZE;
    while (mapped < page_count) {
        if (map_page(
                space,
                virtual_base + mapped * PAGE_SIZE,
                physical_base + mapped * PAGE_SIZE,
                flags
            ) != 0) {
            return -1;
        }
        ++mapped;
    }
    return 0;
}

static int initialize_space(vex_address_space_t* space) {
    const u64 pml4_page = memory_alloc_page();
    if (pml4_page == 0u) {
        return -1;
    }

    space->cr3_phys = pml4_page;
    space->pml4 = (vex_page_table_t*)(usize)pml4_page;
    zero_page(space->pml4);
    space->mapped_pages = 0;
    space->kernel_window_base = g_kernel_phys_base;
    space->kernel_window_size = g_kernel_phys_size;
    space->user_window_base = 0;
    space->user_window_size = 0;

    if (map_range(space, g_kernel_phys_base, g_kernel_phys_base, g_kernel_phys_size, PT_WRITABLE) != 0) {
        console_write_line("vex:as:map_kernel=FAIL");
        return -1;
    }
    if (map_range(space, g_boot_info_phys_base, g_boot_info_phys_base, g_boot_info_phys_size, PT_WRITABLE | PT_NX) != 0) {
        console_write_line("vex:as:map_bootinfo=FAIL");
        return -1;
    }
    for (u32 index = 0; index < g_package_window_count; ++index) {
        if (g_package_phys_size[index] == 0u) {
            continue;
        }
        console_write("vex:as:map_pkg index=");
        console_write_u64(index);
        console_write(" base=");
        console_write_hex(g_package_phys_base[index]);
        console_write(" size=");
        console_write_u64(g_package_phys_size[index]);
        console_write("\n");
        if (map_range(
                space,
                g_package_phys_base[index],
                g_package_phys_base[index],
                g_package_phys_size[index],
                PT_USER | PT_NX
            ) != 0) {
            console_write_line("vex:as:map_pkg=FAIL");
            return -1;
        }
    }
    return 0;
}

void address_space_init(const vex_boot_info_t* boot_info) {
    g_kernel_phys_base = align_down((u64)(usize)__kernel_start);
    g_kernel_phys_size = align_up((u64)(usize)__kernel_end - g_kernel_phys_base);
    g_boot_info_phys_base = align_down((u64)(usize)boot_info);
    g_boot_info_phys_size = align_up(sizeof(vex_boot_info_t) + ((u64)(usize)boot_info - g_boot_info_phys_base));
    g_package_window_count = 0u;
    console_write("vex:as:kernel base=");
    console_write_hex(g_kernel_phys_base);
    console_write(" size=");
    console_write_u64(g_kernel_phys_size);
    console_write("\n");
    console_write("vex:as:bootinfo base=");
    console_write_hex(g_boot_info_phys_base);
    console_write(" size=");
    console_write_u64(g_boot_info_phys_size);
    console_write(" apps=");
    console_write_u64(boot_info->app_image_count);
    console_write("\n");

    if (boot_info->init_image.verified != 0u && g_package_window_count < VEX_MAX_APP_IMAGES + 1u) {
        g_package_phys_base[g_package_window_count] = align_down(boot_info->init_image.package_base);
        g_package_phys_size[g_package_window_count] = align_up(
            boot_info->init_image.package_size + (boot_info->init_image.package_base - g_package_phys_base[g_package_window_count])
        );
        console_write("vex:as:init_pkg base=");
        console_write_hex(g_package_phys_base[g_package_window_count]);
        console_write(" size=");
        console_write_u64(g_package_phys_size[g_package_window_count]);
        console_write("\n");
        ++g_package_window_count;
    }

    for (u32 index = 0; index < boot_info->app_image_count && index < VEX_MAX_APP_IMAGES; ++index) {
        if (boot_info->app_images[index].verified == 0u || g_package_window_count >= VEX_MAX_APP_IMAGES + 1u) {
            continue;
        }
        g_package_phys_base[g_package_window_count] = align_down(boot_info->app_images[index].package_base);
        g_package_phys_size[g_package_window_count] = align_up(
            boot_info->app_images[index].package_size + (boot_info->app_images[index].package_base - g_package_phys_base[g_package_window_count])
        );
        console_write("vex:as:app_pkg index=");
        console_write_u64(index);
        console_write(" base=");
        console_write_hex(g_package_phys_base[g_package_window_count]);
        console_write(" size=");
        console_write_u64(g_package_phys_size[g_package_window_count]);
        console_write("\n");
        ++g_package_window_count;
    }

    if (initialize_space(&g_kernel_space) != 0) {
        g_kernel_space.cr3_phys = 0;
        g_kernel_space.pml4 = 0;
        g_kernel_space.mapped_pages = 0;
    }
}

const vex_address_space_t* address_space_kernel(void) {
    return &g_kernel_space;
}

int address_space_create_process(
    u64 user_entrypoint,
    u64 image_phys_base,
    u64 image_size,
    vex_address_space_t* out_space
) {
    if (initialize_space(out_space) != 0) {
        return -1;
    }

    out_space->user_window_base = user_entrypoint;
    out_space->user_window_size = align_up(image_size);

    const u64 aligned_image_base = align_down(image_phys_base);
    const u64 image_bias = image_phys_base - aligned_image_base;
    const u64 mapped_size = align_up(image_size + image_bias);
    if (map_range(
            out_space,
            align_down(user_entrypoint),
            aligned_image_base,
            mapped_size,
            PT_WRITABLE | PT_USER
        ) != 0) {
        return -1;
    }

    return 0;
}

int address_space_map_user_range(
    vex_address_space_t* space,
    u64 virtual_base,
    u64 physical_base,
    u64 size,
    u32 writable,
    u32 executable
) {
    u64 flags = PT_USER;
    if (writable != 0u) {
        flags |= PT_WRITABLE;
    }
    if (executable == 0u) {
        flags |= PT_NX;
    }
    return map_range(space, virtual_base, physical_base, size, flags);
}

int address_space_translate(
    const vex_address_space_t* space,
    u64 virtual_address,
    u64* out_physical,
    u64* out_flags
) {
    if (space == 0 || space->pml4 == 0) {
        return -1;
    }

    const vex_page_table_t* pml4 = space->pml4;
    const u64 pml4_entry = pml4->entries[page_index_for_level(virtual_address, 39u)];
    if ((pml4_entry & PT_PRESENT) == 0u) {
        return -1;
    }

    const vex_page_table_t* pdpt = (const vex_page_table_t*)(usize)(pml4_entry & ~0xFFFu);
    const u64 pdpt_entry = pdpt->entries[page_index_for_level(virtual_address, 30u)];
    if ((pdpt_entry & PT_PRESENT) == 0u) {
        return -1;
    }

    const vex_page_table_t* pd = (const vex_page_table_t*)(usize)(pdpt_entry & ~0xFFFu);
    const u64 pd_entry = pd->entries[page_index_for_level(virtual_address, 21u)];
    if ((pd_entry & PT_PRESENT) == 0u) {
        return -1;
    }

    const vex_page_table_t* pt = (const vex_page_table_t*)(usize)(pd_entry & ~0xFFFu);
    const u64 pt_entry = pt->entries[page_index_for_level(virtual_address, 12u)];
    if ((pt_entry & PT_PRESENT) == 0u) {
        return -1;
    }

    if (out_physical != 0) {
        *out_physical = (pt_entry & ~0xFFFu) | (virtual_address & 0xFFFu);
    }
    if (out_flags != 0) {
        *out_flags = pt_entry & (0xFFFu | PT_NX);
    }
    return 0;
}

u64 address_space_page_count(const vex_address_space_t* space) {
    return space->mapped_pages;
}
