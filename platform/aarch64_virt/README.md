# ARM64 Virt Platform

This folder now owns the machine-specific pieces for the prepared serial-only
`QEMU virt` based `aarch64` bring-up.

## Expected Ownership

- PL011 or equivalent UART selection
- Flattened device tree parsing or fixed QEMU `virt` addresses used at bring-up
- GIC, timer and framebuffer device discovery that is machine specific
- Boot protocol notes for the chosen loader path

## Reuse Goal

Only the machine discovery and device base addresses should live here.
Kernel memory management, event loop logic and most service code must stay
shared with the `x86_64` baseline.

## Current Files

- `src/platform/aarch64/platform.c`: current PL011-backed console path, boot
  heap limit, and headless platform registration

## Next Step

Keep the UART path here, then add real timer, interrupt-controller, and
optional framebuffer discovery without leaking any `virt` addresses into the
shared kernel.
