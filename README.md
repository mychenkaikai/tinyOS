# tinyOS

`tinyOS` is a custom operating system workspace aimed at a small, incremental bring-up.

## Current State

The repository now contains the current `Task9` baseline: a bootable
`x86_64 + QEMU` image, shared kernel services routed through `arch/platform`
boundaries, and a text-mode GUI MVP:

- BIOS boot sector that loads a second-stage loader from disk
- second-stage loader that reads the freestanding kernel, enables long mode and jumps into the kernel entry
- a tiny `x86_64` kernel that logs to `COM1` and VGA text mode
- platform-agnostic display and input interfaces with `x86_64` VGA text and PS/2 keyboard backends
- positioned text drawing primitives used by a lightweight text-mode GUI MVP
- an early bump allocator carved out from the end of the loaded kernel image
- `arch/x86_64` owned `IDT + PIC + PIT` initialization for exceptions, IRQs and the timer tick
- a minimal tick-driven event loop that gets tick, interrupt and idle behavior from `tinyos_arch_current()`
- a keyboard-driven GUI demo with focus switching, page toggling and editable input
- reserved `arch/platform` interface headers and `ARM64/RISC-V` port placeholders
- a documented second-phase validation standard for `ARM64 virt` and `RISC-V virt`
- repeatable image build and QEMU launch scripts

## Repository Layout

```text
.
|-- Makefile
|-- README.md
|-- arch/
|   |-- aarch64/
|   |   `-- README.md
|   |-- riscv64/
|   |   `-- README.md
|   `-- x86_64/
|       |-- arch.c
|       |-- interrupts.c
|       |-- interrupts.h
|       |-- interrupt_stubs.S
|       |-- kernel_entry.S
|       `-- linker.ld
|-- boot/
|   `-- x86_64/
|       |-- boot_sector.S
|       `-- stage2.S
|-- docs/
|   |-- porting/
|   |   `-- task6-arch-platform-boundary.md
|   `-- release-0-scope.md
|-- include/
|   `-- tinyos/
|       |-- arch.h
|       |-- console.h
|       |-- display.h
|       |-- event_loop.h
|       |-- gui.h
|       |-- input.h
|       |-- memory.h
|       |-- platform.h
|       `-- port_io.h
|-- platform/
|   |-- aarch64_virt/
|   |   `-- README.md
|   |-- riscv64_virt/
|   |   `-- README.md
|   `-- x86_64_qemu/
|       `-- README.md
|-- scripts/
|   |-- build_x86_64.sh
|   `-- run_qemu_x86_64.sh
`-- src/
    |-- kernel/
    |   |-- console.c
    |   |-- display.c
    |   |-- event_loop.c
    |   |-- input.c
    |   |-- gui.c
    |   |-- main.c
    |   `-- memory.c
    |-- platform/
    |   `-- x86_64/
    |       |-- keyboard.c
    |       |-- platform.c
    |       `-- text_display.c
    `-- lib/
```

## Build Entry

Build the bootable disk image:

```bash
make build
```

Run under QEMU:

```bash
make run
```

Expected boot log:

```text
tinyOS x86_64 bootstrap ready.
Phase: Task5 text-mode GUI MVP ready.
Boot path: BIOS -> stage2 -> long mode -> kernel_main.
Platform: x86_64-qemu
Display: platform-agnostic interface -> x86_64 VGA text backend + positioned draw ops.
Input: platform-agnostic interface -> x86_64 PS/2 keyboard backend.
Early heap: start=0x...
Interrupts: arch backend ready=yes
Event loop: heartbeat, input and gui tasks armed, enabling interrupts.
[gui] Task5 MVP active. Screen owned by GUI, logs continue on serial.
[event] heartbeat=1 ticks=100 heap_used=...
[input] key#=1 scancode=0x000000000000001E char=a
```

On the VGA screen, the boot log is replaced by a lightweight text-mode GUI with:

- a header showing current page, focused control and timer ticks
- keyboard navigation via `Tab`, `Enter`, printable keys and `Backspace`
- an editable demo input box
- a `Home` page and a `Settings` page driven by the same input queue
