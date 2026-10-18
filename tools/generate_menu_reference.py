#!/usr/bin/env python3
"""Generate docs from actual firmware renderer snapshots and shared error table."""
from pathlib import Path
from html import escape
import json
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'docs'
with tempfile.TemporaryDirectory(prefix='reha-menu-doc-') as temp:
    binary=Path(temp)/'screens'
    subprocess.run(['c++','-std=c++11','-I',str(ROOT/'tests/lcd_stubs'),str(ROOT/'tests/menu_screens.cpp'),'-o',str(binary)],check=True)
    data=json.loads(subprocess.check_output([str(binary)]))
screens=data['screens'];errors=data['errors']
def marked(row):return row[:16].ljust(16)+'-'+row[16:]
for s in screens:
    for full,visible in zip(s['rows'],s['visible']):
        assert visible==full[:16].ljust(16),(s['id'],full,visible)
        line=marked(full);assert line[16]=='-' and line[:16]+line[17:]==full.ljust(16)
assert len({s['id'] for s in screens})==len(screens)
assert [e['code'] for e in errors]==list(range(1,14))
intro='''# LCD menu and screen reference — revision K

Generated from the DesktopPanel renderer and the shared error table in
PanelDisplay.hpp. Each row inserts `-` after column 16; characters to its right
are retained in this reference but are not shown on the LCD. Short rows are padded.
Text does not scroll.

## Home

The top row shows `Tnn-Song`: T00 for an explicitly empty song, T01 for take 1,
T100 for take 100, and T# for an unknown take. The bottom row shows READY, REC,
PLAY or SAVE, with `V:000`–`V:100` in columns 12–16. V:--- means unknown volume.
A pending transport request appends `*` to its status until acknowledgement and
matching state feedback arrive. Record illumination follows confirmed host state.

## Menu and selection

The menu contains Restart take, Stop rehearsal, Remove N take, Brightness and
LCD timing test. Remove is hidden for zero/unknown take. Restart, Stop and Remove
require confirmation: encoder confirms, BACK cancels. Stop and Remove require
READY. Start rehearsal is available at the explicit connected idle prompt and
sends immediately. Missing host capabilities display E013.

Hold TAKE or SONG and turn to preview a cached selection; release to apply it.
TAKE shows `select take` / `take : nn`; SONG shows `select song` / song name.
An incomplete list shows `Updating list` below the unchanged heading. Turns
while loading are ignored, and release sends no command. Background catalog
updates resume selection when complete. BACK cancels preview.

## Brightness

Turn in 5% steps over 0–100%; encoder saves, BACK cancels. Default 10%.
The percentage persists in controller Preferences and scales status colours.
Text contrast has no software setting in the documented LCD instruction set.

## LCD timing test

Encoder press cycles ZERO (LED off) → RUN (green) → STOP (red) → ZERO.
The lower row shows milliseconds, sampled every 10ms during RUN. STOP freezes
elapsed time at accepted input. Rotation is ignored; BACK exits. Host loss or
an incompatible transport state closes the test. Record/Play buttons retain
their transport actions. See [timing diagnostic](controller-diagnostics.md#lcd-timing-test).

## Display priority

1. Disconnected, missing/stale host feedback, incompatible protocol or host error.
2. Conflicting TAKE + SONG buttons (E011).
3. Transient errors on the bottom row; the current heading remains visible.
4. Held selection and catalog-loading screens.
5. Confirmation, idle, menu or home.

Errors omit volume. Transient errors clear after two seconds, BACK, or an accepted
command. Persistent conditions remain until resolved. Additional local presses
while a command is pending are ignored. REC/SAVE closes the menu and disables
selection. Bluetooth loss shows `Bluetooth MIDI` / `Disconnected`; absent or stale
recorder feedback shows `Bluetooth MIDI` / `Waiting recorder`.

## Rendered screens

'''
behavior='''
## Controls and transitions

| Input | Behaviour |
| --- | --- |
| Encoder turn at home | Relative playback volume; wait for host feedback to display it. |
| Encoder press at home | Open menu at READY/PLAY; start rehearsal at explicit idle. |
| Encoder turn in menu | Cycle available items. |
| Encoder press on Restart / Stop / Remove | Open confirmation. |
| Encoder press in confirmation | Submit the action if capability/state and captured identity still match. |
| Encoder press in Brightness | Save changed brightness and return to menu. |
| BACK in confirmation / Brightness / LCD test | Cancel or exit to menu. |
| BACK in menu | Return home. |
| Hold TAKE / SONG and turn | Preview cached takes / songs in READY/PLAY. |
| Stable release | Apply one changed selection from the current catalog. |
| Hold TAKE and SONG | E011; no selection or volume action. |
| BACK while held | Cancel selection until both buttons are released; return home. |
| Encoder press while TAKE / SONG held | Ignored. |
| Press while acknowledgement/state pending | Ignored. |
| Turn during pending selection or mode transition | Discarded. |
| Record / Play | Transport action subject to confirmed host state. |
| BACK held during RESET | ROM download mode; no firmware LCD screen. |

## Errors

'''
error_table='| Code | Meaning | What to do |\n| --- | --- | --- |\n'+''.join(f'| E{e["code"]:03d} | {e["meaning"]} | {e["recovery"]} |\n' for e in errors)
error_footer='''
E001–E005 describe persistent conditions. E001 and E002 use connection-status
text on the LCD. E006–E010 and E012–E013 are transient; E011 lasts while both
selection buttons are held. Error screens omit volume.

Codes are local LCD identifiers. Host state 4 maps to E004; a command Error result
maps to E009. Neither carries the recorder's detailed cause. The recorder log
contains application-specific details. A timeout does not establish whether the
action executed; Record/Play toggles are not retried automatically.
'''
md=intro
for s in screens:
    md+=f'### {s["id"]} — {s["description"]}\n\n```text\n'+'\n'.join(marked(row) for row in s['rows'])+'\n```\n\n'
md+=behavior+error_table+error_footer+'\n## Map\n\n[Action-labelled state map](menu-state-map.svg) · [Offline HTML reference](menu-reference.html)\n'
OUT.mkdir(exist_ok=True)
(OUT/'menu-reference.md').write_text(md)
(OUT/'lcd-errors.md').write_text('# LCD error codes — revision K\n\nGenerated from the shared firmware error catalogue in PanelDisplay.hpp.\n\n'+error_table+error_footer)
# Native SVG with every menu/confirmation state and action labels.
svg=['<svg xmlns="http://www.w3.org/2000/svg" width="1700" height="2650" viewBox="0 0 1700 2650"><defs><marker id="arrow" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto"><path d="M0 0L10 5L0 10Z" fill="#45667a"/></marker></defs><rect width="1700" height="2650" fill="#f8fafc"/><style>text{font-family:Arial,sans-serif;fill:#163447;font-size:18px}.head{font-size:26px;font-weight:bold}.node{fill:white;stroke:#7397ac;stroke-width:2}.edge{fill:none;stroke:#45667a;stroke-width:2;marker-end:url(#arrow)}.label{font-size:16px;paint-order:stroke;stroke:#f8fafc;stroke-width:6px}.lcd{font-family:monospace;white-space:pre}</style>']
by_id={s['id']:s for s in screens}
def t(x,y,value,cls=''):svg.append(f'<text x="{x}" y="{y}" class="{cls}" xml:space="preserve">{escape(value)}</text>')
def node(id,x,y,title):
    svg.append(f'<a href="menu-reference.html#{id}"><rect x="{x}" y="{y}" width="450" height="112" rx="10" class="node"/>')
    t(x+15,y+28,title)
    for i,row in enumerate(by_id[id]['rows']):t(x+15,y+60+i*28,marked(row),'lcd')
    svg.append('</a>')
def edge(points,label,x,y):
    svg.append('<path d="M'+' L'.join(f'{a} {b}' for a,b in points)+'" class="edge"/>');t(x,y,label,'label')
t(40,45,'Revision K — startup, context menu and confirmation','head')
t(40,80,'Firmware renderer snapshots. Idle, lifecycle and deletion require negotiated host capabilities.')
node('flow-power',40,130,'NO BLUETOOTH MIDI CONNECTION')
node('flow-sync',625,130,'PC CONNECTED, WAITING FOR APP')
node('idle-start',1210,130,'EXPLICIT IDLE: NO REHEARSAL')
edge([(490,170),(625,170)],'BLE up',515,155)
edge([(1075,170),(1210,170)],'Idle + bit32',1090,155)
node('home-ready',40,410,'ACTIVE REHEARSAL HOME')
node('idle-start-pending',1210,410,'START REQUEST PENDING')
edge([(1435,242),(1435,410)],'Encoder press → Note71',1450,320)
t(1210,290,'Start sends immediately; no confirmation.')
edge([(1210,475),(490,475)],'READY feedback → HOME',570,460)
edge([(1075,225),(1130,225),(1130,340),(265,340),(265,410)],'Existing rehearsal READY → HOME',510,325)
t(40,575,'Menu: encoder press opens Restart; turn cycles visible items. BACK returns HOME.')
node('menu-restart',40,650,'MENU 1: RESTART TAKE')
node('menu-stop',625,650,'MENU 2: STOP REHEARSAL')
node('menu-remove',1210,650,'MENU 3: REMOVE SELECTED TAKE')
edge([(265,522),(265,650)],'Encoder press',280,610)
edge([(490,695),(625,695)],'Turn +',520,680)
edge([(1075,695),(1210,695)],'Turn +',1100,680)
t(40,810,'Turn wraps both directions. Remove is hidden for T00 / T#. Restart also requires confirmation.')
node('confirm-stop',625,930,'CONFIRM RESTART / STOP / REMOVE')
edge([(850,762),(850,930)],'Press Stop',865,855)
edge([(265,762),(265,840),(735,840),(735,930)],'Press Restart',340,825)
edge([(1435,762),(1435,865),(960,865),(960,930)],'Press Remove N',1120,850)
edge([(625,990),(570,990),(570,795),(850,795),(850,762)],'BACK: cancel; return to same item',80,885)
t(40,1100,'Encoder confirms: Restart → Note63; Stop → Note72; Remove → Note73. Blank second line. Rotation has no effect.')
t(40,1135,'No side effect before confirmation. Only READY can confirm Stop/Remove. Unsupported host → E013, no command.')
t(40,1170,'Remove captures the selected identity; changed metadata cancels confirmation. The host validates its bound target.')
t(40,1205,'Confirmation: encoder press confirms; BACK cancels; encoder rotation is ignored.')
node('held-take',40,1320,'TAKE HELD')
node('held-song',1210,1320,'SONG HELD')
edge([(40,465),(15,465),(15,1370),(40,1370)],'Hold TAKE from HOME/menu; release restores',40,1290)
edge([(490,500),(1170,500),(1170,1370),(1210,1370)],'Hold SONG; release restores',1210,1290)
t(40,1490,'Held turn: local preview only. Stable release commits one selection. BACK or changed catalog cancels; no queued turns.')
t(40,1525,'BACK: return HOME and cancel held modes until released. Both held: E011. Confirmation suppresses held navigation.')
node('error-004',40,1640,'HOST / PROTOCOL / STALE ERROR')
node('error-013',1210,1640,'UNSUPPORTED HOST MENU ACTION')
t(40,1800,'Absent/stale host returns to Waiting recorder; reopening resynchronizes. REC/SAVE closes menus.')
t(40,1835,'Record/Play remain host-gated. Lamp uses confirmed state. Duplicate action presses are silently ignored; no retry.')
t(40,1870,'Startup: power controller, connect Bluetooth MIDI, launch recorder, then start a rehearsal from the laptop or idle prompt.')
t(40,1950,'LOCAL BRIGHTNESS: turn through the menu to Brightness; Remove is skipped when there are no takes.')
node('menu-brightness',40,2000,'MENU: BRIGHTNESS')
node('edit-brightness',1210,2000,'BRIGHTNESS PREVIEW')
edge([(490,2030),(1210,2030)],'Encoder press: edit; turn previews 0–100%',630,2015)
edge([(1210,2090),(490,2090)],'Press: save / BACK: cancel → Brightness menu',580,2135)
t(40,2200,'Stored locally across reboot. No MIDI commands; transport state colours retain their hue.')
t(40,2310,'LCD TIMING DIAGNOSTIC: open from menu. BACK exits; rotation is ignored. No MIDI commands.')
node('lcd-test-zero',40,2360,'ZERO: ONBOARD LED OFF')
node('lcd-test-run',625,2360,'RUN: ONBOARD LED GREEN')
node('lcd-test-stop',1210,2360,'STOP: ONBOARD LED RED')
edge([(490,2400),(625,2400)],'Press',530,2385)
edge([(1075,2400),(1210,2400)],'Press',1115,2385)
edge([(1435,2472),(1435,2540),(265,2540),(265,2472)],'Press: reset to ZERO',720,2580)
svg.append('</svg>');svg_text='\n'.join(svg)
(OUT/'menu-state-map.svg').write_text(svg_text)
html=['''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>LCD menu — revision K</title><style>
body{margin:0;background:#edf2f7;color:#18384a;font:16px/1.5 system-ui,sans-serif}main{max-width:1500px;margin:auto;padding:28px}h1{font-size:30px}.note{padding:18px;background:#fff3d5;border-radius:10px}.map{background:white;overflow:auto;border:1px solid #cedbe5;border-radius:12px}.map svg{width:100%;height:auto;min-width:1050px}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(350px,1fr));gap:16px}.card{background:white;padding:18px;border:1px solid #cbd8e3;border-radius:12px;scroll-margin-top:15px}.card:target{outline:4px solid #d18a25}.card h3{font-size:17px;margin:0 0 10px}.lcd{background:#152d32;color:#d6ffcc;font:17px/1.6 ui-monospace,SFMono-Regular,Consolas,monospace;padding:14px;white-space:pre;overflow:auto;border-radius:8px}.end{color:#ffb64f;font-weight:bold}.id{font:12px monospace;color:#617987}pre.source{white-space:pre-wrap;font-size:14px}a{color:#116484}table{border-collapse:collapse;width:100%;background:white}td,th{border:1px solid #cbd8e3;padding:10px;text-align:left;vertical-align:top}</style><main>
<h1>LCD screens</h1><p>Generated from actual firmware renderer snapshots.</p>
<div class="note">Menu: <strong>Restart take / Stop rehearsal / Remove N take / Brightness / LCD timing test</strong>. Home shows <strong>Tn-Song</strong> and status + <strong>V:nnn</strong> percentage.<br>Each LCD row inserts <strong>- after column 16</strong>. Overflow stays visible for review; it is not displayed on hardware. No scrolling or volume popup.</div>
<h2>Action-labelled map</h2><div class="map">''',svg_text,'</div><h2>All screen variants</h2><div class="grid">']
for s in screens:
    rows=[escape(r[:16].ljust(16))+'<span class="end">-</span>'+escape(r[16:]) for r in s['rows']]
    html.append(f'<article class="card" id="{s["id"]}"><div class="id">{s["id"]}</div><h3>{escape(s["description"])}</h3><div class="lcd">'+'\n'.join(rows)+'</div></article>')
html+=['</div><h2>Error codes</h2><table><tr><th>Code</th><th>Meaning</th><th>What to do</th></tr>']
for e in errors:html.append(f'<tr><td>E{e["code"]:03d}</td><td>{escape(e["meaning"])}</td><td>{escape(e["recovery"])}</td></tr>')
html+=['</table><h2>Detailed behavior</h2><pre class="source">',escape(intro+behavior+error_footer),'</pre></main></html>']
(OUT/'menu-reference.html').write_text('\n'.join(html))
print(f'Generated {len(screens)} actual renderer snapshots; {len(errors)} error codes; HTML/Markdown/SVG/error reference.')
