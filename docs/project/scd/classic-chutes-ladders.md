# Chutes & Ladders — completion guide

**Project ID:** `classic-chutes-ladders` • game • **Initial gate status: unverified pending current-source reconciliation**

## Evidence baseline
Two matches, both winners, exact overshoot and roll history persisted.

## Authority and entry points
- `docs/project/CLASSIC_NATIVE_GAMES.md`
- Production anchor: `app/src/main/jni/native/classic_games_module.cpp`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/verify-classic-saves.sh`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/verify-classic-input-prefix.sh`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/audit/probes/classic_match_endings.cpp`

## Ordered gates
### G01 — Complete title-specific rules and actual playable controls

**Implementation:** Verify legal rolls, ladder/chute endpoints, shared tiles, exact-finish rule and independent retained histories; accessible board labels.

**Does it work?** Complete both wins, overshoot, shared tile and chute/ladder cases; reopen midmatch with next RNG/roll history intact.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G02 — Verify title-specific lifecycle and accessibility

**Implementation:** Exercise actual module input and save callbacks: New, Continue, Restart, pause, Home and process recreation; confirm labels match hit targets and both eyes. Retain board/run state, score and RNG where applicable.

**Does it work?** Start from fresh install, progress, interrupt/resume exactly, finish, reopen terminal state and explicitly restart. Denied actions have truthful feedback; no double activation.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G03 — Finish reference-device acceptance

**Implementation:** Exercise full match/run on installed S9 with controller, recovery and scaled legible text; preserve approved art and audio.

**Does it work?** A human can complete and restart this actual title through intended controls. Record headset, input, audio and performance witness.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

## Completion
Apply the [shared SCD contract](../SUSTAINED_CONTINUOUS_DEVELOPMENT.md) and every applicable common end-to-end condition. Record revision, actual runner/logs/artifacts and source readback in the existing acceptance ledger. Maintain this guide when owner-approved scope changes.
