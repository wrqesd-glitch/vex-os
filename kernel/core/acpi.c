#include "../include/vex/kernel.h"

typedef struct acpi_rsdp_v1 {
    char signature[8];
    u8 checksum;
    char oem_id[6];
    u8 revision;
    u32 rsdt_address;
} acpi_rsdp_v1_t;

typedef struct acpi_rsdp_v2 {
    acpi_rsdp_v1_t first_part;
    u32 length;
    u64 xsdt_address;
    u8 extended_checksum;
    u8 reserved[3];
} acpi_rsdp_v2_t;

typedef struct acpi_sdt_header {
    char signature[4];
    u32 length;
    u8 revision;
    u8 checksum;
    char oem_id[6];
    char oem_table_id[8];
    u32 oem_revision;
    u32 creator_id;
    u32 creator_revision;
} acpi_sdt_header_t;

typedef struct acpi_madt {
    acpi_sdt_header_t header;
    u32 lapic_address;
    u32 flags;
} acpi_madt_t;

typedef struct acpi_hpet {
    acpi_sdt_header_t header;
    u32 hardware_block_id;
    struct {
        u8 address_space_id;
        u8 register_bit_width;
        u8 register_bit_offset;
        u8 reserved;
        u64 address;
    } base_address;
    u8 hpet_number;
    u16 minimum_tick;
    u8 page_protection;
} acpi_hpet_t;

static vex_acpi_state_t g_acpi_state;

static u32 checksum_ok(const void* base, u32 size) {
    const u8* bytes = (const u8*)base;
    u8 sum = 0;
    for (u32 i = 0; i < size; ++i) {
        sum = (u8)(sum + bytes[i]);
    }
    return sum == 0u;
}

static u32 signature_eq(const char* left, const char* right) {
    for (u32 i = 0; i < 4u; ++i) {
        if (left[i] != right[i]) {
            return 0u;
        }
    }
    return 1u;
}

static const acpi_sdt_header_t* validate_sdt(u64 address) {
    if (address == 0u) {
        return 0;
    }

    const acpi_sdt_header_t* header = (const acpi_sdt_header_t*)(usize)address;
    if (header->length < sizeof(acpi_sdt_header_t)) {
        return 0;
    }
    if (checksum_ok(header, header->length) == 0u) {
        return 0;
    }
    return header;
}

static void scan_madt(const acpi_madt_t* madt) {
    g_acpi_state.lapic_address = madt->lapic_address;

    const u8* cursor = (const u8*)madt + sizeof(acpi_madt_t);
    const u8* end = (const u8*)madt + madt->header.length;
    while (cursor + 2u <= end) {
        const u8 type = cursor[0];
        const u8 length = cursor[1];
        if (length < 2u || cursor + length > end) {
            break;
        }

        if (type == 0u && length >= 8u) {
            const u32 flags = *(const u32*)(const void*)(cursor + 4u);
            if ((flags & 0x1u) != 0u) {
                ++g_acpi_state.cpu_count;
            }
        } else if (type == 1u && length >= 12u) {
            ++g_acpi_state.ioapic_count;
        }

        cursor += length;
    }
}

static void scan_root_table(const acpi_sdt_header_t* root, u32 uses_xsdt) {
    const u32 entry_size = uses_xsdt != 0u ? 8u : 4u;
    const u32 entry_count = (root->length - sizeof(acpi_sdt_header_t)) / entry_size;
    const u8* entries = (const u8*)root + sizeof(acpi_sdt_header_t);

    for (u32 i = 0; i < entry_count; ++i) {
        u64 entry_address = 0u;
        if (uses_xsdt != 0u) {
            entry_address = ((const u64*)entries)[i];
        } else {
            entry_address = ((const u32*)entries)[i];
        }

        const acpi_sdt_header_t* header = validate_sdt(entry_address);
        if (header == 0) {
            continue;
        }

        if (signature_eq(header->signature, "APIC") != 0u) {
            g_acpi_state.madt_address = entry_address;
            scan_madt((const acpi_madt_t*)header);
        } else if (signature_eq(header->signature, "HPET") != 0u) {
            const acpi_hpet_t* hpet = (const acpi_hpet_t*)header;
            g_acpi_state.hpet_address = entry_address;
            g_acpi_state.hpet_mmio_base = hpet->base_address.address;
            g_acpi_state.hpet_present = 1u;
        }
    }
}

void acpi_init(const vex_boot_info_t* boot_info) {
    g_acpi_state.rsdp_address = boot_info->rsdp_address;
    g_acpi_state.rsdt_address = 0u;
    g_acpi_state.xsdt_address = 0u;
    g_acpi_state.madt_address = 0u;
    g_acpi_state.hpet_address = 0u;
    g_acpi_state.hpet_mmio_base = 0u;
    g_acpi_state.lapic_address = 0u;
    g_acpi_state.cpu_count = 0u;
    g_acpi_state.ioapic_count = 0u;
    g_acpi_state.hpet_present = 0u;

    if (boot_info->rsdp_address == 0u) {
        return;
    }

    const acpi_rsdp_v2_t* rsdp = (const acpi_rsdp_v2_t*)(usize)boot_info->rsdp_address;
    if (checksum_ok(rsdp, sizeof(acpi_rsdp_v1_t)) == 0u) {
        return;
    }

    g_acpi_state.rsdt_address = rsdp->first_part.rsdt_address;
    if (rsdp->first_part.revision >= 2u && rsdp->length >= sizeof(acpi_rsdp_v2_t) && checksum_ok(rsdp, rsdp->length) != 0u) {
        g_acpi_state.xsdt_address = rsdp->xsdt_address;
    }

    const acpi_sdt_header_t* root = 0;
    if (g_acpi_state.xsdt_address != 0u) {
        root = validate_sdt(g_acpi_state.xsdt_address);
        if (root != 0) {
            scan_root_table(root, 1u);
            return;
        }
    }

    root = validate_sdt(g_acpi_state.rsdt_address);
    if (root != 0) {
        scan_root_table(root, 0u);
    }
}

const vex_acpi_state_t* acpi_state(void) {
    return &g_acpi_state;
}
