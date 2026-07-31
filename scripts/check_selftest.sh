#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

run_step() {
    local label="$1"
    shift

    printf '== %s ==\n' "${label}"
    "$@"
}

maybe_run_qemu_check() {
    local compiler="$1"
    local runtime="$2"
    local label="$3"
    shift 3

    if command -v "${compiler}" >/dev/null 2>&1 && command -v "${runtime}" >/dev/null 2>&1; then
        run_step "${label}" "$@"
    else
        printf '[skip] %s (missing %s or %s)\n' "${label}" "${compiler}" "${runtime}"
    fi
}

run_step "x86 UI regression" make -C "${ROOT_DIR}" check-ui
run_step "x86 baseline regression" make -C "${ROOT_DIR}" check-baseline
run_step "multiarch host smoke" make -C "${ROOT_DIR}" check-multiarch-host
run_step "multiarch preflight" make -C "${ROOT_DIR}" check-multiarch-preflight
maybe_run_qemu_check aarch64-linux-gnu-gcc qemu-system-aarch64 "aarch64 qemu smoke" make -C "${ROOT_DIR}" check-aarch64
maybe_run_qemu_check riscv64-linux-gnu-gcc qemu-system-riscv64 "riscv64 qemu smoke" make -C "${ROOT_DIR}" check-riscv64
