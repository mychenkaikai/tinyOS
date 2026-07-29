#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_SCRIPT="${ROOT_DIR}/scripts/build_x86_64.sh"
IMAGE_BIN="${ROOT_DIR}/build/x86_64/tinyos-x86_64.img"
OVMF_CODE="/usr/share/OVMF/OVMF_CODE_4M.fd"
OVMF_VARS_TEMPLATE="/usr/share/OVMF/OVMF_VARS_4M.fd"

TMP_DIR="${ROOT_DIR}/build/tmp/check-lvgl-interaction"
SERIAL_LOG_FILE="${TMP_DIR}/qemu-serial.log"
DEBUGCON_LOG_FILE="${TMP_DIR}/qemu-debugcon.log"
QMP_SOCKET="${TMP_DIR}/qmp.sock"
BEFORE_PPM="${TMP_DIR}/before.ppm"
AFTER_PPM="${TMP_DIR}/after.ppm"
CHANGED_PIXELS_FILE="${TMP_DIR}/changed_pixels.txt"
OVMF_VARS="${TMP_DIR}/OVMF_VARS_4M.fd"

PASS_COUNT=0
QEMU_PID=""

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

cleanup() {
    if [[ -n "${QEMU_PID}" ]] && kill -0 "${QEMU_PID}" >/dev/null 2>&1; then
        kill "${QEMU_PID}" >/dev/null 2>&1 || true
        wait "${QEMU_PID}" >/dev/null 2>&1 || true
    fi
}

trap cleanup EXIT

printf '== LVGL interaction check ==\n'

if ! command -v qemu-system-x86_64 >/dev/null 2>&1; then
    fail "qemu-system-x86_64 is required for the LVGL interaction check"
fi

if [[ ! -f "${OVMF_CODE}" || ! -f "${OVMF_VARS_TEMPLATE}" ]]; then
    fail "OVMF firmware files are required for the LVGL interaction check"
fi

mkdir -p "${TMP_DIR}"
: > "${SERIAL_LOG_FILE}"
: > "${DEBUGCON_LOG_FILE}"
rm -f "${QMP_SOCKET}" "${BEFORE_PPM}" "${AFTER_PPM}" "${CHANGED_PIXELS_FILE}"

if [[ ! -f "${IMAGE_BIN}" ]]; then
    "${BUILD_SCRIPT}"
fi

cp "${OVMF_VARS_TEMPLATE}" "${OVMF_VARS}"

qemu-system-x86_64 \
    -machine q35 \
    -drive if=pflash,format=raw,readonly=on,file="${OVMF_CODE}" \
    -drive if=pflash,format=raw,file="${OVMF_VARS}" \
    -drive format=raw,file="${IMAGE_BIN}" \
    -serial "file:${SERIAL_LOG_FILE}" \
    -debugcon "file:${DEBUGCON_LOG_FILE}" \
    -global isa-debugcon.iobase=0x402 \
    -display none \
    -qmp "unix:${QMP_SOCKET},server,nowait" \
    -no-reboot \
    -no-shutdown \
    > /dev/null 2>&1 &
QEMU_PID=$!

for _ in $(seq 1 50); do
    if [[ -S "${QMP_SOCKET}" ]]; then
        pass "QEMU QMP socket became ready"
        break
    fi
    sleep 0.2
done

if [[ ! -S "${QMP_SOCKET}" ]]; then
    fail "QEMU QMP socket did not appear"
fi

for _ in $(seq 1 50); do
    if grep -Eq '\[event\] heartbeat=' "${SERIAL_LOG_FILE}"; then
        pass "Kernel heartbeat reached the serial log"
        break
    fi
    sleep 0.2
done

if ! grep -Eq '\[event\] heartbeat=' "${SERIAL_LOG_FILE}"; then
    fail "Kernel heartbeat did not appear before interaction"
fi

python3 - "${QMP_SOCKET}" "${BEFORE_PPM}" "${AFTER_PPM}" <<'PY'
import json
import socket
import sys
import time

sock_path, before_ppm, after_ppm = sys.argv[1:4]

def recv_message(sock):
    data = b""
    while b"\n" not in data:
        chunk = sock.recv(4096)
        if not chunk:
            break
        data += chunk
    if not data:
        raise RuntimeError("no QMP response received")
    return json.loads(data.decode("utf-8"))

def execute_hmp(sock, command):
    payload = {
        "execute": "human-monitor-command",
        "arguments": {
            "command-line": command,
        },
    }
    sock.sendall(json.dumps(payload).encode("utf-8") + b"\n")
    return recv_message(sock)

sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
sock.settimeout(5.0)
sock.connect(sock_path)
recv_message(sock)
sock.sendall(json.dumps({"execute": "qmp_capabilities"}).encode("utf-8") + b"\n")
recv_message(sock)

execute_hmp(sock, f"screendump {before_ppm}")
time.sleep(0.5)
execute_hmp(sock, "sendkey 3")
time.sleep(0.3)
execute_hmp(sock, "sendkey ret")
time.sleep(1.2)
execute_hmp(sock, f"screendump {after_ppm}")
sock.close()
PY
pass "QMP interaction completed"

for _ in $(seq 1 30); do
    if grep -Eq '\[input\].*char=3' "${SERIAL_LOG_FILE}" && grep -Eq '\[input\].*char=enter' "${SERIAL_LOG_FILE}"; then
        pass "Injected keys reached the kernel input log"
        break
    fi
    sleep 0.2
done

require_log_line 'tinyOS UEFI loader starting\.\.\.' "UEFI loader banner reached" "${SERIAL_LOG_FILE}"
require_log_line '\[event\] heartbeat=' "Kernel event loop stayed live during UI interaction" "${SERIAL_LOG_FILE}"
require_log_line '\[input\].*char=3' "Digit hotkey reached the GUI input path" "${SERIAL_LOG_FILE}"
require_log_line '\[input\].*char=enter' "Enter key reached the GUI input path" "${SERIAL_LOG_FILE}"
require_log_line 'LPVEABCDKUS' "UEFI loader and kernel handoff markers observed" "${DEBUGCON_LOG_FILE}"

python3 - "${BEFORE_PPM}" "${AFTER_PPM}" "${CHANGED_PIXELS_FILE}" <<'PY'
import sys

before_path, after_path, output_path = sys.argv[1:4]

def read_ppm(path):
    with open(path, "rb") as handle:
        if handle.readline().strip() != b"P6":
            raise RuntimeError(f"{path}: unsupported PPM format")
        line = handle.readline()
        while line.startswith(b"#"):
            line = handle.readline()
        width, height = map(int, line.split())
        max_value = int(handle.readline().strip())
        payload = handle.read()
        return width, height, max_value, payload

before = read_ppm(before_path)
after = read_ppm(after_path)
if before[:3] != after[:3]:
    raise RuntimeError("screen captures use different dimensions")

changed_pixels = 0
for index in range(0, len(before[3]), 3):
    if before[3][index:index + 3] != after[3][index:index + 3]:
        changed_pixels += 1

with open(output_path, "w", encoding="utf-8") as handle:
    handle.write(f"{changed_pixels}\n")

if changed_pixels < 100:
    raise SystemExit(2)
PY

PIXELS_CHANGED="$(cat "${CHANGED_PIXELS_FILE}")"
pass "Frame buffer changed after injected interaction (${PIXELS_CHANGED} pixels)"

printf '== LVGL interaction check passed: %d checks ==\n' "${PASS_COUNT}"
