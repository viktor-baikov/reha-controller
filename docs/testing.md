# Tests and simulation

## Firmware

`sh tools/build_desktop_panel.sh` runs nine C++ suites: session, volume, text rows,
panel, diagnostics, selection reference, browse catalog, LCD driver and desktop
panel. It then compiles the ESP32-C3 firmware. Tests use fake GPIO, time and I²C;
they do not open a serial port or control audio.

`python3 tools/generate_menu_reference.py` compiles the firmware renderer against
the test stubs and generates Markdown, HTML and SVG screen references. Each
snapshot checks the visible 16-character rows against the retained full text.

`python3 hardware/schematic/generate_full.py` regenerates the schematic SVG and
checks its GPIO assignments against BoardConfig.hpp.

## macOS tools

`sh tools/build_mac_probes.sh` builds discovery, connection picker, bridge and
basic host simulator tools. It runs BLE codec and simulator protocol tests plus
session, volume and text-row tests. Build outputs remain outside the tracked tree.

`mac_controller_simulator` provides basic transport feedback without audio.
It does not implement all lifecycle, deletion or catalog extensions. Use a single
feedback publisher for a controller connection.

## Recorder integration

The recorder is a separate checkout with its own dependencies and tests.
`python tests/run_all.py` in the matching controller integration runs twelve common
Python suites using fake MIDI/audio and disposable data.

`python3 tools/remote_recorder_probe.py --recorder <source-checkout>` exercises
the recorder model with real MIDI I/O and fake audio. Its JSON stdin/stdout interface
supports isolated rehearsal setup, status, test audio samples and transport
operations. EOF or `{"op":"quit"}` closes MIDI and removes disposable data.
It uses the recorder's test environment and opens no listening socket.

Serial action commands can drive a real BLE round trip through the controller,
recorder backend and return feedback. Wait for pending/awaiting flags to clear
between actions. [Serial diagnostics](controller-diagnostics.md) describe status,
logs and commands. Logs may contain song/take titles.

## Validation scope

Firmware simulation covers protocol parsing, state transitions, stale/duplicate
requests, catalog assembly, selection-on-release, display rendering and I²C writes.
The macOS and Windows 11 x64 integrations have exercised bidirectional BLE MIDI,
transport, playback volume, song/take selection and connection recovery.
Simulation does not measure physical switch contacts, optical display response
or audio quality. Linux and the required LCD level-shifter installation remain
unvalidated.
