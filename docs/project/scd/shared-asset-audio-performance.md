# Shared native SDK, assets, audio and performance — completion guide

**Project ID:** `shared-asset-audio-performance` • application • **Initial gate status: unverified pending current-source reconciliation**

## Evidence baseline
Existing native ABI, shared resources and S9 parser; physical thermal evidence remains open.

## Authority and entry points
- `docs/project/INSTRUCTIONS.md`
- `docs/project/ACCESSIBILITY_AND_COMFORT.md`
- `docs/project/INCREMENTAL_DEVELOPMENT_PLAN.md`
- Production anchor: `app/src/main/java/io/github/mrcalzon02/reverievr/NativeModuleRuntime.java`
- Production anchor: `app/src/main/java/io/github/mrcalzon02/reverievr/HomeAmbientAudio.java`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/verify-shared-native-assets.py`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/verify-native-doc-sync.py`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/verify-native-vector-font.py`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/perf/s9-session-sampler.py`

## Ordered gates
### G01 — Preserve real shared SDK contracts

**Implementation:** Read native API/contract index; audit module isolation, text, GL thread ownership, audio and runtime services; update mapped docs with code.

**Does it work?** All registered modules use actual shared ABI and approved assets; missing asset/rejection is visible, no arbitrary cross-namespace access.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G02 — Finish reusable assets and feedback

**Implementation:** Integrate provenance-approved textures/models/sounds, normalized controls, success/rejection cues and actual audio focus.

**Does it work?** All enabled controls have matching visual/audio feedback; required assets load and render in actual game, not generated proof; no audio leak.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G03 — Measure actual sustained performance

**Implementation:** Benchmark before/after using same source/device/scenario, frame pacing, memory, CPU/GPU, thermal and eye render scale; optimize without changing sim cadence or readability.

**Does it work?** Physical S9 sustained workload per experience reports p50/p95/p99 and stalls, thermal/memory; compare baseline and retain playable quality. No cosmetic regression is a publication blocker.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

## Completion
Apply the [shared SCD contract](../SUSTAINED_CONTINUOUS_DEVELOPMENT.md) and every applicable common end-to-end condition. Record revision, actual runner/logs/artifacts and source readback in the existing acceptance ledger. Maintain this guide when owner-approved scope changes.
