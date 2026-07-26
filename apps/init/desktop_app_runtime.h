#ifndef VEX_INIT_DESKTOP_APP_RUNTIME_H
#define VEX_INIT_DESKTOP_APP_RUNTIME_H

#include "desktop_animation.h"
#include "desktop_domain.h"
#include "desktop_input.h"
#include "vex_boot_info.h"

#define VEX_EXPLORER_ROOT_MAX 16u
#define VEX_EXPLORER_PATH_MAX 96u
#define VEX_LINE_MAX 48u

#define VIEW_COUNT 6u

typedef struct desktop_app_state {
    u32 running;
    u32 session_id;
    u32 launch_count;
    u32 uptime_ticks;
    u32 heartbeat;
    u32 meter_primary;
    u32 meter_secondary;
    u32 meter_tertiary;
    u32 refresh_divider;
    u32 terminal_cursor;
    u32 terminal_history_count;
    u32 browser_cursor;
    u32 diagnostics_page;
    u32 test_cursor;
    u32 test_run_count;
    u32 test_pass_count;
    u32 test_armed;
    u32 test_armed_ticks;
    u32 test_demo_active;
    u32 test_demo_ticks;
    u32 gpu_suspend_requested;
    u32 gpu_suspend_sequence;
    u32 gpu_suspend_applied_sequence;
    char headline[32];
    char detail[48];
    char lines[6][48];
    char test_list[6][48];
    char test_result[2][48];
    char terminal_input[40];
    char terminal_history[8][48];
    char browser_prefix[96];
    u32 explorer_selected_index;
    u32 explorer_left_selected;
    u32 explorer_focus_left;
    char explorer_current_path[96];
} desktop_app_state_t;

void desktop_app_runtime_init(void);
void desktop_app_runtime_sync_view(
    shell_view_t view,
    u32 engaged,
    const desktop_domain_descriptor_t* descriptor
);
u32 desktop_app_runtime_tick(
    const vex_boot_info_t* boot_info,
    const desktop_animation_state_t* animation
);
u32 desktop_app_runtime_handle_key(shell_view_t view, ui_key_t key, const vex_boot_info_t* boot_info);
u32 desktop_app_runtime_handle_char(shell_view_t view, char ch);
const desktop_app_state_t* desktop_app_runtime_state(shell_view_t view);

const vex_boot_info_t* desktop_app_runtime_boot_info(void);
u32 explorer_visible_count(const vex_boot_info_t* boot_info, const desktop_app_state_t* state);
const vex_boot_file_entry_t* explorer_entry_at(const vex_boot_info_t* boot_info, const desktop_app_state_t* state, u32 index);
void explorer_root_dirs(const vex_boot_info_t* boot_info, char roots[][VEX_EXPLORER_PATH_MAX], u32* count);

#endif
