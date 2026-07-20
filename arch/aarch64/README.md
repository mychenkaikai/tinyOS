# aarch64 Port Placeholder

This directory reserves the architecture-owned code required by the future
`ARM64 virt` bring-up.

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

## Expected Future Files

- `kernel_entry.S`
- `exceptions.S`
- `linker.ld`
- `interrupts_aarch64.c`
