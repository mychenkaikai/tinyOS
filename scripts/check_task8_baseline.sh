#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_SCRIPT="${ROOT_DIR}/scripts/build_x86_64.sh"
IMAGE_BIN="${ROOT_DIR}/build/x86_64/tinyos-x86_64.img"
VALIDATION_DOC="${ROOT_DIR}/docs/validation/task8-validation-baseline.md"
TASK6_DOC="${ROOT_DIR}/docs/porting/task6-arch-platform-boundary.md"
TASK7_DOC="${ROOT_DIR}/docs/porting/task7-mcu-subset-roadmap.md"

TMP_DIR="$(mktemp -d)"
LOG_FILE="${TMP_DIR}/qemu-serial.log"

PASS_COUNT=0

cleanup() {
    rm -rf "${TMP_DIR}"
}
trap cleanup EXIT

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

    if grep -Eq "${pattern}" "${LOG_FILE}"; then
        pass "${description}"
    else
        printf 'Captured serial log:\n' >&2
        sed -n '1,160p' "${LOG_FILE}" >&2 || true
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

printf 'Booting QEMU headless and capturing serial log...\n'
set +e
timeout 8s qemu-system-x86_64 \
    -drive format=raw,file="${IMAGE_BIN}" \
    -serial stdio \
    -display none \
    -monitor none \
    -no-reboot \
    -no-shutdown \
    > "${LOG_FILE}" 2>&1
QEMU_STATUS=$?
set -e

if [[ "${QEMU_STATUS}" -ne 0 && "${QEMU_STATUS}" -ne 124 ]]; then
    sed -n '1,160p' "${LOG_FILE}" >&2 || true
    fail "QEMU exited unexpectedly with status ${QEMU_STATUS}"
fi

pass "QEMU reached the runtime capture window"

require_log_line 'tinyOS x86_64 bootstrap ready\.' "Kernel banner reached"
require_log_line 'Phase: Task5 text-mode GUI MVP ready\.' "Task5 phase banner reached"
require_log_line 'Platform: x86_64-qemu' "Platform binding reported"
require_log_line 'Display: platform-agnostic interface -> x86_64 VGA text backend \+ positioned draw ops\.' "Display backend reported"
require_log_line 'Input: platform-agnostic interface -> x86_64 PS/2 keyboard backend\.' "Input backend reported"
require_log_line 'Early heap: start=0x[0-9A-F]+' "Early heap initialization reported"
require_log_line 'Interrupts: arch backend ready=yes' "Interrupt backend ready"
require_log_line '\[gui\] Task5 MVP active\. Screen owned by GUI, logs continue on serial\.' "GUI ownership reported"
require_log_line 'Event loop: heartbeat, input and gui tasks armed, enabling interrupts\.' "Event loop arming reported"
require_log_line '\[event\] heartbeat=[0-9]+ ticks=[0-9]+ heap_used=[0-9]+' "Heartbeat log observed"

printf '== Task8 baseline check passed: %d checks ==\n' "${PASS_COUNT}"
