# x86_64 UEFI Real-Hardware Bring-Up

## Goal

This document describes the first real-hardware preparation path for the
current `UEFI-first` tinyOS image. The repository has already validated the
same image shape under `QEMU + OVMF`; this document narrows the next step to a
repeatable USB smoke test instead of an informal "try it on a machine" step.

## Current Support Boundary

- Supported and verified now:
  - `x86_64`
  - `UEFI`
  - `QEMU + OVMF`
  - raw disk image with an `ESP + BOOTX64.EFI + KERNEL.BIN` layout
- Not yet verified:
  - real hardware boot success
  - `legacy BIOS`
  - Secure Boot enabled systems
  - non-`OVMF` virtual machine products
- Treat this document as a preparation and smoke-test guide, not as a claim
  that real hardware is already supported.

## Produced Artifacts

Run:

```bash
make build
```

This produces:

- `build/x86_64/tinyos-x86_64.img`
  - the raw disk image intended for `QEMU` now and USB writing later
- `build/x86_64/tinyos-x86_64-esp.img`
  - the standalone ESP image
- `build/x86_64/esp/EFI/BOOT/BOOTX64.EFI`
  - the `UEFI` loader in the removable-media default path
- `build/x86_64/KERNEL.BIN`
  - the freestanding kernel payload loaded by the `UEFI` loader

## Data-Risk Warning

Writing a raw image to a USB drive overwrites the selected block device.

Before running any write command:

- unplug unrelated removable drives if possible
- identify the target device twice with `lsblk`
- use the whole device path such as `/dev/sdX`, not a partition such as `/dev/sdX1`
- assume all data on that USB drive will be destroyed

## Host-Side Prerequisites

Use a target machine that satisfies all of these:

- `x86_64`
- `UEFI` firmware
- can boot from external USB media
- Secure Boot can be disabled if unsigned EFI binaries are rejected
- available screen output, since the current success signal is the framebuffer
  demo screen

Optional but useful:

- firmware boot menu hotkey access
- another machine or phone for photographing failure states
- a USB serial adapter only if you later add hardware serial logging

## Write The USB Image

1. Build the image:

```bash
make build
```

2. Identify the USB device:

```bash
lsblk -o NAME,SIZE,MODEL,TRAN
```

3. Write the raw image to the whole USB device:

```bash
sudo dd if=build/x86_64/tinyos-x86_64.img of=/dev/sdX bs=4M conv=fsync status=progress
```

4. Flush pending writes:

```bash
sync
```

5. Reinsert the USB drive if your desktop environment cached the old partition
   view.

## Firmware Setup Checklist

On the target machine:

- enter firmware setup or the one-shot boot menu
- select the USB device as a `UEFI` boot target
- disable Secure Boot if the firmware refuses unsigned EFI binaries
- prefer pure `UEFI` mode over any `CSM` or `legacy` compatibility mode

## Expected Success Signal

The current smoke-test success condition is visual:

- firmware launches the removable-media path `EFI/BOOT/BOOTX64.EFI`
- tinyOS reaches the kernel handoff
- the screen shows:
  - a bright blue top band
  - a darker center panel
  - `TINYOS`
  - `UEFI BOOT`

This should match the validated `QEMU + OVMF` screen outcome.

## Minimal Smoke-Test Record

Record each real-hardware attempt with this template:

```text
Date:
Machine:
Firmware vendor/version:
CPU:
GPU/display output:
USB device:
Secure Boot state:
Boot mode selected:
Image commit:
Write command used:
Observed result:
Reached tinyOS screen: yes/no
If no, last visible stage:
Photo/log path:
Notes:
```

## Failure Buckets

Use these buckets to avoid vague bug reports:

- `F0 firmware did not list the USB device`
  - likely USB write, partition visibility, or firmware media-detection issue
- `F1 firmware listed the device but did not start tinyOS`
  - likely Secure Boot, firmware policy, or removable-media path issue
- `F2 tinyOS loader started but screen never reached the expected demo`
  - likely `UEFI` loader, framebuffer, or kernel handoff issue
- `F3 screen appeared but was visibly corrupted`
  - likely GOP mode or framebuffer-format handling issue

## Useful Inspection Commands

On the development machine, these commands help inspect the image before
writing:

```bash
file build/x86_64/tinyos-x86_64.img
```

```bash
fdisk -l build/x86_64/tinyos-x86_64.img
```

```bash
grep -n 'BOOTX64.EFI\|KERNEL.BIN' scripts/build_uefi_disk.py
```

The last command is only a quick reminder that the disk image is assembled by
`scripts/build_uefi_disk.py`; the actual build entry remains
`scripts/build_x86_64.sh`.

## Next Step After First Hardware Attempt

Once the first real machine attempt exists, promote the result into:

- a checked-in smoke-test note under `docs/validation/`
- a narrower support statement in `README.md`
- follow-up fixes for whichever failure bucket was hit first
