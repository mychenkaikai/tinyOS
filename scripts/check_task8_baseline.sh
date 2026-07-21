#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_SCRIPT="${ROOT_DIR}/scripts/build_x86_64.sh"
IMAGE_BIN="${ROOT_DIR}/build/x86_64/tinyos-x86_64.img"
OVMF_CODE="/usr/share/OVMF/OVMF_CODE_4M.fd"
OVMF_VARS_TEMPLATE="/usr/share/OVMF/OVMF_VARS_4M.fd"
OVMF_VARS="${ROOT_DIR}/build/x86_64/OVMF_VARS_4M.fd"
VALIDATION_DOC="${ROOT_DIR}/docs/validation/task8-validation-baseline.md"
TASK6_DOC="${ROOT_DIR}/docs/porting/task6-arch-platform-boundary.md"
TASK7_DOC="${ROOT_DIR}/docs/porting/task7-mcu-subset-roadmap.md"

TMP_DIR="${ROOT_DIR}/build/tmp/check-baseline"
SERIAL_LOG_FILE="${TMP_DIR}/qemu-serial.log"
DEBUGCON_LOG_FILE="${TMP_DIR}/qemu-debugcon.log"

PASS_COUNT=0

pass() {
    PASS_COUNT=$((PASS_COUNT + 1))
    printf '[PASS] %s\n' "$1"
}

fail() {
    printf '[FAIL] %s\n' "$1" >&2
    exit 1
}

require_file() {
    local file_path="$1"
    local description="$2"

    if [[ -f "${file_path}" ]]; then
        pass "${description}"
    else
        fail "${description}: missing ${file_path}"
    fi
}

require_log_line() {
    local pattern="$1"
    local description="$2"
    local log_file="$3"

    if grep -Eq "${pattern}" "${log_file}"; then
        pass "${description}"
    else
        printf 'Captured log (%s):\n' "${log_file}" >&2
        sed -n '1,160p' "${log_file}" >&2 || true
        fail "${description}: pattern not found: ${pattern}"
    fi
}

printf '== Task8 baseline check ==\n'

require_file "${TASK6_DOC}" "Task6 porting boundary document present"
require_file "${TASK7_DOC}" "Task7 MCU roadmap document present"
require_file "${VALIDATION_DOC}" "Task8 validation baseline document present"
require_file "${ROOT_DIR}/arch/aarch64/README.md" "ARM64 architecture placeholder present"
require_file "${ROOT_DIR}/arch/riscv64/README.md" "RISC-V architecture placeholder present"
require_file "${ROOT_DIR}/platform/aarch64_virt/README.md" "ARM64 platform placeholder present"
require_file "${ROOT_DIR}/platform/riscv64_virt/README.md" "RISC-V platform placeholder present"

if grep -Fq 'STM32F429I-DISC1' "${TASK7_DOC}"; then
    pass "Task7 roadmap pins the first MCU reference board"
else
    fail "Task7 roadmap must name STM32F429I-DISC1"
fi

printf 'Building x86_64 image...\n'
"${BUILD_SCRIPT}"

require_file "${IMAGE_BIN}" "Bootable x86_64 image produced"

if ! command -v qemu-system-x86_64 >/dev/null 2>&1; then
    fail "qemu-system-x86_64 is required for the Task8 runtime baseline check"
fi

if [[ ! -f "${OVMF_CODE}" || ! -f "${OVMF_VARS_TEMPLATE}" ]]; then
    fail "OVMF firmware files are required for the Task8 runtime baseline check"
fi

mkdir -p "${TMP_DIR}"
: > "${SERIAL_LOG_FILE}"
: > "${DEBUGCON_LOG_FILE}"

if [[ ! -f "${OVMF_VARS}" ]]; then
    cp "${OVMF_VARS_TEMPLATE}" "${OVMF_VARS}"
fi

printf 'Booting QEMU headless and capturing serial log...\n'
set +e
timeout -k 2s 8s qemu-system-x86_64 \
    -machine q35 \
    -drive if=pflash,format=raw,readonly=on,file="${OVMF_CODE}" \
    -drive if=pflash,format=raw,file="${OVMF_VARS}" \
    -drive format=raw,file="${IMAGE_BIN}" \
    -serial "file:${SERIAL_LOG_FILE}" \
    -debugcon "file:${DEBUGCON_LOG_FILE}" \
    -global isa-debugcon.iobase=0x402 \
    -display none \
    -monitor none \
    -no-reboot \
    -no-shutdown \
    > /dev/null 2>&1
QEMU_STATUS=$?
set -e

if [[ "${QEMU_STATUS}" -ne 0 && "${QEMU_STATUS}" -ne 124 ]]; then
    sed -n '1,160p' "${SERIAL_LOG_FILE}" >&2 || true
    fail "QEMU exited unexpectedly with status ${QEMU_STATUS}"
fi

pass "QEMU reached the runtime capture window"

require_log_line 'tinyOS UEFI loader starting\.\.\.' "UEFI loader banner reached" "${SERIAL_LOG_FILE}"
require_log_line 'LPVEABCDKUS' "UEFI loader, kernel handoff and framebuffer demo markers observed" "${DEBUGCON_LOG_FILE}"

printf '== Task8 baseline check passed: %d checks ==\n' "${PASS_COUNT}"
