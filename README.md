# tinyOS

`tinyOS` is a custom operating system workspace aimed at a small, incremental bring-up.

## Current State

The repository now contains the current `UEFI-first` `x86_64` baseline: a bootable
`QEMU + OVMF` image, shared kernel services routed through `arch/platform`
boundaries, and a GOP framebuffer demo path:

- a `UEFI` boot path built around `BOOTX64.EFI`, `KERNEL.BIN` and a raw disk image with an ESP layout
- a tiny `x86_64` kernel that receives a `boot_info` handoff with framebuffer and ACPI context
- `QEMU + OVMF` startup scripts and repeatable runtime validation artifacts
- a GOP framebuffer demo that draws a visible `TINYOS / UEFI BOOT` screen
- an early bump allocator carved out from the end of the loaded kernel image
- `arch/x86_64` owned `IDT + PIC + PIT` initialization for exceptions, IRQs and the timer tick
- a minimal tick-driven event loop that gets tick, interrupt and idle behavior from `tinyos_arch_current()`
- retained platform backends for the earlier `x86_64` text/keyboard path while the `UEFI` route is being brought forward
- reserved `arch/platform` interface headers and `ARM64/RISC-V` port placeholders
- a documented second-phase validation standard for `ARM64 virt` and `RISC-V virt`
- repeatable image build, `QEMU + OVMF` launch scripts and real-hardware prep notes

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
|   |-- uefi/
|   |   `-- loader.c
|   `-- x86_64/
|       |-- boot_sector.S
|       `-- stage2.S
|-- docs/
|   |-- boot/
|   |   `-- x86_64-uefi-real-hardware.md
|   |-- porting/
|   |   `-- task6-arch-platform-boundary.md
|   |-- release-0-scope.md
|   `-- validation/
|       `-- task8-validation-baseline.md
|-- include/
|   `-- tinyos/
|       |-- arch.h
|       |-- boot_info.h
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
|   |-- check_task8_baseline.sh
|   |-- build_uefi_disk.py
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
    |   |-- memory.c
    |   `-- uefi_boot_demo.c
    |-- platform/
    |   `-- x86_64/
    |       |-- keyboard.c
    |       |-- platform.c
    |       `-- text_display.c
    `-- lib/
```

## Build Entry

Build the bootable `UEFI` disk image:

```bash
make build
```

Run under `QEMU + OVMF`:

```bash
make run
```

Expected runtime evidence:

```text
tinyOS UEFI loader starting...
LPVEABCDKUS
```

Current visual outcome on the framebuffer:

- a dark background panel with a bright top band
- a centered `TINYOS` title
- a second line reading `UEFI BOOT`

## Current Boundary

- Supported now: `x86_64`, `UEFI`, `QEMU + OVMF`
- Not yet claimed: `legacy BIOS`, `VirtualBox`, `VMware`, real hardware success
- Next hardware step: follow [x86_64-uefi-real-hardware.md](file:///home/cyk/work/tinyOS/docs/boot/x86_64-uefi-real-hardware.md) to prepare a USB image and record a smoke test result
