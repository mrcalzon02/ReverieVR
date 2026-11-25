# Desktop games workbench / internal editor — completion guide

**Project ID:** `desktop-workbench` • application • **Initial gate status: unverified pending current-source reconciliation**

## Evidence baseline
Java 17 workbench with portable pinned Beckenburg editor and REVSCENE. Not complete native desktop hub.

## Authority and entry points
- `docs/project/INTERNAL_GAME_EDITOR_BECKENBURG.md`
- `docs/project/DESKTOP_PLATFORM_EXPANSION.md`
- `docs/project/SITKA_EDITOR_PIPELINE.md`
- Production anchor: `tools/desktop/ReverieSceneProject.java`
- Production anchor: `tools/desktop/SceneDocument.java`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/verify-desktop-scene-bridge.sh`
- Existing diagnostic/probe (read its documented arguments before running): `tools/desktop/player/test.sh`

## Ordered gates
### G01 — Complete editor capabilities against actual project state

**Implementation:** Retain pinned upstream editor, real document, palette, hierarchy and transactional history; add only scoped missing operations.

**Does it work?** Select each current project, create/import, manipulate, undo/redo, save/reopen separate scene/region with actual properties and correct IDs.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G02 — Complete authoring/runtime bridge

**Implementation:** Integrate scene, lighting, water/weather, terrain, assets and collision where supported by native host; report unsupported fields explicitly.

**Does it work?** Change actual scene, export through existing bridge, launch runtime and demonstrate corresponding geometry/behavior without parallel demo data.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G03 — Verify desktop build/packaging

**Implementation:** Exercise editor on supported Windows/Linux/macOS, source pin, licenses and local storage.

**Does it work?** Actual install/launch, edit/save/reopen and runtime handoff on each claimed OS; no Android or cloud prerequisite.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

## Completion
Apply the [shared SCD contract](../SUSTAINED_CONTINUOUS_DEVELOPMENT.md) and every applicable common end-to-end condition. Record revision, actual runner/logs/artifacts and source readback in the existing acceptance ledger. Maintain this guide when owner-approved scope changes.
