#include "../include/vex/kernel.h"

void ipc_init(vex_channel_t* channel) {
    channel->head = 0;
    channel->tail = 0;
    channel->count = 0;
}

int ipc_send(vex_channel_t* channel, const vex_process_t* process, const vex_channel_message_t* message) {
    if ((process->capability_mask & VEX_CAP_IPC) == 0u || channel->count >= 16u) {
        return -1;
    }

    channel->ring[channel->tail] = *message;
    channel->tail = (channel->tail + 1u) % 16u;
    ++channel->count;
    return 0;
}

int ipc_receive(vex_channel_t* channel, vex_channel_message_t* out_message) {
    if (channel->count == 0u) {
        return -1;
    }

    *out_message = channel->ring[channel->head];
    channel->head = (channel->head + 1u) % 16u;
    --channel->count;
    return 0;
}
