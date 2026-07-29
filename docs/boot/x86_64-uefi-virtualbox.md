# x86_64 UEFI VirtualBox Bring-Up

## Goal

This document defines the first non-`QEMU` virtual-machine preparation path for
the current `UEFI-first` tinyOS image.

Current meaning:

- `QEMU + OVMF` is verified
- `VirtualBox UEFI` is verified
- the first successful record is checked in under `docs/validation/`

## Produced Artifacts

Build the VirtualBox disk with:

```bash
make build-vbox
```

This produces:

- `build/x86_64/tinyos-x86_64.img`
  - the existing raw disk image
- `build/x86_64/tinyos-x86_64.vdi`
  - a `VirtualBox`-attachable disk image converted from the raw image

## Preflight Check

Before trying `VirtualBox`, run:

```bash
make check-vbox
```

This confirms:

- `qemu-img` is available
- the `VDI` image is produced
- the `VDI` format is correct
- the virtual size matches the source raw disk image

## VirtualBox VM Settings

Create a new VM with at least these settings:

- type: `Other/Unknown (64-bit)` or equivalent `x86_64` profile
- firmware: `EFI`
- memory: `256 MiB` or more
- CPU: `1` or more
- graphics: default `VirtualBox` adapter is acceptable for the first smoke test
- disk: attach `build/x86_64/tinyos-x86_64.vdi` as the primary disk

Important:

- do not use `legacy BIOS`
- do not enable Secure Boot style restrictions through extra platform settings
- keep the VM simple; avoid additional disks or unusual chipset tweaks for the first run

## Expected Success Signal

The target success condition is the same as the `QEMU + OVMF` `LVGL` baseline:

- `VirtualBox` firmware starts the attached `VDI`
- tinyOS reaches the framebuffer GUI runtime
- the screen shows:
  - `TINYOS`
  - `PAGE HOME`
  - `STATUS LVGL UI ACTIVE`

## Smoke-Test Record

Use the checked-in template:

- [x86_64-uefi-virtualbox-smoke-template.md](file:///home/cyk/work/tinyOS/docs/validation/x86_64-uefi-virtualbox-smoke-template.md)
- first successful record: [x86_64-uefi-virtualbox-2026-07-23.md](file:///home/cyk/work/tinyOS/docs/validation/x86_64-uefi-virtualbox-2026-07-23.md)

## Failure Buckets

- `V0 VM does not start EFI boot from the disk`
  - likely wrong firmware mode or disk attachment
- `V1 firmware starts but tinyOS does not visibly load`
  - likely `VirtualBox` firmware compatibility issue
- `V2 tinyOS starts but does not reach the expected screen`
  - likely loader, GOP compatibility or kernel handoff issue under `VirtualBox`
- `V3 screen appears but is corrupted`
  - likely framebuffer format or mode handling issue

## Current Verification Rule

Keep `VirtualBox` at `verified` only while at least one concrete smoke-test
record exists with:

- the VM settings used
- the observed result
- a screenshot or concise note
- either the expected `LVGL` home screen or a narrower documented success stage
