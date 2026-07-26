#include "../../include/vex/kernel.h"

typedef struct hpet_register_block {
    u64 general_capabilities;
    u64 reserved0;
    u64 general_configuration;
    u64 reserved1;
    u64 general_interrupt_status;
    u64 reserved2[25];
    u64 main_counter_value;
} hpet_register_block_t;

static vex_timer_state_t g_timer_state;

void timer_init(void) {
    g_timer_state.counter_base = 0u;
    g_timer_state.counter_period_femtoseconds = 0u;
    g_timer_state.source_kind = 0u;
    g_timer_state.available = 0u;

    const vex_acpi_state_t* acpi = acpi_state();
    if (acpi->hpet_present == 0u || acpi->hpet_mmio_base == 0u) {
        return;
    }

    volatile hpet_register_block_t* hpet = (volatile hpet_register_block_t*)(usize)acpi->hpet_mmio_base;
    const u64 caps = hpet->general_capabilities;
    g_timer_state.counter_base = acpi->hpet_mmio_base;
    g_timer_state.counter_period_femtoseconds = caps >> 32u;
    g_timer_state.source_kind = 1u;
    g_timer_state.available = g_timer_state.counter_period_femtoseconds != 0u ? 1u : 0u;
}

const vex_timer_state_t* timer_state(void) {
    return &g_timer_state;
}
