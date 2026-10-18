# Controller serial diagnostics

Revision K wiring is unchanged. The diagnostic firmware retains its last 32
structured events in RAM; reset/power-off clears them. UART is 115200 baud.
No logging is performed inside encoder interrupts. Live event writes are skipped
when the UART buffer is full; the event remains in the ring and the drop counter
increments. Heartbeat/state duplicates are not printed continuously.

Read-only commands:

- `i`: uptime, revision, BLE/freshness, state/capabilities, token, pending/awaiting
  status, result, per-connection HELLO flag, both metadata-row commit flags,
  LCD presence and the two rendered rows.
- `d`: dump the last 32 events, oldest first, and live-write drop count.
- `l`: toggle live trace output; RAM events continue. Reset enables live output.
- `?`: command help. Other existing serial commands can operate real audio;
  use only the read-only commands when collecting diagnostics.

Events include connection transitions, command/token and rejected/ignored
requests, acknowledgements/timeouts, host state/capabilities, volume turns,
encoder detents/menu mode, input changes and changed screen/metadata rows.
`released_mask` bits 0..5 are Record, Play, Take, Song, Back, encoder push;
a set bit is released/high and a cleared bit is pressed/low. Screen events are
limited to 10/second. Row text is the actual fixed-width rendered screen.

Capture using the actual USB serial port (the name can change after reconnect):

```sh
mkdir -p output
python3 tools/serial_probe.py <serial-port> --seconds 600 > output/controller-serial.log
```

Ctrl-C stops capture and closes the serial port. Do not leave a capture open
while uploading firmware. `--send i --seconds 3` reads status; `--send d --seconds 3`
collects the ring. Neither requests audio recording or deletion. Capture while
reproducing a fault, then note the action order and approximate time. The trace
includes displayed song/take titles, so review it before sharing publicly.

## Reconnect snapshot request

Each BLE connection sends HELLO even if a still-running host supplied state
first. It retries after one second without fresh feedback, or after 2.5 seconds
when text support is advertised but either initial row has not committed.
The text interval allows both maximum-size rows to arrive at the bounded host
rate. Retries wait for any pending command/state acknowledgement to settle;
commands themselves are never replayed. Explicitly empty committed rows count
as received. HELLO events and i status expose receipt as text=1/1.

## LCD timing test

Open Menu → LCD timing test. Encoder press cycles ZERO → RUN → STOP → ZERO.
The onboard LED is off at ZERO, green at RUN and red at STOP. The bottom row
shows milliseconds and freezes at the accepted stop input. Rotation is ignored;
BACK exits. Record/Play buttons retain their transport actions.

Serial `LCD_TEST` records include `stage`, `event_ms`, `final_write_ms`,
`write_delay_ms` and `timer_ms`. Completion is logged after the last changed
character has been written successfully. Timestamps have 1ms resolution and
exclude physical pixel settling. The LED marks accepted debounced input, not
initial mechanical contact.

To measure visible response, record the LCD and onboard LED together at high
frame rate. Count frames from red LED onset to settled STOP digits, using the
capture frame rate rather than a slow-motion playback rate. Compare this with
`write_delay_ms`. Running digits alone provide no independent timing reference.

## Serial action commands

These commands operate the connected recorder and can create real recordings.

| Command | Action |
| --- | --- |
| `r` | Record/Stop |
| `p` | Play/Stop |
| `b` / `f` | Seek backward/forward |
| `t` / `T` | Previous/next take |
| `s` / `S` | Previous/next song |
| `h` | Restart playback position |
| `+` / `-` | Relative playback volume |
| `m` | Toggle playback mute |
| `0`–`9` | Diagnostic absolute playback volume |
