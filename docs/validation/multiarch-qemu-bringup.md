# Multiarch QEMU Bring-Up

## Goal

This document tracks the prepared serial-only bring-up path for the next two
kernel targets:

- `aarch64 virt`
- `riscv64 virt`

These targets are intentionally smaller than the current `x86_64 + UEFI + LVGL`
path. The first milestone is only:

`serial console -> allocator -> shared event loop -> heartbeat`

No framebuffer, no keyboard, and no architecture-specific scheduler fork are
required for this stage.

## Shared Kernel Contract

The prepared `aarch64` and `riscv64` targets reuse these shared kernel files:

- `src/kernel/console.c`
- `src/kernel/display.c`
- `src/kernel/gui.c`
- `src/kernel/input.c`
- `src/kernel/memory.c`
- `src/kernel/event_loop.c`
- `src/kernel/main.c`

The `UEFI` GUI path is replaced with `src/kernel/gui_uefi_stub.c`, which keeps
the generic GUI interface linkable without pulling `UEFI` or `LVGL` into the
serial-only bring-up stage.

## Platform And Architecture Pieces

Prepared target-specific files:

- `arch/aarch64/arch.c`
- `arch/aarch64/kernel_entry.S`
- `arch/aarch64/linker.ld`
- `src/platform/aarch64/platform.c`
- `arch/riscv64/arch.c`
- `arch/riscv64/kernel_entry.S`
- `arch/riscv64/linker.ld`
- `src/platform/riscv64/platform.c`

Current design notes:

- the console backend now comes from `tinyos_platform_current()->console`
- both non-`x86_64` targets currently use a minimal fake-tick backend in
  `cpu_idle()` to let the shared event loop advance before a real timer driver
  lands
- both targets are headless for now and leave `display` and `input` detached

## Build And Run Entry

```bash
make check-multiarch-preflight
```

```bash
make check-multiarch-host
make check-multiarch-host-gui
```

```bash
make check-selftest
```

```bash
make build-aarch64
make run-aarch64
make check-aarch64
```

```bash
make build-riscv64
make run-riscv64
make check-riscv64
```

## Tooling Requirement

Expected tools:

- `aarch64-linux-gnu-gcc`
- `riscv64-linux-gnu-gcc`
- `qemu-system-aarch64`
- `qemu-system-riscv64`

`make check-multiarch-preflight` reports which pieces are available in the
current environment before the architecture-specific checks are attempted.

`make check-multiarch-host` only needs the host `gcc`. It runs the real shared
kernel with stub `aarch64` and `riscv64` arch/platform registrations so the
generic allocator, input path, event loop, and heartbeat logging can still be
validated before the cross compiler and `QEMU` runtimes are installed.

`make check-multiarch-host-gui` adds a host display backend on top of that
setup and verifies the shared text GUI path, including `HOME` detail toggling,
`SETTINGS` key-echo toggling plus input clearing, `ABOUT` notes toggling, and
the rendered screen contents for both architectures.

`make check-selftest` is the top-level closure command. It runs the current
`x86_64` UI regression, the baseline regression, the host smoke checks, the
tooling preflight, and conditionally the `QEMU` checks for `aarch64` and
`riscv64` when the required tools are present.

## Expected Runtime Evidence

The prepared `aarch64` check expects:

- `tinyOS kernel bootstrap ready.`
- `Architecture: aarch64`
- `Platform: aarch64-virt`
- `Interrupts: arch backend ready=yes`
- `[event] heartbeat=`
- `[gui] Headless GUI loop active.`

The prepared `riscv64` check expects:

- `tinyOS kernel bootstrap ready.`
- `Architecture: riscv64`
- `Platform: riscv64-virt`
- `Interrupts: arch backend ready=yes`
- `[event] heartbeat=`
- `[gui] Headless GUI loop active.`

## Status Discipline

- `x86_64 + UEFI + LVGL`: verified
- `aarch64/riscv64` shared kernel on host: verified by
  `make check-multiarch-host` and `make check-multiarch-host-gui`
- `aarch64 virt`: prepared, not verified until both the cross compiler and
  `QEMU` runtime are present
- `riscv64 virt`: prepared, not verified until both the cross compiler and
  `QEMU` runtime are present

Do not collapse these into one blanket "multiarch verified" statement.
