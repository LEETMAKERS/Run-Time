#!/usr/bin/env bash
set -euo pipefail

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"

usage() {
  cat <<'EOF'
Usage: ./run-game.sh [run|build|buttons|build-buttons|test|monitor|ports] [serial-port]

  run      Build, upload to the Uno, and open Serial Monitor (default).
  build    Compile without needing a connected board.
  monitor  Open Serial Monitor without uploading.
  ports    List available serial devices.
  buttons  Upload button diagnostics and open Serial Monitor.
  build-buttons  Compile button diagnostics without a board.
  test     Run host simulations (requires g++).

Examples:
  ./run-game.sh
  ./run-game.sh run /dev/ttyACM0

Connect the Uno by USB before uploading. Close other serial monitors first.
Press Ctrl+C to exit Serial Monitor.
EOF
}

action="${1:-run}"
case "$action" in
  -h|--help|help) usage; exit 0 ;;
  run|build|buttons|build-buttons|test|monitor|ports) ;;
  *) usage >&2; exit 2 ;;
esac
if (( $# > 2 )); then
  usage >&2
  exit 2
fi

if [[ "$action" == test ]]; then
  exec ./tests/run-tests.sh
fi

environment=uno
if [[ "$action" == buttons || "$action" == build-buttons ]]; then
  environment=buttons
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
  build|build-buttons)
    exec "$pio_bin" run -e "$environment"
    ;;
  ports)
    exec "$pio_bin" device list
    ;;
  run|buttons)
    echo "Building and uploading to Arduino Uno..."
    if ! "$pio_bin" run -e "$environment" -t upload "${upload_args[@]}"; then
      echo "Upload failed. Check the USB cable and close other serial monitors." >&2
      echo "Run './run-game.sh ports' to find the port, then './run-game.sh run /dev/ttyACM0' (using your port)." >&2
      exit 1
    fi
    ;;
esac

echo "Opening Serial Monitor at 9600 baud. Press RESET on the Uno to see the menu again."
echo "Follow the device prompts. Press Ctrl+C to exit this monitor."
exec "$pio_bin" device monitor -e "$environment" --baud 9600 "${monitor_args[@]}"
