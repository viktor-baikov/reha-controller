# Revision K electrical reference

[Full schematic SVG](controller-full.svg) · [Connector pinouts](connectors.md)

The diagram is viewed from the component side: antenna up, USB down, J1 on the
left and J3 on the right. The solder-side view is mirrored. Matching power/GND
net labels are connected electrically. Header references apply to the official
ESP32-C3-DevKitM-1. Firmware pins are defined in BoardConfig.hpp.

## LCD interface

The FLWBBA/5V LCD module's isolated SDA/SCL lines measured approximately 5V with
3.3V supplied to the module. The design powers the LCD from USB 5V and uses U2,
a bidirectional open-drain I²C translator, between the LCD and C3. Installation
and resulting display timing have not been bench-validated.

The schematic shows a BSS138-type module. Its printed pin labels determine
connections; commercial modules do not share a universal physical pin order.

| U2 label | Connection |
| --- | --- |
| LV | C3 3.3V |
| LV1 | GPIO18 / J3.13, SDA_L |
| LV2 | GPIO19 / J3.14, SCL_L |
| HV | USB 5V |
| HV1 | X7.1, LCD SDA_H |
| HV2 | X7.2, LCD SCL_H |
| GND | Common C3/LCD ground |

LCD VCC is X7.3 / USB 5V. Pull-ups on the C3 side connect to 3.3V; LCD-side
pull-ups connect to the 5V domain. Account for existing module pull-ups.
Leave unused translator channels unconnected.

With C3 signal wires detached, power the LCD and translator and check idle
LOW-side SDA/SCL near 3.3V and HIGH-side SDA/SCL near 5V. Power off before
connecting C3. Series resistors and resistor dividers do not provide bidirectional
I²C translation. [NXP AN10441](https://cdn-shop.adafruit.com/datasheets/AN10441.pdf),
[ESP32-C3 electrical limits](https://documentation.espressif.com/esp32-c3_datasheet_en.html).

## GPIO assignments

| Function | GPIO | Header | Harness |
| --- | --- | --- | --- |
| Record | 1 | J1.10 | X1.2 |
| Record LED driver | 10 | J1.11 | R12 → Q1 base |
| Play | 7 | J3.8 | X2.2 |
| TAKE | 0 | J1.9 | X3.2 |
| SONG | 3 | J1.5 | X4.2 |
| BACK / BOOT | 9 | J3.5 | X5.2 |
| Encoder A / CLK | 6 | J3.9 | X6.1 |
| Encoder B / DT | 5 | J3.10 | X6.2 |
| Encoder SW | 4 | J3.11 | X6.3 |
| LCD SDA | 18 | J3.13 | U2 LV1 |
| LCD SCL | 19 | J3.14 | U2 LV2 |

GPIO2 is unassigned and is a strapping pin. GPIO8 drives the onboard RGB LED.
GPIO20/21 serve USB-UART. BACK is parallel with onboard BOOT; holding it during
RESET enters download mode. Inputs are normally open switches to GND.

## Record LED driver

The Record button contains a bare red LED without a series resistor.
Its switch contacts and LED form separate circuits.

```text
USB 5V → R11 1kΩ → X1.3 → LED anode
LED cathode → X1.4 → Q1 collector
Q1 emitter → GND
GPIO10 → R12 1kΩ → Q1 base
Q1 base → R13 100kΩ → GND
```

For the onsemi BC547C TO-92 reference, flat marked face toward the viewer and
legs down: left = collector, middle = base, right = emitter. The SVG includes
this front view. The fitted transistor manufacturer is unidentified; identify
its C/B/E before relying on the reference orientation.
[onsemi datasheet](https://www.onsemi.com/pdf/datasheet/bc550-d.pdf).

With assumed red LED Vf 1.8–2.2V and transistor VCE 0.2V, R11 limits current
to approximately 2.6–3.0mA at 5V and dissipates less than 10mW. The fitted LED's
part number, Vf and current rating are unidentified. R11 and R12 are separate
resistors. GPIO10 connects only to R12, never to the 5V LED supply.

## Power and passive components

The board's USB socket is the sole supply. J1.2/.3 provide 3.3V; J1.13/.14 provide
USB 5V. J3 has GND contacts but no 3.3V pin. C3 inputs, encoder module and U2 LOW
side use 3.3V. LCD, U2 HIGH side and LED supply use USB 5V. Grounds are common.
Battery, charging and a second power input are not included.

R1–R8 are 10kΩ pull-ups to 3.3V on Record, Play, TAKE, SONG, BACK, encoder A,
encoder B and encoder push. Existing equivalent module/BOOT pull-ups can replace
individual resistors. A bare encoder has no VCC connection: common and one
push contact connect to GND; the other push contact connects to SW.

R9/R10 provide effective 4.7kΩ pull-ups on U2 LOW side. Suitable translator
pull-ups can replace them. LCD pull-ups belong to the HIGH side.
C1 is 100nF ceramic at LCD VCC/GND. C2 is optional 10µF across 3.3V/GND.
Ceramics are nonpolar; electrolytics require correct polarity.

## Assembly checks

Disconnect USB before soldering or removing/reinserting the C3. Verify harness
pin 1, supply continuity, absence of shorts, LED polarity and transistor pins.
Keep the antenna clear of copper and wiring. Build the desktop profile matching
revision K. Check released inputs near 3.3V and pressed switches near GND.
Use a simulator for the initial control and LED check.

A socketed C3 can be programmed outside the panel. Mounted programming is also
supported with correct wiring and USB as the sole supply.

## Sources

- [Espressif board headers](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c3/esp32-c3-devkitm-1/user_guide.html#header-block)
- [Espressif board schematic](https://dl.espressif.com/dl/schematics/SCH_ESP32-C3-DEVKITM-1_V1_20200915A.pdf)
- [DFRobot DFR0464](https://www.dfrobot.com/product-1609.html)
