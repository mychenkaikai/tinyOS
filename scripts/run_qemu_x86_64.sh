#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE_BIN="${ROOT_DIR}/build/x86_64/tinyos-x86_64.img"
OVMF_CODE="/usr/share/OVMF/OVMF_CODE_4M.fd"
OVMF_VARS_TEMPLATE="/usr/share/OVMF/OVMF_VARS_4M.fd"
OVMF_VARS="${ROOT_DIR}/build/x86_64/OVMF_VARS_4M.fd"

if [[ ! -f "${IMAGE_BIN}" ]]; then
    "${ROOT_DIR}/scripts/build_x86_64.sh"
fi

if ! command -v qemu-system-x86_64 >/dev/null 2>&1; then
    echo "qemu-system-x86_64 is required but not installed." >&2
    exit 1
fi

if [[ ! -f "${OVMF_CODE}" || ! -f "${OVMF_VARS_TEMPLATE}" ]]; then
    echo "OVMF firmware files are required under /usr/share/OVMF." >&2
    exit 1
fi

if [[ ! -f "${OVMF_VARS}" ]]; then
    cp "${OVMF_VARS_TEMPLATE}" "${OVMF_VARS}"
fi

exec qemu-system-x86_64 \
    -machine q35 \
    -drive if=pflash,format=raw,readonly=on,file="${OVMF_CODE}" \
    -drive if=pflash,format=raw,file="${OVMF_VARS}" \
    -drive format=raw,file="${IMAGE_BIN}" \
    -serial stdio \
    -display default \
    -no-reboot
