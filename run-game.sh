#!/usr/bin/env bash
set -euo pipefail

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"

usage() {
  cat <<'EOF'
Usage: ./run-game.sh [run|build|buttons|build-buttons|display|build-display|test|monitor|ports] [serial-port]

  run      Build, upload to the ESP32, and open Serial Monitor (default).
  build    Compile without needing a connected board.
  monitor  Open Serial Monitor without uploading.
  ports    List available serial devices.
  buttons  Upload button diagnostics and open Serial Monitor.
  build-buttons  Compile button diagnostics without a board.
  display  Upload screen driver/color diagnostics (START cycles tests).
  build-display  Compile screen diagnostics without a board.
  test     Run host simulations (requires g++).

Examples:
  ./run-game.sh
  ./run-game.sh run /dev/ttyACM0

Connect the ESP32 by USB before uploading. Close other serial monitors first.
Press Ctrl+C to exit Serial Monitor.
EOF
}

action="${1:-run}"
case "$action" in
  -h|--help|help) usage; exit 0 ;;
  run|build|buttons|build-buttons|display|build-display|test|monitor|ports) ;;
  *) usage >&2; exit 2 ;;
esac
if (( $# > 2 )); then
  usage >&2
  exit 2
fi

if [[ "$action" == test ]]; then
  exec ./tests/run-tests.sh
fi

environment=esp32dev
if [[ "$action" == buttons || "$action" == build-buttons ]]; then
  environment=buttons
fi

if [[ "$action" == display || "$action" == build-display ]]; then
  environment=display-test
fi

if command -v pio >/dev/null 2>&1; then
  pio_bin="$(command -v pio)"
elif [[ -x "${PLATFORMIO_CORE_DIR:-$HOME/.platformio}/penv/bin/pio" ]]; then
  pio_bin="${PLATFORMIO_CORE_DIR:-$HOME/.platformio}/penv/bin/pio"
else
  echo "PlatformIO was not found. Open the PlatformIO IDE extension in VS Code and let its setup finish." >&2
  exit 1
fi

upload_args=()
monitor_args=()
if [[ -n "${2:-}" ]]; then
  upload_args=(--upload-port "$2")
  monitor_args=(--port "$2")
fi

case "$action" in
  build|build-buttons|build-display)
    exec "$pio_bin" run -e "$environment"
    ;;
  ports)
    exec "$pio_bin" device list
    ;;
  run|buttons|display)
    echo "Building and uploading to ESP32..."
    if ! "$pio_bin" run -e "$environment" -t upload "${upload_args[@]}"; then
      echo "Upload failed. Check the USB cable and close other serial monitors." >&2
      echo "Run './run-game.sh ports' to find the port, then './run-game.sh run /dev/ttyACM0' (using your port)." >&2
      exit 1
    fi
    ;;
esac

echo "Opening Serial Monitor at 115200 baud. Press RESET on the ESP32 to see the menu again."
echo "Follow the device prompts. Press Ctrl+C to exit this monitor."
exec "$pio_bin" device monitor -e "$environment" --baud 115200 "${monitor_args[@]}"
