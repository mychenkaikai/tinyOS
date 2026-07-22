# x86_64 UEFI Real-Hardware Smoke Test

## Purpose

This document is the checked-in record for the first real-hardware `UEFI` USB
boot attempts of the current `tinyOS` image.

Use it together with `docs/boot/x86_64-uefi-real-hardware.md`:

- the boot document explains how to prepare and run the attempt
- this record captures what actually happened on each machine

Until a real attempt is recorded here, the repository must continue to state
that real hardware support is not yet verified.

## Status Summary

- `QEMU + OVMF`: validated
- real hardware `UEFI` USB boot: pending first recorded attempt
- `legacy BIOS`: unsupported
- Secure Boot enabled systems: not verified
- non-`OVMF` virtual machine products: not verified

## How To Use This Record

1. Keep one entry per hardware attempt.
2. Do not overwrite an older result; append a new attempt section instead.
3. If an attempt fails, classify it with the failure buckets from
   `docs/boot/x86_64-uefi-real-hardware.md`.
4. Link or store photos, serial captures or notes under a durable path that can
   be reviewed later.

## Attempt Template

Copy this block for each machine attempt:

```text
Attempt ID:
Date:
Operator:
Image commit:
Image path:
Write command used:

Machine:
Firmware vendor/version:
CPU:
GPU/display output:
USB device:

Secure Boot state:
Boot mode selected:
Was the USB device listed by firmware: yes/no
Did firmware start BOOTX64.EFI: yes/no/unknown
Reached tinyOS screen: yes/no
Observed result:
Failure bucket:
Last visible stage:
Photo/log path:
Notes:
```

## Attempt Log

No real-hardware attempt has been recorded yet.

Add the first attempt below this line.
