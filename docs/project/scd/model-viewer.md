# Immersive OBJ 3D Model Viewer — completion guide

**Project ID:** `model-viewer` • application • **Initial gate status: unverified pending current-source reconciliation**

## Evidence baseline
Source-integrated shell renderer, basic geometry import and orbit; no claim of finished material support.

## Authority and entry points
- `docs/project/INTERNAL_3D_VIEWER.md`
- Production anchor: `app/src/main/java/io/github/mrcalzon02/reverievr/ObjMeshViewerRenderer.java`
- Production anchor: `app/src/main/java/io/github/mrcalzon02/reverievr/MainActivity.java`
- No dedicated existing acceptance command verified for this scope; use the actual host build/runtime and existing project diagnostics. Do not create a mock executable to obtain a pass.

## Ordered gates
### G01 — Complete robust bounded local import

**Implementation:** Use existing OBJ parser and private atomic import, validate indices and size, preserve last valid model on bad import. Implement supported material/texture subset only per plan.

**Does it work?** Import positive/negative indices and valid polygons; reject malformed/nonfinite/oversized models without replacing previous; render actual geometry, not billboard.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G02 — Finish viewer navigation and appearance

**Implementation:** Add documented material/texture support where in scope, orbit/pan/zoom/recenter, controls and missing asset feedback.

**Does it work?** Inspect model from multiple angles with correct geometry, visible materials, both eyes and predictable reset; safe offline exit.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G03 — Verify real headset use

**Implementation:** Test model limits and frame timing, context loss and re-entry on APK/S9.

**Does it work?** Open, inspect, leave/reopen, replace model and recover; capture actual geometry in both eyes.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

## Completion
Apply the [shared SCD contract](../SUSTAINED_CONTINUOUS_DEVELOPMENT.md) and every applicable common end-to-end condition. Record revision, actual runner/logs/artifacts and source readback in the existing acceptance ledger. Maintain this guide when owner-approved scope changes.
