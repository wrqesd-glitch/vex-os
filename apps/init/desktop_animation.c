#include "desktop_animation.h"

static u32 ping_pong(u32 value, u32 period) {
    const u32 wrapped = value % (period * 2u);
    return wrapped < period ? wrapped : (period * 2u - 1u - wrapped);
}

void desktop_animation_init(desktop_animation_state_t* state) {
    state->frame = 0u;
    state->loop_divider = 0u;
    state->pulse_fast = 0u;
    state->pulse_slow = 0u;
    state->cursor_pulse = 0u;
    state->sweep = 0u;
}

u32 desktop_animation_tick(desktop_animation_state_t* state) {
    state->loop_divider += 1u;
    if (state->loop_divider < 6000u) {
        return 0u;
    }
    state->loop_divider = 0u;
    state->frame += 1u;
    state->pulse_fast = ping_pong(state->frame, 18u);
    state->pulse_slow = ping_pong(state->frame, 36u);
    state->cursor_pulse = ping_pong(state->frame, 12u);
    state->sweep = state->frame % 96u;
    return 1u;
}
