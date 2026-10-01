#!/bin/sh
set -eu

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
SKETCH_DIR="${REPO_ROOT}/firmware/MiniTV"
FQBN="${AIPI_LITE_FQBN:-esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,PartitionScheme=default_16MB}"
CORE_VERSION="2.0.17"

upload=false
allow_partition_change=false
port=""

usage() {
  cat <<'EOF'
Build the AI-PI Lite MiniTV firmware, optionally uploading it to a device.

Usage: tools/install_firmware.sh [--upload] [--port PORT]
                                  [--allow-unverified-partition-table]
                                  [--help]

By default, this installs the pinned Arduino toolchain packages in the local
arduino-cli environment and builds the sketch. Uploading is blocked unless
--allow-unverified-partition-table is also supplied because the device's
existing partition table and MicroPython filesystem have not been verified.

The override may replace the device partition table. Back up and verify the
device's contents and recovery procedure before using it. This script never
requests a full-chip erase or formats a filesystem.

Environment:
  ARDUINO_CLI       arduino-cli executable (default: arduino-cli)
  AIPI_LITE_FQBN    override the board FQBN (default targets 16 MB/OCT PSRAM)
EOF
}

die() {
  printf 'error: %s\n' "$*" >&2
  exit 2
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --upload) upload=true ;;
    --port)
      [ "$#" -ge 2 ] || die "--port requires a value"
      port="$2"
      shift
      ;;
    --allow-unverified-partition-table) allow_partition_change=true ;;
    --help|-h) usage; exit 0 ;;
    *) die "unknown argument: $1 (use --help)" ;;
  esac
  shift
done

if [ "$allow_partition_change" = true ] && [ "$upload" != true ]; then
  die "--allow-unverified-partition-table only applies with --upload"
fi
if [ "$upload" = true ] && [ "$allow_partition_change" != true ]; then
  die "upload blocked: verify the device partition table and data backup first, then pass --allow-unverified-partition-table"
fi

arduino_cli="${ARDUINO_CLI:-arduino-cli}"
command -v "$arduino_cli" >/dev/null 2>&1 || die "arduino-cli was not found; install Arduino CLI and retry"

"$arduino_cli" core update-index
"$arduino_cli" core install "esp32:esp32@${CORE_VERSION}"
"$arduino_cli" lib install "GFX Library for Arduino@1.6.0"
"$arduino_cli" lib install "JPEGDEC@1.8.4"
"$arduino_cli" lib install "ArduinoJson@7.4.3"

build_dir="$(mktemp -d "${TMPDIR:-/tmp}/aipi-lite-firmware.XXXXXX")"
trap 'rm -rf "$build_dir"' EXIT

"$arduino_cli" compile --fqbn "$FQBN" --build-path "$build_dir" "$SKETCH_DIR"

if [ "$upload" = true ]; then
  if [ -z "$port" ]; then
    "$arduino_cli" upload --fqbn "$FQBN" "$SKETCH_DIR"
  else
    "$arduino_cli" upload --fqbn "$FQBN" --port "$port" "$SKETCH_DIR"
  fi
fi

printf 'Firmware build completed successfully.\n'
if [ "$upload" != true ]; then
  printf 'No device was changed. To upload, first verify the partition table and back up device data; see docs/COMPATIBILITY.md.\n'
fi
