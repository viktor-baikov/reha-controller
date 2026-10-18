# Rehearsal MIDI controller

ESP32-C3 desktop control surface for
[rehearsal-recorder](https://github.com/voronizer/rehearsal-recorder).
Commands and feedback use Bluetooth LE MIDI. USB supplies power, programming
and serial diagnostics.

The panel has Record/Stop, Play/Stop, TAKE, SONG, BACK, a push encoder and a
DFRobot DFR0464 RGB 16×2 LCD. The encoder controls playback volume at home.
Hold TAKE or SONG and turn to preview a selection; release to apply it.
The recorder owns recordings, transport, selections and playback volume.
The controller displays recorder feedback and tracks pending requests.

## Build

Requirements:

- Arduino CLI and Arduino-ESP32 **3.3.11**.
- Control Surface **2.1.2** with its NimBLE backend, installed in `libraries/`.
- A C++11 compiler for native firmware tests.

```sh
sh tools/build_desktop_panel.sh
```

The script runs nine native test suites and compiles the desktop firmware.
It does not upload firmware or open a serial port. Arduino dependencies and
build outputs are excluded from Git. See [build and upload](rehearsal_remote/README.md).

## Connections

- **macOS:** connect using Audio MIDI Setup → MIDI Studio → Configure Bluetooth,
  then select controller input and feedback output in the recorder.
- **Windows 11 x64:** pair in Bluetooth settings and select the Bluetooth MIDI
  input in the recorder. The matching output is selected automatically when unique.
  The recorder uses pylibremidi's WinRT backend for controller ports when
  instrument MIDI uses WinMM. Instrument MIDI keeps its existing backend.
- **Linux:** not validated.

One computer owns the BLE connection at a time. Both MIDI directions are
required. The recorder needs the controller integration; its source is maintained
in a separate repository. See [connection setup](docs/connection.md).

## Reference

- [Controls and firmware](rehearsal_remote/README.md)
- [Equipment](hardware/equipment.md)
- [Schematic](hardware/schematic/controller-full.svg), [electrical reference](hardware/schematic/schematic.md), [connectors](hardware/schematic/connectors.md)
- [MIDI protocol](docs/controller-protocol-v1.md)
- [LCD screens](docs/menu-reference.md), [interactive reference](docs/menu-reference.html), [state map](docs/menu-state-map.svg)
- [Error codes](docs/lcd-errors.md), [display registers](docs/lcd-controller-registers.md)
- [Diagnostics](docs/controller-diagnostics.md), [simulation](docs/testing.md)

The schematic includes a required LCD level shifter. Its installation and the
resulting display timing have not been bench-validated. Battery and charging
hardware are outside the USB-powered design.

## License

[MIT](LICENSE). Third-party dependencies and the recorder retain their own licenses.
