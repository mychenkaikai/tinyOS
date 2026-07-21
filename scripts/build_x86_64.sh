#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build/x86_64"
EFI_PKG_DIR="${ROOT_DIR}/third_party/gnu-efi/x86_64"
EFI_DIR="${BUILD_DIR}/esp/EFI/BOOT"
KERNEL_ELF="${BUILD_DIR}/kernel.elf"
KERNEL_BIN="${BUILD_DIR}/kernel.bin"
LOADER_OBJ="${BUILD_DIR}/loader.o"
LOADER_ELF="${BUILD_DIR}/loader.so"
LOADER_EFI="${EFI_DIR}/BOOTX64.EFI"
ESP_IMAGE="${BUILD_DIR}/tinyos-x86_64-esp.img"
IMAGE_BIN="${BUILD_DIR}/tinyos-x86_64.img"

mkdir -p "${BUILD_DIR}" "${EFI_DIR}"

if [[ ! -f "${EFI_PKG_DIR}/crt0-efi-x86_64.o" ]] || [[ ! -f "${EFI_PKG_DIR}/elf_x86_64_efi.lds" ]] || [[ ! -f "${EFI_PKG_DIR}/libefi.a" ]] || [[ ! -f "${EFI_PKG_DIR}/libgnuefi.a" ]]; then
    echo "Missing vendored gnu-efi runtime files under ${EFI_PKG_DIR}." >&2
    echo "Expected: crt0-efi-x86_64.o, elf_x86_64_efi.lds, libefi.a, libgnuefi.a" >&2
    exit 1
fi

C_SOURCES=(
    "${ROOT_DIR}/arch/x86_64/arch.c"
    "${ROOT_DIR}/arch/x86_64/interrupts.c"
    "${ROOT_DIR}/src/kernel/console.c"
    "${ROOT_DIR}/src/kernel/display.c"
    "${ROOT_DIR}/src/kernel/gui.c"
    "${ROOT_DIR}/src/kernel/input.c"
    "${ROOT_DIR}/src/kernel/memory.c"
    "${ROOT_DIR}/src/kernel/event_loop.c"
    "${ROOT_DIR}/src/kernel/main.c"
    "${ROOT_DIR}/src/kernel/uefi_boot_demo.c"
    "${ROOT_DIR}/src/platform/x86_64/platform.c"
    "${ROOT_DIR}/src/platform/x86_64/text_display.c"
    "${ROOT_DIR}/src/platform/x86_64/keyboard.c"
)

ASM_SOURCES=(
    "${ROOT_DIR}/arch/x86_64/kernel_entry.S"
    "${ROOT_DIR}/arch/x86_64/interrupt_stubs.S"
)

OBJECTS=()

for source_file in "${C_SOURCES[@]}"; do
    object_file="${BUILD_DIR}/$(basename "${source_file%.*}").o"
    gcc -m64 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fcf-protection=none -mno-red-zone -Wall -Wextra -Werror -I"${ROOT_DIR}/include" -c "${source_file}" -o "${object_file}"
    OBJECTS+=("${object_file}")
done

for source_file in "${ASM_SOURCES[@]}"; do
    object_file="${BUILD_DIR}/$(basename "${source_file%.*}").o"
    gcc -m64 -ffreestanding -c "${source_file}" -o "${object_file}"
    OBJECTS+=("${object_file}")
done

ld -nostdlib -z max-page-size=0x1000 -T "${ROOT_DIR}/arch/x86_64/linker.ld" -o "${KERNEL_ELF}" "${OBJECTS[@]}"
objcopy -O binary "${KERNEL_ELF}" "${KERNEL_BIN}"
kernel_start_hex=$(nm -n "${KERNEL_ELF}" | awk '/ __kernel_start$/ {print $1}')
kernel_end_hex=$(nm -n "${KERNEL_ELF}" | awk '/ __kernel_end$/ {print $1}')
if [[ -z "${kernel_start_hex}" || -z "${kernel_end_hex}" ]]; then
    echo "failed to locate __kernel_start/__kernel_end in ${KERNEL_ELF}" >&2
    exit 1
fi
kernel_image_size=$((0x${kernel_end_hex} - 0x${kernel_start_hex}))
truncate -s "${kernel_image_size}" "${KERNEL_BIN}"
cp "${KERNEL_BIN}" "${BUILD_DIR}/KERNEL.BIN"
gcc -m64 -ffreestanding -fshort-wchar -fpic -fno-stack-protector -fcf-protection=none -mno-red-zone -maccumulate-outgoing-args -Wall -Wextra -Werror \
    -I"${ROOT_DIR}/include" -I"${ROOT_DIR}/third_party/gnu-efi/include" \
    -c "${ROOT_DIR}/boot/uefi/loader.c" -o "${LOADER_OBJ}"

ld -nostdlib -znocombreloc -T "${EFI_PKG_DIR}/elf_x86_64_efi.lds" -shared -Bsymbolic \
    "${EFI_PKG_DIR}/crt0-efi-x86_64.o" "${LOADER_OBJ}" \
    -L "${EFI_PKG_DIR}" -lefi -lgnuefi \
    -o "${LOADER_ELF}"

objcopy \
    -j .text -j .text.* -j .sdata -j .data -j .data.* -j .rodata -j .rodata.* \
    -j .dynamic -j .dynsym -j .rel -j .rel.* -j .rela -j .rela.* -j .reloc -j .eh_frame \
    -O pei-x86-64 \
    --subsystem=10 \
    "${LOADER_ELF}" \
    "${LOADER_EFI}"

python3 "${ROOT_DIR}/scripts/build_uefi_disk.py" \
    "${LOADER_EFI}" \
    "${BUILD_DIR}/KERNEL.BIN" \
    "${ESP_IMAGE}" \
    "${IMAGE_BIN}"

cat <<SUMMARY
Built tinyOS x86_64 UEFI image:
  kernel elf : ${KERNEL_ELF}
  kernel bin : ${BUILD_DIR}/KERNEL.BIN
  efi loader : ${LOADER_EFI}
  esp image  : ${ESP_IMAGE}
  disk image : ${IMAGE_BIN}
SUMMARY
