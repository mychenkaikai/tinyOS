# riscv64 Port Placeholder

This directory reserves the architecture-owned code required by the future
`RISC-V virt` bring-up.

## Owned By `arch/riscv64`

- CPU entry and trap vector assembly
- Privilege-mode setup and early page table bootstrap if an MMU path is used
- CLINT or SBI timer hookup and interrupt dispatch wiring
- Architecture-specific linker script and CSR helpers

## Must Reuse

- `src/kernel/memory.c`
- `src/kernel/event_loop.c`
- Most of `src/kernel/main.c` after boot strings and idle calls are routed
  through `tinyos_arch_current()` and `tinyos_platform_current()`

## Expected Future Files

- `kernel_entry.S`
- `trap_vector.S`
- `linker.ld`
- `interrupts_riscv64.c`
