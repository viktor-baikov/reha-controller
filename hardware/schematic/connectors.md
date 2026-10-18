# Revision K panel connector pinouts

[Zoomable SVG](controller-full.svg) · [Electrical notes](schematic.md)

Board orientation: **component side facing you, C3 antenna up, USB down**.
J1 is the left C3 header and J3 is the right. Header position 1 is at the antenna
end on both sides. Looking at solder joints from below mirrors this view.

**X1-X7 identify perfboard harness connectors.** J1/J3 identify the C3 headers.
Harness numbers are independent of manufacturer connector numbering. Mark pin 1 (square in the drawing) on both
board and cable. Connector housing/latch orientation has not been selected;
use actual molded pin numbers when present. The numbers below identify electrical
contacts, not wire colours or a presumed view into an unplugged housing.

## Connector contacts

| Connector | Pin | Signal | Solder on controller board | Solder at panel component |
| --- | --- | --- | --- | --- |
| X1 RECORD + LED | 1 | GND | GND | Switch COM |
| X1 | 2 | RECORD | GPIO1 / J1.10; R1 10k to 3V3 | Switch NO; leave NC unused |
| X1 | 3 | REC_A | After R11 1k from USB 5V | Red LED anode + |
| X1 | 4 | REC_K | Q1 collector | Red LED cathode - |
| X2 PLAY | 1 | GND | GND | COM |
| X2 | 2 | PLAY | GPIO7 / J3.8; R2 10k to 3V3 | NO |
| X3 TAKE | 1 | GND | GND | COM |
| X3 | 2 | TAKE | GPIO0 / J1.9; R3 10k to 3V3 | NO |
| X4 SONG | 1 | GND | GND | COM |
| X4 | 2 | SONG | GPIO3 / J1.5; R4 10k to 3V3 | NO |
| X5 BACK / BOOT | 1 | GND | GND | COM |
| X5 | 2 | BACK | GPIO9 / J3.5; R5 10k to 3V3 | NO; parallel with onboard BOOT |
| X6 ENCODER | 1 | A | GPIO6 / J3.9; R6 10k to 3V3 | A / CLK |
| X6 | 2 | B | GPIO5 / J3.10; R7 10k to 3V3 | B / DT |
| X6 | 3 | SW | GPIO4 / J3.11; R8 10k to 3V3 | Push output SW |
| X6 | 4 | 3V3 | 3.3V | Module + / VCC, if applicable |
| X6 | 5 | GND | GND | Module GND / encoder common |
| X7 LCD | 1 | SDA | U2 HV1; GPIO18 / J3.13 connects to U2 LV1 | LCD SDA |
| X7 | 2 | SCL | U2 HV2; GPIO19 / J3.14 connects to U2 LV2 | LCD SCL |
| X7 | 3 | 5V | USB5V, also U2 HV | LCD VCC |
| X7 | 4 | GND | GND | LCD GND |

X1 switch and LED are electrically separate inside the button. Do not connect
LED cathode directly to X1.1: it goes through Q1 so firmware controls the lamp.
X2-X5 carry switch contacts only; any separate lamps remain disconnected.
A two-terminal bare NO switch has no polarity.

X6 uses **A, B, SW, 3V3, GND**. Its signal pins connect to J3.9/.10/.11. Adapt the harness to the actual
encoder module's printed labels; factory CLK/DT/SW/+/GND order can differ.
For a bare encoder: X6.1 to A, X6.2 to B, encoder common and one push contact to
X6.5, other push contact to X6.3; leave X6.4 unused. A bare encoder needs no VCC.
Suitable module pull-ups at 3.3V can replace their matching external resistors.

X7 numbering identifies the perfboard connector contacts. Adapt to the display's actual
VCC/GND/SDA/SCL labels; do not infer its connector order from this table.

## LCD level shifter

The LCD SDA/SCL lines measured approximately 5V with a 3.3V supply. U2 provides
the required level translation; installation has not been bench-validated.
Use a bidirectional I2C module with LV=3.3V, HV=USB5V, common GND;
LV1/LV2 go to GPIO18/19 and HV1/HV2 go to X7.1/.2. Low-side pull-ups go
only to 3.3V. See the [LCD interface](schematic.md#lcd-interface)
for wiring and pre-connection voltage checks. Physical module pin order is
unspecified until the actual module is selected. Harness contact numbers stay unchanged.

## Driver and supplies

GPIO10 / J1.11 -> R12 1k -> Q1 base. R13 100k from base to emitter/GND.
Q1 emitter -> GND. Q1 collector -> X1.4. USB 5V -> R11 1k -> X1.3.
Neither GPIO10 nor any input receives 5V. Record and its complete driver stay
on J1's side. Q1 pin identification and resistor calculations are in the
[electrical notes](schematic.md).

3.3V and GND are shown as net labels; route them as convenient. All matching
labels are connected electrically. J1.2/.3 supply 3.3V; J1.13/.14 supply USB 5V.
J3 has GND contacts but no 3.3V pin. C1 100nF across X7.3/X7.4 near the LCD;
C2 10uF optional across 3.3V/GND. Each side of U2 needs its own pull-ups
to that side's rail; account for existing module/LCD pull-ups.

The schematic places C3 sockets in the middle, X1 and its LED driver on the
left, X6/X7 on the right. Keep the antenna clear of copper and wires and USB
accessible. The diagram does not specify perfboard dimensions or drill positions.

Header reference: [Espressif official DevKitM-1 guide](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c3/esp32-c3-devkitm-1/user_guide.html#header-block).
