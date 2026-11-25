# Guess the Door — completion guide

**Project ID:** `guess-the-door` • game • **Initial gate status: unverified pending current-source reconciliation**

## Evidence baseline
November 16: full 12-room seeded route, pit, room-six restore, victory/restart host replay; headset timing, art and speech remain.

## Authority and entry points
- `docs/project/GUESS_THE_DOOR.md`
- Production anchor: `app/src/main/jni/native/guess_the_door_module.cpp`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/verify-guess-the-door.sh`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/audit/probes/guess_full_maze.cpp`

## Ordered gates
### G01 — Complete clue and lever presentation

**Implementation:** Use real door features, readable timed clue, selectable physical levers, approved Mechanist-derived or owned art and speech with captions.

**Does it work?** All generated clue combinations uniquely describe an available door; correct and wrong pulls produce intended outcome; clue visible and understandable through each eye, speech/caption equivalent.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G02 — Validate full maze and state recovery

**Implementation:** Preserve seed, progress, timing, pit, win and restart; ensure release edge after new seed.

**Does it work?** Play a complete 12-room maze from cold launch, fail deliberately, replay same seed, reopen mid-maze and victory, and start fresh seed without automatically pulling first lever.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G03 — Finish reference headset UX

**Implementation:** Tune targeting, timing, feedback and comfort against physical device.

**Does it work?** Installed headset play through victory and failure with usable clue timing, audio and recovery.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

## Completion
Apply the [shared SCD contract](../SUSTAINED_CONTINUOUS_DEVELOPMENT.md) and every applicable common end-to-end condition. Record revision, actual runner/logs/artifacts and source readback in the existing acceptance ledger. Maintain this guide when owner-approved scope changes.
