#ifndef TINYOS_ARCH_H
#define TINYOS_ARCH_H

#include <stdbool.h>
#include <stdint.h>

struct tinyos_arch_ops {
    const char *name;

    /*
     * Brings up CPU-local state required before the generic kernel services run.
     * Examples: descriptor tables, trap vectors, exception delegation.
     */
    void (*early_init)(void);

    /*
     * Sets up the architecture interrupt controller and the periodic timer
     * source used by the generic event loop.
     */
    void (*interrupts_init)(uint32_t tick_hz);
    void (*interrupts_enable)(void);
    void (*interrupts_disable)(void);
    uint64_t (*timer_ticks)(void);
    bool (*interrupts_ready)(void);

    /*
     * Places the current CPU in a low-power wait-for-interrupt style state.
     * The generic kernel can use this instead of embedding x86_64 hlt.
     */
    void (*cpu_idle)(void);
};

const struct tinyos_arch_ops *tinyos_arch_current(void);

#endif
