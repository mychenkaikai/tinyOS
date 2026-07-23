# x86_64 UEFI Hardware Smoke Test Template

Use this template for every real-hardware USB boot attempt.

Keep one filled copy per machine or per materially different firmware setup.

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
Image path:
Write command used:
Observed result:
Reached tinyOS screen: yes/no
If no, last visible stage:
Failure bucket (F0/F1/F2/F3 or n/a):
Photo/log path:
Notes:
```

## Minimum Record Quality

Do not leave these fields blank:

- `Machine`
- `Firmware vendor/version`
- `Secure Boot state`
- `Image commit`
- `Observed result`
- `Reached tinyOS screen`

## Result Classification

- `yes`: the machine reached the expected `TINYOS / UEFI BOOT` framebuffer screen
- `no`: record the last visible stage and assign `F0`, `F1`, `F2` or `F3`

## Storage Convention

Until real-hardware runs exist, keep this file as the canonical blank template.
When the first machine attempt happens, create a sibling record file under
`docs/validation/` rather than overwriting this template.
