# aarch64 Port

This directory now contains the first architecture-owned pieces for the
serial-only `ARM64 virt` bring-up.

## Owned By `arch/aarch64`

- CPU entry and exception vector assembly
- MMU or page table bootstrap that is specific to `aarch64`
- Generic interrupt and timer backend wiring for the chosen ARM virtual machine
- Architecture-specific linker script and low-level register helpers

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

Replace the fake tick path with a real generic timer interrupt backend while
keeping `src/kernel/memory.c`, `src/kernel/event_loop.c` and `src/kernel/main.c`
shared.
