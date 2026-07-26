#include "../include/vex/kernel.h"

static const char* find_token(const char* text, u64 length, const char* token) {
    u64 token_length = 0;
    while (token[token_length] != '\0') {
        ++token_length;
    }

    if (token_length == 0 || token_length > length) {
        return 0;
    }

    for (u64 i = 0; i + token_length <= length; ++i) {
        u64 matched = 0;
        while (matched < token_length && text[i + matched] == token[matched]) {
            ++matched;
        }
        if (matched == token_length) {
            return text + i;
        }
    }
    return 0;
}

static int copy_json_string_value(const char* key, const char* source, u64 length, char* out, u32 out_capacity) {
    const char* key_position = find_token(source, length, key);
    if (key_position == 0) {
        return -1;
    }

    const char* end = source + length;
    const char* cursor = key_position;
    while (cursor < end && *cursor != ':') {
        ++cursor;
    }
    if (cursor == end) {
        return -1;
    }
    ++cursor;

    while (cursor < end && (*cursor == ' ' || *cursor == '\n' || *cursor == '\r' || *cursor == '\t')) {
        ++cursor;
    }
    if (cursor == end || *cursor != '"') {
        return -1;
    }
    ++cursor;

    u32 index = 0;
    while (cursor < end && *cursor != '"') {
        if (index + 1u >= out_capacity) {
            return -1;
        }
        out[index++] = *cursor++;
    }
    if (cursor == end) {
        return -1;
    }

    out[index] = '\0';
    return 0;
}

static int json_array_contains(const char* key, const char* source, u64 length, const char* value) {
    const char* key_position = find_token(source, length, key);
    if (key_position == 0) {
        return 0;
    }

    const char* end = source + length;
    const char* cursor = key_position;
    while (cursor < end && *cursor != '[') {
        ++cursor;
    }
    if (cursor == end) {
        return 0;
    }

    const char* closing = cursor;
    while (closing < end && *closing != ']') {
        ++closing;
    }
    if (closing == end) {
        return 0;
    }

    return find_token(cursor, (u64)(closing - cursor), value) != 0;
}

static int parse_u32_value(const char* key, const char* source, u64 length, u32* out_value) {
    const char* key_position = find_token(source, length, key);
    if (key_position == 0) {
        return -1;
    }

    const char* end = source + length;
    const char* cursor = key_position;
    while (cursor < end && *cursor != ':') {
        ++cursor;
    }
    if (cursor == end) {
        return -1;
    }
    ++cursor;

    while (cursor < end && (*cursor == ' ' || *cursor == '\n' || *cursor == '\r' || *cursor == '\t')) {
        ++cursor;
    }
    if (cursor == end || *cursor < '0' || *cursor > '9') {
        return -1;
    }

    u32 value = 0;
    while (cursor < end && *cursor >= '0' && *cursor <= '9') {
        value = value * 10u + (u32)(*cursor - '0');
        ++cursor;
    }
    *out_value = value;
    return 0;
}

int manifest_parse_package_image(const vex_package_image_info_t* image, vex_manifest_view_t* out_manifest) {
    const char* manifest;
    const u64 length = image->manifest_size;

    if (image == 0 || out_manifest == 0 || image->verified == 0u || image->manifest_base == 0u || length == 0u) {
        return -1;
    }
    manifest = (const char*)(usize)image->manifest_base;

    if (copy_json_string_value("\"name\"", manifest, length, out_manifest->name, sizeof(out_manifest->name)) != 0) {
        return -1;
    }
    if (copy_json_string_value("\"version\"", manifest, length, out_manifest->version, sizeof(out_manifest->version)) != 0) {
        return -1;
    }
    if (copy_json_string_value("\"entrypoint\"", manifest, length, out_manifest->entrypoint, sizeof(out_manifest->entrypoint)) != 0) {
        return -1;
    }
    if (parse_u32_value("\"abi_version\"", manifest, length, &out_manifest->abi_version) != 0) {
        return -1;
    }

    out_manifest->capability_mask = 0u;
    if (json_array_contains("\"permissions\"", manifest, length, "\"log\"")) {
        out_manifest->capability_mask |= VEX_CAP_LOG;
    }
    if (json_array_contains("\"permissions\"", manifest, length, "\"ipc\"")) {
        out_manifest->capability_mask |= VEX_CAP_IPC;
    }
    if (json_array_contains("\"required_capabilities\"", manifest, length, "\"service.locate\"")) {
        out_manifest->capability_mask |= VEX_CAP_SERVICE_LOCATE;
    }
    if (json_array_contains("\"required_capabilities\"", manifest, length, "\"channel.open\"")) {
        out_manifest->capability_mask |= VEX_CAP_PACKAGE_EXEC;
    }
    if (json_array_contains("\"permissions\"", manifest, length, "\"graphics\"")) {
        out_manifest->capability_mask |= VEX_CAP_GRAPHICS;
    }
    if (json_array_contains("\"required_capabilities\"", manifest, length, "\"gpu.device\"")) {
        out_manifest->capability_mask |= VEX_CAP_GPU;
    }

    return 0;
}

int manifest_parse_init(const vex_boot_info_t* boot_info, vex_manifest_view_t* out_manifest) {
    if (boot_info == 0) {
        return -1;
    }
    return manifest_parse_package_image(&boot_info->init_image, out_manifest);
}
