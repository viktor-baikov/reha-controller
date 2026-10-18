# LCD error codes — revision K

Generated from the shared firmware error catalogue in PanelDisplay.hpp.

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
| E010 | Command confirmation timed out | Check both MIDI directions and recorder health. Legacy hosts time out after 1.5 seconds for acknowledgement or 3.5 seconds for state. With readiness support, the controller stays blocked and requests status every 1.5 seconds. Record/Play toggles are never replayed. |
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
