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
| `Task1` | Repository scope is locked to `x86_64 + QEMU` and the core directories exist | `README.md`, `docs/release-0-scope.md`, `boot/`, `arch/`, `platform/`, `src/` | Review the scope doc and repository layout |
| `Task2` | A bootable `x86_64` image is produced and QEMU reaches the kernel entry | `build/x86_64/tinyos-x86_64.img`, serial boot log | Run `./scripts/build_x86_64.sh` and boot QEMU |
| `Task3` | The kernel allocator, arch interrupt backend and event loop reach a live heartbeat | `Early heap`, `Interrupts`, `[event] heartbeat=` log lines | Check the serial log for allocator, interrupt readiness and heartbeat output |
| `Task4` | Display and input are attached through generic interfaces instead of direct kernel wiring | `Display:` and `Input:` log lines, keyboard event logs | Confirm the boot log advertises backend binding and observe `[input]` lines after key presses |
| `Task5` | The text-mode GUI MVP owns the screen and keeps updating from the event loop | `[gui] Task5 MVP active.` log line and VGA text screen | Confirm GUI activation in serial and visually inspect the screen in QEMU |
| `Task6` | `ARM64` and `RISC-V` migration boundaries are documented around `boot/`, `arch/` and `platform/` | `docs/porting/task6-arch-platform-boundary.md`, placeholder port directories | Review the porting document and ensure the placeholder directories remain reserved |
| `Task7` | The MCU subset is constrained to a shared-kernel, single-board roadmap | `docs/porting/task7-mcu-subset-roadmap.md` | Review the roadmap for shared abstractions, exclusions and the fixed reference board |
| `Task8` | Validation expectations are explicit and re-runnable | this document and `scripts/check_task8_baseline.sh` | Run the script and review the summary |

## Validation Matrix

### Boot

- Automated evidence: `tinyOS x86_64 bootstrap ready.`
- Pass condition: the headless QEMU serial log reaches the kernel banner

### Display

- Automated evidence: `Display: platform-agnostic interface -> x86_64 VGA text backend + positioned draw ops.`
- Manual evidence: the QEMU VGA window shows the text-mode GUI page instead of
  only boot logs
- Pass condition: the serial log proves backend binding and the screen is
  visibly owned by the GUI

### Input

- Automated evidence: `Input: platform-agnostic interface -> x86_64 PS/2 keyboard backend.`
- Manual evidence: press `Tab`, `Enter`, printable keys and `Backspace`, then
  observe serial `[input]` lines and visible GUI changes
- Pass condition: the backend is registered, input events are logged and the
  GUI reacts to focus or text changes

### GUI

- Automated evidence: `[gui] Task5 MVP active. Screen owned by GUI, logs continue on serial.`
- Manual evidence: the screen shows a header, page state and editable demo
  input
- Pass condition: the GUI stays active while heartbeat logs continue on serial

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
2. builds the bootable `x86_64` image
3. boots QEMU in headless mode and captures serial output
4. verifies the expected boot, allocator, arch interrupt, event loop and GUI log
   markers
5. checks that the `ARM64`, `RISC-V` and MCU roadmap placeholders remain in the
   repository

## Manual Demo Steps

Use `make run` for the screen-visible demo and perform this quick check:

1. wait for the GUI screen to replace the boot log on VGA
2. press `Tab` to switch focus between controls
3. press `Enter` on the page toggle control
4. type `abc`, then `Backspace`, inside the editable field
5. confirm serial logs show heartbeat and input activity while the GUI remains
   responsive

## Acceptance Result

Task8 is complete when:

1. each stage has a visible outcome and an evidence path
2. boot, display, input, GUI, cross-architecture and MCU subset validation are
   explicitly defined
3. `scripts/check_task8_baseline.sh` passes on a prepared development machine
4. future iterations can re-run the same script and manual checklist instead of
   relying on oral status reports
