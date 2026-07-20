# ARM64 Virt Platform Placeholder

This folder reserves the board or machine specific pieces for the future
`QEMU virt` based `aarch64` port.

## Expected Ownership

- PL011 or equivalent UART selection
- Flattened device tree parsing or fixed QEMU `virt` addresses used at bring-up
- GIC, timer and framebuffer device discovery that is machine specific
- Boot protocol notes for the chosen loader path

## Reuse Goal

Only the machine discovery and device base addresses should live here.
Kernel memory management, event loop logic and most service code must stay
shared with the `x86_64` baseline.
