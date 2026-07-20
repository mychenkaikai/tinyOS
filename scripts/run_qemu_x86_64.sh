#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE_BIN="${ROOT_DIR}/build/x86_64/tinyos-x86_64.img"

if [[ ! -f "${IMAGE_BIN}" ]]; then
    "${ROOT_DIR}/scripts/build_x86_64.sh"
fi

if ! command -v qemu-system-x86_64 >/dev/null 2>&1; then
    echo "qemu-system-x86_64 is required but not installed." >&2
    exit 1
fi

exec qemu-system-x86_64 \
    -drive format=raw,file="${IMAGE_BIN}" \
    -serial stdio \
    -display default \
    -no-reboot
