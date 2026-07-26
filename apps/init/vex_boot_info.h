#ifndef VEX_INIT_BOOT_INFO_H
#define VEX_INIT_BOOT_INFO_H

typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;

#define VEX_MAX_APP_IMAGES 7u
#define VEX_MAX_BOOT_FILES 32u
#define VEX_MAX_SHARED_SURFACES 6u

enum {
    VEX_BOOT_FILE_DIRECTORY = 0x1u,
    VEX_BOOT_FILE_PACKAGE = 0x2u,
    VEX_BOOT_FILE_VERIFIED = 0x4u
};

typedef struct vex_framebuffer_info {
    u64 base;
    u64 size;
    u32 width;
    u32 height;
    u32 pixels_per_scanline;
    u32 format;
} vex_framebuffer_info_t;

typedef struct vex_package_image_info {
    u64 package_base;
    u64 package_size;
    u64 manifest_base;
    u64 manifest_size;
    u64 payload_base;
    u64 payload_size;
    u8 payload_hash[32];
    u8 public_key[32];
    u32 verified;
    u32 reserved;
} vex_package_image_info_t;

typedef vex_package_image_info_t vex_init_image_info_t;

typedef struct vex_boot_file_entry {
    char path[96];
    u64 size;
    u32 flags;
    u32 depth;
} vex_boot_file_entry_t;

typedef struct vex_shared_surface_info {
    u32 owner_pid;
    u32 surface_handle;
    u32 fence_handle;
    u32 width;
    u32 height;
    u32 stride;
    u32 buffer_count;
    u32 present_index;
    u64 private_mapping_base;
    u64 shared_mapping_base;
    u64 private_mailbox_base;
    u64 shared_mailbox_base;
    u64 compositor_mapping_base;
    u64 compositor_mailbox_base;
    u64 gpu_mapping_base;
    u64 gpu_mailbox_base;
    u64 bytes_per_buffer;
} vex_shared_surface_info_t;

typedef struct vex_boot_info {
    u32 revision;
    u32 memory_descriptor_version;
    u64 rsdp_address;
    u64 memory_map;
    u64 memory_map_size;
    u64 memory_descriptor_size;
    vex_framebuffer_info_t framebuffer;
    vex_init_image_info_t init_image;
    u32 app_image_count;
    u32 boot_file_count;
    char boot_volume_name[16];
    u32 reserved;
    u64 compositor_scene_mailbox_base;
    u64 gpu_compositor_scene_mailbox_base;
    u64 compositor_gpu_mailbox_base;
    u64 init_gpu_mailbox_base;
    vex_package_image_info_t app_images[VEX_MAX_APP_IMAGES];
    vex_boot_file_entry_t boot_files[VEX_MAX_BOOT_FILES];
    vex_shared_surface_info_t shared_surfaces[VEX_MAX_SHARED_SURFACES];
} vex_boot_info_t;

#endif
