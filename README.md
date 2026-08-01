# tinyOS

`tinyOS` is a custom operating system workspace aimed at a small, incremental bring-up.

## Current State

The repository now contains the current `UEFI-first` `x86_64` baseline: a bootable
`QEMU + OVMF` image, shared kernel services routed through `arch/platform`
boundaries, and a GOP framebuffer `LVGL` GUI path:

- a `UEFI` boot path built around `BOOTX64.EFI`, `KERNEL.BIN` and a raw disk image with an ESP layout
- a tiny `x86_64` kernel that receives a `boot_info` handoff with framebuffer and ACPI context
- `QEMU + OVMF` startup scripts and repeatable runtime validation artifacts
- a GOP framebuffer `LVGL` UI that draws a visible `TINYOS` dashboard with multiple pages
- an early bump allocator carved out from the end of the loaded kernel image
- `arch/x86_64` owned `IDT + PIC + PIT` initialization for exceptions, IRQs and the timer tick
- a minimal tick-driven event loop that gets tick, interrupt and idle behavior from `tinyos_arch_current()`
- retained platform backends for the earlier `x86_64` text/keyboard path while the `UEFI` route is being brought forward
- reserved `arch/platform` interface headers and `ARM64/RISC-V` port placeholders
- a documented second-phase validation standard for `ARM64 virt` and `RISC-V virt`
- prepared `aarch64` and `riscv64` serial-only `QEMU virt` bring-up targets that reuse the shared kernel event loop
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
|   |   |-- x86_64-uefi-virtualbox.md
|   |   `-- x86_64-uefi-real-hardware.md
|   |-- porting/
|   |   `-- task6-arch-platform-boundary.md
|   |-- release-0-scope.md
|   `-- validation/
|       |-- task8-validation-baseline.md
|       |-- x86_64-uefi-boot-status.md
|       |-- x86_64-uefi-hardware-smoke-template.md
|       |-- x86_64-uefi-virtualbox-2026-07-23.md
|       `-- x86_64-uefi-virtualbox-smoke-template.md
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
|   |-- check_uefi_image_layout.py
|   |-- build_virtualbox_disk.sh
|   |-- check_task8_baseline.sh
|   |-- check_virtualbox_disk.sh
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

Verify the raw image layout before writing USB media:

```bash
make check-image
```

Verify the interactive `LVGL` GUI path:

```bash
make check-ui
```

This boots the `LVGL` dashboard headlessly, drives `TAB / ENTER / D / I / X /
1` through the `PS/2 -> event loop -> LVGL` path, verifies the `HOME /
SETTINGS / ABOUT / CLEAR` state-machine logs, checks the `HOME` page
toggle for `DETAIL MODE ON -> DETAIL MODE OFF`, the `SETTINGS` page
toggle for `KEY ECHO OFF -> blocked input -> KEY ECHO ON`, the `ABOUT`
page toggle for `SYSTEM NOTES ON -> SYSTEM NOTES OFF`, and requires
visible framebuffer changes for each page switch and input action.

This checks the `MBR`, `ESP/FAT16`, mirrored FAT tables, directory entries, and
verifies that the packed `BOOTX64.EFI` and `KERNEL.BIN` match the build
artifacts byte-for-byte.

Check cross-architecture tooling availability:

```bash
make check-multiarch-preflight
```

Run the shared-kernel host smoke checks:

```bash
make check-multiarch-host
make check-multiarch-host-gui
```

Run the full self-test closure for the current environment:

```bash
make check-selftest
```

Prepared cross-architecture build and run entry points:

```bash
make build-aarch64
make run-aarch64
make check-aarch64

make build-riscv64
make run-riscv64
make check-riscv64
```

These targets build a minimal serial-only kernel for `QEMU virt`, route logging
through the platform console backend, and expect the shared allocator, event
loop and heartbeat path to come up without any `x86_64`-specific code.

`make check-multiarch-host` complements those targets when cross toolchains are
not installed. It runs the real shared kernel on the host with stubbed
`aarch64` and `riscv64` arch/platform objects, injects input events, and checks
for the expected serial evidence from `kernel_main`, the input path, and the
heartbeat task.

`make check-multiarch-host-gui` extends that closure to the shared text GUI
path. It attaches a host display backend, drives the `HOME / SETTINGS / ABOUT`
page-local actions through the generic `1 / D`, `2 / E / 0`, and `3 / I`
hotkey paths, verifies that the `HOME` detail panel can be turned on and back
off, and checks the rendered screen content for both `aarch64` and `riscv64`.

`make check-selftest` runs the current `x86_64` UI and baseline checks, the
multiarch host smoke path, the multiarch preflight, and automatically folds in
the `aarch64/riscv64` `QEMU` checks when both the matching cross compiler and
runtime are available.

Prepare a `VirtualBox` disk image:

```bash
make build-vbox
```

Check the converted `VDI`:

```bash
make check-vbox
```

Expected runtime evidence:

```text
tinyOS UEFI loader starting...
LPVEABCDKUS
```

Current visual outcome on the framebuffer:

- a dark `LVGL` dashboard with a bright top band
- a `TINYOS` title with `HOME / SETTINGS / ABOUT / CLEAR`
- visible `PAGE HOME`, `RUNTIME`, `DASHBOARD`, `INPUT` and `STATUS LVGL UI ACTIVE`
- stable navigation where `HOME / SETTINGS / ABOUT` switch pages and `CLEAR`
  empties the input box without disturbing the active content page
- a `HOME` page with a real `DETAIL MODE` toggle that can be focused with
  `TAB` and activated with `ENTER` or `D`
- a `SETTINGS` page with a real `KEY ECHO` toggle button that can be focused
  with `TAB` and activated with `ENTER`
- an `ABOUT` page with a real `SYSTEM NOTES` toggle that can be focused with
  `TAB` and activated with `ENTER` or `I`

## Current Boundary

- Supported now: `x86_64`, `UEFI`, `QEMU + OVMF`, `VirtualBox UEFI`
- Prepared but not verified in this environment: `aarch64 virt` and
  `riscv64 virt` serial bring-up targets plus matching `make check-*` scripts
- Verified in this environment without cross toolchains: shared-kernel
  `aarch64/riscv64` host smoke path via `make check-multiarch-host` and
  `make check-multiarch-host-gui`
- Prepared but not yet verified: real-hardware USB boot flow
- Not yet claimed: `legacy BIOS`, `VMware`, `Hyper-V`, real hardware success
- Boot status source of truth: [x86_64-uefi-boot-status.md](file:///home/cyk/work/tinyOS/docs/validation/x86_64-uefi-boot-status.md)
- Cross-architecture bring-up notes: [multiarch-qemu-bringup.md](file:///home/cyk/work/tinyOS/docs/validation/multiarch-qemu-bringup.md)
- VirtualBox bring-up guide: [x86_64-uefi-virtualbox.md](file:///home/cyk/work/tinyOS/docs/boot/x86_64-uefi-virtualbox.md)
- Next hardware step: follow [x86_64-uefi-real-hardware.md](file:///home/cyk/work/tinyOS/docs/boot/x86_64-uefi-real-hardware.md) and fill [x86_64-uefi-hardware-smoke-template.md](file:///home/cyk/work/tinyOS/docs/validation/x86_64-uefi-hardware-smoke-template.md)
