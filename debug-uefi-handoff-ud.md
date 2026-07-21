# [RESOLVED] UEFI Handoff UD

## Symptoms
- `QEMU + OVMF` can load and start `BOOTX64.EFI`.
- After the EFI loader hands off to the embedded kernel payload, firmware reports `#UD - Invalid Opcode`.
- Exception snapshot consistently shows:
  - `RIP = 0x000000000080201A`
  - `RAX = 0x0000000000100000`
  - `RSP = 0x0000000007E8E700`
- `BOOTX64.EFI` no longer fails with `Invalid Parameter` or `Not Found`; failure now happens after `StartImage`.

## Final Evidence
- Minimal standalone EFI test app boots without exception under the same `gnu-efi + OVMF` build chain.
- The embedded-payload experiment was a dead end: the loader image contained the payload bytes in `loader.so`, but the runtime path remained fragile and repeatedly produced invalid source/target validation markers.
- The stable fix was to stop embedding the kernel inside `BOOTX64.EFI` and instead use the mature UEFI file-loading path:
  - `LoadedImageProtocol`
  - `LibOpenRoot()`
  - `EFI_FILE_PROTOCOL`
  - root-level / fallback `\EFI\BOOT\KERNEL.BIN`
- The loader now allocates fixed pages at `0x02000000`, reads `KERNEL.BIN` from the ESP, validates the first instruction bytes, captures framebuffer / ACPI state, exits boot services, and directly jumps into the kernel using the firmware-provided address space.
- The custom identity-mapped `CR3` handoff was removed from the loader. Page-table ownership can move into the kernel later, after the basic UEFI boot chain is stable.
- Clean `debugcon` markers now show a full successful path:
  - `LPVEABCDKUS`
  - meaning:
    - `L/P`: loader initialized and kernel file loaded
    - `V`: kernel bytes at target address validate
    - `E`: `ExitBootServices()` succeeds
    - `A/B/C/D`: kernel entry stub executes through stack setup and `.bss` clear
    - `K/U/S`: `kernel_main()` reaches the UEFI demo path and then enters the expected steady state

## Root Cause
- The original design combined two high-risk moving parts at once:
  - embedding the kernel payload into the EFI image
  - switching to a loader-built identity-mapped `CR3` before kernel entry
- That made the final handoff depend on loader image residency, section packaging, and address-space assumptions that were easy to invalidate under OVMF.
- Replacing that with a standard ESP file load removed the fragile dependency on embedded-payload residency and immediately produced a stable handoff.
