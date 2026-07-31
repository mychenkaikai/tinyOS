# RISC-V Virt Platform

This folder now owns the machine-specific pieces for the prepared serial-only
`QEMU virt` based `riscv64` bring-up.

## Expected Ownership

- SBI or firmware handoff assumptions
- UART, timer and interrupt controller base discovery for the `virt` machine
- Device tree parsing or fixed machine map used during early bring-up
- Framebuffer and input probing that should not leak into generic kernel code

## Reuse Goal

The reusable kernel should only see the `tinyos/platform.h` surface.
Machine addresses, UART choices and `virt` quirks stay in this directory.

## Current Files

- `src/platform/riscv64/platform.c`: current UART-backed console path, boot
  heap limit, and headless platform registration

## Next Step

Keep the machine map here, then add real timer and trap-controller discovery
without pushing `virt`-specific details into the shared kernel.
