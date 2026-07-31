#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE="${ROOT_DIR}/build/riscv64_virt/tinyos-riscv64-virt.elf"

if ! command -v qemu-system-riscv64 >/dev/null 2>&1; then
    echo "qemu-system-riscv64 is required to run the riscv64 virt target." >&2
    exit 1
fi

if [[ ! -f "${IMAGE}" ]]; then
    "${ROOT_DIR}/scripts/build_riscv64_virt.sh"
fi

exec qemu-system-riscv64 \
    -machine virt \
    -bios none \
    -kernel "${IMAGE}" \
    -serial stdio \
    -monitor none \
    -display none
