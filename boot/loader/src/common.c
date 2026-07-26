#include "../include/loader.h"

void* loader_memcpy(void* destination, const void* source, u64 size) {
    u8* out = (u8*)destination;
    const u8* in = (const u8*)source;
    for (u64 i = 0; i < size; ++i) {
        out[i] = in[i];
    }
    return destination;
}

void* loader_memset(void* destination, int value, u64 size) {
    u8* out = (u8*)destination;
    for (u64 i = 0; i < size; ++i) {
        out[i] = (u8)value;
    }
    return destination;
}

int loader_memcmp(const void* left, const void* right, u64 size) {
    const u8* a = (const u8*)left;
    const u8* b = (const u8*)right;
    for (u64 i = 0; i < size; ++i) {
        if (a[i] != b[i]) {
            return (int)a[i] - (int)b[i];
        }
    }
    return 0;
}

u64 loader_align_up(u64 value, u64 alignment) {
    return (value + alignment - 1u) & ~(alignment - 1u);
}

u64 loader_align_down(u64 value, u64 alignment) {
    return value & ~(alignment - 1u);
}
