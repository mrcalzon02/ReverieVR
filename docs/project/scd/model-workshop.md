# Reverie Model Workshop — completion guide

**Project ID:** `model-workshop` • application • **Initial gate status: unverified pending current-source reconciliation**

## Evidence baseline
Creative suite planned; immersive OBJ viewer is not a model editor.

## Authority and entry points
- `docs/project/NATIVE_CREATIVE_SUITE_PLAN.md`
- `docs/project/INTERNAL_3D_VIEWER.md`
- Production anchor: `app/src/main/java/io/github/mrcalzon02/reverievr/ObjMeshViewerRenderer.java`
- No dedicated existing acceptance command verified for this scope; use the actual host build/runtime and existing project diagnostics. Do not create a mock executable to obtain a pass.

## Ordered gates
### G01 — Establish real mesh workspace

**Implementation:** Reuse editor selection/gizmo/history and viewer geometry; import/create actual editable primitive/mesh.

**Does it work?** Create and select mesh, move/rotate/scale and save/reopen; exported geometry matches edit.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G02 — Complete declared modeling workflow

**Implementation:** Implement intended mesh/primitive operations, material assignment, hierarchy, snapping, undo/redo and supported import/export.

**Does it work?** Author a usable multi-object model, edit its geometry and materials, export valid supported file, reimport without silent loss.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G03 — Validate headset and desktop authoring handoff

**Implementation:** Exercise bounded geometry and actual preview/runtime.

**Does it work?** Real user can author and export from supported environment; validate frame timing, interrupted saves and project compatibility.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

## Completion
Apply the [shared SCD contract](../SUSTAINED_CONTINUOUS_DEVELOPMENT.md) and every applicable common end-to-end condition. Record revision, actual runner/logs/artifacts and source readback in the existing acceptance ledger. Maintain this guide when owner-approved scope changes.
