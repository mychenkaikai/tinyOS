# x86_64 UEFI Boot Status

## Goal

This document is the single status board for `x86_64` boot-media validation.
It exists to keep three states separate:

- `QEMU + OVMF` verified
- other virtual machines verified, prepared or unverified
- real-hardware USB path prepared but not yet verified

Do not collapse these into a single vague statement such as "the image boots".

## Current Status Matrix

| Target | Firmware / Boot Path | Medium | Status | Evidence | Notes |
| --- | --- | --- | --- | --- | --- |
| `QEMU` | `OVMF UEFI` | `build/x86_64/tinyos-x86_64.img` | `verified` | `make check-baseline`, `make check-ui`, `LPVEABCDKUS`, visible `LVGL` home screen | Verified with boot, heartbeat and interactive GUI evidence |
| `VirtualBox` | `UEFI` | `build/x86_64/tinyos-x86_64.vdi` | `verified` | `make build-vbox`, `make check-vbox`, [x86_64-uefi-virtualbox.md](file:///home/cyk/work/tinyOS/docs/boot/x86_64-uefi-virtualbox.md), [x86_64-uefi-virtualbox-2026-07-23.md](file:///home/cyk/work/tinyOS/docs/validation/x86_64-uefi-virtualbox-2026-07-23.md) | Verified by a successful Windows-host VirtualBox boot to the expected framebuffer screen |
| `VMware` | `UEFI` | same raw image shape | `unverified` | none | Do not claim support yet |
| `Hyper-V` | `UEFI` | same raw image shape | `unverified` | none | Do not claim support yet |
| real hardware | `x86_64 UEFI` USB boot | USB written from `build/x86_64/tinyos-x86_64.img` | `prepared` | [x86_64-uefi-real-hardware.md](file:///home/cyk/work/tinyOS/docs/boot/x86_64-uefi-real-hardware.md), [x86_64-uefi-hardware-smoke-template.md](file:///home/cyk/work/tinyOS/docs/validation/x86_64-uefi-hardware-smoke-template.md) | Writing flow, prerequisites and failure buckets are documented, but no successful machine record exists yet |

## Acceptance Rules

Mark a target as `verified` only when all of these are true:

1. the exact boot path is named
2. the image or medium is named
3. at least one evidence source is recorded
4. the observed result is specific enough to distinguish loader failure, kernel handoff failure and framebuffer success

If any item is missing, keep the target at `prepared` or `unverified`.

## QEMU + OVMF Baseline

Current verified baseline:

- build: `make build`
- automated check: `make check-baseline`
- interactive check: `make check-ui`
- visible run: `make run`
- expected serial evidence: `tinyOS UEFI loader starting...`
- expected `debugcon` evidence: `LPVEABCDKUS`
- expected screen: `TINYOS`, `PAGE HOME` and `STATUS LVGL UI ACTIVE`

## Real-Hardware USB Baseline

Current prepared-but-unverified baseline:

- raw image self-check: `make check-image`
- write instructions: [x86_64-uefi-real-hardware.md](file:///home/cyk/work/tinyOS/docs/boot/x86_64-uefi-real-hardware.md)
- smoke-test template: [x86_64-uefi-hardware-smoke-template.md](file:///home/cyk/work/tinyOS/docs/validation/x86_64-uefi-hardware-smoke-template.md)
- failure buckets: `F0` to `F3`

The raw image self-check is stronger than a directory listing: it validates the
`MBR`, `ESP/FAT16`, mirrored FAT copies and byte-for-byte equality between the
packed `BOOTX64.EFI` / `KERNEL.BIN` and the current build artifacts.

Real hardware becomes `verified` only after a concrete machine attempt is
recorded with the template and reaches either:

- the expected framebuffer screen, or
- a narrower, documented intermediate success criterion adopted later

## VirtualBox UEFI Baseline

Current verified baseline:

- build `VDI`: `make build-vbox`
- disk self-check: `make check-vbox`
- run guide: [x86_64-uefi-virtualbox.md](file:///home/cyk/work/tinyOS/docs/boot/x86_64-uefi-virtualbox.md)
- smoke-test template: [x86_64-uefi-virtualbox-smoke-template.md](file:///home/cyk/work/tinyOS/docs/validation/x86_64-uefi-virtualbox-smoke-template.md)
- first successful record: [x86_64-uefi-virtualbox-2026-07-23.md](file:///home/cyk/work/tinyOS/docs/validation/x86_64-uefi-virtualbox-2026-07-23.md)

Current success signal:

- `VirtualBox` firmware starts the attached `VDI`
- tinyOS reaches the framebuffer GUI runtime
- the screen shows the expected `LVGL` dashboard or a later equivalent interactive page

## Troubleshooting Entry

Use these entry points when the state is not `verified`:

- `F0`: firmware did not list the USB device
- `F1`: firmware listed the device but did not start tinyOS
- `F2`: tinyOS loader started but screen never reached the demo
- `F3`: screen appeared but was corrupted

For real hardware, always attach a photo or short note for the last visible
stage instead of reporting only "failed".
