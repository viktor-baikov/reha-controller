# LCD menu and screen reference — revision K

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

### boot — Startup buffer only; normal update may replace it before LCD initialization completes

```text
Starting...     -
Reha controller -
```

### home-ready — Home: ready; volume 50 percent

```text
T12-Midnight reh-earsal song
READY      V:050-
```

### menu-lcd-test — Temporary LCD measurement menu

```text
LCD timing test -
Press to test   -
```

### lcd-test-zero — LED off; press to start

```text
LCD timer ZERO  -
0000000000 ms   -
```

### lcd-test-run — LED green; press freezes elapsed milliseconds

```text
LCD timer RUN   -
0000001234 ms   -
```

### lcd-test-stop — LED red; press resets to zero

```text
LCD timer STOP  -
0000001234 ms   -
```

### menu-brightness — Local backlight setting

```text
Brightness      -
Press to adjust -
```

### edit-brightness — Turn to preview; press saves, BACK cancels

```text
Brightness      -
50%  Press save -
```

### home-rec — Home: recording; volume remains visible

```text
T12-Midnight reh-earsal song
REC        V:050-
```

### home-play — Home: playback; volume remains visible

```text
T12-Midnight reh-earsal song
PLAY       V:050-
```

### home-save — Home: saving; lamp flashes

```text
T12-Midnight reh-earsal song
SAVE       V:050-
```

### home-zero — Home: volume zero

```text
T12-Midnight reh-earsal song
READY      V:000-
```

### home-full — Home: volume 100 percent

```text
T12-Midnight reh-earsal song
READY      V:100-
```

### home-volume-unknown — Home: no volume feedback yet

```text
T12-Midnight reh-earsal song
READY      V:----
```

### home-take-unknown — Host supplied song without take number: do not invent one

```text
T#-Midnight rehe-arsal song
READY      V:050-
```

### waiting-initial — Feedback not fresh, with no identity or volume available

```text
Bluetooth MIDI  -
Waiting recorder-
```

### home-empty — No identity metadata yet

```text
T#-?            -
READY      V:050-
```

### menu-restart — Restart take item: press requests restart

```text
Restart take    -
Press to restart-
```

### menu-stop — Stop rehearsal menu item; press opens confirmation

```text
Stop rehearsal  -
Press to stop   -
```

### menu-remove — Remove selected take; hidden for zero/unknown

```text
Remove 12 take  -
Press to remove -
```

### confirm-stop — Stop rehearsal confirmation: encoder yes, BACK no; blank second row

```text
are you sure?   -
                -
```

### confirm-remove — Remove selected take confirmation: encoder yes, BACK no

```text
are you sure?   -
                -
```

### idle-start — Explicit host idle plus capability: connected / start rehersal?

```text
connected       -
start rehersal? -
```

### idle-start-pending — Start sends immediately; pending star without confirmation

```text
connected      *-
start rehersal? -
```

### confirm-restart — Restart take confirmation: encoder yes, BACK no

```text
are you sure?   -
                -
```

### menu-start-unavailable — Recorder not ready overrides and closes the menu

```text
T12-Midnight reh-earsal song
E005            -
```

### home-pending — Home: pending acknowledgement adds a star to status, not song

```text
T12-Midnight reh-earsal song
READY*     V:050-
```

### menu-pending — Menu: command awaiting acknowledgement

```text
Restart take   *-
Press to restart-
```

### held-take — Hold TAKE; turn for previous/next take

```text
select take     -
take : 12       -
```

### held-song — Hold SONG; turn for previous/next song

```text
select song     -
Midnight rehears-al song
```

### held-take-updating — Take list loading: knob ignored, heading unchanged

```text
select take     -
Updating list   -
```

### held-song-updating — Song list loading: knob ignored, heading unchanged

```text
select song     -
Updating list   -
```

### held-take-unknown — TAKE: number not supplied by host

```text
select take     -
take : 00       -
```

### held-song-unknown — SONG: name not supplied by host

```text
select song     -
?               -
```

### held-take-pending — TAKE request awaiting acknowledgement

```text
select take     -
Updating list   -
```

### held-song-pending — SONG request awaiting acknowledgement

```text
select song     -
Updating list   -
```

### flow-power — Controller powers up before BLE connects

```text
Bluetooth MIDI  -
Disconnected    -
```

### flow-sync — BLE connected; waiting for initial recorder feedback

```text
Bluetooth MIDI  -
Waiting recorder-
```

### flow-idle — Healthy recorder connected; no rehearsal open

```text
connected       -
start rehersal? -
```

### home-no-takes — Host explicitly reports no takes as T0

```text
T00-Midnight    -
READY      V:050-
```

### flow-ready — Laptop has started rehearsal; song Midnight, no kept takes yet

```text
T00-Midnight    -
READY      V:050-
```

### flow-record-request — Record request sent; REC* means requested, not confirmed

```text
T00-Midnight    -
REC*       V:050-
```

### flow-record-ack — Acknowledgement accepted; retain REC* until recording state arrives

```text
T00-Midnight    -
REC*       V:050-
```

### flow-recording — Recorder confirms recording take 1; lamp now on

```text
T01-Midnight    -
REC        V:050-
```

### flow-stop-request — Record pressed again; stop/keep requested

```text
T01-Midnight    -
SAVE*      V:050-
```

### flow-stop-ack — Stop accepted; waiting for recorder Saving/Ready state

```text
T01-Midnight    -
SAVE*      V:050-
```

### flow-saving — Recorder confirms Saving; lamp blinks

```text
T01-Midnight    -
SAVE       V:050-
```

### flow-kept — Recorder confirms Ready; take kept; sequence ends

```text
T01-Midnight    -
READY      V:050-
```

### error-001 — Bluetooth MIDI disconnected

```text
Bluetooth MIDI  -
Disconnected    -
```

### error-002 — Recorder feedback missing or stale (diagnostic category)

```text
Bluetooth MIDI  -
Waiting recorder-
```

### error-003 — Unsupported protocol version

```text
T12-Midnight reh-earsal song
E003            -
```

### error-004 — Recorder reported ERROR

```text
T12-Midnight reh-earsal song
E004            -
```

### error-005 — Recorder not ready

```text
T12-Midnight reh-earsal song
E005            -
```

### error-006 — Command busy

```text
T12-Midnight reh-earsal song
E006            -
```

### error-007 — Requested action unavailable

```text
T12-Midnight reh-earsal song
E007            -
```

### error-008 — Relative volume unsupported

```text
T12-Midnight reh-earsal song
E008            -
```

### error-009 — Recorder command failed

```text
T12-Midnight reh-earsal song
E009            -
```

### error-010 — Command confirmation timed out

```text
T12-Midnight reh-earsal song
E010            -
```

### error-011 — TAKE and SONG held together

```text
T12-Midnight reh-earsal song
E011            -
```

### error-012 — Start rehearsal unsupported in this context

```text
T12-Midnight reh-earsal song
E012            -
```

### error-013 — Recorder does not support this menu action

```text
T12-Midnight reh-earsal song
E013            -
```


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

| Code | Meaning | What to do |
| --- | --- | --- |
| E001 | Bluetooth MIDI disconnected | Connect the controller to one MIDI host. USB provides power, not USB MIDI. |
| E002 | Recorder feedback missing or stale (diagnostic category) | The LCD shows Bluetooth MIDI / Waiting recorder, including after app closure. Feedback expires after 3.5 seconds. Reopen the recorder and check both MIDI ports; commands stay disabled until fresh feedback returns. |
| E003 | Unsupported protocol version | Use matching controller/recorder protocol versions. No command is sent until compatible. |
| E004 | Recorder reported ERROR | Inspect the recorder's actual error/log and preserve pending takes. CC119=4 carries no detailed cause; this code does not diagnose it. |
| E005 | Recorder not ready | Recorder sends state 5. It indicates a pending take or library problem; an older host also uses it for no active rehearsal. A healthy new host sends explicit idle state 6. Inspect the laptop; the code alone cannot distinguish these. |
| E006 | Command busy | The recorder rejected this request as Busy; it was not queued. Wait for its current work to finish before deliberately retrying. Additional local presses while waiting are silently ignored and never show this code. No automatic retry. |
| E007 | Requested action unavailable | Check recorder state and navigation support. Also covers a host Unavailable result. Stop recording/saving before navigation. |
| E008 | Relative volume unsupported | Update the recorder to support relative-volume capability. No guessed absolute setting is sent. |
| E009 | Recorder command failed | Inspect recorder error/log; retain any unsaved take. Result Error carries no detailed cause. |
| E010 | Command confirmation timed out | Check both MIDI directions and recorder health. Acknowledgement timeout is 1.5 seconds; after an accepted transport request, state confirmation timeout is 3.5 seconds; do not blindly retry a Record/Play toggle. |
| E011 | TAKE and SONG held together | Release one button, or BACK to cancel held modes until both are released. |
| E012 | Start rehearsal unsupported in this context | Use the explicit connected idle start screen with a lifecycle-capable host. Check saved audio setup and matching lifecycle-capable recorder. |
| E013 | Recorder does not support this menu action | Stop rehearsal and Remove take require the new optional host capabilities. No command is sent to an older recorder. Removal also requires matching selection-reference support. |

E001–E005 describe persistent conditions. E001 and E002 use connection-status
text on the LCD. E006–E010 and E012–E013 are transient; E011 lasts while both
selection buttons are held. Error screens omit volume.

Codes are local LCD identifiers. Host state 4 maps to E004; a command Error result
maps to E009. Neither carries the recorder's detailed cause. The recorder log
contains application-specific details. A timeout does not establish whether the
action executed; Record/Play toggles are not retried automatically.

## Map

[Action-labelled state map](menu-state-map.svg) · [Offline HTML reference](menu-reference.html)
