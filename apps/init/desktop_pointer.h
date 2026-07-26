#ifndef VEX_INIT_DESKTOP_POINTER_H
#define VEX_INIT_DESKTOP_POINTER_H

#include "desktop_session.h"

typedef unsigned char u8;

typedef u32 (*desktop_pointer_hit_test_fn)(u32 x, u32 y);
typedef void (*desktop_pointer_view_handler_t)(shell_view_t view);
typedef void (*desktop_pointer_index_handler_t)(u32 index);
typedef void (*desktop_pointer_slot_handler_t)(u32 slot);
typedef u32 (*desktop_pointer_dimension_fn)(void);

typedef struct desktop_pointer_routes {
    desktop_pointer_hit_test_fn desktop_icon_hit;
    desktop_pointer_hit_test_fn taskbar_hit;
    desktop_pointer_hit_test_fn start_item_hit;
    desktop_pointer_hit_test_fn hub_card_hit;
    desktop_pointer_hit_test_fn window_slot_hit;
    desktop_pointer_hit_test_fn start_menu_hit;
    desktop_pointer_hit_test_fn window_close_hit;
    desktop_pointer_hit_test_fn window_minimize_hit;
    desktop_pointer_hit_test_fn window_title_hit;
    desktop_pointer_view_handler_t open_view;
    desktop_pointer_index_handler_t activate_desktop_entry;
    desktop_pointer_index_handler_t activate_start_entry;
    desktop_pointer_index_handler_t activate_hub_card;
    desktop_pointer_slot_handler_t activate_window_slot;
    desktop_pointer_dimension_fn framebuffer_width;
    desktop_pointer_dimension_fn framebuffer_height;
} desktop_pointer_routes_t;

u32 desktop_pointer_is_hot(
    const shell_state_t* state,
    const shell_window_t* windows,
    u32 window_count,
    const desktop_pointer_routes_t* routes
);
void desktop_pointer_handle_click(
    shell_state_t* state,
    shell_window_t* windows,
    u32* order,
    u32 window_count,
    const desktop_pointer_routes_t* routes
);
void desktop_pointer_handle_packet(
    shell_state_t* state,
    shell_window_t* windows,
    u32* order,
    u32 window_count,
    const u8 packet[3],
    const desktop_pointer_routes_t* routes
);

#endif
