#ifndef VEX_GPU_PROTO_H
#define VEX_GPU_PROTO_H

#include "vex_ui_proto.h"

typedef enum vex_gpu_backend_kind {
    VEX_GPU_BACKEND_NONE = 0,
    VEX_GPU_BACKEND_BOOT_FB = 1,
    VEX_GPU_BACKEND_VULKAN_STUB = 2,
    VEX_GPU_BACKEND_MESA_STUB = 3
} vex_gpu_backend_kind_t;

enum {
    VEX_GPU_ABI_VERSION = 1u,
    VEX_GPU_SURFACE_SLOTS = VEX_COMPOSITOR_SCENE_SLOTS
};

enum {
    VEX_GPU_FEATURE_TRIPLE_BUFFER = 0x00000001u,
    VEX_GPU_FEATURE_PRESENT_FENCE = 0x00000002u,
    VEX_GPU_FEATURE_SHARED_SURFACE_IMPORT = 0x00000004u,
    VEX_GPU_FEATURE_SCENE_IMPORT = 0x00000008u,
    VEX_GPU_FEATURE_COMPOSITOR_LINK = 0x00000010u,
    VEX_GPU_FEATURE_EXPLICIT_SYNC_STUB = 0x00000020u
};

typedef struct vex_gpu_request {
    unsigned int desired_backend;
    unsigned int requested_features;
    unsigned int scene_sequence;
    unsigned int reserved0;
} vex_gpu_request_t;

typedef struct vex_gpu_control {
    unsigned int command;
    unsigned int flags;
    unsigned int sequence;
    unsigned int completed_sequence;
} vex_gpu_control_t;

typedef struct vex_gpu_status {
    unsigned int selected_backend;
    unsigned int active_features;
    unsigned int ready;
    unsigned int imported_surface_count;
    unsigned int visible_window_count;
    unsigned int scene_sequence;
    unsigned int last_present_sequence;
    unsigned int last_completed_sequence;
    unsigned int last_fence_value;
    unsigned int heartbeat;
} vex_gpu_status_t;

typedef struct vex_gpu_surface_status {
    unsigned int slot;
    unsigned int visible;
    unsigned int present_index;
    unsigned int buffer_count;
    unsigned int width;
    unsigned int height;
    unsigned int present_sequence;
    unsigned int fence_value;
} vex_gpu_surface_status_t;

typedef struct vex_gpu_mailbox {
    unsigned int magic;
    unsigned int abi_version;
    unsigned int request_sequence;
    unsigned int completed_request_sequence;
    vex_gpu_request_t request;
    vex_gpu_control_t control;
    vex_gpu_status_t status;
    vex_gpu_surface_status_t surfaces[VEX_GPU_SURFACE_SLOTS];
} vex_gpu_mailbox_t;

enum {
    VEX_GPU_MAILBOX_MAGIC = 0x56475055u
};

enum {
    VEX_GPU_CONTROL_NOP = 0u,
    VEX_GPU_CONTROL_SUSPEND = 1u,
    VEX_GPU_CONTROL_RESUME = 2u
};

#endif
