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
HOME_DETAILS_ON_PPM="${TMP_DIR}/home-details-on.ppm"
HOME_DETAILS_OFF_PPM="${TMP_DIR}/home-details-off.ppm"
SETTINGS_PPM="${TMP_DIR}/settings.ppm"
SETTINGS_OFF_PPM="${TMP_DIR}/settings-off.ppm"
SETTINGS_BLOCKED_PPM="${TMP_DIR}/settings-blocked.ppm"
SETTINGS_ON_PPM="${TMP_DIR}/settings-on.ppm"
ABOUT_PPM="${TMP_DIR}/about.ppm"
ABOUT_NOTES_ON_PPM="${TMP_DIR}/about-notes-on.ppm"
ABOUT_NOTES_OFF_PPM="${TMP_DIR}/about-notes-off.ppm"
ABOUT_INPUT_PPM="${TMP_DIR}/about-input.ppm"
CLEAR_PPM="${TMP_DIR}/clear.ppm"
HOME_RETURN_PPM="${TMP_DIR}/home-return.ppm"
PIXEL_DIFFS_FILE="${TMP_DIR}/pixel_diffs.txt"
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
rm -f \
    "${QMP_SOCKET}" \
    "${BEFORE_PPM}" \
    "${HOME_DETAILS_ON_PPM}" \
    "${HOME_DETAILS_OFF_PPM}" \
    "${SETTINGS_PPM}" \
    "${SETTINGS_OFF_PPM}" \
    "${SETTINGS_BLOCKED_PPM}" \
    "${SETTINGS_ON_PPM}" \
    "${ABOUT_PPM}" \
    "${ABOUT_NOTES_ON_PPM}" \
    "${ABOUT_NOTES_OFF_PPM}" \
    "${ABOUT_INPUT_PPM}" \
    "${CLEAR_PPM}" \
    "${HOME_RETURN_PPM}" \
    "${PIXEL_DIFFS_FILE}"

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

python3 - "${QMP_SOCKET}" "${BEFORE_PPM}" "${HOME_DETAILS_ON_PPM}" "${HOME_DETAILS_OFF_PPM}" "${SETTINGS_PPM}" "${SETTINGS_OFF_PPM}" "${SETTINGS_BLOCKED_PPM}" "${SETTINGS_ON_PPM}" "${ABOUT_PPM}" "${ABOUT_NOTES_ON_PPM}" "${ABOUT_NOTES_OFF_PPM}" "${ABOUT_INPUT_PPM}" "${CLEAR_PPM}" "${HOME_RETURN_PPM}" <<'PY'
import json
import socket
import sys
import time

sock_path, before_ppm, home_details_on_ppm, home_details_off_ppm, settings_ppm, settings_off_ppm, settings_blocked_ppm, settings_on_ppm, about_ppm, about_notes_on_ppm, about_notes_off_ppm, about_input_ppm, clear_ppm, home_return_ppm = sys.argv[1:15]

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

def send_key(sock, key_name, delay_seconds):
    execute_hmp(sock, f"sendkey {key_name}")
    time.sleep(delay_seconds)

sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
sock.settimeout(5.0)
sock.connect(sock_path)
recv_message(sock)
sock.sendall(json.dumps({"execute": "qmp_capabilities"}).encode("utf-8") + b"\n")
recv_message(sock)

execute_hmp(sock, f"screendump {before_ppm}")
time.sleep(0.5)
send_key(sock, "ret", 0.8)
execute_hmp(sock, f"screendump {home_details_on_ppm}")
time.sleep(0.4)
send_key(sock, "d", 0.8)
execute_hmp(sock, f"screendump {home_details_off_ppm}")
time.sleep(0.4)
send_key(sock, "tab", 0.4)
send_key(sock, "ret", 1.0)
execute_hmp(sock, f"screendump {settings_ppm}")
time.sleep(0.4)
send_key(sock, "ret", 0.8)
execute_hmp(sock, f"screendump {settings_off_ppm}")
time.sleep(0.4)
send_key(sock, "x", 0.6)
execute_hmp(sock, f"screendump {settings_blocked_ppm}")
time.sleep(0.4)
send_key(sock, "ret", 1.0)
execute_hmp(sock, f"screendump {settings_on_ppm}")
time.sleep(0.4)
send_key(sock, "tab", 0.4)
send_key(sock, "ret", 1.0)
execute_hmp(sock, f"screendump {about_ppm}")
time.sleep(0.4)
send_key(sock, "ret", 0.8)
execute_hmp(sock, f"screendump {about_notes_on_ppm}")
time.sleep(0.4)
send_key(sock, "i", 0.8)
execute_hmp(sock, f"screendump {about_notes_off_ppm}")
time.sleep(0.4)
send_key(sock, "x", 0.6)
execute_hmp(sock, f"screendump {about_input_ppm}")
time.sleep(0.4)
send_key(sock, "tab", 0.4)
send_key(sock, "ret", 1.0)
execute_hmp(sock, f"screendump {clear_ppm}")
time.sleep(0.4)
send_key(sock, "1", 0.8)
execute_hmp(sock, f"screendump {home_return_ppm}")
sock.close()
PY
pass "QMP interaction completed"

for _ in $(seq 1 30); do
    if grep -Eq '\[lvgl\] action=HOME page=HOME focus=HOME_TOGGLE .*status=HOME PAGE ACTIVE' "${SERIAL_LOG_FILE}"; then
        pass "LVGL state-machine logs reached the serial log"
        break
    fi
    sleep 0.2
done

for _ in $(seq 1 30); do
    if grep -Eq '\[input\].*char=tab' "${SERIAL_LOG_FILE}" && \
       grep -Eq '\[input\].*char=enter' "${SERIAL_LOG_FILE}" && \
       grep -Eq '\[input\].*char=d' "${SERIAL_LOG_FILE}" && \
       grep -Eq '\[input\].*char=i' "${SERIAL_LOG_FILE}" && \
       grep -Eq '\[input\].*char=x' "${SERIAL_LOG_FILE}" && \
       grep -Eq '\[input\].*char=1' "${SERIAL_LOG_FILE}"; then
        pass "Injected keys reached the kernel input log"
        break
    fi
    sleep 0.2
done

require_log_line 'tinyOS UEFI loader starting\.\.\.' "UEFI loader banner reached" "${SERIAL_LOG_FILE}"
require_log_line '\[event\] heartbeat=' "Kernel event loop stayed live during UI interaction" "${SERIAL_LOG_FILE}"
require_log_line '\[input\].*char=tab' "Tab key reached the GUI input path" "${SERIAL_LOG_FILE}"
require_log_line '\[input\].*char=enter' "Enter key reached the GUI input path" "${SERIAL_LOG_FILE}"
require_log_line '\[input\].*char=d' "Home details hotkey reached the GUI input path" "${SERIAL_LOG_FILE}"
require_log_line '\[input\].*char=i' "About notes hotkey reached the GUI input path" "${SERIAL_LOG_FILE}"
require_log_line '\[input\].*char=x' "Printable key reached the GUI input path" "${SERIAL_LOG_FILE}"
require_log_line '\[input\].*char=1' "Direct page hotkey reached the GUI input path" "${SERIAL_LOG_FILE}"
require_log_line '\[lvgl\] action=TOGGLE_HOME_DETAILS page=HOME focus=HOME_TOGGLE .*status=DETAIL MODE ENABLED' "HOME toggle enabled detail mode" "${SERIAL_LOG_FILE}"
require_log_line '\[lvgl\] action=TOGGLE_HOME_DETAILS page=HOME focus=HOME_TOGGLE .*status=DETAIL MODE DISABLED' "HOME toggle disabled detail mode" "${SERIAL_LOG_FILE}"
require_log_line '\[lvgl\] focus=SETTINGS page=HOME focus=SETTINGS .*status=FOCUS MOVED' "Focus moved onto SETTINGS before activation" "${SERIAL_LOG_FILE}"
require_log_line '\[lvgl\] action=SETTINGS page=SETTINGS focus=SETTINGS_TOGGLE .*status=SETTINGS PAGE ACTIVE' "SETTINGS page activation executed" "${SERIAL_LOG_FILE}"
require_log_line '\[lvgl\] action=TOGGLE_KEY_ECHO page=SETTINGS focus=SETTINGS_TOGGLE key_echo=OFF .*status=KEY ECHO DISABLED' "SETTINGS toggle disabled key echo" "${SERIAL_LOG_FILE}"
require_log_line '\[lvgl\] input=SUPPRESSED page=SETTINGS focus=SETTINGS_TOGGLE key_echo=OFF input_len=0 status=INPUT BLOCKED' "Printable input was blocked while key echo was off" "${SERIAL_LOG_FILE}"
require_log_line '\[lvgl\] action=TOGGLE_KEY_ECHO page=SETTINGS focus=SETTINGS_TOGGLE key_echo=ON .*status=KEY ECHO ENABLED' "SETTINGS toggle re-enabled key echo" "${SERIAL_LOG_FILE}"
require_log_line '\[lvgl\] action=ABOUT page=ABOUT focus=ABOUT_TOGGLE .*status=ABOUT PAGE ACTIVE' "ABOUT page activation executed" "${SERIAL_LOG_FILE}"
require_log_line '\[lvgl\] action=TOGGLE_ABOUT_NOTES page=ABOUT focus=ABOUT_TOGGLE .*status=SYSTEM NOTES ENABLED' "ABOUT toggle enabled system notes" "${SERIAL_LOG_FILE}"
require_log_line '\[lvgl\] action=TOGGLE_ABOUT_NOTES page=ABOUT focus=ABOUT_TOGGLE .*status=SYSTEM NOTES DISABLED' "ABOUT toggle disabled system notes" "${SERIAL_LOG_FILE}"
require_log_line '\[lvgl\] input=APPEND page=ABOUT focus=ABOUT_TOGGLE key_echo=ON input_len=1 status=INPUT UPDATED' "Printable input updated the LVGL input box" "${SERIAL_LOG_FILE}"
require_log_line '\[lvgl\] action=CLEAR page=ABOUT focus=CLEAR .*input_len=0 status=INPUT CLEARED' "CLEAR action emptied the LVGL input box" "${SERIAL_LOG_FILE}"
require_log_line '\[lvgl\] action=HOME page=HOME focus=HOME_TOGGLE .*status=HOME PAGE ACTIVE' "HOME hotkey returned to the home page" "${SERIAL_LOG_FILE}"
require_log_line 'LPVEABCDKUS' "UEFI loader and kernel handoff markers observed" "${DEBUGCON_LOG_FILE}"

python3 - "${BEFORE_PPM}" "${HOME_DETAILS_ON_PPM}" "${HOME_DETAILS_OFF_PPM}" "${SETTINGS_PPM}" "${SETTINGS_OFF_PPM}" "${SETTINGS_BLOCKED_PPM}" "${SETTINGS_ON_PPM}" "${ABOUT_PPM}" "${ABOUT_NOTES_ON_PPM}" "${ABOUT_NOTES_OFF_PPM}" "${ABOUT_INPUT_PPM}" "${CLEAR_PPM}" "${HOME_RETURN_PPM}" "${PIXEL_DIFFS_FILE}" <<'PY'
import sys

before_path, home_details_on_path, home_details_off_path, settings_path, settings_off_path, settings_blocked_path, settings_on_path, about_path, about_notes_on_path, about_notes_off_path, about_input_path, clear_path, home_return_path, output_path = sys.argv[1:15]

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

def changed_pixels(first, second):
    if first[:3] != second[:3]:
        raise RuntimeError("screen captures use different dimensions")
    changed = 0
    for index in range(0, len(first[3]), 3):
        if first[3][index:index + 3] != second[3][index:index + 3]:
            changed += 1
    return changed

captures = {
    "home_to_details_on": (read_ppm(before_path), read_ppm(home_details_on_path), 100),
    "details_on_to_off": (read_ppm(home_details_on_path), read_ppm(home_details_off_path), 100),
    "home_to_settings": (read_ppm(home_details_off_path), read_ppm(settings_path), 100),
    "settings_to_toggle_off": (read_ppm(settings_path), read_ppm(settings_off_path), 100),
    "toggle_off_to_blocked": (read_ppm(settings_off_path), read_ppm(settings_blocked_path), 16),
    "blocked_to_toggle_on": (read_ppm(settings_blocked_path), read_ppm(settings_on_path), 100),
    "settings_to_about": (read_ppm(settings_on_path), read_ppm(about_path), 100),
    "about_to_notes_on": (read_ppm(about_path), read_ppm(about_notes_on_path), 100),
    "notes_on_to_off": (read_ppm(about_notes_on_path), read_ppm(about_notes_off_path), 100),
    "about_to_input": (read_ppm(about_notes_off_path), read_ppm(about_input_path), 16),
    "input_to_clear": (read_ppm(about_input_path), read_ppm(clear_path), 16),
    "clear_to_home": (read_ppm(clear_path), read_ppm(home_return_path), 100),
}

with open(output_path, "w", encoding="utf-8") as handle:
    for name, (first, second, minimum) in captures.items():
        changed = changed_pixels(first, second)
        handle.write(f"{name}={changed}\n")
        if changed < minimum:
            raise SystemExit(2)
PY

while IFS='=' read -r name changed; do
    pass "Frame buffer changed for ${name} (${changed} pixels)"
done < "${PIXEL_DIFFS_FILE}"

printf '== LVGL interaction check passed: %d checks ==\n' "${PASS_COUNT}"
