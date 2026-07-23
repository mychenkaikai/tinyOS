# x86_64 UEFI VirtualBox Smoke Test Record 2026-07-23

```text
Date: 2026-07-23
Host machine: Windows host with WSL-built VDI
VirtualBox version: not recorded in repository
VM name: tinyos
Firmware mode: EFI
Chipset: not recorded
Video controller: not recorded
Memory: not recorded
CPU count: not recorded
Disk image: build/x86_64/tinyos-x86_64.vdi
Image commit: uncommitted working tree after VirtualBox preparation updates
Observed result: VirtualBox booted the VDI successfully and reached the expected framebuffer demo screen
Reached tinyOS screen: yes
If no, last visible stage: n/a
Failure bucket (V0/V1/V2/V3 or n/a): n/a
Screenshot path: user-provided chat screenshot
Notes: The visible result matches the expected TINYOS / UEFI BOOT screen with the top accent band and centered panel
```

## Result

This run documents the first successful `VirtualBox UEFI` verification for the
current framebuffer-demo acceptance slice.
