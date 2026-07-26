#include "../include/vex/kernel.h"

static vex_thread_t g_threads[16];
static u32 g_thread_slots_used;
static vex_thread_t* g_ready_head;
static vex_thread_t* g_ready_tail;
static vex_thread_t* g_current;
static u32 g_ready_count;

void scheduler_init(void) {
    g_thread_slots_used = 0;
    g_ready_head = 0;
    g_ready_tail = 0;
    g_current = 0;
    g_ready_count = 0;
}

vex_thread_t* scheduler_create_thread(u32 tid, u32 priority, u32 slice_ticks) {
    if (g_thread_slots_used >= 16u) {
        return 0;
    }

    vex_thread_t* thread = &g_threads[g_thread_slots_used++];
    thread->tid = tid;
    thread->priority = priority;
    thread->time_slice_ticks = slice_ticks;
    thread->remaining_ticks = slice_ticks;
    thread->next = 0;
    return thread;
}

void scheduler_enqueue(vex_thread_t* thread) {
    thread->next = 0;
    if (g_ready_tail == 0) {
        g_ready_head = thread;
        g_ready_tail = thread;
    } else {
        g_ready_tail->next = thread;
        g_ready_tail = thread;
    }
    ++g_ready_count;
}

vex_thread_t* scheduler_tick(void) {
    if (g_current == 0) {
        g_current = g_ready_head;
        if (g_current == 0) {
            return 0;
        }
    }

    if (g_current->remaining_ticks > 0u) {
        --g_current->remaining_ticks;
    }

    if (g_current->remaining_ticks == 0u && g_ready_head != 0 && g_ready_head->next != 0) {
        vex_thread_t* completed = g_ready_head;
        g_ready_head = completed->next;
        g_ready_tail->next = completed;
        g_ready_tail = completed;
        g_ready_tail->next = 0;
        completed->remaining_ticks = completed->time_slice_ticks;
        g_current = g_ready_head;
    }

    return g_current;
}

u32 scheduler_ready_count(void) {
    return g_ready_count;
}
