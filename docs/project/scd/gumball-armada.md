# Gumball Armada — completion guide

**Project ID:** `gumball-armada` • game • **Initial gate status: unverified pending current-source reconciliation**

## Evidence baseline
November 16: all four general flags and rescue/homecoming ending reachable; fresh simulated campaign 67 sorties, 61 wins. Distinct boss and authored region work remains.

## Authority and entry points
- `docs/project/GUMBALL_ARMADA.md`
- `docs/project/GUMBALL_ARMADA_STAGED_PLAN.md`
- `docs/project/GUMBALL_ARMADA_ROUTES_AND_SUPPORT.md`
- `docs/project/GUMBALL_ARMADA_CONTENT_MATRIX.md`
- `docs/project/GUMBALL_ARMADA_CAST_BIBLE.md`
- Production anchor: `app/src/main/jni/native/gumball_armada_module.cpp`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/verify-gumball-armada.sh`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/verify-featured-persistence.sh`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/verify-gumball-native-module.sh`

## Ordered gates
### G01 — Complete parts economy and distinct fitted combat

**Implementation:** Implement intended buy/sell/earn/steal and readiness, meaningful weapons, defense, actives and support; preserve actual route/ship APIs.

**Does it work?** Two materially different legal builds demonstrate different behavior; failed and successful purchase, hired escort, defeat and salvage preserve finite resources and do not duplicate.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G02 — Complete revisitable authored regional graph

**Implementation:** Map real persistent nodes and return edges, vendors, unlocks, optional secrets and saved territory; distinguish actual graph from numeric sequential addresses.

**Does it work?** Travel forward and back through each declared region; vendor/territory/progression persists on reopen; required route never depends on rare drop.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G03 — Finish Sally, Cringe, Pucker and Sauer campaigns

**Implementation:** Implement authored regional mechanics and distinct multi-phase bosses with actual fitted-ship responses; connect story events and consequences.

**Does it work?** Reach and defeat each general via legitimate progress, demonstrate unique attack/objective phases, and persist unlocks; cover failure/retry.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G04 — Polish existing King rescue and credits

**Implementation:** Retain signal relay, tow, convoy and homecoming; finish approved art, audio and announcer, comfort options and postgame.

**Does it work?** Cold-start-to-credits replay with all required story gates, recovery during finale, saved free flight; no optional requirement blocks ending.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G05 — Verify finished hosted game

**Implementation:** Exercise shipbuilding, steering/fire and boss load on installed device.

**Does it work?** Reference S9 proves comfortable input/readability, audio, sustained combat and all common gates.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

## Completion
Apply the [shared SCD contract](../SUSTAINED_CONTINUOUS_DEVELOPMENT.md) and every applicable common end-to-end condition. Record revision, actual runner/logs/artifacts and source readback in the existing acceptance ledger. Maintain this guide when owner-approved scope changes.
