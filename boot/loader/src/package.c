#include "../include/loader.h"
#include "../include/tweetnacl.h"

static const u8 k_package_magic[8] = {'V', 'E', 'X', 'P', 'K', 'G', '2', 0};
static const u8 k_signing_context[8] = {'V', 'E', 'X', 'S', 'I', 'G', '1', 0};

static u32 read_u32_le(const u8* bytes) {
    return (u32)bytes[0] |
           ((u32)bytes[1] << 8u) |
           ((u32)bytes[2] << 16u) |
           ((u32)bytes[3] << 24u);
}

static u64 read_u64_le(const u8* bytes) {
    return (u64)bytes[0] |
           ((u64)bytes[1] << 8u) |
           ((u64)bytes[2] << 16u) |
           ((u64)bytes[3] << 24u) |
           ((u64)bytes[4] << 32u) |
           ((u64)bytes[5] << 40u) |
           ((u64)bytes[6] << 48u) |
           ((u64)bytes[7] << 56u);
}

static void write_u32_le(u8* out, u32 value) {
    out[0] = (u8)value;
    out[1] = (u8)(value >> 8u);
    out[2] = (u8)(value >> 16u);
    out[3] = (u8)(value >> 24u);
}

static void write_u64_le(u8* out, u64 value) {
    out[0] = (u8)value;
    out[1] = (u8)(value >> 8u);
    out[2] = (u8)(value >> 16u);
    out[3] = (u8)(value >> 24u);
    out[4] = (u8)(value >> 32u);
    out[5] = (u8)(value >> 40u);
    out[6] = (u8)(value >> 48u);
    out[7] = (u8)(value >> 56u);
}

static int verify_signature(
    const u8* signature,
    const u8* public_key,
    const u8* message,
    u64 message_size
) {
    u8 opened[176];
    u8 signed_message[176];
    unsigned long long opened_size = 0u;

    if (message_size > 112u) {
        return -1;
    }

    loader_memcpy(signed_message, signature, VEX_PACKAGE_SIGNATURE_LEN);
    loader_memcpy(signed_message + VEX_PACKAGE_SIGNATURE_LEN, message, message_size);

    if (crypto_sign_ed25519_tweet_open(
            opened,
            &opened_size,
            signed_message,
            message_size + VEX_PACKAGE_SIGNATURE_LEN,
            public_key
        ) != 0) {
        return -1;
    }

    if (opened_size != message_size || loader_memcmp(opened, message, message_size) != 0) {
        return -1;
    }
    return 0;
}

int verify_package(const vex_loader_file_t* file, vex_loaded_package_t* out_package) {
    if (file->size < 92u + VEX_PACKAGE_SIGNATURE_LEN) {
        return -1;
    }

    const u8* bytes = (const u8*)file->base;
    if (loader_memcmp(bytes, k_package_magic, 8u) != 0) {
        return -1;
    }

    const u32 version = read_u32_le(bytes + 8u);
    const u32 manifest_size = read_u32_le(bytes + 12u);
    const u64 payload_size = read_u64_le(bytes + 16u);
    const u32 signature_size = read_u32_le(bytes + 24u);
    if (version != VEX_PACKAGE_FORMAT_VERSION || signature_size != VEX_PACKAGE_SIGNATURE_LEN) {
        return -1;
    }

    const u64 manifest_offset = 92u;
    const u64 payload_offset = manifest_offset + manifest_size;
    const u64 signature_offset = payload_offset + payload_size;
    const u64 expected_size = signature_offset + VEX_PACKAGE_SIGNATURE_LEN;
    if (expected_size != file->size) {
        return -1;
    }

    const u8* manifest = bytes + manifest_offset;
    const u8* payload = bytes + payload_offset;
    const u8* signature = bytes + signature_offset;
    const u8* public_key = bytes + 60u;

    u8 payload_hash[32];
    sha256_bytes(payload, payload_size, payload_hash);
    if (loader_memcmp(payload_hash, bytes + 28u, 32u) != 0) {
        return -1;
    }

    u8 manifest_hash[32];
    sha256_bytes(manifest, manifest_size, manifest_hash);

    u8 signing_message[88];
    loader_memcpy(signing_message, k_signing_context, 8u);
    write_u32_le(signing_message + 8u, version);
    write_u32_le(signing_message + 12u, manifest_size);
    write_u64_le(signing_message + 16u, payload_size);
    loader_memcpy(signing_message + 24u, manifest_hash, 32u);
    loader_memcpy(signing_message + 56u, payload_hash, 32u);

    if (verify_signature(signature, public_key, signing_message, sizeof(signing_message)) != 0) {
        return -1;
    }

    out_package->package_base = bytes;
    out_package->package_size = file->size;
    out_package->manifest_base = manifest;
    out_package->manifest_size = manifest_size;
    out_package->payload_base = payload;
    out_package->payload_size = payload_size;
    loader_memcpy(out_package->payload_hash, payload_hash, 32u);
    loader_memcpy(out_package->public_key, public_key, 32u);
    return 0;
}

int verify_init_package(const vex_loader_file_t* file, vex_loaded_package_t* out_package) {
    return verify_package(file, out_package);
}
