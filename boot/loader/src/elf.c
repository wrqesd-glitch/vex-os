#include "../include/loader.h"

typedef struct elf64_header {
    u8 ident[16];
    u16 type;
    u16 machine;
    u32 version;
    u64 entry;
    u64 phoff;
    u64 shoff;
    u32 flags;
    u16 ehsize;
    u16 phentsize;
    u16 phnum;
    u16 shentsize;
    u16 shnum;
    u16 shstrndx;
} elf64_header_t;

typedef struct elf64_program_header {
    u32 type;
    u32 flags;
    u64 offset;
    u64 vaddr;
    u64 paddr;
    u64 filesz;
    u64 memsz;
    u64 align;
} elf64_program_header_t;

static const u8 k_elf_magic[4] = {0x7Fu, 'E', 'L', 'F'};
static const u16 ELF64_EXEC = 2u;
static const u16 ELF64_X86_64 = 62u;
static const u32 ELF64_PT_LOAD = 1u;

int parse_kernel_image(const vex_loader_file_t* file, vex_loader_kernel_t* out_kernel) {
    if (file->size < sizeof(elf64_header_t)) {
        return -1;
    }

    const elf64_header_t* header = (const elf64_header_t*)file->base;
    if (loader_memcmp(header->ident, k_elf_magic, 4u) != 0 ||
        header->ident[4] != 2u ||
        header->ident[5] != 1u ||
        header->type != ELF64_EXEC ||
        header->machine != ELF64_X86_64) {
        return -1;
    }

    if (header->phentsize != sizeof(elf64_program_header_t)) {
        return -1;
    }
    if (header->phnum == 0u || header->phnum > 8u) {
        return -1;
    }

    out_kernel->entrypoint = header->entry;
    out_kernel->segment_count = 0u;

    for (u32 i = 0; i < header->phnum; ++i) {
        const u64 ph_offset = header->phoff + (u64)i * header->phentsize;
        if (ph_offset + sizeof(elf64_program_header_t) > file->size) {
            return -1;
        }

        const elf64_program_header_t* segment =
            (const elf64_program_header_t*)((const u8*)file->base + ph_offset);
        if (segment->type != ELF64_PT_LOAD || segment->memsz == 0u) {
            continue;
        }

        if (segment->offset + segment->filesz > file->size || segment->filesz > segment->memsz) {
            return -1;
        }

        vex_loader_segment_t* out = &out_kernel->segments[out_kernel->segment_count++];
        out->virtual_address = segment->vaddr;
        out->memory_size = segment->memsz;
        out->file_size = segment->filesz;
        out->file_bytes = (const u8*)file->base + segment->offset;
    }

    return out_kernel->segment_count == 0u ? -1 : 0;
}
