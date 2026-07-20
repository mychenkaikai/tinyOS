# RISC-V Virt Platform Placeholder

This folder reserves the board or machine specific pieces for the future
`QEMU virt` based `riscv64` port.

## Expected Ownership

- SBI or firmware handoff assumptions
- UART, timer and interrupt controller base discovery for the `virt` machine
- Device tree parsing or fixed machine map used during early bring-up
- Framebuffer and input probing that should not leak into generic kernel code

## Reuse Goal

The reusable kernel should only see the `tinyos/platform.h` surface.
Machine addresses, UART choices and `virt` quirks stay in this directory.
