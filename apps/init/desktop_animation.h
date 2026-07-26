#ifndef VEX_INIT_DESKTOP_ANIMATION_H
#define VEX_INIT_DESKTOP_ANIMATION_H

typedef unsigned int u32;

typedef struct desktop_animation_state {
    u32 frame;
    u32 loop_divider;
    u32 pulse_fast;
    u32 pulse_slow;
    u32 cursor_pulse;
    u32 sweep;
} desktop_animation_state_t;

void desktop_animation_init(desktop_animation_state_t* state);
u32 desktop_animation_tick(desktop_animation_state_t* state);

#endif
