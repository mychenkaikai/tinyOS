# Release 0 Scope

## First Release Goal

The first release is a bring-up milestone, not a feature-complete operating
system.

Primary goal:

- boot a freestanding `x86_64` kernel image under QEMU from a reproducible repository
- keep the generic kernel core small while routing architecture and machine details through `arch/` and `platform/`
- demonstrate the minimum visible system path: boot, console, memory, timer, event loop, display, input and GUI MVP
- preserve clear extension points for future `aarch64` and `riscv64` ports without rewriting the shared kernel

## In Scope

- BIOS boot sector, second-stage loader and `x86_64` long mode hand-off
- freestanding kernel entry, early serial/text logging and bump allocation
- `arch/x86_64` owned interrupt and timer backend (`IDT + PIC + PIT`)
- shared kernel event loop driven by `tinyos_arch_current()`
- platform-agnostic display and input abstractions with `x86_64` VGA text and PS/2 backends
- text-mode GUI MVP that renders status, reacts to keyboard input and coexists with serial logs
- build, run and baseline verification scripts for `x86_64 + QEMU`
- reserved `arch/aarch64`, `arch/riscv64`, `platform/aarch64_virt` and `platform/riscv64_virt` bring-up placeholders

## Out Of Scope

- protected user space, processes, virtual memory or syscall ABI
- filesystem, networking, storage stack or package/runtime services
- SMP, advanced scheduler policies, power management or drivers beyond the current demo path
- real hardware portability beyond the current `x86_64 + QEMU` reference target
- completed `aarch64` or `riscv64` implementations in this release
- full graphics stack or external GUI runtime integration such as LVGL

## Constraints

- prefer the smallest implementation that still provides visible bring-up evidence
- keep architecture-specific code under `boot/` and `arch/`, and machine-specific code under `platform/`
- avoid ISA-specific instructions or x86 port I/O in the shared kernel core
- treat `x86_64 + QEMU` as the reference target and generalize only through validated abstractions
