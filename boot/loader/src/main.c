#include "../include/loader.h"

const efi_guid_t EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID_VALUE = {0x9042A9DEu, 0x23DCu, 0x4A38u, {0x96u, 0xFBu, 0x7Au, 0xDEu, 0xD0u, 0x80u, 0x51u, 0x6Au}};
const efi_guid_t EFI_ACPI_20_TABLE_GUID_VALUE = {0x8868E871u, 0xE4F1u, 0x11D3u, {0xBCu, 0x22u, 0x00u, 0x80u, 0xC7u, 0x3Cu, 0x88u, 0x81u}};
const efi_guid_t EFI_ACPI_TABLE_GUID_VALUE = {0xEB9D2D30u, 0x2D88u, 0x11D3u, {0x9Au, 0x16u, 0x00u, 0x90u, 0x27u, 0x3Fu, 0xC1u, 0x4Du}};
const efi_guid_t EFI_LOADED_IMAGE_PROTOCOL_GUID_VALUE = {0x5B1B31A1u, 0x9562u, 0x11D2u, {0x8Eu, 0x3Fu, 0x00u, 0xA0u, 0xC9u, 0x69u, 0x72u, 0x3Bu}};
const efi_guid_t EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID_VALUE = {0x0964E5B2u, 0x6459u, 0x11D2u, {0x8Eu, 0x39u, 0x00u, 0xA0u, 0xC9u, 0x69u, 0x72u, 0x3Bu}};
const efi_guid_t EFI_FILE_INFO_GUID_VALUE = {0x09576E92u, 0x6D3Fu, 0x11D2u, {0x8Eu, 0x39u, 0x00u, 0xA0u, 0xC9u, 0x69u, 0x72u, 0x3Bu}};

static vex_boot_info_t g_boot_info = {
    .revision = VEX_BOOTINFO_REVISION,
    .compositor_scene_mailbox_base = 0u,
    .gpu_compositor_scene_mailbox_base = 0u,
    .compositor_gpu_mailbox_base = 0u,
    .init_gpu_mailbox_base = 0u
};

extern unsigned char vex_kernel_blob[];
extern usize vex_kernel_blob_size;
extern unsigned char vex_init_blob[];
extern usize vex_init_blob_size;
extern unsigned char vex_diag_blob[];
extern usize vex_diag_blob_size;
extern unsigned char vex_tests_blob[];
extern usize vex_tests_blob_size;
extern unsigned char vex_services_blob[];
extern usize vex_services_blob_size;
extern unsigned char vex_terminal_blob[];
extern usize vex_terminal_blob_size;
extern unsigned char vex_terminal2_blob[];
extern usize vex_terminal2_blob_size;
extern unsigned char vex_compositor_blob[];
extern usize vex_compositor_blob_size;
extern unsigned char vex_gpu_blob[];
extern usize vex_gpu_blob_size;

static void loader_write_text(efi_system_table_t* system_table, const efi_char16_t* text) {
    if (system_table == 0 || system_table->con_out == 0 || system_table->con_out->output_string == 0) {
        return;
    }
    system_table->con_out->output_string(system_table->con_out, (efi_char16_t*)text);
}

static efi_status_t load_kernel_segments(
    efi_system_table_t* system_table,
    const vex_loader_kernel_t* kernel
) {
    for (u32 i = 0; i < kernel->segment_count; ++i) {
        const vex_loader_segment_t* segment = &kernel->segments[i];
        const u64 segment_base = loader_align_down(segment->virtual_address, EFI_PAGE_SIZE);
        const u64 segment_bias = segment->virtual_address - segment_base;
        const u64 segment_span = loader_align_up(segment->memory_size + segment_bias, EFI_PAGE_SIZE);
        efi_physical_address_t destination = segment_base;
        efi_status_t status = system_table->boot_services->allocate_pages(
            EFI_ALLOCATE_ADDRESS,
            EFI_LOADER_DATA,
            segment_span / EFI_PAGE_SIZE,
            &destination
        );
        if (status != EFI_SUCCESS) {
            return status;
        }

        loader_memset((void*)(usize)segment_base, 0, segment_span);
        loader_memcpy((void*)(usize)segment->virtual_address, segment->file_bytes, segment->file_size);
    }
    return EFI_SUCCESS;
}

static void capture_acpi(efi_system_table_t* system_table) {
    g_boot_info.rsdp_address = 0u;
    for (u64 i = 0; i < system_table->number_of_table_entries; ++i) {
        const efi_configuration_table_t* table = &system_table->configuration_table[i];
        const int acpi20 = loader_memcmp(&table->vendor_guid, &EFI_ACPI_20_TABLE_GUID_VALUE, sizeof(efi_guid_t)) == 0;
        const int acpi10 = loader_memcmp(&table->vendor_guid, &EFI_ACPI_TABLE_GUID_VALUE, sizeof(efi_guid_t)) == 0;
        if (acpi20 || acpi10) {
            g_boot_info.rsdp_address = (u64)(usize)table->vendor_table;
            return;
        }
    }
}

static void capture_framebuffer(efi_system_table_t* system_table) {
    loader_memset(&g_boot_info.framebuffer, 0, sizeof(g_boot_info.framebuffer));

    efi_graphics_output_protocol_t* gop = 0;
    if (system_table->boot_services->locate_protocol(
            (efi_guid_t*)&EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID_VALUE,
            0,
            (void**)&gop
        ) != EFI_SUCCESS || gop == 0 || gop->mode == 0 || gop->mode->info == 0) {
        return;
    }

    if (gop->set_mode != 0 && gop->mode->mode < gop->mode->max_mode) {
        gop->set_mode(gop, gop->mode->mode);
    }
    if (gop->mode == 0 || gop->mode->info == 0) {
        return;
    }

    g_boot_info.framebuffer.base = gop->mode->frame_buffer_base;
    g_boot_info.framebuffer.size = gop->mode->frame_buffer_size;
    g_boot_info.framebuffer.width = gop->mode->info->horizontal_resolution;
    g_boot_info.framebuffer.height = gop->mode->info->vertical_resolution;
    g_boot_info.framebuffer.pixels_per_scanline = gop->mode->info->pixels_per_scan_line;
    g_boot_info.framebuffer.format = gop->mode->info->pixel_format + 1u;
}

static void populate_init_image(const vex_loaded_package_t* package) {
    g_boot_info.init_image.package_base = (u64)(usize)package->package_base;
    g_boot_info.init_image.package_size = package->package_size;
    g_boot_info.init_image.manifest_base = (u64)(usize)package->manifest_base;
    g_boot_info.init_image.manifest_size = package->manifest_size;
    g_boot_info.init_image.payload_base = (u64)(usize)package->payload_base;
    g_boot_info.init_image.payload_size = package->payload_size;
    loader_memcpy(g_boot_info.init_image.payload_hash, package->payload_hash, 32u);
    loader_memcpy(g_boot_info.init_image.public_key, package->public_key, 32u);
    g_boot_info.init_image.verified = 1u;
}

static void populate_app_image(u32 slot, const vex_loaded_package_t* package) {
    if (slot >= VEX_MAX_APP_IMAGES) {
        return;
    }

    g_boot_info.app_images[slot].package_base = (u64)(usize)package->package_base;
    g_boot_info.app_images[slot].package_size = package->package_size;
    g_boot_info.app_images[slot].manifest_base = (u64)(usize)package->manifest_base;
    g_boot_info.app_images[slot].manifest_size = package->manifest_size;
    g_boot_info.app_images[slot].payload_base = (u64)(usize)package->payload_base;
    g_boot_info.app_images[slot].payload_size = package->payload_size;
    loader_memcpy(g_boot_info.app_images[slot].payload_hash, package->payload_hash, 32u);
    loader_memcpy(g_boot_info.app_images[slot].public_key, package->public_key, 32u);
    g_boot_info.app_images[slot].verified = 1u;
    if (g_boot_info.app_image_count < slot + 1u) {
        g_boot_info.app_image_count = slot + 1u;
    }
}

static void copy_ascii(char* dst, u32 dst_size, const char* src) {
    u32 index = 0u;
    if (dst == 0 || dst_size == 0u) {
        return;
    }
    if (src == 0) {
        dst[0] = 0;
        return;
    }
    while (src[index] != 0 && index + 1u < dst_size) {
        dst[index] = src[index];
        ++index;
    }
    dst[index] = 0;
}

static void append_ascii(char* dst, u32 dst_size, const char* src) {
    u32 index = 0u;
    while (index + 1u < dst_size && dst[index] != 0) {
        ++index;
    }
    if (index + 1u >= dst_size) {
        return;
    }
    copy_ascii(dst + index, dst_size - index, src);
}

static void utf16_to_ascii(char* dst, u32 dst_size, const efi_char16_t* src) {
    u32 index = 0u;
    if (dst == 0 || dst_size == 0u) {
        return;
    }
    while (src[index] != 0 && index + 1u < dst_size) {
        const u16 value = src[index];
        dst[index] = value >= 32u && value <= 126u ? (char)value : '?';
        ++index;
    }
    dst[index] = 0;
}

static int ascii_equals(const char* left, const char* right) {
    u32 index = 0u;
    while (left[index] != 0 || right[index] != 0) {
        if (left[index] != right[index]) {
            return 0;
        }
        ++index;
    }
    return 1;
}

static int ascii_ends_with(const char* text, const char* suffix) {
    u32 text_len = 0u;
    u32 suffix_len = 0u;
    while (text[text_len] != 0) {
        ++text_len;
    }
    while (suffix[suffix_len] != 0) {
        ++suffix_len;
    }
    if (suffix_len > text_len) {
        return 0;
    }
    return ascii_equals(text + text_len - suffix_len, suffix);
}

static u32 verified_flag_for_path(const char* path) {
    if (ascii_equals(path, "EFI/BOOT/INIT.VEX") ||
        ascii_equals(path, "EFI/BOOT/DIAG.VEX") ||
        ascii_equals(path, "EFI/BOOT/TESTS.VEX") ||
        ascii_equals(path, "EFI/BOOT/SERVICES.VEX") ||
        ascii_equals(path, "EFI/BOOT/TERMINAL.VEX") ||
        ascii_equals(path, "EFI/BOOT/TERMINAL2.VEX") ||
        ascii_equals(path, "EFI/BOOT/COMPOSITOR.VEX") ||
        ascii_equals(path, "EFI/BOOT/GPU.VEX")) {
        return VEX_BOOT_FILE_VERIFIED;
    }
    return 0u;
}

static void catalog_append_entry(const char* path, u64 size, u32 flags, u32 depth) {
    vex_boot_file_entry_t* entry;
    if (g_boot_info.boot_file_count >= VEX_MAX_BOOT_FILES) {
        return;
    }
    entry = &g_boot_info.boot_files[g_boot_info.boot_file_count++];
    loader_memset(entry, 0, sizeof(*entry));
    copy_ascii(entry->path, sizeof(entry->path), path);
    entry->size = size;
    entry->flags = flags | verified_flag_for_path(path);
    entry->depth = depth;
}

static void catalog_walk_directory(
    efi_file_protocol_t* directory,
    const char* base_path,
    u32 depth,
    u32 max_depth
) {
    u8 info_buffer[512];
    efi_uintn_t read_size;

    if (directory == 0 || depth > max_depth) {
        return;
    }

    directory->set_position(directory, 0u);
    for (;;) {
        read_size = sizeof(info_buffer);
        if (directory->read(directory, &read_size, info_buffer) != EFI_SUCCESS || read_size == 0u) {
            return;
        }
        {
            efi_file_info_t* info = (efi_file_info_t*)info_buffer;
            char name[96];
            char path[96];
            u32 flags = 0u;
            utf16_to_ascii(name, sizeof(name), info->file_name);
            if (name[0] == 0 || ascii_equals(name, ".") || ascii_equals(name, "..")) {
                continue;
            }
            path[0] = 0;
            if (base_path != 0 && base_path[0] != 0) {
                copy_ascii(path, sizeof(path), base_path);
                append_ascii(path, sizeof(path), "/");
            }
            append_ascii(path, sizeof(path), name);
            if ((info->attribute & EFI_FILE_DIRECTORY) != 0u) {
                efi_file_protocol_t* child = 0;
                flags |= VEX_BOOT_FILE_DIRECTORY;
                catalog_append_entry(path, 0u, flags, depth);
                if (depth < max_depth &&
                    directory->open(directory, &child, info->file_name, EFI_FILE_MODE_READ, 0u) == EFI_SUCCESS &&
                    child != 0) {
                    catalog_walk_directory(child, path, depth + 1u, max_depth);
                    child->close(child);
                }
                continue;
            }
            if (ascii_ends_with(path, ".VEX") != 0) {
                flags |= VEX_BOOT_FILE_PACKAGE;
            }
            catalog_append_entry(path, info->file_size, flags, depth);
        }
    }
}

static void capture_boot_catalog(efi_handle_t image_handle, efi_system_table_t* system_table) {
    efi_loaded_image_protocol_t* loaded_image = 0;
    efi_simple_file_system_protocol_t* fs = 0;
    efi_file_protocol_t* root = 0;

    g_boot_info.boot_file_count = 0u;
    loader_memset(g_boot_info.boot_volume_name, 0, sizeof(g_boot_info.boot_volume_name));
    copy_ascii(g_boot_info.boot_volume_name, sizeof(g_boot_info.boot_volume_name), "EFI0");

    if (system_table->boot_services->open_protocol(
            image_handle,
            (efi_guid_t*)&EFI_LOADED_IMAGE_PROTOCOL_GUID_VALUE,
            (void**)&loaded_image,
            image_handle,
            0,
            EFI_OPEN_PROTOCOL_GET_PROTOCOL
        ) != EFI_SUCCESS || loaded_image == 0) {
        return;
    }
    if (system_table->boot_services->open_protocol(
            loaded_image->device_handle,
            (efi_guid_t*)&EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID_VALUE,
            (void**)&fs,
            image_handle,
            0,
            EFI_OPEN_PROTOCOL_GET_PROTOCOL
        ) != EFI_SUCCESS || fs == 0) {
        return;
    }
    if (fs->open_volume(fs, &root) != EFI_SUCCESS || root == 0) {
        return;
    }

    catalog_append_entry("EFI0", 0u, VEX_BOOT_FILE_DIRECTORY, 0u);
    catalog_walk_directory(root, "", 1u, 3u);
    root->close(root);
}

static efi_status_t finalize_memory_map(
    efi_handle_t image_handle,
    efi_system_table_t* system_table
) {
    efi_uintn_t memory_map_size = 0u;
    efi_uintn_t map_key = 0u;
    efi_uintn_t descriptor_size = 0u;
    u32 descriptor_version = 0u;
    efi_status_t status = system_table->boot_services->get_memory_map(
        &memory_map_size,
        0,
        &map_key,
        &descriptor_size,
        &descriptor_version
    );
    if (status != EFI_BUFFER_TOO_SMALL) {
        return status;
    }

    memory_map_size += descriptor_size * 8u;
    void* memory_map = 0;
    status = system_table->boot_services->allocate_pool(EFI_LOADER_DATA, memory_map_size, &memory_map);
    if (status != EFI_SUCCESS) {
        return status;
    }

    while (1) {
        efi_uintn_t actual_size = memory_map_size;
        status = system_table->boot_services->get_memory_map(
            &actual_size,
            (efi_memory_descriptor_t*)memory_map,
            &map_key,
            &descriptor_size,
            &descriptor_version
        );
        if (status == EFI_BUFFER_TOO_SMALL) {
            system_table->boot_services->free_pool(memory_map);
            memory_map_size = actual_size + descriptor_size * 8u;
            status = system_table->boot_services->allocate_pool(EFI_LOADER_DATA, memory_map_size, &memory_map);
            if (status != EFI_SUCCESS) {
                return status;
            }
            continue;
        }
        if (status != EFI_SUCCESS) {
            return status;
        }

        g_boot_info.memory_descriptor_version = descriptor_version;
        g_boot_info.memory_map = (u64)(usize)memory_map;
        g_boot_info.memory_map_size = actual_size;
        g_boot_info.memory_descriptor_size = descriptor_size;

        status = system_table->boot_services->exit_boot_services(image_handle, map_key);
        if (status == EFI_SUCCESS) {
            return EFI_SUCCESS;
        }
        if (status != EFI_INVALID_PARAMETER) {
            return status;
        }
    }
}

typedef void(__attribute__((sysv_abi)) * vex_kernel_entry_t)(const vex_boot_info_t* boot_info);

efi_status_t EFIAPI efi_main(efi_handle_t image_handle, efi_system_table_t* system_table) {
    system_table->boot_services->set_watchdog_timer(0u, 0u, 0u, 0);
    loader_write_text(system_table, L"Vex loader: kernel parse\r\n");

    vex_loader_file_t kernel_file = {
        .base = (u8*)vex_kernel_blob,
        .size = (u64)vex_kernel_blob_size
    };
    vex_loader_kernel_t kernel;
    if (parse_kernel_image(&kernel_file, &kernel) != 0) {
        loader_write_text(system_table, L"Vex loader: kernel parse fail\r\n");
        return EFI_LOAD_ERROR;
    }
    loader_write_text(system_table, L"Vex loader: kernel parsed\r\n");

    loader_write_text(system_table, L"Vex loader: kernel load\r\n");
    efi_status_t status = load_kernel_segments(system_table, &kernel);
    if (status != EFI_SUCCESS) {
        loader_write_text(system_table, L"Vex loader: kernel load fail\r\n");
        return status;
    }
    loader_write_text(system_table, L"Vex loader: kernel loaded\r\n");

    vex_loader_file_t package_file = {
        .base = (u8*)vex_init_blob,
        .size = (u64)vex_init_blob_size
    };
    vex_loaded_package_t package;
    if (verify_init_package(&package_file, &package) != 0) {
        loader_write_text(system_table, L"Vex loader: init verify fail\r\n");
        return EFI_SECURITY_VIOLATION;
    }
    loader_write_text(system_table, L"Vex loader: init verified\r\n");

    vex_loader_file_t diagnostics_file = {
        .base = (u8*)vex_diag_blob,
        .size = (u64)vex_diag_blob_size
    };
    vex_loaded_package_t diagnostics_package;
    if (verify_package(&diagnostics_file, &diagnostics_package) != 0) {
        loader_write_text(system_table, L"Vex loader: diag verify fail\r\n");
        return EFI_SECURITY_VIOLATION;
    }
    loader_write_text(system_table, L"Vex loader: diag verified\r\n");

    vex_loader_file_t tests_file = {
        .base = (u8*)vex_tests_blob,
        .size = (u64)vex_tests_blob_size
    };
    vex_loaded_package_t tests_package;
    if (verify_package(&tests_file, &tests_package) != 0) {
        loader_write_text(system_table, L"Vex loader: tests verify fail\r\n");
        return EFI_SECURITY_VIOLATION;
    }
    loader_write_text(system_table, L"Vex loader: tests verified\r\n");

    vex_loader_file_t services_file = {
        .base = (u8*)vex_services_blob,
        .size = (u64)vex_services_blob_size
    };
    vex_loaded_package_t services_package;
    if (verify_package(&services_file, &services_package) != 0) {
        loader_write_text(system_table, L"Vex loader: services verify fail\r\n");
        return EFI_SECURITY_VIOLATION;
    }
    loader_write_text(system_table, L"Vex loader: services verified\r\n");

    vex_loader_file_t terminal_file = {
        .base = (u8*)vex_terminal_blob,
        .size = (u64)vex_terminal_blob_size
    };
    vex_loaded_package_t terminal_package;
    if (verify_package(&terminal_file, &terminal_package) != 0) {
        loader_write_text(system_table, L"Vex loader: terminal verify fail\r\n");
        return EFI_SECURITY_VIOLATION;
    }
    loader_write_text(system_table, L"Vex loader: terminal verified\r\n");

    vex_loader_file_t terminal2_file = {
        .base = (u8*)vex_terminal2_blob,
        .size = (u64)vex_terminal2_blob_size
    };
    vex_loaded_package_t terminal2_package;
    if (verify_package(&terminal2_file, &terminal2_package) != 0) {
        loader_write_text(system_table, L"Vex loader: terminal2 verify fail\r\n");
        return EFI_SECURITY_VIOLATION;
    }
    loader_write_text(system_table, L"Vex loader: terminal2 verified\r\n");

    vex_loader_file_t compositor_file = {
        .base = (u8*)vex_compositor_blob,
        .size = (u64)vex_compositor_blob_size
    };
    vex_loaded_package_t compositor_package;
    if (verify_package(&compositor_file, &compositor_package) != 0) {
        loader_write_text(system_table, L"Vex loader: compositor verify fail\r\n");
        return EFI_SECURITY_VIOLATION;
    }
    loader_write_text(system_table, L"Vex loader: compositor verified\r\n");

    vex_loader_file_t gpu_file = {
        .base = (u8*)vex_gpu_blob,
        .size = (u64)vex_gpu_blob_size
    };
    vex_loaded_package_t gpu_package;
    if (verify_package(&gpu_file, &gpu_package) != 0) {
        loader_write_text(system_table, L"Vex loader: gpu verify fail\r\n");
        return EFI_SECURITY_VIOLATION;
    }
    loader_write_text(system_table, L"Vex loader: gpu verified\r\n");

    loader_write_text(system_table, L"Vex loader: capture boot state\r\n");
    capture_acpi(system_table);
    capture_framebuffer(system_table);
    capture_boot_catalog(image_handle, system_table);
    loader_write_text(system_table, L"Vex loader: boot state captured\r\n");
    populate_init_image(&package);
    populate_app_image(0u, &diagnostics_package);
    populate_app_image(1u, &tests_package);
    populate_app_image(2u, &services_package);
    populate_app_image(3u, &terminal_package);
    populate_app_image(4u, &compositor_package);
    populate_app_image(5u, &gpu_package);
    populate_app_image(6u, &terminal2_package);

    status = finalize_memory_map(image_handle, system_table);
    if (status != EFI_SUCCESS) {
        return status;
    }

    loader_write_text(system_table, L"Vex loader: enter kernel\r\n");
    ((vex_kernel_entry_t)(usize)kernel.entrypoint)(&g_boot_info);
    return EFI_ABORTED;
}
