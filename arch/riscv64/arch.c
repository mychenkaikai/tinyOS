#include "tinyos/arch.h"

#include <stdbool.h>
#include <stdint.h>

static volatile uint64_t g_riscv64_ticks = 0u;
static bool g_riscv64_interrupts_ready = false;

static void riscv64_arch_early_init(void) {
}

static void riscv64_interrupts_init(uint32_t tick_hz) {
    (void)tick_hz;
    g_riscv64_ticks = 0u;
    g_riscv64_interrupts_ready = true;
}

static void riscv64_interrupts_enable(void) {
}

static void riscv64_interrupts_disable(void) {
}

static uint64_t riscv64_timer_ticks(void) {
    return g_riscv64_ticks;
}

static bool riscv64_interrupts_ready(void) {
    return g_riscv64_interrupts_ready;
}

static void riscv64_cpu_idle(void) {
    ++g_riscv64_ticks;
}

static const struct tinyos_arch_ops g_riscv64_arch_ops = {
    .name = "riscv64",
    .early_init = riscv64_arch_early_init,
    .interrupts_init = riscv64_interrupts_init,
    .interrupts_enable = riscv64_interrupts_enable,
    .interrupts_disable = riscv64_interrupts_disable,
    .timer_ticks = riscv64_timer_ticks,
    .interrupts_ready = riscv64_interrupts_ready,
    .cpu_idle = riscv64_cpu_idle
};

const struct tinyos_arch_ops *tinyos_arch_current(void) {
    return &g_riscv64_arch_ops;
}
