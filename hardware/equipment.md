# Equipment

Revision K is a USB-powered ESP32-C3 controller.

| Quantity | Component | Function |
| --- | --- | --- |
| 1 | ESP32-C3-DevKitM-1 | BLE MIDI, USB-UART programming and diagnostics |
| 1 | DFRobot DFR0464 Gravity RGB 16×2 LCD | FLWBBA/5V module; text 0x3E, RGB 0x60 |
| 1 | 33mm red momentary arcade button | Record/Stop; separate bare red LED |
| 4 | 12mm threaded momentary NO buttons | Play/Stop, TAKE, SONG, BACK |
| 1 | Threaded-shaft incremental encoder with push switch | Volume, menu and selection; part number unidentified |
| 1 | BC547C-class NPN, TO-92 | Record LED driver; fitted manufacturer unidentified |
| 1 | Solderable perfboard | Component mounting |
| As needed | C3 sockets, harness connectors and wire | Removable board and panel connections |
| 1 | USB data cable | Power, firmware upload and diagnostics |

## Passive components

- R1–R8: 10kΩ input pull-ups to 3.3V; omit where equivalent pull-ups are present.
- R9/R10: effective 4.7kΩ I²C pull-ups on the translator's 3.3V side; account for module pull-ups.
- R11: 1kΩ Record LED series resistor from USB 5V.
- R12: 1kΩ GPIO10-to-transistor-base resistor.
- R13: 100kΩ base-to-emitter resistor.
- C1: 100nF ceramic at LCD VCC/GND.
- C2: optional 10µF bypass across 3.3V/GND.

## Required LCD translator

The schematic uses a bidirectional open-drain I²C level-shifter module with at
least two channels, BSS138 topology. LV is 3.3V; HV is USB 5V. Installation has
not been bench-validated. The LCD must not connect directly to C3 GPIOs.

A Joy-IT COM-KY051VT is an alternative based on TXS0104E. It uses VCCA/A1/A2 for
the C3 side and VCCB/B1/B2 for the LCD side; OE can remain unconnected on that
module. Its pin names differ from the BSS138 module shown in the schematic.
[Manufacturer specification](https://joy-it.net/en/products/COM-KY051VT).

Battery, charger and separate USB-C power input are not included in this design.
See [schematic](schematic/schematic.md) and [connector pinouts](schematic/connectors.md).
