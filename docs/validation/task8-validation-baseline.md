# Task8 Validation And Demo Baseline

## Goal

Task8 turns the existing `Task1` to `Task7` work into a repeatable validation
baseline. The project should now have:

- a visible outcome for each completed stage
- a concrete validation method for boot, display, input, GUI and portability
- a repeatable script entry that re-checks the baseline without relying on
  verbal confirmation

Boot-media status beyond `QEMU + OVMF` is tracked separately in
[x86_64-uefi-boot-status.md](file:///home/cyk/work/tinyOS/docs/validation/x86_64-uefi-boot-status.md).
The first verified non-`QEMU` path is `VirtualBox UEFI`, tracked in
[x86_64-uefi-virtualbox.md](file:///home/cyk/work/tinyOS/docs/boot/x86_64-uefi-virtualbox.md)
with a concrete record in
[x86_64-uefi-virtualbox-2026-07-23.md](file:///home/cyk/work/tinyOS/docs/validation/x86_64-uefi-virtualbox-2026-07-23.md).

## Stage Outcomes

| Stage | Minimum visible outcome | Primary evidence | Validation method |
| --- | --- | --- | --- |
| `Task1` | Repository scope is locked to `x86_64 + QEMU/OVMF` and the core directories exist | `README.md`, `docs/release-0-scope.md`, `boot/`, `arch/`, `platform/`, `src/` | Review the scope doc and repository layout |
| `Task2` | A bootable `x86_64 UEFI` image is produced and `QEMU + OVMF` reaches the kernel entry | `build/x86_64/tinyos-x86_64.img`, serial log, `debugcon` log | Run `./scripts/build_x86_64.sh` and boot QEMU |
| `Task3` | The kernel allocator, arch interrupt backend and event loop reach a live heartbeat | `Early heap`, `Interrupts`, `[event] heartbeat=` log lines | Check the serial log for allocator, interrupt readiness and heartbeat output |
| `Task4` | Display and input are attached through generic interfaces instead of direct kernel wiring | `Display:` and `Input:` log lines, keyboard event logs | Confirm the boot log advertises backend binding and observe `[input]` lines after key presses |
| `Task5` | The framebuffer GUI owns the screen and presents the live `LVGL` home page | `LPVEABCDKUS`, `STATUS LVGL UI ACTIVE` and the `PAGE HOME` screen | Confirm the handoff markers and visually inspect the `LVGL` UI in QEMU |
| `Task6` | `ARM64` and `RISC-V` migration boundaries are documented around `boot/`, `arch/` and `platform/` | `docs/porting/task6-arch-platform-boundary.md`, placeholder port directories | Review the porting document and ensure the placeholder directories remain reserved |
| `Task7` | The MCU subset is constrained to a shared-kernel, single-board roadmap | `docs/porting/task7-mcu-subset-roadmap.md` | Review the roadmap for shared abstractions, exclusions and the fixed reference board |
| `Task8` | Validation expectations are explicit and re-runnable | this document and `scripts/check_task8_baseline.sh` | Run the script and review the summary |

## Validation Matrix

### Boot

- Automated evidence: `tinyOS UEFI loader starting...` plus `LPVEABCDKUS`
- Pass condition: the headless `QEMU + OVMF` logs reach the loader banner and the full handoff marker sequence

### Display

- Automated evidence: the `debugcon` log reaches `...KUS`, proving the kernel entered the `UEFI` GUI path
- Manual evidence: the window shows the blue `LVGL` dashboard with `TINYOS`, navigation buttons and the runtime panel
- Pass condition: the `UEFI` GUI path executes and the GOP framebuffer is visibly owned by the `LVGL` screen

### Input

- Automated evidence: `make check-ui` injects `3` and `Enter`, then verifies both `[input]` log lines and a changed screen dump
- Manual evidence: optional; a visible page switch to `ABOUT` should match the injected key path
- Pass condition: the `QMP sendkey -> PS/2 IRQ -> input queue -> LVGL render` chain completes without hanging

### GUI

- Automated evidence: `make check-ui` records screen dumps before and after interaction and requires visible pixel changes
- Manual evidence: the screen shows `PAGE HOME`, `RUNTIME`, `DASHBOARD`, `INPUT` and `STATUS LVGL UI ACTIVE`
- Pass condition: the `LVGL` UI remains stable after kernel handoff and updates in response to injected input

### Cross-Architecture Portability

- Automated evidence: `docs/porting/task6-arch-platform-boundary.md`,
  `arch/aarch64/README.md`, `arch/riscv64/README.md`,
  `platform/aarch64_virt/README.md` and `platform/riscv64_virt/README.md`
  exist
- Manual evidence: reviewers can trace the documented replacement boundary to
  `boot/`, `arch/` and `platform/`
- Pass condition: the portability contract exists and keeps generic kernel code
  shared

### MCU Subset

- Automated evidence: `docs/porting/task7-mcu-subset-roadmap.md` exists and
  names `STM32F429I-DISC1`
- Manual evidence: reviewers can find the explicit exclusions for MMU, VFS,
  user mode and SMP
- Pass condition: the MCU work stays a constrained subset instead of a forked
  OS plan

## Repeatable Check Entry

Run the baseline check with either of these commands:

```bash
./scripts/check_task8_baseline.sh
```

```bash
make check-baseline
```

Run the interactive framebuffer GUI check with either of these commands:

```bash
./scripts/check_lvgl_interaction.sh
```

```bash
make check-ui
```

The script automates the following:

1. checks that the Task6, Task7 and Task8 validation documents exist
2. builds the bootable `x86_64 UEFI` image
3. boots `QEMU + OVMF` in headless mode and captures serial output
4. verifies the expected loader, handoff and framebuffer GUI markers
5. checks that the `ARM64`, `RISC-V` and MCU roadmap placeholders remain in the
   repository

The interaction check automates the following:

1. boots `QEMU + OVMF` with a `QMP` socket
2. waits for the kernel heartbeat to prove the event loop is live
3. injects `3` and `Enter` into the PS/2 path
4. verifies the expected `[input]` log lines
5. captures framebuffer dumps before and after the interaction and requires visible pixel changes

For pre-hardware media inspection, run:

```bash
make check-image
```

This verifies that the raw image still contains the expected `MBR -> ESP ->
EFI/BOOT/BOOTX64.EFI + KERNEL.BIN` layout, that the two `FAT` copies match,
and that the packed files match the current build artifacts before any USB
write step.

## Manual Demo Steps

Use `make run` for the screen-visible demo and perform this quick check:

1. wait for the `UEFI` demo screen to appear
2. confirm the top band, navigation row and content panels are visible
3. confirm the text reads `TINYOS`, `PAGE HOME` and `STATUS LVGL UI ACTIVE`
4. confirm the serial / `debugcon` evidence matches the current expected handoff sequence

## Boot-Media Status Discipline

When reporting boot support, keep these three statements separate:

1. `QEMU + OVMF` verified
2. other virtual machines verified, prepared or unverified
3. real-hardware USB path prepared or verified

Use [x86_64-uefi-boot-status.md](file:///home/cyk/work/tinyOS/docs/validation/x86_64-uefi-boot-status.md)
as the source of truth instead of summarizing all boot targets as one status.

## Acceptance Result

Task8 is complete when:

1. each stage has a visible outcome and an evidence path
2. boot, display, input, GUI, cross-architecture and MCU subset validation are
   explicitly defined
3. `scripts/check_task8_baseline.sh` and `scripts/check_lvgl_interaction.sh` pass on a prepared development machine
4. future iterations can re-run the same scripts and manual checklist instead of
   relying on oral status reports
