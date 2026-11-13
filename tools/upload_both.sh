#!/usr/bin/env bash
set -euo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
SKETCH="$ROOT/firmware"
FQBN='esp32:esp32:esp32s3:FlashSize=16M,PSRAM=enabled,PartitionScheme=app3M_fat9M_16MB'

if [[ $# -ne 0 ]]; then
  echo "Usage: $0" >&2
  exit 2
fi

ports=()
while IFS= read -r port; do
  ports+=("$port")
done < <(
  arduino-cli board list --format json |
    jq -r '.detected_ports[] | select(.port.protocol_label == "Serial Port (USB)" and .port.properties.vid == "0x1A86" and .port.properties.pid == "0x55D3") | .port.address'
)

if [[ ${#ports[@]} -ne 2 ]]; then
  echo "Expected exactly 2 Waveshare ESP32 USB ports; found ${#ports[@]}." >&2
  echo "Connect both boards and run: arduino-cli board list" >&2
  exit 1
fi

BUILD_DIR=$(mktemp -d "${TMPDIR:-/tmp}/ninajirachi-build.XXXXXX")
trap 'rm -rf "$BUILD_DIR"' EXIT

arduino-cli compile --fqbn "$FQBN" --build-path "$BUILD_DIR" "$SKETCH"

for port in "${ports[@]}"; do
  echo "Uploading to $port"
  arduino-cli upload --fqbn "$FQBN" --input-dir "$BUILD_DIR" --port "$port" "$SKETCH"
done

echo "Uploaded firmware to both ESPs."
