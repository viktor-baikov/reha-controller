"""Revision K: physical header sides, explicit panel connector pin numbering.

Top/component view, antenna up, USB down. Named power nets intentionally replace
power-routing lines. Connector numbering is assigned here, not a vendor pinout.
"""
from pathlib import Path
from html import escape
import re
P=Path(__file__).resolve().parent
config=(P.parent.parent/'rehearsal_remote/BoardConfig.hpp').read_text().split('#endif\n// No external')[0]
expected={'REHA_RECORD_PIN':1,'REHA_PLAY_PIN':7,'REHA_TAKE_PIN':0,'REHA_SONG_PIN':3,'REHA_PANEL_BACK_PIN':9,'REHA_PANEL_RECORD_LED':10,'REHA_LEVEL_A':6,'REHA_LEVEL_B':5,'REHA_LEVEL_PUSH':4,'REHA_SDA':18,'REHA_SCL':19}
for macro,pin in expected.items():
    assert int(re.search(r'^#define '+macro+r' (\d+)\s*$',config,re.M)[1])==pin, macro
assert len(set(expected.values()))==len(expected)
assert not set(expected.values()) & {2,8,20,21}
W,H=3100,3740
s=[f'''<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}"><rect width="100%" height="100%" fill="white"/><style>
text{{font-family:Arial,sans-serif;font-size:23px;fill:#20394d}} .title{{font-size:39px;font-weight:bold}} .head{{font-size:28px;font-weight:bold}} .small{{font-size:20px}} .pin{{font-size:23px;font-weight:bold}} .box{{fill:#f4f8fc;stroke:#a9bdcc;stroke-width:2}} .note{{fill:#fff3dc;stroke:#cfb582;stroke-width:2}} .sig{{stroke:#20394d;stroke-width:3;fill:none}} .gnd{{stroke:#2875ae;stroke-width:3;fill:none}} .pwr{{stroke:#cc3535;stroke-width:3;fill:none}} .five{{stroke:#ac640d;stroke-width:3;fill:none}}
</style>''']
def text(x,y,v,cls=''):s.append(f'<text x="{x}" y="{y}" class="{cls}">{escape(v)}</text>')
def box(x,y,w,h,cls='box'):s.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="10" class="{cls}"/>')
def wire(points,cls='sig'):s.append('<path d="M'+' L'.join(f'{x} {y}' for x,y in points)+f'" class="{cls}"/>')
def line(x,y,X,Y,cls='sig'):wire([(x,y),(X,Y)],cls)
def dot(x,y):s.append(f'<circle cx="{x}" cy="{y}" r="5" fill="#20394d"/>')
def terminal(x,y,one=False):
    s.append(f'<rect x="{x-7}" y="{y-7}" width="14" height="14" fill="white" stroke="#20394d" stroke-width="3"/>' if one else f'<circle cx="{x}" cy="{y}" r="7" fill="white" stroke="#20394d" stroke-width="3"/>')
def net(x,y,label,cls='gnd',direction=1):
    line(x,y,x+direction*70,y,cls);text(x+direction*70+(8 if direction==1 else -85),y-8,label,'small')
def resistor(x,y,label,vertical=False):
    if vertical:
        line(x,y,x,y+12);s.append(f'<rect x="{x-9}" y="{y+12}" width="18" height="38" fill="white" stroke="#20394d" stroke-width="2.5"/>');line(x,y+50,x,y+65);text(x+18,y+36,label,'small')
    else:
        line(x,y,x+12,y);s.append(f'<rect x="{x+12}" y="{y-9}" width="54" height="18" fill="white" stroke="#20394d" stroke-width="2.5"/>');line(x+66,y,x+80,y);text(x,y-19,label,'small')
def pullup(x,y,r):
    line(x,y,x,y-12)
    s.append(f'<rect x="{x-8}" y="{y-40}" width="16" height="28" fill="white" stroke="#20394d" stroke-width="2.5"/>')
    line(x,y-40,x,y-55);line(x,y-55,x+70,y-55,'pwr')
    text(x+78,y-48,'3V3','small');text(x+18,y-15,f'R{r} 10k*','small');dot(x,y)
def connector(x,y,w,h,title,rows,right=False):
    box(x,y,w,h);text(x+20,y+38,title,'head')
    edge=x if right else x+w
    for number,yy,label,kind in rows:
        terminal(edge,yy,number==1)
        text(x+28 if right else x+20,yy+8,f'{number}  {label}','pin')
        if kind:net(edge,yy,'3V3' if kind=='pwr' else label.split(' /')[0],kind,-1 if right else 1)

text(55,62,'Rehearsal controller - revision K','title')
text(55,105,'TOP / COMPONENT VIEW: antenna UP, USB DOWN. J1 = left header; J3 = right header. Solder-side view is mirrored.','small')
text(55,142,'X1-X7 identify panel harness contacts. Square contact = pin 1. Match wire labels at the device; do not assume its factory order.','small')
box(55,180,2990,105,'note')
text(80,220,'LCD INTERFACE: module SDA/SCL measured about 5V at 3.3V supply. U2 provides bidirectional I2C level translation.','pin')
text(80,257,'C3 + encoder + U2 LV = 3.3V. LCD VCC + U2 HV + Record LED branch = USB 5V. U2 is required; GPIO map / firmware unchanged.','small')
# Physical header order verified against Espressif DevKitM-1 pin-layout image.
box(1160,365,640,1315)
box(1360,305,240,90);text(1382,346,'ANTENNA','head');text(1382,377,'keep clear','small')
text(1275,438,'ESP32-C3-DevKitM-1','head')
text(1180,474,'J1 - LEFT','pin');text(1620,474,'J3 - RIGHT','pin')
left=['GND','3V3','3V3','IO2','IO3','GND','RST','GND','IO0','IO1','IO10','GND','5V','5V','GND']
right=['GND','TX / IO21','RX / IO20','GND','IO9','IO8','GND','IO7','IO6','IO5','IO4','GND','IO18','IO19','GND']
def y(n):return 520+(n-1)*76
roles_left={4:'free / strap',5:'SONG',7:'RESET',9:'TAKE',10:'RECORD',11:'REC LED CTRL'}
roles_right={2:'USB-UART',3:'USB-UART',5:'BACK / BOOT',6:'onboard RGB',8:'PLAY',9:'ENC A',10:'ENC B',11:'ENC SW',13:'LCD SDA',14:'LCD SCL'}
for n,(a,b) in enumerate(zip(left,right),1):
    yy=y(n)
    terminal(1160,yy,n==1);terminal(1800,yy,n==1)
    text(1180,yy+7,a,'pin');text(1585,yy+7,b,'pin')
    if n in roles_left:text(1180,yy+31,roles_left[n],'small')
    if n in roles_right:text(1585,yy+31,roles_right[n],'small')
    if a in ['GND','3V3','5V']:net(1160,yy,a,{'GND':'gnd','3V3':'pwr','5V':'five'}[a],-1)
    if b=='GND':net(1800,yy,b,'gnd')
box(1380,1655,200,65);text(1410,1697,'USB','head')
# Panel connector signal wires, each component's GPIOs stay on one header.
# X1 has separate switch and LED circuits; no raw 5V on harness.
connector(80,690,610,180,'X4 SONG (2 pins)',[(1,750,'GND / COM','gnd'),(2,y(5),'SONG / NO',None)])
line(690,y(5),1160,y(5));pullup(955,y(5),4)
connector(80,915,610,140,'X3 TAKE (2 pins)',[(1,970,'GND / COM','gnd'),(2,1010,'TAKE / NO',None)])
wire([(690,1010),(860,1010),(860,y(9)),(1160,y(9))]);pullup(1030,y(9),3)
connector(80,1100,610,320,'X1 RECORD + red LED (4 pins)',[(1,1160,'GND / switch COM','gnd'),(2,y(10),'RECORD / switch NO',None),(3,1270,'REC_A / LED anode +',None),(4,1340,'REC_K / LED cathode -',None)])
line(690,y(10),1160,y(10));pullup(955,y(10),1)
net(690,1270,'REC_A','five');net(690,1340,'REC_K','sig')
# LED drive stays on left with Record; route by a named signal to local Q1.
line(1160,y(11),990,y(11));text(940,y(11)+32,'REC_LED_CTL','small')
connector(2310,710,690,130,'X5 BACK / BOOT (2 pins)',[(1,770,'GND / COM','gnd'),(2,y(5),'BACK / NO',None)],True)
line(1800,y(5),2310,y(5));pullup(2040,y(5),5)
connector(2310,915,690,130,'X2 PLAY (2 pins)',[(1,975,'GND / COM','gnd'),(2,1015,'PLAY / NO',None)],True)
wire([(1800,y(8)),(2120,y(8)),(2120,1015),(2310,1015)]);pullup(1990,y(8),2)
# Order deliberately follows adjacent J3 signals A/B/SW to avoid crossing them.
connector(2310,1070,690,375,'X6 ENCODER (5 pins)',[(1,y(9),'A / CLK',None),(2,y(10),'B / DT',None),(3,y(11),'SW / push',None),(4,1375,'3V3 / module +','pwr'),(5,1420,'GND / common','gnd')],True)
for n,r in [(9,6),(10,7),(11,8)]:line(1800,y(n),2310,y(n));pullup(2010,y(n),r)
connector(2310,1500,690,255,'X7 LCD DFR0464 (4 pins)',[(1,1560,'SDA',None),(2,1610,'SCL',None),(3,1665,'5V / VCC','five'),(4,1720,'GND','gnd')],True)
for n,dest,x in [(13,1560,2130),(14,1610,2030)]:
    wire([(1800,y(n)),(x,y(n)),(x,dest),(2150,dest)])
    line(2280,dest,2310,dest)
    # Bus pull-ups shown in the lower electrical section to keep this map clear.
box(2150,1515,130,120);text(2158,1492,'U2 I2C shift','small')
text(2168,1568,'LV1 / HV1','small');text(2168,1618,'LV2 / HV2','small')
text(80,1450,'RECORD: J1.10 / GPIO1; LED: J1.11 / GPIO10.','pin')
text(80,1490,'RIGHT: encoder A/B/SW on J3.9 / .10 / .11;','pin')
text(80,1525,'PLAY; BACK; LCD SDA/SCL. No signal crossover.','pin')
text(80,1570,'GPIO2 spare. GPIO8 / 20 / 21 unchanged.','small')
text(80,1605,'BACK held during RESET enters download mode.','small')
text(80,1640,'J1/J3 numbers apply to official DevKitM-1.','small')
text(80,1675,'For a clone, verify silkscreen and header order.','small')
text(80,1710,'Use Rev K firmware with this pin map.','small')
# Electrical detail, with net labels deliberately avoiding long supply wires.
box(55,1820,1650,640);text(80,1865,'Record driver - place beside X1 / J1 on the small board','head')
text(80,1910,'REC_A / REC_K labels above connect to the same labels below.','small')
net(1290,1965,'5V','five',-1);resistor(1290,1965,'R11 1k',True)
line(1290,2030,1290,2070);text(1330,2050,'REC_A -> X1.3','pin')
s.append('<polygon points="1270,2070 1310,2070 1290,2100" fill="#dc3434" stroke="#20394d" stroke-width="3"/>');line(1270,2100,1310,2100)
text(1330,2100,'red LED in button','small');line(1290,2100,1290,2160);text(1330,2140,'REC_K -> X1.4','pin')
# Transistor symbol C top, B left, E lower right.
s.append('<circle cx="1270" cy="2210" r="51" fill="white" stroke="#20394d" stroke-width="3"/>')
line(1245,2175,1245,2245);line(1245,2190,1290,2160);line(1245,2230,1290,2260)
s.append('<path d="M1274 2238 L1287 2257 L1265 2251Z" fill="#20394d"/>')
text(1350,2205,'Q1 BC547C','pin');text(1350,2240,'C top; B left; E bottom','small')
line(1290,2260,1290,2300);net(1290,2300,'GND','gnd')
text(80,2180,'GPIO10 / J1.11','pin');text(80,2218,'REC_LED_CTL','small');line(330,2210,420,2210);resistor(420,2210,'R12 1k BASE');line(500,2210,1245,2210)
line(840,2210,840,2265);dot(840,2210);resistor(840,2265,'R13 100k',True);net(840,2330,'GND','gnd')
text(80,2390,'R11 is the LED current limit; R12 is the base resistor. Keep BOTH. No 5V on any GPIO.','small')
text(80,2425,'Red Vf estimated 1.8-2.2V, Q1 drop 0.2V: 1k at USB 5V gives 2.6-3mA. Rating/Vf unmeasured.','small')
box(1745,1820,1300,640);text(1770,1865,'C3-side pull-ups / bypass + transistor pinout','head')
for x,name,netname in [(1830,'R9 4.7k*','SDA_L / U2 LV1'),(2120,'R10 4.7k*','SCL_L / U2 LV2')]:
    net(x,1950,'3V3','pwr');resistor(x,1950,name,True);line(x,2015,x,2050);text(x-25,2085,netname,'small')
text(1770,2140,'C1 100nF ceramic: X7.3 (5V) to X7.4 (GND), near LCD.','small')
text(1770,2175,'C2 10uF optional across 3V3 / GND. Ceramic has no polarity.','small')
text(1770,2235,'Q1 reference: flat face toward you, legs DOWN:','pin')
box(2240,2260,225,70);text(2290,2306,'BC547C','pin')
for x,name in [(2260,'1 C'),(2350,'2 B'),(2440,'3 E')]:line(x,2330,x,2370);text(x-20,2400,name,'pin')
text(1770,2435,'onsemi BC550/D p1. Unmarked part: identify C/B/E before soldering.','small')
box(55,2505,2990,325,'note');text(80,2550,'Connector convention and component-end soldering','head')
for i,v in enumerate([
'Numbered contacts are seen from the COMPONENT SIDE of the perfboard. Mark square pin 1 on board and cable; the solder-side view is mirrored.',
'X1: pin1 -> switch COM; pin2 -> switch NO; pin3 -> LED +; pin4 -> LED -. Switch and LED are separate circuits; leave any NC terminal unused.',
'X2-X5: pin1 -> COM / GND; pin2 -> NO / signal. A bare two-terminal NO button has no polarity. No external lamp connection on these headers.',
'X6: 1 A/CLK, 2 B/DT, 3 SW, 4 +3.3V, 5 GND. Match actual module labels. Bare encoder: pin4 unused; common + one push terminal -> pin5.',
'X7: 1 SDA_H, 2 SCL_H, 3 VCC/USB 5V, 4 GND. Adapt to the LCD connector labels; this is NOT a claim about the factory connector order.',
'Use separate keyed plugs or clear labels. If your connector has molded numbers, use its pin-1 marking and map wires accordingly; do not guess latch orientation.'
]):text(80,2600+i*36,v,'small')
box(55,2870,2990,290);text(80,2915,'Small-board placement and power notes','head')
for i,v in enumerate([
'R1-R8: 10k pull-ups to 3V3 on all eight input nets. Omit matching encoder pull-ups only if the powered module already supplies them at 3.3V.',
'R9/R10: effective 4.7k pull-ups to 3V3 on U2 LOW side. Module resistors can replace these. LCD pull-ups stay on HIGH side.',
'Power: board USB is the sole source; J1.2/.3 = 3V3, J1.13/.14 = USB 5V. J3 has GND pins but no 3V3 pin. Route supplies as convenient.',
'Place sockets for C3 in the middle; X1 + Q1/R11/R12/R13 on J1 side, X6 + X7 on J3 side. Leave antenna clear of wiring, copper and battery.',
'Power OFF for rewiring. Load Rev K firmware, then verify controls/encoder direction/LED with simulator. No battery/charger wiring is included.'
]):text(80,2965+i*36,v,'small')
box(55,3205,2990,455,'note');text(80,3250,'U2 REQUIRED: two-channel bidirectional open-drain I2C level shifter (BSS138-type module)','head')
for i,v in enumerate([
'U2 installation has not been bench-validated. Connections below use module LABELS; physical pin order depends on the module.',
'LOW side: LV -> C3 3V3; LV1 -> GPIO18 / SDA_L; LV2 -> GPIO19 / SCL_L. Pull-ups on this side go ONLY to 3V3.',
'HIGH side: HV -> USB 5V; HV1 -> X7.1 / LCD SDA_H; HV2 -> X7.2 / LCD SCL_H. LCD VCC / X7.3 -> USB 5V.',
'GND: U2, LCD / X7.4 and C3 share ground. Leave unused translator channels unconnected. Keep existing onboard LCD pull-ups.',
'Check both sides with C3 signal wires detached: LOW SDA/SCL near 3.3V, HIGH SDA/SCL near 5V. Then power OFF and connect C3.',
'A plain resistor divider, series resistor or one-way logic buffer is not a substitute for bidirectional I2C translation.',
'U2 uses the existing SDA/SCL GPIOs. Optical LCD response after translator installation has not been measured.',
'Reference topology: NXP AN10441 section 2.1. For a selected commercial module verify its supply labels and schematic.'
]):text(80,3300+i*41,v,'small')
s.append('</svg>')
(P/'controller-full.svg').write_text('\n'.join(s))
print('Revision K schematic generated; firmware GPIO map matches and has no collisions.')
