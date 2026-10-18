#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
probe_build_dir="${REHA_PROBE_BUILD_DIR:-${TMPDIR:-/tmp}/reha-probe-build}"
probe_module_cache="${REHA_SWIFT_CACHE:-${TMPDIR:-/tmp}/reha-swift-cache}"
mkdir -p "$probe_build_dir" "$probe_module_cache"
swiftc -module-cache-path "$probe_module_cache" tools/mac_midi_setup.swift -o "$probe_build_dir/mac_midi_setup"
swiftc -module-cache-path "$probe_module_cache" tools/mac_bluetooth_probe.swift \
  -Xlinker -sectcreate -Xlinker __TEXT -Xlinker __info_plist \
  -Xlinker tools/BluetoothProbe-Info.plist -o "$probe_build_dir/mac_bluetooth_probe"
swiftc -module-cache-path "$probe_module_cache" tools/BLEMIDI.swift tools/mac_reconnect_bridge.swift \
  -Xlinker -sectcreate -Xlinker __TEXT -Xlinker __info_plist \
  -Xlinker tools/BluetoothProbe-Info.plist -o "$probe_build_dir/mac_reconnect_bridge"
swiftc -module-cache-path "$probe_module_cache" tools/BLEMIDI.swift tools/test_ble_codec.swift \
  -o "$probe_build_dir/test_ble_codec"
"$probe_build_dir/test_ble_codec"
swiftc -module-cache-path "$probe_module_cache" tools/BLEMIDI.swift tools/RemoteProtocol.swift \
  tools/mac_controller_simulator.swift -o "$probe_build_dir/mac_controller_simulator"
swiftc -module-cache-path "$probe_module_cache" tools/RemoteProtocol.swift tools/test_remote_protocol.swift \
  -o "$probe_build_dir/test_remote_protocol"
"$probe_build_dir/test_remote_protocol"
c++ -std=c++11 tests/session_test.cpp -o "$probe_build_dir/test_session"
"$probe_build_dir/test_session"
c++ -std=c++11 tests/volume_test.cpp -o "$probe_build_dir/test_volume"
"$probe_build_dir/test_volume"
c++ -std=c++11 tests/text_rows_test.cpp -o "$probe_build_dir/test_text_rows"
"$probe_build_dir/test_text_rows"
printf 'Built Mac probes in %s\n' "$probe_build_dir"
