#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build/host_multiarch_smoke"
BIN_PATH="${BUILD_DIR}/host-multiarch-smoke"
LOG_AARCH64="${BUILD_DIR}/aarch64-gui.log"
LOG_RISCV64="${BUILD_DIR}/riscv64-gui.log"

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
        sed -n '1,240p' "${log_file}" >&2 || true
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

"${BIN_PATH}" aarch64 gui > "${LOG_AARCH64}" 2>&1
"${BIN_PATH}" riscv64 gui > "${LOG_RISCV64}" 2>&1

require_log_line 'Architecture: aarch64' "AArch64 host GUI smoke reported architecture" "${LOG_AARCH64}"
require_log_line 'Platform: aarch64-host-smoke' "AArch64 host GUI smoke reported platform" "${LOG_AARCH64}"
require_log_line '\[input\] key#=1 .*char=3' "AArch64 host GUI processed ABOUT hotkey" "${LOG_AARCH64}"
require_log_line '\[input\] key#=2 .*char=i' "AArch64 host GUI processed ABOUT toggle hotkey" "${LOG_AARCH64}"
require_log_line '\[input\] key#=3 .*char=2' "AArch64 host GUI processed SETTINGS hotkey" "${LOG_AARCH64}"
require_log_line '\[input\] key#=4 .*char=e' "AArch64 host GUI processed settings toggle hotkey" "${LOG_AARCH64}"
require_log_line '\[input\] key#=6 .*char=0' "AArch64 host GUI processed clear hotkey" "${LOG_AARCH64}"
require_log_line '\[display\] row=00 text=\+- tinyOS GUI MVP' "AArch64 host GUI drew the main frame" "${LOG_AARCH64}"
require_log_line '\[display\] row=01 text=\| Page: SETTINGS' "AArch64 host GUI switched to the settings page" "${LOG_AARCH64}"
require_log_line '\[display\] row=02 text=\| Tab switch focus  Enter activate  123 pages  0 clear  D/E/I toggle' "AArch64 host GUI rendered direct hotkey help" "${LOG_AARCH64}"
require_log_line '\[display\] row=03 text=\| \[Home\]    \[Settings\]     \[About\]   >Clear Input<' "AArch64 host GUI focused the clear action" "${LOG_AARCH64}"
require_log_line '\[display\] row=10 text=\| Status       Demo input cleared\.' "AArch64 host GUI reported clear action" "${LOG_AARCH64}"
require_log_line '\[display\] row=17 text=\| Use >Settings< \+ Enter to toggle a live GUI flag\.' "AArch64 host GUI rendered settings content" "${LOG_AARCH64}"
require_log_line '\[display\] row=18 text=\| Key echo: disabled' "AArch64 host GUI toggled key echo off" "${LOG_AARCH64}"
require_log_line '\[display\] row=14 text=\| Chars: 0  /32' "AArch64 host GUI cleared the input box" "${LOG_AARCH64}"
require_log_line '\[host-smoke\] completed arch=aarch64 profile=gui ticks=240 events=6' "AArch64 host GUI smoke completed run" "${LOG_AARCH64}"

require_log_line 'Architecture: riscv64' "RISC-V host GUI smoke reported architecture" "${LOG_RISCV64}"
require_log_line 'Platform: riscv64-host-smoke' "RISC-V host GUI smoke reported platform" "${LOG_RISCV64}"
require_log_line '\[input\] key#=1 .*char=3' "RISC-V host GUI processed ABOUT hotkey" "${LOG_RISCV64}"
require_log_line '\[input\] key#=2 .*char=i' "RISC-V host GUI processed ABOUT toggle hotkey" "${LOG_RISCV64}"
require_log_line '\[input\] key#=3 .*char=2' "RISC-V host GUI processed SETTINGS hotkey" "${LOG_RISCV64}"
require_log_line '\[input\] key#=4 .*char=e' "RISC-V host GUI processed settings toggle hotkey" "${LOG_RISCV64}"
require_log_line '\[input\] key#=6 .*char=0' "RISC-V host GUI processed clear hotkey" "${LOG_RISCV64}"
require_log_line '\[display\] row=00 text=\+- tinyOS GUI MVP' "RISC-V host GUI drew the main frame" "${LOG_RISCV64}"
require_log_line '\[display\] row=01 text=\| Page: SETTINGS' "RISC-V host GUI switched to the settings page" "${LOG_RISCV64}"
require_log_line '\[display\] row=02 text=\| Tab switch focus  Enter activate  123 pages  0 clear  D/E/I toggle' "RISC-V host GUI rendered direct hotkey help" "${LOG_RISCV64}"
require_log_line '\[display\] row=03 text=\| \[Home\]    \[Settings\]     \[About\]   >Clear Input<' "RISC-V host GUI focused the clear action" "${LOG_RISCV64}"
require_log_line '\[display\] row=10 text=\| Status       Demo input cleared\.' "RISC-V host GUI reported clear action" "${LOG_RISCV64}"
require_log_line '\[display\] row=17 text=\| Use >Settings< \+ Enter to toggle a live GUI flag\.' "RISC-V host GUI rendered settings content" "${LOG_RISCV64}"
require_log_line '\[display\] row=18 text=\| Key echo: disabled' "RISC-V host GUI toggled key echo off" "${LOG_RISCV64}"
require_log_line '\[display\] row=14 text=\| Chars: 0  /32' "RISC-V host GUI cleared the input box" "${LOG_RISCV64}"
require_log_line '\[host-smoke\] completed arch=riscv64 profile=gui ticks=240 events=6' "RISC-V host GUI smoke completed run" "${LOG_RISCV64}"

printf '== multiarch host gui smoke passed: %d checks ==\n' "${PASS_COUNT}"
