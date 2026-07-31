# riscv64 Port

This directory now contains the first architecture-owned pieces for the
serial-only `RISC-V virt` bring-up.

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

## Current Files

- `arch.c`: current fake-tick architecture backend used to validate the shared
  event loop before a real timer driver lands
- `kernel_entry.S`: bare-metal entry, stack setup and `.bss` clear path
- `linker.ld`: fixed `virt` RAM placement for the serial bring-up image

## Next Step

Replace the fake tick path with a real timer and trap backend while keeping
`src/kernel/memory.c`, `src/kernel/event_loop.c` and `src/kernel/main.c`
shared.
