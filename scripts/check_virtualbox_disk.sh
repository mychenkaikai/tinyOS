#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_SCRIPT="${ROOT_DIR}/scripts/build_virtualbox_disk.sh"
VDI_IMAGE="${ROOT_DIR}/build/x86_64/tinyos-x86_64.vdi"
PASS_COUNT=0

pass() {
    PASS_COUNT=$((PASS_COUNT + 1))
    printf '[PASS] %s\n' "$1"
}

fail() {
    printf '[FAIL] %s\n' "$1" >&2
    exit 1
}

printf '== VirtualBox disk check ==\n'

if ! command -v qemu-img >/dev/null 2>&1; then
    fail "qemu-img is required for VirtualBox disk preparation"
fi
pass "qemu-img is available"

bash "${BUILD_SCRIPT}"

if [[ ! -f "${VDI_IMAGE}" ]]; then
    fail "VDI image was not produced"
fi
pass "VDI image produced"

VDI_INFO="$(qemu-img info --output=json "${VDI_IMAGE}")"

if printf '%s' "${VDI_INFO}" | grep -Fq '"format": "vdi"'; then
    pass "VDI image format is correct"
else
    fail "VDI image format is not vdi"
fi

if printf '%s' "${VDI_INFO}" | grep -Fq '"virtual-size": 17825792'; then
    pass "VDI virtual size matches the raw disk image"
else
    fail "VDI virtual size does not match the expected raw image size"
fi

printf '== VirtualBox disk check passed: %d checks ==\n' "${PASS_COUNT}"
