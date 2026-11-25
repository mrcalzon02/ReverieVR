# Reverie Game Workshop — completion guide

**Project ID:** `game-workshop` • application • **Initial gate status: unverified pending current-source reconciliation**

## Evidence baseline
Test Chamber author/save/play foundation and desktop editor exist; complete app scope open.

## Authority and entry points
- `docs/project/NATIVE_CREATIVE_SUITE_PLAN.md`
- `docs/project/TEST_CHAMBER_SHOWCASE.md`
- `docs/project/INTERNAL_GAME_EDITOR_BECKENBURG.md`
- Production anchor: `app/src/main/jni/native/test_chamber_module.cpp`
- Production anchor: `tools/desktop/ReverieSceneProject.java`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/verify-test-chamber-authoring.sh`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/verify-desktop-scene-bridge.sh`

## Ordered gates
### G01 — Unify shared scene authoring contract

**Implementation:** Use existing REVSCENE and actual native scene bridge; do not duplicate save format or Beckenburg engine.

**Does it work?** Author and reopen a scene with geometry, hierarchy, light, sound and motion; import/export supported scene data.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G02 — Complete playable scene creation

**Implementation:** Implement documented triggers, interactions, objectives, spawn and gameplay preview using actual host APIs.

**Does it work?** Author a small playable scene, configure objective and trigger, enter Play, finish objective, return to editor, save and reopen.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G03 — Complete safe project distribution

**Implementation:** Validate assets, versioning, authoring-to-runtime parity and scoped packaging; no arbitrary executable content or extra required APK.

**Does it work?** Export/import allowed project with identical supported behavior and assets; invalid project rejected, edit/play and recovery work on reference device.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

## Completion
Apply the [shared SCD contract](../SUSTAINED_CONTINUOUS_DEVELOPMENT.md) and every applicable common end-to-end condition. Record revision, actual runner/logs/artifacts and source readback in the existing acceptance ledger. Maintain this guide when owner-approved scope changes.
