#ifndef VEX_BOOT_LOADER_H
#define VEX_BOOT_LOADER_H

#include "efi.h"

#define VEX_PACKAGE_FORMAT_VERSION 2u
#define VEX_PACKAGE_SIGNATURE_LEN 64u

typedef struct vex_loader_file {
    void* base;
    u64 size;
} vex_loader_file_t;

typedef struct vex_loader_segment {
    u64 virtual_address;
    u64 memory_size;
    u64 file_size;
    const u8* file_bytes;
} vex_loader_segment_t;

typedef struct vex_loader_kernel {
    u64 entrypoint;
    u32 segment_count;
    vex_loader_segment_t segments[8];
} vex_loader_kernel_t;

typedef struct vex_loaded_package {
    const u8* package_base;
    u64 package_size;
    const u8* manifest_base;
    u64 manifest_size;
    const u8* payload_base;
    u64 payload_size;
    u8 payload_hash[32];
    u8 public_key[32];
} vex_loaded_package_t;

typedef struct vex_sha256 {
    u32 state[8];
    u64 total_size;
    u32 buffer_size;
    u8 buffer[64];
} vex_sha256_t;

void* loader_memcpy(void* destination, const void* source, u64 size);
void* loader_memset(void* destination, int value, u64 size);
int loader_memcmp(const void* left, const void* right, u64 size);
u64 loader_align_up(u64 value, u64 alignment);
u64 loader_align_down(u64 value, u64 alignment);

void sha256_init(vex_sha256_t* context);
void sha256_update(vex_sha256_t* context, const void* bytes, u64 size);
void sha256_finalize(vex_sha256_t* context, u8 out_hash[32]);
void sha256_bytes(const void* bytes, u64 size, u8 out_hash[32]);

int parse_kernel_image(const vex_loader_file_t* file, vex_loader_kernel_t* out_kernel);
int verify_package(const vex_loader_file_t* file, vex_loaded_package_t* out_package);
int verify_init_package(const vex_loader_file_t* file, vex_loaded_package_t* out_package);

int crypto_sign_ed25519_tweet_open(
    unsigned char* message,
    unsigned long long* message_length,
    const unsigned char* signed_message,
    unsigned long long signed_length,
    const unsigned char* public_key
);

#endif
