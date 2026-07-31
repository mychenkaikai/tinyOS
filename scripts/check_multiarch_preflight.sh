#!/usr/bin/env bash
set -euo pipefail

pass() {
    printf '[ready] %s\n' "$1"
}

warn() {
    printf '[missing] %s\n' "$1"
}

check_tool() {
    local tool="$1"
    local description="$2"

    if command -v "${tool}" >/dev/null 2>&1; then
        pass "${description}: ${tool}"
    else
        warn "${description}: ${tool}"
    fi
}

printf '== multiarch qemu preflight ==\n'

check_tool qemu-system-x86_64 "x86_64 qemu runtime"
check_tool qemu-system-aarch64 "aarch64 qemu runtime"
check_tool qemu-system-riscv64 "riscv64 qemu runtime"
check_tool aarch64-linux-gnu-gcc "aarch64 cross compiler"
check_tool riscv64-linux-gnu-gcc "riscv64 cross compiler"
check_tool gcc "host compiler for shared-kernel smoke checks"

printf '== multiarch qemu preflight complete ==\n'
