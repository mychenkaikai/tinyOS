#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE="${ROOT_DIR}/build/aarch64_virt/tinyos-aarch64-virt.elf"

if ! command -v qemu-system-aarch64 >/dev/null 2>&1; then
    echo "qemu-system-aarch64 is required to run the aarch64 virt target." >&2
    exit 1
fi

if [[ ! -f "${IMAGE}" ]]; then
    "${ROOT_DIR}/scripts/build_aarch64_virt.sh"
fi

exec qemu-system-aarch64 \
    -machine virt \
    -cpu cortex-a57 \
    -bios none \
    -kernel "${IMAGE}" \
    -serial stdio \
    -monitor none \
    -display none
