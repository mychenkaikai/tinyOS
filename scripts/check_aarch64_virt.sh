#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_SCRIPT="${ROOT_DIR}/scripts/build_aarch64_virt.sh"
TMP_DIR="${ROOT_DIR}/build/tmp/check-aarch64-virt"
SERIAL_LOG_FILE="${TMP_DIR}/serial.log"

PASS_COUNT=0

pass() {
    PASS_COUNT=$((PASS_COUNT + 1))
    printf '[PASS] %s\n' "$1"
}

fail() {
    printf '[FAIL] %s\n' "$1" >&2
    exit 1
}

require_log_line() {
    local pattern="$1"
    local description="$2"

    if grep -Eq "${pattern}" "${SERIAL_LOG_FILE}"; then
        pass "${description}"
    else
        printf 'Captured serial log:\n' >&2
        sed -n '1,220p' "${SERIAL_LOG_FILE}" >&2 || true
        fail "${description}: pattern not found: ${pattern}"
    fi
}

printf '== aarch64 virt check ==\n'

if ! command -v qemu-system-aarch64 >/dev/null 2>&1; then
    fail "qemu-system-aarch64 is required for the aarch64 virt check"
fi

"${BUILD_SCRIPT}"
mkdir -p "${TMP_DIR}"

timeout -k 2s 6s "${ROOT_DIR}/scripts/run_qemu_aarch64_virt.sh" > "${SERIAL_LOG_FILE}" 2>&1 || true

require_log_line 'tinyOS kernel bootstrap ready\.' "Kernel bootstrap banner reached"
require_log_line 'Architecture: aarch64' "Architecture banner reached"
require_log_line 'Platform: aarch64-virt' "Platform banner reached"
require_log_line 'Interrupts: arch backend ready=yes' "Architecture backend reported ready"
require_log_line '\[event\] heartbeat=' "Event loop heartbeat reached"
require_log_line '\[gui\] Headless GUI loop active\.' "Headless GUI path reported"

printf '== aarch64 virt check passed: %d checks ==\n' "${PASS_COUNT}"
