#ifndef VEX_UI_PROTO_H
#define VEX_UI_PROTO_H

typedef enum vex_window_kind {
    VEX_WINDOW_EXPLORER = 1,
    VEX_WINDOW_DIAGNOSTICS = 2,
    VEX_WINDOW_TESTS = 3,
    VEX_WINDOW_SERVICES = 4,
    VEX_WINDOW_TERMINAL = 5
} vex_window_kind_t;

typedef enum vex_compositor_opcode {
    VEX_COMPOSITOR_NOP = 0,
    VEX_COMPOSITOR_CREATE_WINDOW = 1,
    VEX_COMPOSITOR_DESTROY_WINDOW = 2,
    VEX_COMPOSITOR_PRESENT_SURFACE = 3,
    VEX_COMPOSITOR_SET_TITLE = 4,
    VEX_COMPOSITOR_INPUT_EVENT = 5,
    VEX_COMPOSITOR_BIND_SURFACE = 6,
    VEX_COMPOSITOR_SIGNAL_FENCE = 7
} vex_compositor_opcode_t;

enum {
    VEX_COMPOSITOR_ABI_VERSION = 1u,
    VEX_COMPOSITOR_SCENE_SLOTS = 4u
};

typedef struct vex_surface_descriptor {
    unsigned int width;
    unsigned int height;
    unsigned int stride;
    unsigned int buffer_count;
    unsigned int present_index;
    unsigned int surface_handle;
    unsigned int fence_handle;
    unsigned long long mapping_base;
    unsigned long long bytes_per_buffer;
} vex_surface_descriptor_t;

typedef struct vex_window_descriptor {
    unsigned int window_id;
    unsigned int kind;
    unsigned int x;
    unsigned int y;
    unsigned int width;
    unsigned int height;
    char title[48];
} vex_window_descriptor_t;

typedef struct vex_compositor_packet {
    unsigned int abi_version;
    unsigned int opcode;
    unsigned int flags;
    unsigned int payload_bytes;
    unsigned int sequence;
    vex_window_descriptor_t window;
    vex_surface_descriptor_t surface;
} vex_compositor_packet_t;

typedef struct vex_compositor_mailbox {
    unsigned int magic;
    unsigned int abi_version;
    unsigned int sequence;
    unsigned int acknowledged_sequence;
    vex_compositor_packet_t packet;
} vex_compositor_mailbox_t;

typedef struct vex_compositor_scene_entry {
    unsigned int slot;
    unsigned int visible;
    unsigned int sequence;
    unsigned int acknowledged_sequence;
    unsigned int window_id;
    unsigned int kind;
    unsigned int present_index;
    unsigned int buffer_count;
    unsigned int width;
    unsigned int height;
    unsigned int stride;
    char title[48];
} vex_compositor_scene_entry_t;

typedef struct vex_compositor_scene_mailbox {
    unsigned int magic;
    unsigned int abi_version;
    unsigned int frame_sequence;
    unsigned int entry_count;
    vex_compositor_scene_entry_t entries[VEX_COMPOSITOR_SCENE_SLOTS];
} vex_compositor_scene_mailbox_t;

enum {
    VEX_COMPOSITOR_MAILBOX_MAGIC = 0x58434D42u,
    VEX_COMPOSITOR_SCENE_MAGIC = 0x5643534Eu
};

#endif
