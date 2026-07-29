# Release 0 Scope

## First Release Goal

The first release is a bring-up milestone, not a feature-complete operating
system.

Primary goal:

- boot a freestanding `x86_64` kernel image under `QEMU + OVMF` from a reproducible repository
- keep the generic kernel core small while routing architecture and machine details through `arch/` and `platform/`
- demonstrate the minimum visible system path: `UEFI` boot, kernel handoff, memory, timer, event loop and GOP framebuffer `LVGL` UI
- preserve clear extension points for future `aarch64` and `riscv64` ports without rewriting the shared kernel

## In Scope

- `UEFI` loader, `BOOTX64.EFI` packaging, ESP layout and `x86_64` kernel hand-off
- freestanding kernel entry, early serial / `debugcon` logging and bump allocation
- `arch/x86_64` owned interrupt and timer backend (`IDT + PIC + PIT`)
- shared kernel event loop driven by `tinyos_arch_current()`
- platform-agnostic display abstractions with a `UEFI` GOP GUI path and retained `x86_64` text/input backends
- a framebuffer `LVGL` UI that renders a visible `TINYOS` dashboard with multiple pages
- build, run and baseline verification scripts for `x86_64 + QEMU + OVMF`
- reserved `arch/aarch64`, `arch/riscv64`, `platform/aarch64_virt` and `platform/riscv64_virt` bring-up placeholders

## Out Of Scope

- protected user space, processes, virtual memory or syscall ABI
- filesystem, networking, storage stack or package/runtime services
- SMP, advanced scheduler policies, power management or drivers beyond the current GUI bring-up path
- verified real hardware portability beyond the current `QEMU + OVMF` reference target
- `legacy BIOS` compatibility
- completed `aarch64` or `riscv64` implementations in this release
- full graphics stack features beyond the current software-rendered `LVGL` MVP

## Constraints

- prefer the smallest implementation that still provides visible bring-up evidence
- keep architecture-specific code under `boot/` and `arch/`, and machine-specific code under `platform/`
- avoid ISA-specific instructions or x86 port I/O in the shared kernel core
- treat `x86_64 + QEMU + OVMF` as the current reference target and generalize only through validated abstractions
