#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build/host_multiarch_smoke"
BIN_PATH="${BUILD_DIR}/host-multiarch-smoke"
LOG_AARCH64="${BUILD_DIR}/aarch64.log"
LOG_RISCV64="${BUILD_DIR}/riscv64.log"

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
    local log_file="$3"

    if grep -Eq "${pattern}" "${log_file}"; then
        pass "${description}"
    else
        printf 'Captured log (%s):\n' "${log_file}" >&2
        sed -n '1,220p' "${log_file}" >&2 || true
        fail "${description}: pattern not found: ${pattern}"
    fi
}

mkdir -p "${BUILD_DIR}"

gcc \
    -Wall -Wextra -Werror \
    -DTINYOS_HOSTED_TEST \
    -I"${ROOT_DIR}/include" \
    "${ROOT_DIR}/tests/host_multiarch_smoke.c" \
    "${ROOT_DIR}/src/kernel/console.c" \
    "${ROOT_DIR}/src/kernel/display.c" \
    "${ROOT_DIR}/src/kernel/gui.c" \
    "${ROOT_DIR}/src/kernel/gui_uefi_stub.c" \
    "${ROOT_DIR}/src/kernel/input.c" \
    "${ROOT_DIR}/src/kernel/main.c" \
    "${ROOT_DIR}/src/kernel/uefi_boot_demo.c" \
    "${ROOT_DIR}/tests/host_memory.c" \
    -o "${BIN_PATH}"

"${BIN_PATH}" aarch64 > "${LOG_AARCH64}" 2>&1
"${BIN_PATH}" riscv64 > "${LOG_RISCV64}" 2>&1

require_log_line 'tinyOS kernel bootstrap ready\.' "AArch64 host smoke reached kernel banner" "${LOG_AARCH64}"
require_log_line 'Architecture: aarch64' "AArch64 host smoke reported architecture" "${LOG_AARCH64}"
require_log_line 'Platform: aarch64-host-smoke' "AArch64 host smoke reported platform" "${LOG_AARCH64}"
require_log_line 'Boot path: host-smoke->kernel_main' "AArch64 host smoke reported boot path" "${LOG_AARCH64}"
require_log_line 'Input: platform input backend attached' "AArch64 host smoke reported input backend" "${LOG_AARCH64}"
require_log_line '\[input\] key#=1 .*char=tab' "AArch64 host smoke processed tab input" "${LOG_AARCH64}"
require_log_line '\[input\] key#=2 .*char=enter' "AArch64 host smoke processed enter input" "${LOG_AARCH64}"
require_log_line '\[input\] key#=3 .*char=x' "AArch64 host smoke processed printable input" "${LOG_AARCH64}"
require_log_line '\[event\] heartbeat=1 ' "AArch64 host smoke reached heartbeat" "${LOG_AARCH64}"
require_log_line '\[host-smoke\] completed arch=aarch64 ticks=240 events=3' "AArch64 host smoke completed run" "${LOG_AARCH64}"

require_log_line 'tinyOS kernel bootstrap ready\.' "RISC-V host smoke reached kernel banner" "${LOG_RISCV64}"
require_log_line 'Architecture: riscv64' "RISC-V host smoke reported architecture" "${LOG_RISCV64}"
require_log_line 'Platform: riscv64-host-smoke' "RISC-V host smoke reported platform" "${LOG_RISCV64}"
require_log_line 'Boot path: host-smoke->kernel_main' "RISC-V host smoke reported boot path" "${LOG_RISCV64}"
require_log_line 'Input: platform input backend attached' "RISC-V host smoke reported input backend" "${LOG_RISCV64}"
require_log_line '\[input\] key#=1 .*char=tab' "RISC-V host smoke processed tab input" "${LOG_RISCV64}"
require_log_line '\[input\] key#=2 .*char=enter' "RISC-V host smoke processed enter input" "${LOG_RISCV64}"
require_log_line '\[input\] key#=3 .*char=x' "RISC-V host smoke processed printable input" "${LOG_RISCV64}"
require_log_line '\[event\] heartbeat=1 ' "RISC-V host smoke reached heartbeat" "${LOG_RISCV64}"
require_log_line '\[host-smoke\] completed arch=riscv64 ticks=240 events=3' "RISC-V host smoke completed run" "${LOG_RISCV64}"

printf '== multiarch host smoke passed: %d checks ==\n' "${PASS_COUNT}"
