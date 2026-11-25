# Test Chamber / VR Workshop — completion guide

**Project ID:** `procedural-test-chamber` • game • **Initial gate status: unverified pending current-source reconciliation**

## Evidence baseline
November 16: blank project, primitive placement, light/motion/spatial sound, saved slot replay in host. Physical authoring open.

## Authority and entry points
- `docs/project/TEST_CHAMBER_SHOWCASE.md`
- `docs/project/NATIVE_CREATIVE_SUITE_PLAN.md`
- Production anchor: `app/src/main/jni/native/test_chamber_module.cpp`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/verify-test-chamber-authoring.sh`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/modules/smoke-test-chamber-emulator.sh`

## Ordered gates
### G01 — Finish genuine portable authoring

**Implementation:** Build on actual Test Chamber scene format and editor; add selection, transform, snapping, hierarchy, properties, undo/redo, lighting, motion and spatial sound per workshop contract.

**Does it work?** Create from blank, edit at least two objects, undo/redo, configure light/motion/sound, preview actual runtime and reopen same project with identical editable state.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G02 — Complete safe save and play handoff

**Implementation:** Implement dirty state, transactional save/import/export, corruption recovery, bounded asset references and explicit editor/play transitions.

**Does it work?** Simulate failed write, preserve dirty work; reopen prior valid slot, save another without corrupting it, export/import supported scene and play actual authored behavior.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G03 — Verify headset authoring and sound

**Implementation:** Use installed unified APK, preserve shell recovery and controller actions.

**Does it work?** Author, save, reopen and play using physical Daydream controller; record audible positional sound and two-eye optical readability.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

## Completion
Apply the [shared SCD contract](../SUSTAINED_CONTINUOUS_DEVELOPMENT.md) and every applicable common end-to-end condition. Record revision, actual runner/logs/artifacts and source readback in the existing acceptance ledger. Maintain this guide when owner-approved scope changes.
