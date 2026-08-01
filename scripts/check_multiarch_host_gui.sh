#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build/host_multiarch_smoke"
BIN_PATH="${BUILD_DIR}/host-multiarch-smoke"
LOG_AARCH64_HOME="${BUILD_DIR}/aarch64-gui-home.log"
LOG_AARCH64_SETTINGS="${BUILD_DIR}/aarch64-gui-settings.log"
LOG_AARCH64_ABOUT="${BUILD_DIR}/aarch64-gui-about.log"
LOG_RISCV64_HOME="${BUILD_DIR}/riscv64-gui-home.log"
LOG_RISCV64_SETTINGS="${BUILD_DIR}/riscv64-gui-settings.log"
LOG_RISCV64_ABOUT="${BUILD_DIR}/riscv64-gui-about.log"

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
        sed -n '1,260p' "${log_file}" >&2 || true
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

"${BIN_PATH}" aarch64 gui home-details > "${LOG_AARCH64_HOME}" 2>&1
"${BIN_PATH}" aarch64 gui settings-clear > "${LOG_AARCH64_SETTINGS}" 2>&1
"${BIN_PATH}" aarch64 gui about-notes > "${LOG_AARCH64_ABOUT}" 2>&1
"${BIN_PATH}" riscv64 gui home-details > "${LOG_RISCV64_HOME}" 2>&1
"${BIN_PATH}" riscv64 gui settings-clear > "${LOG_RISCV64_SETTINGS}" 2>&1
"${BIN_PATH}" riscv64 gui about-notes > "${LOG_RISCV64_ABOUT}" 2>&1

require_log_line 'Architecture: aarch64' "AArch64 home scenario reported architecture" "${LOG_AARCH64_HOME}"
require_log_line '\[input\] key#=1 .*char=1' "AArch64 home scenario processed HOME hotkey" "${LOG_AARCH64_HOME}"
require_log_line '\[input\] key#=2 .*char=d' "AArch64 home scenario processed details hotkey" "${LOG_AARCH64_HOME}"
require_log_line '\[display\] row=01 text=\| Page: HOME' "AArch64 home scenario stayed on the home page" "${LOG_AARCH64_HOME}"
require_log_line '\[display\] row=03 text=\| >Home<    \[Settings\]     \[About\]   \[Clear Input\]' "AArch64 home scenario focused the home action" "${LOG_AARCH64_HOME}"
require_log_line '\[display\] row=10 text=\| Status       Desktop detail cards enabled\.' "AArch64 home scenario enabled detail cards" "${LOG_AARCH64_HOME}"
require_log_line '\[display\] row=21 text=\| detail: timer ticks keep refreshing the runtime panel' "AArch64 home scenario rendered the detail panel" "${LOG_AARCH64_HOME}"
require_log_line '\[host-smoke\] completed arch=aarch64 profile=gui scenario=home-details ticks=240 events=2' "AArch64 home scenario completed run" "${LOG_AARCH64_HOME}"

require_log_line 'Architecture: aarch64' "AArch64 settings scenario reported architecture" "${LOG_AARCH64_SETTINGS}"
require_log_line 'Platform: aarch64-host-smoke' "AArch64 settings scenario reported platform" "${LOG_AARCH64_SETTINGS}"
require_log_line '\[input\] key#=1 .*char=3' "AArch64 settings scenario processed ABOUT hotkey" "${LOG_AARCH64_SETTINGS}"
require_log_line '\[input\] key#=2 .*char=i' "AArch64 settings scenario processed ABOUT toggle hotkey" "${LOG_AARCH64_SETTINGS}"
require_log_line '\[input\] key#=3 .*char=2' "AArch64 settings scenario processed SETTINGS hotkey" "${LOG_AARCH64_SETTINGS}"
require_log_line '\[input\] key#=4 .*char=e' "AArch64 settings scenario processed settings toggle hotkey" "${LOG_AARCH64_SETTINGS}"
require_log_line '\[input\] key#=6 .*char=0' "AArch64 settings scenario processed clear hotkey" "${LOG_AARCH64_SETTINGS}"
require_log_line '\[display\] row=01 text=\| Page: SETTINGS' "AArch64 settings scenario switched to the settings page" "${LOG_AARCH64_SETTINGS}"
require_log_line '\[display\] row=02 text=\| Tab switch focus  Enter activate  123 pages  0 clear  D/E/I toggle' "AArch64 settings scenario rendered direct hotkey help" "${LOG_AARCH64_SETTINGS}"
require_log_line '\[display\] row=03 text=\| \[Home\]    \[Settings\]     \[About\]   >Clear Input<' "AArch64 settings scenario focused the clear action" "${LOG_AARCH64_SETTINGS}"
require_log_line '\[display\] row=10 text=\| Status       Demo input cleared\.' "AArch64 settings scenario reported clear action" "${LOG_AARCH64_SETTINGS}"
require_log_line '\[display\] row=18 text=\| Key echo: disabled' "AArch64 settings scenario toggled key echo off" "${LOG_AARCH64_SETTINGS}"
require_log_line '\[host-smoke\] completed arch=aarch64 profile=gui scenario=settings-clear ticks=240 events=6' "AArch64 settings scenario completed run" "${LOG_AARCH64_SETTINGS}"

require_log_line 'Architecture: aarch64' "AArch64 about scenario reported architecture" "${LOG_AARCH64_ABOUT}"
require_log_line '\[input\] key#=1 .*char=3' "AArch64 about scenario processed ABOUT hotkey" "${LOG_AARCH64_ABOUT}"
require_log_line '\[input\] key#=2 .*char=i' "AArch64 about scenario processed notes hotkey" "${LOG_AARCH64_ABOUT}"
require_log_line '\[display\] row=01 text=\| Page: ABOUT' "AArch64 about scenario switched to the about page" "${LOG_AARCH64_ABOUT}"
require_log_line '\[display\] row=03 text=\| \[Home\]    \[Settings\]     >About<   \[Clear Input\]' "AArch64 about scenario focused the about action" "${LOG_AARCH64_ABOUT}"
require_log_line '\[display\] row=10 text=\| Status       About notes enabled\.' "AArch64 about scenario reported notes enablement" "${LOG_AARCH64_ABOUT}"
require_log_line '\[display\] row=18 text=\| Notes:  enabled' "AArch64 about scenario rendered enabled notes state" "${LOG_AARCH64_ABOUT}"
require_log_line '\[display\] row=21 text=\| note: host smoke validates the screen contents' "AArch64 about scenario rendered host validation note" "${LOG_AARCH64_ABOUT}"
require_log_line '\[host-smoke\] completed arch=aarch64 profile=gui scenario=about-notes ticks=240 events=2' "AArch64 about scenario completed run" "${LOG_AARCH64_ABOUT}"

require_log_line 'Architecture: riscv64' "RISC-V home scenario reported architecture" "${LOG_RISCV64_HOME}"
require_log_line '\[input\] key#=1 .*char=1' "RISC-V home scenario processed HOME hotkey" "${LOG_RISCV64_HOME}"
require_log_line '\[input\] key#=2 .*char=d' "RISC-V home scenario processed details hotkey" "${LOG_RISCV64_HOME}"
require_log_line '\[display\] row=01 text=\| Page: HOME' "RISC-V home scenario stayed on the home page" "${LOG_RISCV64_HOME}"
require_log_line '\[display\] row=03 text=\| >Home<    \[Settings\]     \[About\]   \[Clear Input\]' "RISC-V home scenario focused the home action" "${LOG_RISCV64_HOME}"
require_log_line '\[display\] row=10 text=\| Status       Desktop detail cards enabled\.' "RISC-V home scenario enabled detail cards" "${LOG_RISCV64_HOME}"
require_log_line '\[display\] row=21 text=\| detail: timer ticks keep refreshing the runtime panel' "RISC-V home scenario rendered the detail panel" "${LOG_RISCV64_HOME}"
require_log_line '\[host-smoke\] completed arch=riscv64 profile=gui scenario=home-details ticks=240 events=2' "RISC-V home scenario completed run" "${LOG_RISCV64_HOME}"

require_log_line 'Architecture: riscv64' "RISC-V settings scenario reported architecture" "${LOG_RISCV64_SETTINGS}"
require_log_line 'Platform: riscv64-host-smoke' "RISC-V settings scenario reported platform" "${LOG_RISCV64_SETTINGS}"
require_log_line '\[input\] key#=1 .*char=3' "RISC-V settings scenario processed ABOUT hotkey" "${LOG_RISCV64_SETTINGS}"
require_log_line '\[input\] key#=2 .*char=i' "RISC-V settings scenario processed ABOUT toggle hotkey" "${LOG_RISCV64_SETTINGS}"
require_log_line '\[input\] key#=3 .*char=2' "RISC-V settings scenario processed SETTINGS hotkey" "${LOG_RISCV64_SETTINGS}"
require_log_line '\[input\] key#=4 .*char=e' "RISC-V settings scenario processed settings toggle hotkey" "${LOG_RISCV64_SETTINGS}"
require_log_line '\[input\] key#=6 .*char=0' "RISC-V settings scenario processed clear hotkey" "${LOG_RISCV64_SETTINGS}"
require_log_line '\[display\] row=01 text=\| Page: SETTINGS' "RISC-V settings scenario switched to the settings page" "${LOG_RISCV64_SETTINGS}"
require_log_line '\[display\] row=02 text=\| Tab switch focus  Enter activate  123 pages  0 clear  D/E/I toggle' "RISC-V settings scenario rendered direct hotkey help" "${LOG_RISCV64_SETTINGS}"
require_log_line '\[display\] row=03 text=\| \[Home\]    \[Settings\]     \[About\]   >Clear Input<' "RISC-V settings scenario focused the clear action" "${LOG_RISCV64_SETTINGS}"
require_log_line '\[display\] row=10 text=\| Status       Demo input cleared\.' "RISC-V settings scenario reported clear action" "${LOG_RISCV64_SETTINGS}"
require_log_line '\[display\] row=18 text=\| Key echo: disabled' "RISC-V settings scenario toggled key echo off" "${LOG_RISCV64_SETTINGS}"
require_log_line '\[host-smoke\] completed arch=riscv64 profile=gui scenario=settings-clear ticks=240 events=6' "RISC-V settings scenario completed run" "${LOG_RISCV64_SETTINGS}"

require_log_line 'Architecture: riscv64' "RISC-V about scenario reported architecture" "${LOG_RISCV64_ABOUT}"
require_log_line '\[input\] key#=1 .*char=3' "RISC-V about scenario processed ABOUT hotkey" "${LOG_RISCV64_ABOUT}"
require_log_line '\[input\] key#=2 .*char=i' "RISC-V about scenario processed notes hotkey" "${LOG_RISCV64_ABOUT}"
require_log_line '\[display\] row=01 text=\| Page: ABOUT' "RISC-V about scenario switched to the about page" "${LOG_RISCV64_ABOUT}"
require_log_line '\[display\] row=03 text=\| \[Home\]    \[Settings\]     >About<   \[Clear Input\]' "RISC-V about scenario focused the about action" "${LOG_RISCV64_ABOUT}"
require_log_line '\[display\] row=10 text=\| Status       About notes enabled\.' "RISC-V about scenario reported notes enablement" "${LOG_RISCV64_ABOUT}"
require_log_line '\[display\] row=18 text=\| Notes:  enabled' "RISC-V about scenario rendered enabled notes state" "${LOG_RISCV64_ABOUT}"
require_log_line '\[display\] row=21 text=\| note: host smoke validates the screen contents' "RISC-V about scenario rendered host validation note" "${LOG_RISCV64_ABOUT}"
require_log_line '\[host-smoke\] completed arch=riscv64 profile=gui scenario=about-notes ticks=240 events=2' "RISC-V about scenario completed run" "${LOG_RISCV64_ABOUT}"

printf '== multiarch host gui smoke passed: %d checks ==\n' "${PASS_COUNT}"
