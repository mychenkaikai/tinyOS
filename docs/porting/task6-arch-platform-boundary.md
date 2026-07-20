# Task6 Arch And Platform Boundary

## Goal

Task6 defines the smallest reusable boundary that allows the current
`x86_64 + QEMU` baseline to stay the reference implementation while future
`ARM64 virt` and `RISC-V virt` ports replace only boot, architecture and
machine specific code.

## Minimal Interface Surface

### `include/tinyos/arch.h`

The architecture layer owns CPU-local bootstrap and the interrupt plus timer
backend needed by the generic event loop.

- `early_init()`: install descriptor tables, trap vectors or delegation state
- `interrupts_init(tick_hz)`: bring up the timer source and interrupt delivery
- `interrupts_enable()` and `interrupts_disable()`: expose the CPU interrupt
  mask without leaking ISA instructions into generic code
- `timer_ticks()`: supply the monotonic tick source consumed by
  `src/kernel/event_loop.c`
- `interrupts_ready()`: preserve the existing readiness check
- `cpu_idle()`: replace direct `hlt` style instructions in generic code

### `include/tinyos/platform.h`

The platform layer owns machine discovery and device attachment that are not
portable across boards or virtual machines.

- `early_init()`: board or VM specific discovery
- `boot_heap_limit()`: initial heap ceiling before a richer memory map parser
- `console.init()` and `console.write_char()`: early logging sink
- `display`: points at the existing `display_backend` abstraction without
  redefining it in Task6
- `input`: points at the existing `input_backend` abstraction without
  redefining it in Task6

## Current x86_64 Replacement Map

These files are the concrete `x86_64 + QEMU` implementation points that future
ports should replace, not the generic kernel core.

### Boot path

- `boot/x86_64/boot_sector.S`: BIOS boot sector, not reusable by `aarch64` or
  `riscv64`
- `boot/x86_64/stage2.S`: long mode transition and disk loading path, should be
  replaced by the target firmware or loader flow used by the new platform

### Architecture-owned path

- `arch/x86_64/arch.c`: current `tinyos_arch_current()` baseline that exposes
  the `x86_64` interrupt, timer and idle behavior behind `tinyos_arch_ops`
- `arch/x86_64/kernel_entry.S`: `x86_64` entry sequence
- `arch/x86_64/interrupts.c`: concrete `x86_64` interrupt backend with
  `IDT + PIC + PIT`
- `arch/x86_64/interrupt_stubs.S`: `x86_64` ISR stub table
- `arch/x86_64/linker.ld`: current linker layout
- `include/tinyos/port_io.h`: x86 port I/O helper, not portable to
  memory-mapped I/O targets

### Platform-owned path

- `src/platform/x86_64/platform.c`: binds the current platform name, heap limit
  and the existing display and input backends
- `src/platform/x86_64/text_display.c`: current `x86_64` text display backend
- `src/platform/x86_64/keyboard.c`: current `x86_64` PS/2 keyboard backend
- `src/kernel/console.c`: still contains the current `COM1` serial sink and
  should move behind the platform boundary when non-x86 console backends arrive
- `scripts/run_qemu_x86_64.sh`: machine launch contract for the current target
- The boot banners in `src/kernel/main.c`: the architecture and platform names
  should become data from the selected backends instead of hardcoded strings

### Generic kernel core that should stay shared

- `src/kernel/memory.c`
- `src/kernel/event_loop.c`, which now gets tick and idle behavior from
  `tinyos_arch_current()`
- Most of `src/kernel/main.c`, which now uses `tinyos_arch_current()` for
  interrupt bring-up, tick visibility and halt handling while keeping generic
  task wiring shared
- The public allocator and event loop headers in `include/tinyos/`

## Second-Phase Port Validation Standard

Task6 is complete only if later `ARM64 virt` and `RISC-V virt` work can be
measured against the following standard.

1. The port introduces new code mainly under `boot/`, `arch/` and `platform/`.
1. `src/kernel/memory.c` and `src/kernel/event_loop.c` remain shared rather
   than forked per architecture.
1. The generic kernel reaches an equivalent boot milestone on each target:
   early console works, allocator initializes, timer ticks increase and the
   heartbeat task runs.
1. The machine specific display and input backends are attached through
   `tinyos_platform_current()->display` and
   `tinyos_platform_current()->input` without replacing the shared
   `display.h` and `input.h` abstractions.
1. The interrupt plus timer backend is attached through
   `tinyos_arch_current()`.
1. No new architecture may require generic kernel code to call ISA-specific
   instructions, use x86 port I/O or hardcode QEMU machine addresses.

## Suggested Bring-Up Evidence

- `x86_64`: existing `make run` log stays green
- `aarch64`: QEMU `virt` boot log reaches the same allocator and heartbeat lines
- `riscv64`: QEMU `virt` boot log reaches the same allocator and heartbeat lines
- A reviewer can diff a new port and see that most changes stay in
  `boot/`, `arch/` and `platform/`
