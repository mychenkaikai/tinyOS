#include "tinyos/arch.h"

#include "interrupts.h"

static void x86_64_arch_early_init(void) {
}

static void x86_64_arch_cpu_idle(void) {
    __asm__ volatile ("hlt");
}

static const struct tinyos_arch_ops g_x86_64_arch_ops = {
    .name = "x86_64",
    .early_init = x86_64_arch_early_init,
    .interrupts_init = x86_64_interrupts_init,
    .interrupts_enable = x86_64_interrupts_enable,
    .interrupts_disable = x86_64_interrupts_disable,
    .timer_ticks = x86_64_interrupts_ticks,
    .interrupts_ready = x86_64_interrupts_ready,
    .cpu_idle = x86_64_arch_cpu_idle
};

const struct tinyos_arch_ops *tinyos_arch_current(void) {
    return &g_x86_64_arch_ops;
}
