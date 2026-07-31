#include "tinyos/arch.h"

#include <stdbool.h>
#include <stdint.h>

static volatile uint64_t g_aarch64_ticks = 0u;
static bool g_aarch64_interrupts_ready = false;

static void aarch64_arch_early_init(void) {
}

static void aarch64_interrupts_init(uint32_t tick_hz) {
    (void)tick_hz;
    g_aarch64_ticks = 0u;
    g_aarch64_interrupts_ready = true;
}

static void aarch64_interrupts_enable(void) {
}

static void aarch64_interrupts_disable(void) {
}

static uint64_t aarch64_timer_ticks(void) {
    return g_aarch64_ticks;
}

static bool aarch64_interrupts_ready(void) {
    return g_aarch64_interrupts_ready;
}

static void aarch64_cpu_idle(void) {
    ++g_aarch64_ticks;
}

static const struct tinyos_arch_ops g_aarch64_arch_ops = {
    .name = "aarch64",
    .early_init = aarch64_arch_early_init,
    .interrupts_init = aarch64_interrupts_init,
    .interrupts_enable = aarch64_interrupts_enable,
    .interrupts_disable = aarch64_interrupts_disable,
    .timer_ticks = aarch64_timer_ticks,
    .interrupts_ready = aarch64_interrupts_ready,
    .cpu_idle = aarch64_cpu_idle
};

const struct tinyos_arch_ops *tinyos_arch_current(void) {
    return &g_aarch64_arch_ops;
}
