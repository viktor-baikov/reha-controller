# Controller protocol v1

All traffic uses MIDI 1.0 channel 16 on a dedicated controller route, separate
from recorded instrument lanes. The contract is independent of native BLE MIDI
or the optional bridge. The recorder owns transport and file state.

## Base messages

| Direction | Message | Meaning |
| --- | --- | --- |
| Controller → host | CC117 = 1 | HELLO; new connection epoch / snapshot request |
| Host → controller | CC117 = 1 | Supported protocol major version |
| Host → controller | CC119 = state | Confirmed application state |
| Host → controller | CC118 = counter | Heartbeat every second, wraps at 127 |
| Controller → host | CC113 = token | Token for following command, modulo 128 |
| Controller → host | Note On, velocity 127 | Command press |
| Controller → host | Matching Note Off, velocity 0 | Release; no action |
| Host → controller | CC114 = token, then CC116 = result | Command acknowledgement |

States: **0 Ready, 1 Recording, 2 Playing, 3 Saving, 4 Error, 5 Unavailable/not
ready, 6 Idle**. Idle requires negotiated lifecycle support. State5 is not a
Bluetooth disconnection; legacy hosts also use it for no open rehearsal.
Results: **1 Accepted, 2 Busy, 3 Unavailable, 4 Error**. Result5 is a local timeout,
never sent by the host.

HELLO clears duplicate/held bookkeeping and queued relative volume, not audio
state. The host responds with version, state and heartbeat, then negotiated
metadata. Snapshots repeat each second and after changes. Duplicate token/note
pairs return cached results without repeating an operation. Only positive-velocity
Note On acts; repeated presses while held and Note Off do not act.

## Handshake, timing and recovery

Commands require a supported version, current state and fresh heartbeat. Heartbeat
expires after **3.5s**. One command may be outstanding. Acknowledgement timeout is
**1.5s**; tokens must match. Transport/lifecycle requests also await matching
state: Accepted alone does not clear the pending star. Missing matching state
times out at 3.5s from the request. No toggle is automatically replayed.

Each BLE connection sends HELLO at least once, even if periodic feedback arrives
first. Retry after 1s without fresh feedback, or after 2.5s when advertised text
support has not committed both initial rows. Valid empty rows count as received.
Retries wait for pending commands to settle and never repeat actions.

Disconnect clears synchronization and pending requests. Host loss invalidates
metadata, volume and cached previews, disables controls, and displays
`Bluetooth MIDI` / `Waiting recorder`. Reopening the recorder resynchronizes.
Actual BLE disconnect displays `Bluetooth MIDI` / `Disconnected`. Neither event
stops recorder audio. Old queued commands are discarded.

## Capabilities

Controller CC111 bits:

| Bit | Meaning |
| --- | --- |
| 1 | Bounded two-row text |
| 2 | Atomic selected-take removal reference |
| 4 | Explicit idle/lifecycle |
| 8 | Cached browsing |
| 16 | Token-bound readiness and status requests |

Current desktop firmware advertises **31**. Host CC112 bits are 1 navigation,
2 absolute playback volume, 4 text, 8 relative volume, 16 mute, 32 lifecycle,
64 removal. Lifecycle/removal are enabled only for compatible controller bits.
Host CC110 bit1 independently advertises cached browsing; bit2 advertises
readiness status for controllers advertising CC111 bit16. Current fully capable
host CC112 is 127. Capabilities clear on disconnect. Optional extensions retain
major1; unsupported commands are not sent.

## Commands

All notes use token + positive Note On + Note Off, with host acknowledgement.

| Note | Operation / capability |
| --- | --- |
| 60 | Record/Stop |
| 62 | Play/Stop |
| 63 | Restart playback position / navigation |
| 64 / 66 | Previous / next kept take / navigation |
| 65 / 67 | Seek −1s / +1s, clamped / navigation |
| 68 / 69 | Previous / next song, clamped / navigation |
| 70 | Toggle playback mute / mute |
| 71 / 72 | Start / Stop rehearsal / lifecycle |
| 73 | Remove selected take / removal + immutable reference |
| 74 / 75 | Absolute take / song selection / cached browsing + request frame |

Record from Ready starts a take; from Playing stops playback and starts recording;
from Recording stops, finalizes and keeps it. Play from Ready opens the selected
kept take, defaulting to the current song's latest; with none it is unavailable.
Play from Playing stops/reset playback. Physical Play presses during Recording
or Saving are silently ignored locally. Saving rejects other unsupported actions
as Busy. Ignored duplicate local requests create no token, queue or screen flash.

Take navigation is confined to the current song; empty songs remain T00 and
boundaries are successful no-ops. Changing song closes playback and chooses the
song for the next recording. Navigation is unavailable during recording/saving.
Busy navigation/release acknowledgements are silently dropped, without retry.
Other command errors use the [LCD error table](lcd-errors.md).

Start uses saved available recording setup and creates a rehearsal without
recording. Missing setup is refused without creating one. Stop finishes only
at Ready and preserves kept takes; pending drafts or active playback prevent it.
Restart, Stop and Remove require panel confirmation; Start does not. Stop and
Remove require Ready. Removal uses the recorder's ordinary undoable path.
Failures preserve unsaved drafts. Host execution rechecks actual state under
its transport gate.

## Playback volume and mute

CC20 is absolute master playback volume 0..127 in both directions; it never
changes recording gain. CC21 controller → host is relative volume, delta=value−64
(64 no-op). Desktop turns use ±2 per detent, coalesced at up to 20Hz. Host operations
apply in order against live volume, clamp to 0..127 and return CC20. The encoder uses relative changes against host feedback.

Host CC22 reports mute (0 unmuted,127 muted). Note70 toggles it; no inbound CC22
command exists. Mute preserves saved volume/track balance and persists between
takes during the app run. The current desktop panel has no mute or seek gesture;
those wire/serial commands remain available for diagnostics.

## Text rows

CC100 selects row0/1; CC101 declares length0..32; CC102 repeats printable ASCII
bytes; CC103 commits checksum `(row + length + sum(bytes)) & 127`. Invalid or
incomplete frames never replace a row. Assembly expires after 1.5s. Host text
traffic is bounded to four CCs per 50ms tick, with unchanged rows suppressed and
Unicode normalized for the character display.

The desktop renderer displays static 16-character rows, truncating without
scrolling. It formats host take numbers to at least two digits: T00, T01, …,
T100. This is presentation only. No host identity is inferred when missing.
Home is `Tnn-Song` / status plus `V:nnn` percentage. Pending transport marks the
status (`REC*`, `SAVE*`, etc.), never the song. See [screen reference](menu-reference.html).

## Immutable removal reference

Host CC104..108 carry a 32-bit reference as five low-to-high 7-bit chunks (top ≤15).
CC109 commits sum(chunks)&127. Zero invalidates selection; CC104 invalidates a
previous reference immediately. The host sends zero before changed rows, then
both rows, then the new nonzero reference. No take means numeric zero and no
removable reference.

Confirmation captures displayed identity and reference; changes cancel it.
Before Note73, the controller echoes the six reference CCs before token and
press/release. The host consumes the reference once, checks the published
selection and binds it to rehearsal, database identity and files. It checks
again under transport/file locks, so changed selection cannot retarget removal.
HELLO/reconnect invalidates previous references; duplicate tokens cannot repeat
removal. GUI Undo remains available through the normal recorder path.

## Cached browsing and selection on release

Hold TAKE/SONG and rotate to preview a local cached list; no per-detent commands
are sent. TAKE contains actual kept takes for the selected song, including number
gaps. Empty songs remain `take : 00`. Stable release (60ms after debounced release)
sends one absolute selection only if changed. BACK, conflicting modifiers,
changed catalog/transport or host loss cancels. Mode transitions discard stale
detents. A pending request blocks another preview/commit; volume never consumes
held-mode detents. Unsupported hosts show `Update recorder`.

A stale/incomplete list shows `Updating list` on the bottom row and keeps the
`select take` / `select song` heading unchanged. Knob turns are discarded until
a complete list commits; releasing during loading sends nothing. A replacement
generation starts a fresh preview at the host selection, without carrying over
old detents. Lists arrive automatically in the background; completing recording
also requests a read-only refresh before browsing resumes. Transient notices
replace only the bottom row and do not change selection/menu headings.

Frames: CC90=kind, CC91=length 0..48, repeated CC92=7-bit payload byte, CC93=
`(kind + length + sum(payload)) & 127`. Indices/counts are two little-endian 7-bit
bytes; 16383 means no selected index. Generation/take numbers use five 7-bit chunks,
top ≤15.

| Kind | Payload |
| --- | --- |
| 0 | generation(5), song count(2), take count(2), selected song index(2), selected take index(2) |
| 1 | generation(5), song index(2), ASCII name(0..32) |
| 2 | generation(5), take index(2), actual take number(5) |
| 3 | generation(5); publish only after every expected item commits |
| 4 | generation(5), absolute take index(2); controller request |
| 5 | generation(5), absolute song index(2); controller request |

Bounds: 128 song names and 512 take numbers for the current song. The adapter
selects a bounded prefix and retains the current selection when the source catalog
is larger. That bounded catalog is transmitted as a complete generation. Partial,
corrupt or oversized wire frames are rejected. The host sends
an explicit empty idle catalog too. Text is independently prioritized; catalog
traffic drains up to eight CCs per 50ms tick. CC89=1 requests a read-only resend
after 2.5s without expected transfer activity.

Commit is kind4 + token + Note74 On/Off, or kind5 + token + Note75 On/Off. Host
validates kind/generation/index, captures immutable catalog identity and checks
again under the transport gate. A changed rehearsal/list/selection cannot retarget
the request. Selection refreshes the GUI. The bridge preserves order and bounds
pending feedback to 64 messages, batched to the BLE MTU.

## Builds and diagnostics

[Desktop firmware](../rehearsal_remote/README.md) describes current revision K.
Without `REHA_DESKTOP_PANEL`, the default sketch is BOOT-only; old
`REHA_PANEL`/`REHA_NAV_BUTTONS` profiles must not be used with current wiring.

`sh tools/build_mac_probes.sh` builds bridge, diagnostic hosts and codec tests.
The basic `mac_controller_simulator` creates no audio; it does not establish
acceptance for every optional extension. Use recorder tests for the complete
host model. Never run conflicting feedback publishers.

## Token-bound readiness

This optional extension uses CC115. Controller CC113=query token followed by
CC115=127 requests status without HELLO, epoch changes or action replay.
Host CC114=query token followed by
CC115 reports 0 permits control, 1 busy, 2 requires attention in the recorder.
Status is also published in normal snapshots with the last command token (127
before any command); queued/running actions report busy. A query echoes its token
even if the original command never reached the host.

With the extension negotiated, acknowledgement/state timeout retains a blocked
waiting view and starts status requests every 1.5 seconds. Only a matching-token
status permitting control clears pending commands and stale local command errors.
Busy or attention keeps controls blocked. Heartbeat loss continues to show loss
of recorder communication. Legacy hosts retain the local timeout behaviour.
The controller never repeats a timed-out action or releases recorder locks.
