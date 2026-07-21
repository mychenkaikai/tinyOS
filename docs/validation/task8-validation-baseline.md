# Task8 Validation And Demo Baseline

## Goal

Task8 turns the existing `Task1` to `Task7` work into a repeatable validation
baseline. The project should now have:

- a visible outcome for each completed stage
- a concrete validation method for boot, display, input, GUI and portability
- a repeatable script entry that re-checks the baseline without relying on
  verbal confirmation

## Stage Outcomes

| Stage | Minimum visible outcome | Primary evidence | Validation method |
| --- | --- | --- | --- |
| `Task1` | Repository scope is locked to `x86_64 + QEMU/OVMF` and the core directories exist | `README.md`, `docs/release-0-scope.md`, `boot/`, `arch/`, `platform/`, `src/` | Review the scope doc and repository layout |
| `Task2` | A bootable `x86_64 UEFI` image is produced and `QEMU + OVMF` reaches the kernel entry | `build/x86_64/tinyos-x86_64.img`, serial log, `debugcon` log | Run `./scripts/build_x86_64.sh` and boot QEMU |
| `Task3` | The kernel allocator, arch interrupt backend and event loop reach a live heartbeat | `Early heap`, `Interrupts`, `[event] heartbeat=` log lines | Check the serial log for allocator, interrupt readiness and heartbeat output |
| `Task4` | Display and input are attached through generic interfaces instead of direct kernel wiring | `Display:` and `Input:` log lines, keyboard event logs | Confirm the boot log advertises backend binding and observe `[input]` lines after key presses |
| `Task5` | The framebuffer demo owns the screen and presents a visible `UEFI` boot page | `LPVEABCDKUS` marker sequence and the `TINYOS / UEFI BOOT` screen | Confirm the marker sequence and visually inspect the screen in QEMU |
| `Task6` | `ARM64` and `RISC-V` migration boundaries are documented around `boot/`, `arch/` and `platform/` | `docs/porting/task6-arch-platform-boundary.md`, placeholder port directories | Review the porting document and ensure the placeholder directories remain reserved |
| `Task7` | The MCU subset is constrained to a shared-kernel, single-board roadmap | `docs/porting/task7-mcu-subset-roadmap.md` | Review the roadmap for shared abstractions, exclusions and the fixed reference board |
| `Task8` | Validation expectations are explicit and re-runnable | this document and `scripts/check_task8_baseline.sh` | Run the script and review the summary |

## Validation Matrix

### Boot

- Automated evidence: `tinyOS UEFI loader starting...` plus `LPVEABCDKUS`
- Pass condition: the headless `QEMU + OVMF` logs reach the loader banner and the full handoff marker sequence

### Display

- Automated evidence: the `debugcon` log reaches `...KUS`, proving the kernel entered the `UEFI` demo path
- Manual evidence: the window shows the blue panel with `TINYOS` and `UEFI BOOT`
- Pass condition: the `UEFI` demo path executes and the GOP framebuffer is visibly owned by the demo screen

### Input

- Automated evidence: none in the current `UEFI` demo path; input remains part of the retained text-mode path
- Manual evidence: not required for the current `UEFI-first` acceptance slice
- Pass condition: this row is informational only until the full interactive GUI path is moved onto the `UEFI` framebuffer route

### GUI

- Automated evidence: `LPVEABCDKUS`
- Manual evidence: the screen shows the expected `TINYOS / UEFI BOOT` framebuffer panel
- Pass condition: the demo remains stable on screen after the kernel handoff

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

The script automates the following:

1. checks that the Task6, Task7 and Task8 validation documents exist
2. builds the bootable `x86_64 UEFI` image
3. boots `QEMU + OVMF` in headless mode and captures serial output
4. verifies the expected loader, handoff and framebuffer-demo markers
5. checks that the `ARM64`, `RISC-V` and MCU roadmap placeholders remain in the
   repository

## Manual Demo Steps

Use `make run` for the screen-visible demo and perform this quick check:

1. wait for the `UEFI` demo screen to appear
2. confirm the top band and center panel are visible
3. confirm the text reads `TINYOS` and `UEFI BOOT`
4. confirm the serial / `debugcon` evidence matches the current expected handoff sequence

## Acceptance Result

Task8 is complete when:

1. each stage has a visible outcome and an evidence path
2. boot, display, input, GUI, cross-architecture and MCU subset validation are
   explicitly defined
3. `scripts/check_task8_baseline.sh` passes on a prepared development machine
4. future iterations can re-run the same script and manual checklist instead of
   relying on oral status reports
