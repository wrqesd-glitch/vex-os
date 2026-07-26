#include "../include/loader.h"

static const u32 k_sha256_table[64] = {
    0x428A2F98u, 0x71374491u, 0xB5C0FBCFu, 0xE9B5DBA5u, 0x3956C25Bu, 0x59F111F1u, 0x923F82A4u, 0xAB1C5ED5u,
    0xD807AA98u, 0x12835B01u, 0x243185BEu, 0x550C7DC3u, 0x72BE5D74u, 0x80DEB1FEu, 0x9BDC06A7u, 0xC19BF174u,
    0xE49B69C1u, 0xEFBE4786u, 0x0FC19DC6u, 0x240CA1CCu, 0x2DE92C6Fu, 0x4A7484AAu, 0x5CB0A9DCu, 0x76F988DAu,
    0x983E5152u, 0xA831C66Du, 0xB00327C8u, 0xBF597FC7u, 0xC6E00BF3u, 0xD5A79147u, 0x06CA6351u, 0x14292967u,
    0x27B70A85u, 0x2E1B2138u, 0x4D2C6DFCu, 0x53380D13u, 0x650A7354u, 0x766A0ABBu, 0x81C2C92Eu, 0x92722C85u,
    0xA2BFE8A1u, 0xA81A664Bu, 0xC24B8B70u, 0xC76C51A3u, 0xD192E819u, 0xD6990624u, 0xF40E3585u, 0x106AA070u,
    0x19A4C116u, 0x1E376C08u, 0x2748774Cu, 0x34B0BCB5u, 0x391C0CB3u, 0x4ED8AA4Au, 0x5B9CCA4Fu, 0x682E6FF3u,
    0x748F82EEu, 0x78A5636Fu, 0x84C87814u, 0x8CC70208u, 0x90BEFFFau, 0xA4506CEBu, 0xBEF9A3F7u, 0xC67178F2u
};

static u32 rotate_right(u32 value, u32 shift) {
    return (value >> shift) | (value << (32u - shift));
}

static void sha256_transform(vex_sha256_t* context, const u8 block[64]) {
    u32 w[64];
    for (u32 i = 0; i < 16u; ++i) {
        const u32 base = i * 4u;
        w[i] = ((u32)block[base] << 24u) |
               ((u32)block[base + 1u] << 16u) |
               ((u32)block[base + 2u] << 8u) |
               (u32)block[base + 3u];
    }

    for (u32 i = 16u; i < 64u; ++i) {
        const u32 s0 = rotate_right(w[i - 15u], 7u) ^ rotate_right(w[i - 15u], 18u) ^ (w[i - 15u] >> 3u);
        const u32 s1 = rotate_right(w[i - 2u], 17u) ^ rotate_right(w[i - 2u], 19u) ^ (w[i - 2u] >> 10u);
        w[i] = w[i - 16u] + s0 + w[i - 7u] + s1;
    }

    u32 a = context->state[0];
    u32 b = context->state[1];
    u32 c = context->state[2];
    u32 d = context->state[3];
    u32 e = context->state[4];
    u32 f = context->state[5];
    u32 g = context->state[6];
    u32 h = context->state[7];

    for (u32 i = 0; i < 64u; ++i) {
        const u32 sigma1 = rotate_right(e, 6u) ^ rotate_right(e, 11u) ^ rotate_right(e, 25u);
        const u32 choice = (e & f) ^ ((~e) & g);
        const u32 temp1 = h + sigma1 + choice + k_sha256_table[i] + w[i];
        const u32 sigma0 = rotate_right(a, 2u) ^ rotate_right(a, 13u) ^ rotate_right(a, 22u);
        const u32 majority = (a & b) ^ (a & c) ^ (b & c);
        const u32 temp2 = sigma0 + majority;

        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
    context->state[5] += f;
    context->state[6] += g;
    context->state[7] += h;
}

void sha256_init(vex_sha256_t* context) {
    context->state[0] = 0x6A09E667u;
    context->state[1] = 0xBB67AE85u;
    context->state[2] = 0x3C6EF372u;
    context->state[3] = 0xA54FF53Au;
    context->state[4] = 0x510E527Fu;
    context->state[5] = 0x9B05688Cu;
    context->state[6] = 0x1F83D9ABu;
    context->state[7] = 0x5BE0CD19u;
    context->total_size = 0;
    context->buffer_size = 0;
}

void sha256_update(vex_sha256_t* context, const void* bytes, u64 size) {
    const u8* input = (const u8*)bytes;
    context->total_size += size;

    while (size > 0u) {
        const u32 remaining = 64u - context->buffer_size;
        const u32 chunk = (size < remaining) ? (u32)size : remaining;
        loader_memcpy(&context->buffer[context->buffer_size], input, chunk);
        context->buffer_size += chunk;
        input += chunk;
        size -= chunk;

        if (context->buffer_size == 64u) {
            sha256_transform(context, context->buffer);
            context->buffer_size = 0;
        }
    }
}

void sha256_finalize(vex_sha256_t* context, u8 out_hash[32]) {
    const u64 bit_count = context->total_size * 8u;
    context->buffer[context->buffer_size++] = 0x80u;

    if (context->buffer_size > 56u) {
        while (context->buffer_size < 64u) {
            context->buffer[context->buffer_size++] = 0u;
        }
        sha256_transform(context, context->buffer);
        context->buffer_size = 0u;
    }

    while (context->buffer_size < 56u) {
        context->buffer[context->buffer_size++] = 0u;
    }

    for (u32 i = 0; i < 8u; ++i) {
        context->buffer[63u - i] = (u8)(bit_count >> (i * 8u));
    }
    sha256_transform(context, context->buffer);

    for (u32 i = 0; i < 8u; ++i) {
        out_hash[i * 4u] = (u8)(context->state[i] >> 24u);
        out_hash[i * 4u + 1u] = (u8)(context->state[i] >> 16u);
        out_hash[i * 4u + 2u] = (u8)(context->state[i] >> 8u);
        out_hash[i * 4u + 3u] = (u8)context->state[i];
    }
}

void sha256_bytes(const void* bytes, u64 size, u8 out_hash[32]) {
    vex_sha256_t context;
    sha256_init(&context);
    sha256_update(&context, bytes, size);
    sha256_finalize(&context, out_hash);
}
