# Firmware

## Build and upload

The desktop profile uses revision K wiring in [BoardConfig.hpp](BoardConfig.hpp).
Install Arduino-ESP32 3.3.11 and Control Surface 2.1.2 with NimBLE support.
The build script reads libraries from the repository's ignored `libraries/` directory.

```sh
sh tools/build_desktop_panel.sh
```

Build settings:

- FQBN: `esp32:esp32:esp32c3:CDCOnBoot=default,PartitionScheme=huge_app,DebugLevel=info`
- Compiler flag: `-DREHA_DESKTOP_PANEL`
- Output: `${TMPDIR:-/tmp}/reha-probe-build/desktop-panel`

The script runs nine native suites and compiles the sketch. `REHA_ARDUINO_CLI`
selects the Arduino CLI executable; `REHA_PANEL_BUILD_DIR` selects the output directory.
To upload, substitute the board's serial port and the build directory:

```sh
arduino-cli upload --port <serial-port> \
  --fqbn esp32:esp32:esp32c3:CDCOnBoot=default,PartitionScheme=huge_app,DebugLevel=info \
  --input-dir <build-directory> rehearsal_remote
```

USB serial runs at 115200 baud. Close serial readers before uploading.
The BLE advertising name is `Reha Probe`.

Encoder defaults: four quadrature edges per detent, clockwise positive.
For a reversed two-edge encoder:

```sh
REHA_PANEL_FLAGS='-DREHA_LEVEL_DIRECTION=-1 -DREHA_ENCODER_EDGES=2' \
  sh tools/build_desktop_panel.sh
```

Without `REHA_DESKTOP_PANEL`, the sketch uses BOOT as Record/Stop.
The `REHA_PANEL` and `REHA_NAV_BUTTONS` profiles use different wiring and are
incompatible with the revision K panel.

## Controls

| Control | Action |
| --- | --- |
| Record | Start recording, or stop and keep the active take |
| Play | Play/Stop the selected take; ignored during recording/saving |
| Encoder turn at home | Relative playback volume, ±2 MIDI units per detent |
| Encoder press at home | Open the menu; at explicit idle, start a rehearsal |
| TAKE + encoder turn | Preview kept takes for the selected song; apply on release |
| SONG + encoder turn | Preview song names; apply on release |
| BACK | Cancel selection/confirmation, leave menu, or dismiss a transient error |
| BACK held during RESET | Enter ROM download mode through the BOOT circuit |

TAKE/SONG previews use a complete cached catalog. An incomplete list displays
`Updating list`; rotation is ignored until the list is complete. Release commits
one changed selection after a 60ms stable-release interval. BACK cancels it.
Holding TAKE and SONG together displays E011. Encoder presses while either
selection button is held are ignored. Detents spanning a mode change are discarded.
Navigation is disabled during recording/saving and host errors.

## Menu

- Restart take: reset playback position; confirmation required.
- Stop rehearsal: finish at READY; confirmation required.
- Remove N take: remove the selected kept take; hidden for zero/unknown take;
  confirmation required.
- Brightness: preview 0–100% in 5% steps; encoder saves, BACK cancels. Default 10%.
- LCD timing test: encoder cycles ZERO → RUN → STOP → ZERO; BACK exits.

Confirmation displays `are you sure?` with a blank bottom row. Encoder confirms;
BACK cancels. A changed selected-take reference cancels deletion confirmation.
Start rehearsal uses the idle prompt and requires no confirmation.

## Feedback

Home shows `Tnn-Song` on the top row. Take numbers have at least two digits;
T00 means no takes and T# means unknown. The bottom row shows READY, REC, PLAY
or SAVE with volume `V:000`–`V:100`; `V:---` means unknown. Text is static and
truncated to 16 columns. Pending transport requests append `*` to the status.
Only one request is outstanding; additional presses are ignored.

The Record LED is solid during confirmed recording and toggles every 200ms
during saving. It is off when feedback is stale or disconnected.
A BLE connection without recorder feedback shows `Bluetooth MIDI` /
`Waiting recorder`. A disconnected BLE link shows `Bluetooth MIDI` /
`Disconnected`. Explicit recorder idle shows `connected` / `start rehersal?`.
Controls require fresh, compatible recorder feedback.

## Display driver

`DfrLcd.hpp` implements the DFR0464 text interface at 0x3E and RGB variants
at 0x60, 0x6B, 0x30 and 0x2D. The bus is dedicated to the LCD. This driver does
not support PCF8574 backpacks.

I²C runs at 100kHz with a 4ms timeout. Initialization is nonblocking. Updates
write only changed characters, at most four adjacent characters in one row per
data transaction. Transactions are separated by at least 1ms. A complete
32-character rewrite uses two cursor commands and eight data transfers;
RGB updates, health checks and main-loop scheduling add time. Unchanged text
causes no writes. Failure marks the LCD offline; retries occur after three seconds.
BLE and buttons continue operating without a display.

Backlight brightness is stored in ESP32 Preferences namespace `reha-panel`, key
`brightness`, only when a changed value is confirmed. Text contrast depends on
hardware bias; the documented text controller has no contrast command.
The [timing diagnostic](../docs/controller-diagnostics.md#lcd-timing-test)
separates input acceptance, I²C completion and visible LCD response.

Electrical connections are specified in the [schematic](../hardware/schematic/schematic.md)
and [connector reference](../hardware/schematic/connectors.md).
