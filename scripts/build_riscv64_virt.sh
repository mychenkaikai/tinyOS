#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build/riscv64_virt"
CC="${CC:-riscv64-linux-gnu-gcc}"
KERNEL_ELF="${BUILD_DIR}/tinyos-riscv64-virt.elf"

require_tool() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "$1 is required to build the riscv64 virt image" >&2
        exit 1
    fi
}

require_tool "${CC}"

mkdir -p "${BUILD_DIR}"

C_SOURCES=(
    "${ROOT_DIR}/arch/riscv64/arch.c"
    "${ROOT_DIR}/src/kernel/console.c"
    "${ROOT_DIR}/src/kernel/display.c"
    "${ROOT_DIR}/src/kernel/gui.c"
    "${ROOT_DIR}/src/kernel/gui_uefi_stub.c"
    "${ROOT_DIR}/src/kernel/input.c"
    "${ROOT_DIR}/src/kernel/memory.c"
    "${ROOT_DIR}/src/kernel/event_loop.c"
    "${ROOT_DIR}/src/kernel/main.c"
    "${ROOT_DIR}/src/kernel/uefi_boot_demo.c"
    "${ROOT_DIR}/src/platform/riscv64/platform.c"
)

ASM_SOURCES=(
    "${ROOT_DIR}/arch/riscv64/kernel_entry.S"
)

OBJECTS=()
CFLAGS=(
    -ffreestanding
    -fno-pic
    -fno-pie
    -fno-stack-protector
    -Wall
    -Wextra
    -Werror
    -march=rv64imac_zicsr_zifencei
    -mabi=lp64
    -I"${ROOT_DIR}/include"
)

for source_file in "${C_SOURCES[@]}"; do
    object_file="${BUILD_DIR}/$(basename "${source_file%.*}").o"
    "${CC}" "${CFLAGS[@]}" -c "${source_file}" -o "${object_file}"
    OBJECTS+=("${object_file}")
done

for source_file in "${ASM_SOURCES[@]}"; do
    object_file="${BUILD_DIR}/$(basename "${source_file%.*}").o"
    "${CC}" -ffreestanding -march=rv64imac_zicsr_zifencei -mabi=lp64 -c "${source_file}" -o "${object_file}"
    OBJECTS+=("${object_file}")
done

"${CC}" -nostdlib -march=rv64imac_zicsr_zifencei -mabi=lp64 -T "${ROOT_DIR}/arch/riscv64/linker.ld" -o "${KERNEL_ELF}" "${OBJECTS[@]}"

cat <<SUMMARY
Built tinyOS riscv64 virt image:
  kernel elf : ${KERNEL_ELF}
SUMMARY
