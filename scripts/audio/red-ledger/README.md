# Red Ledger recorded-audio lifecycle regression

Run `./scripts/audio/verify-red-ledger-lifecycle.sh` with Java 17 or newer. An
optional checkout argument selects production sources and their actual WAV files.

The runner compiles the production `RedLedgerSound`, `UiFeedback`,
`BreakwaterAudio`, `BreakwaterAudioPolicy`, and `MusicVolumePolicy`. Its named
Android platform doubles supply resource IDs, asynchronous load completion,
bounded SoundPool streams, monotonic time, and UI callbacks. It reads the actual
six Red Ledger WAV headers to model natural sample duration; it does not decode
or play audio on a device.

Checks cover an already-playing recording across Activity pause, Quick Menu or
pointer loss, Home, collapsed stop/start, focus suspension, close, pending decode,
playback rejection, bounded overlapping streams, natural completion, and real
Breakwater bank routing. Resume must wait for a fresh validated frame and cue.

This is a local regression command, not a new APK publication gate. Physical
audio output, platform decoding, real focus delivery and route latency remain
device acceptance work.
