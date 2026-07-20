#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build/x86_64"
GENERATED_DIR="${BUILD_DIR}/generated"
KERNEL_ELF="${BUILD_DIR}/kernel.elf"
KERNEL_BIN="${BUILD_DIR}/kernel.bin"
STAGE2_BIN="${BUILD_DIR}/stage2.bin"
BOOT_BIN="${BUILD_DIR}/boot_sector.bin"
IMAGE_BIN="${BUILD_DIR}/tinyos-x86_64.img"
LAYOUT_INC="${GENERATED_DIR}/boot_layout.inc"

mkdir -p "${BUILD_DIR}" "${GENERATED_DIR}"

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
    "${ROOT_DIR}/src/platform/x86_64/platform.c"
    "${ROOT_DIR}/src/platform/x86_64/text_display.c"
    "${ROOT_DIR}/src/platform/x86_64/keyboard.c"
)

ASM_SOURCES=(
    "${ROOT_DIR}/arch/x86_64/kernel_entry.S"
    "${ROOT_DIR}/arch/x86_64/interrupt_stubs.S"
)

write_layout() {
    cat > "${LAYOUT_INC}" <<LAYOUT
.set STAGE2_LBA, 1
.set STAGE2_SECTORS, ${1}
.set KERNEL_LBA, ${2}
.set KERNEL_SECTORS, ${3}
.set KERNEL_ENTRY_ADDR, 0x10000
LAYOUT
}

sector_count() {
    local file_size
    file_size=$(stat -c '%s' "$1")
    echo $(((file_size + 511) / 512))
}

write_layout 1 2 1

OBJECTS=()

for source_file in "${C_SOURCES[@]}"; do
    object_file="${BUILD_DIR}/$(basename "${source_file%.*}").o"
    gcc -m64 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -mno-red-zone -Wall -Wextra -Werror -I"${ROOT_DIR}/include" -c "${source_file}" -o "${object_file}"
    OBJECTS+=("${object_file}")
done

for source_file in "${ASM_SOURCES[@]}"; do
    object_file="${BUILD_DIR}/$(basename "${source_file%.*}").o"
    gcc -m64 -ffreestanding -c "${source_file}" -o "${object_file}"
    OBJECTS+=("${object_file}")
done

ld -nostdlib -z max-page-size=0x1000 -T "${ROOT_DIR}/arch/x86_64/linker.ld" -o "${KERNEL_ELF}" "${OBJECTS[@]}"
objcopy -O binary "${KERNEL_ELF}" "${KERNEL_BIN}"

gcc -m64 -ffreestanding -x assembler-with-cpp -I"${GENERATED_DIR}" -c "${ROOT_DIR}/boot/x86_64/stage2.S" -o "${BUILD_DIR}/stage2.o"
ld -nostdlib -n -Ttext 0x8000 --oformat binary -o "${STAGE2_BIN}" "${BUILD_DIR}/stage2.o"

stage2_sectors=$(sector_count "${STAGE2_BIN}")
kernel_sectors=$(sector_count "${KERNEL_BIN}")
kernel_lba=$((1 + stage2_sectors))

write_layout "${stage2_sectors}" "${kernel_lba}" "${kernel_sectors}"

gcc -m64 -ffreestanding -x assembler-with-cpp -I"${GENERATED_DIR}" -c "${ROOT_DIR}/boot/x86_64/stage2.S" -o "${BUILD_DIR}/stage2.o"
ld -nostdlib -n -Ttext 0x8000 --oformat binary -o "${STAGE2_BIN}" "${BUILD_DIR}/stage2.o"

gcc -m64 -ffreestanding -x assembler-with-cpp -I"${GENERATED_DIR}" -c "${ROOT_DIR}/boot/x86_64/boot_sector.S" -o "${BUILD_DIR}/boot_sector.o"
ld -nostdlib -n -Ttext 0x7c00 --oformat binary -o "${BOOT_BIN}" "${BUILD_DIR}/boot_sector.o"

boot_size=$(stat -c '%s' "${BOOT_BIN}")
if [[ "${boot_size}" -ne 512 ]]; then
    echo "boot sector size must be exactly 512 bytes, got ${boot_size}" >&2
    exit 1
fi

image_sectors=$((1 + stage2_sectors + kernel_sectors))
truncate -s "$((image_sectors * 512))" "${IMAGE_BIN}"
dd if="${BOOT_BIN}" of="${IMAGE_BIN}" conv=notrunc status=none
dd if="${STAGE2_BIN}" of="${IMAGE_BIN}" bs=512 seek=1 conv=notrunc status=none
dd if="${KERNEL_BIN}" of="${IMAGE_BIN}" bs=512 seek="${kernel_lba}" conv=notrunc status=none

cat <<SUMMARY
Built tinyOS x86_64 image:
  boot sector : ${BOOT_BIN}
  stage2      : ${STAGE2_BIN} (${stage2_sectors} sectors)
  kernel      : ${KERNEL_BIN} (${kernel_sectors} sectors)
  disk image  : ${IMAGE_BIN}
SUMMARY
