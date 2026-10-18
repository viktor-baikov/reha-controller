#!/bin/sh
# Build only: never uploads or opens a serial port.
set -eu
cd "$(dirname "$0")/.."
panel_build_dir="${REHA_PANEL_BUILD_DIR:-${TMPDIR:-/tmp}/reha-probe-build/desktop-panel}"
if [ -n "${REHA_ARDUINO_CLI:-}" ]; then
    panel_arduino_cli="$REHA_ARDUINO_CLI"
elif command -v arduino-cli >/dev/null 2>&1; then
    panel_arduino_cli="$(command -v arduino-cli)"
else
    panel_arduino_cli='/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli'
fi
mkdir -p "$panel_build_dir"
for model in session volume text_rows panel diagnostics selection_reference browse; do
    c++ -std=c++11 "tests/${model}_test.cpp" -o "$panel_build_dir/test_${model}"
    "$panel_build_dir/test_${model}"
done
for driver in lcd desktop_panel; do
    c++ -std=c++11 -I tests/lcd_stubs "tests/${driver}_test.cpp" -o "$panel_build_dir/test_${driver}"
    "$panel_build_dir/test_${driver}"
done
"$panel_arduino_cli" compile \
    --fqbn esp32:esp32:esp32c3:CDCOnBoot=default,PartitionScheme=huge_app,DebugLevel=info \
    --build-property "compiler.cpp.extra_flags=-DREHA_DESKTOP_PANEL ${REHA_PANEL_FLAGS:-}" \
    --build-path "$panel_build_dir" --libraries libraries rehearsal_remote
