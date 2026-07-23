#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RAW_IMAGE="${ROOT_DIR}/build/x86_64/tinyos-x86_64.img"
VDI_IMAGE="${ROOT_DIR}/build/x86_64/tinyos-x86_64.vdi"

"${ROOT_DIR}/scripts/build_x86_64.sh"

if ! command -v qemu-img >/dev/null 2>&1; then
    echo "qemu-img is required to build the VirtualBox VDI image" >&2
    exit 1
fi

qemu-img convert -f raw -O vdi "${RAW_IMAGE}" "${VDI_IMAGE}"

cat <<SUMMARY
Built tinyOS x86_64 VirtualBox disk:
  raw image : ${RAW_IMAGE}
  vdi image : ${VDI_IMAGE}
SUMMARY
