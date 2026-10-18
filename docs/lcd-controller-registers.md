# DFR0464 display controllers

The original DFR0464 module documentation specifies AiP31068L (or equivalent)
for LCD text and PCA9633DP2 (or equivalent) for RGB backlight. The connected
module responds at 7-bit addresses 0x3E and 0x60, matching that interface; an
address scan does not identify the silicon revision.

Sources:
- [DFRobot module datasheet](https://github.com/DFRobot/DFRobot_RGBLCD/blob/master/DFR0464%20Datasheet.pdf)
- [I-CORE AiP31068L datasheet, hosted by Waveshare](https://files.waveshare.com/wiki/LCD1602-I2C-Module/AIP31068L.pdf), instruction table p18; bias pins p4; application circuits pp21–22.
- [NXP PCA9633 datasheet](https://www.nxp.com/docs/en/data-sheet/PCA9633.pdf), register summary pp8–9.

## Electrical interface

The FLWBBA/5V module's isolated SDA/SCL lines measured approximately 5V with
3.3V supplied to the module. The design uses a bidirectional I²C translator:
C3 side at 3.3V, LCD side and LCD supply at USB 5V. See the
[schematic](../hardware/schematic/schematic.md#lcd-interface).
The translator installation has not been bench-validated. Optical response is
separate from I²C write completion.

## LCD text — 0x3E

This is an instruction/data interface, rather than a bank of numbered control
registers. The firmware sends command bytes after control byte 0x80 and text
bytes after control byte 0x40. The module's published 0x7C is the 8-bit write
address, equivalent to 7-bit 0x3E.

| Instruction byte | Function | Firmware use |
| --- | --- | --- |
| 0x01 | Clear display | Initialization only |
| 0x02 | Return home | Not needed for normal updates |
| 0x04–0x07 | Entry direction and display shift | 0x06: advance cursor, no shift |
| 0x08–0x0F | Display/cursor/blink enable bits | 0x0C: display on, cursor/blink off |
| 0x10–0x1F | Cursor/display shift | Not used |
| 0x20–0x3F | Function configuration | 0x28: two lines, 5×8 font |
| 0x40–0x7F | Set custom-character RAM address | Not used |
| 0x80–0xFF | Set display RAM address | 0x80 + column, 0xC0 + column |

Visible character RAM: row1 0x00–0x0F, row2 0x40–0x4F. Writing data advances
the address with entry mode 0x06. Spaces erase previous characters without
clearing the entire screen.

There is no electronic contrast instruction in this documented command set.
Contrast depends on LCD bias voltages V1–V5 and the module's hardware. Do not
apply contrast commands for a different controller (such as ST7032) merely
because its I2C address is also 0x3E. Text pixels are not separately illuminated.

## RGB backlight — 0x60

| Register | Function | Module/firmware use |
| --- | --- | --- |
| 0x00 MODE1 | Oscillator, sleep, address response | 0x00: wake oscillator |
| 0x01 MODE2 | Output configuration/group mode | 0x20 |
| 0x02 PWM0 | Channel 0 duty | Blue, 0–255 |
| 0x03 PWM1 | Channel 1 duty | Green, 0–255 |
| 0x04 PWM2 | Channel 2 duty | Red, 0–255 |
| 0x05 PWM3 | Channel 3 duty | Not used by this RGB module |
| 0x06 GRPPWM | Shared dimming duty | Not used |
| 0x07 GRPFREQ | Shared blink rate | Not used |
| 0x08 LEDOUT | Per-channel off/on/PWM/group selection | 0xFF |
| 0x09–0x0B SUBADR1–3 | I2C subaddresses | Not changed |
| 0x0C ALLCALLADR | Shared I2C address | Not changed |

Firmware scales each status colour to the selected peak brightness using the
individual RGB PWM channels. Brightness is local to the controller, not a MIDI
command. A saved percentage persists in ESP32 NVS namespace reha-panel, key
brightness. Default 10%; editing steps 5%; range 0–100%. Press saves a changed
value once; BACK discards the preview. No pairing or recorder settings change.
