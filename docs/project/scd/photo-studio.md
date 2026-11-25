# Reverie Photo Studio — completion guide

**Project ID:** `photo-studio` • application • **Initial gate status: unverified pending current-source reconciliation**

## Evidence baseline
Creative suite design scope; do not claim implementation until runtime is found.

## Authority and entry points
- `docs/project/NATIVE_CREATIVE_SUITE_PLAN.md`
- Production anchor: `app/src/main/java/io/github/mrcalzon02/reverievr/NativeModuleRuntime.java`
- No dedicated existing acceptance command verified for this scope; use the actual host build/runtime and existing project diagnostics. Do not create a mock executable to obtain a pass.

## Ordered gates
### G01 — Establish shared creative workspace

**Implementation:** Reuse shell, asset library, transform/UI, undo/save kernel and host actions. Inspect design before adding app entry.

**Does it work?** A real hosted workspace opens blank project, imports image, manipulates view and saves/reopens editable project.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G02 — Complete photo editing operations

**Implementation:** Implement documented nondestructive crop, transform, color/adjustments, layers/masks and export with supported formats; preserve source.

**Does it work?** Import actual image, edit distinct layers/adjustments, undo/redo, export valid output, reopen editable state; decoded output matches operations.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G03 — Finish usable offline authoring

**Implementation:** Exercise recovery, permissions, invalid input, large image memory and headset controls.

**Does it work?** A user can finish image edit/export on installed app; sustained memory and actual captures prove usability.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

## Completion
Apply the [shared SCD contract](../SUSTAINED_CONTINUOUS_DEVELOPMENT.md) and every applicable common end-to-end condition. Record revision, actual runner/logs/artifacts and source readback in the existing acceptance ledger. Maintain this guide when owner-approved scope changes.
