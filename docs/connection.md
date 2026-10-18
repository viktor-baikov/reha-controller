# MIDI connection

The controller advertises as `Reha Probe`. MIDI uses Bluetooth LE; USB is for
power, programming and serial diagnostics. Pairing, a live MIDI link and recorder
feedback are separate states. The recorder requires both MIDI input and output.

## macOS

Open Audio MIDI Setup → MIDI Studio → Configure Bluetooth, connect the advertising
controller, then select its input and feedback output in the recorder's Controls
settings. `tools/mac_midi_setup.swift` opens Apple's Bluetooth MIDI picker.
Automatic native reconnection has not been consistently validated.

## Windows 11 x64

Pair in Windows Bluetooth settings. In the recorder's Controls settings, select
the Bluetooth MIDI IN port and apply. The integration selects the unique matching
Bluetooth MIDI OUT port; ambiguous outputs require explicit selection.
The recorder uses its existing pylibremidi dependency with the native UWP backend.
No external MIDI bridge is required for the validated Windows path.

## Linux

Controller operation on Linux has not been validated.

## Optional macOS bridge

The development bridge connects over CoreBluetooth and publishes virtual CoreMIDI
ports. Build the tools:

```sh
sh tools/build_mac_probes.sh
```

Run `mac_bluetooth_probe` from the reported output directory, identify the peripheral
UUID and stop discovery. Run `mac_reconnect_bridge <peripheral-uuid>` from the same
directory. Select **Reha Probe Bridge** as both recorder input and feedback output.
Do not also connect through the native picker or another computer.

The bridge forwards controller protocol messages, batches feedback to the BLE MTU,
limits pending feedback to 64 messages and discards queued commands on disconnect.
Ctrl-C closes it and removes its virtual ports. It installs no driver or startup
service. `REHA_PROBE_BUILD_DIR` and `REHA_SWIFT_CACHE` select build/cache directories.

## Controller connection screens

| Condition | Top row | Bottom row |
| --- | --- | --- |
| BLE disconnected | Bluetooth MIDI | Disconnected |
| BLE connected; recorder feedback absent/stale | Bluetooth MIDI | Waiting recorder |
| Recorder responding; no rehearsal open | connected | start rehersal? |
| Rehearsal open | Tnn-Song | READY and volume |

Feedback expires after 3.5 seconds. Host loss clears synchronization, metadata,
volume and previews. Reconnection requests a fresh snapshot; commands are not
replayed. Closing a MIDI connection does not stop recorder audio.
