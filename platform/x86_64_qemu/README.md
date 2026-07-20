# x86_64 QEMU Platform Placeholder

This platform folder documents the machine-owned pieces of the current bring-up.

## Owned By `platform/x86_64_qemu`

- Serial console selection (`COM1`)
- Legacy text output path (`VGA` text memory)
- Machine-facing boot assumptions used by the BIOS plus QEMU flow
- Future framebuffer, keyboard and other virtual device discovery

## Current Backlog To Move Here

- `src/kernel/console.c`
- The machine constants currently embedded in `src/kernel/main.c`
- QEMU machine notes from `scripts/run_qemu_x86_64.sh`

## Interface Contract

The generic kernel should consume this machine through `tinyos/platform.h`
without knowing about I/O ports, VGA addresses or QEMU command lines.
