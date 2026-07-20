#ifndef TINYOS_ARCH_X86_64_INTERRUPTS_H
#define TINYOS_ARCH_X86_64_INTERRUPTS_H

#include <stdbool.h>
#include <stdint.h>

void x86_64_interrupts_init(uint32_t tick_hz);
void x86_64_interrupts_enable(void);
void x86_64_interrupts_disable(void);
uint64_t x86_64_interrupts_ticks(void);
bool x86_64_interrupts_ready(void);

#endif
