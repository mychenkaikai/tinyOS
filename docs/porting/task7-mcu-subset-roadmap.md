# Task7 MCU Subset Roadmap

## Goal

Task7 defines a constrained MCU profile that reuses the same kernel-facing
abstractions as the `x86_64 + QEMU` baseline without pretending that MCU targets
offer the same system model as MPU-class machines.

The outcome is a clear subset plan for:

- what stays shared across MPU and MCU targets
- what is intentionally removed on MCU
- which board becomes the first bring-up target
- how GUI, storage and scheduling stay minimal on that board

## Shared Abstractions That MCU Reuses

The MCU path should keep the existing split introduced by Task6 instead of
forking a separate "small OS" codebase.

### Architecture and platform split

- `include/tinyos/arch.h`: still owns interrupt, timer and CPU idle behavior
- `include/tinyos/platform.h`: still owns board discovery, console selection,
  heap limit and backend binding
- `boot/`, `arch/` and `platform/`: remain the only places where board or CPU
  specifics should dominate

### Generic kernel pieces that stay shared

- `src/kernel/memory.c`: keep the bump-style early allocator for boot and kernel
  objects
- `src/kernel/event_loop.c`: keep the cooperative periodic task runner as the
  first scheduler model
- `src/kernel/display.c` plus `include/tinyos/display.h`: keep one generic
  display attachment point
- `src/kernel/input.c` plus `include/tinyos/input.h`: keep one generic input
  event path
- The generic boot banner, logging flow and heap accounting remain shared where
  they do not assume `x86_64` instructions or device addresses

## Capabilities Intentionally Cut On MCU

The MCU subset must stay explicit about what it does not implement in the first
round.

- No virtual memory, address-space switching or user/kernel split
- No process isolation, ELF loader or POSIX compatibility layer
- No VFS, block-device stack, page cache or journaling filesystem
- No SMP, per-core scheduler or lock-heavy concurrency design
- No GPU driver, window compositor or desktop-style graphics stack
- No requirement that MCU and MPU targets boot through the same firmware flow

These removals keep the MCU path focused on a single-address-space firmware-like
system that still uses the shared tinyOS abstractions.

## First Reference Board

The first MCU reference board is `STM32F429I-DISC1`.

It is selected because it gives Task7 a concrete and testable target:

- `Cortex-M4` is a common MCU baseline and matches the intended lightweight
  subset direction
- The board includes an integrated display, so GUI bring-up does not depend on
  designing a custom shield first
- The board has enough on-chip resources to validate a cooperative scheduler,
  minimal GUI and small persistent settings path without introducing an MMU-like
  design
- Its ecosystem has mature startup code, HAL support and debugger workflows,
  which lowers bring-up risk for the first MCU port

## Minimal MCU Bring-Up Path

Task7 does not ask MCU targets to chase feature parity with the MPU path.
Instead, the first board should prove one minimal end-to-end chain:

`reset -> board early init -> allocator -> timer tick -> event loop -> display backend -> GUI tick -> persistent settings blob`

### GUI path

The MCU GUI path should stay below the `Task5` MPU GUI scope.

1. Bind the board LCD through the existing `display_backend` abstraction.
1. Add a tiny pixel or flush backend that lets LVGL or an equivalent lightweight
   GUI stack render into a single framebuffer or line buffer.
1. Drive GUI timing from the shared event loop using a periodic task instead of
   introducing a preemptive UI thread.
1. Ship one fixed demo screen only, such as a boot status page with heartbeat,
   uptime and the last input event.

This keeps GUI validation tied to the existing display abstraction instead of
creating a board-specific GUI framework.

### Storage path

MCU storage must start as configuration persistence, not as a full filesystem.

1. Use internal flash or board-provided non-volatile memory as the first
   persistence backend.
1. Define the first storage contract as a small key/value or blob save/load API
   for settings and GUI assets metadata.
1. Allow only append-plus-compact or fixed-slot updates in the first round.
1. Defer directories, filenames, block caching and wear-leveling frameworks
   until the MCU path proves stable.

The key rule is that MCU storage stays a narrow persistence service and does not
grow into a VFS clone.

### Scheduling path

The MCU scheduler model should remain cooperative in the first implementation.

1. Reuse `src/kernel/event_loop.c` as the scheduler core.
1. Drive system time from the MCU timer interrupt exposed through
   `tinyos_arch_current()`.
1. Run GUI refresh, input polling and background persistence as periodic tasks.
1. Keep interrupt handlers short and push work into the event loop.
1. Defer priorities, time slicing and sleep queues until a second MCU milestone
   requires them.

This ensures MCU work extends the existing heartbeat-style execution model
instead of replacing it with a separate RTOS design on day one.

## Acceptance Standard For Task7

Task7 is complete only if future MCU work can be judged against the following
standard.

1. The first MCU port targets `STM32F429I-DISC1`.
1. Most board-specific code lands under `boot/`, `arch/` and `platform/`
   instead of forking the generic kernel.
1. `src/kernel/memory.c`, `src/kernel/event_loop.c`, `display` and `input`
   interfaces remain shared with the MPU baseline.
1. The first GUI milestone is a single-screen demo driven by the cooperative
   event loop.
1. The first storage milestone persists only small settings or assets metadata,
   not a general-purpose filesystem.
1. Reviewers can point to explicit exclusions for MMU, VFS, user mode and SMP
   instead of inferring them from missing code.

## Suggested Bring-Up Evidence

- Board boots under debugger and reaches the generic kernel banner adapted for
  the MCU platform name
- Timer ticks advance and the shared heartbeat-style periodic task still runs
- A simple GUI screen appears on the board LCD and updates from event-loop ticks
- A small settings blob survives reboot using the chosen non-volatile backend
