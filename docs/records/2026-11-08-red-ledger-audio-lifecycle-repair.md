# Red Ledger recorded-audio lifecycle repair

The confirmed gap is closed in an isolated copy of merged source at
`3af395812fc062505e6afa05ff3a06063198189d`.

## Change

`RedLedgerSound` owns a fixed four-slot ring of stream IDs, explicitly retires
the oldest owned voice before reuse, records only successful playback IDs, and
stops every owned voice at a lifecycle boundary. A platform stop exception keeps
the ID available for a later bounded retry and prevents overwriting that slot.
Close is idempotent and releases the pool after attempting stream shutdown.
The existing clips, gains, four-voice budget and routing are retained.

`UiFeedback` calls this shutdown at its existing session-generation boundary,
interaction/focus suspension, and Activity pause. Its existing close already
closes the bank. Breakwater production classes and all gameplay/save code are
unchanged. The existing routing-only Red Ledger test double was split into a
separate file so the new test can compile the real bank.

## Executed proof

- **Before:** `before.log`, exit **1**, compiles the unmodified merged production
  classes with the final harness. The actual drawer WAV is 905.578 ms long
  (906 ms when rounded up for the clock). Its stream is live at 100 ms and
  survives `UiFeedback.pauseForLifecycle`, triggering the expected assertion.
- **After:** `after.log`, exit **0**: **12 scenarios, 75 checks**. Includes
  Activity pause, fresh resume, Quick Menu/pointer loss, Home, same-title
  stop/start collapsed into one UI turn, focus loss, real Breakwater routing,
  bounded overlapping voices, rejected/throwing playback, asynchronous decode,
  retained IDs after failed stop, close, and natural completion.
- **Existing integration:** `breakwater-regression.log`, exit **0**:
  UiFeedback routing **18 checks** and real Breakwater bank **16 scenarios,
  226 checks**. Production Breakwater behavior is not replaced with a mock in
  the new integration test.
- **Apply check:** patch applies cleanly to the captured baseline.
- **Prior lanes:** all 41 Red/Gumball and 42 recon files still match their
  frozen manifests, and both previously delivered patch hashes are unchanged.

The actual source WAV metadata and hashes are in `recording-durations.json`.
Each execution JSON records its exact command, exit, time and production hashes.
The Java regression uses a named Android SoundPool/Handler platform double;
Android decoding, audible output, device focus delivery and physical route
latency remain device acceptance work. This adds a local regression command,
not a publication gate.

## Integration

Apply `red-ledger-audio-lifecycle.patch` after the merged remote audio work.
Run `bash scripts/audio/verify-red-ledger-lifecycle.sh` and
`bash scripts/audio/verify-breakwater-backend.sh` from the integrated checkout.
`manifest.json` lists the exact nine changed files and their before/after hashes.
No authoritative checkout was modified by this lane.
