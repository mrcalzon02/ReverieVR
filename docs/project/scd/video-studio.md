# Reverie Video Studio — completion guide

**Project ID:** `video-studio` • application • **Initial gate status: unverified pending current-source reconciliation**

## Evidence baseline
Creative suite design scope; actual native video editor unverified.

## Authority and entry points
- `docs/project/NATIVE_CREATIVE_SUITE_PLAN.md`
- Production anchor: `app/src/main/java/io/github/mrcalzon02/reverievr/LocalVideoPlayer.java`
- No dedicated existing acceptance command verified for this scope; use the actual host build/runtime and existing project diagnostics. Do not create a mock executable to obtain a pass.

## Ordered gates
### G01 — Establish project and media timeline

**Implementation:** Reuse shared editor kernel; local clip import, metadata, timeline selection and bounded preview.

**Does it work?** Create/edit/save/reopen real timeline from two supported media files with stable project-relative references.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G02 — Complete intended edit/export loop

**Implementation:** Implement trim/split/reorder, timing, audio/level and transitions or overlays per design, bounded offline export.

**Does it work?** Export a short edited sequence; decode output and compare ordering, duration and audio; undo/redo and relaunch preserve edits.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G03 — Qualify performance and controls

**Implementation:** Avoid unbounded decode buffers; handle cancellation, failure and storage limits.

**Does it work?** On supported device, edit and export representative project; no data loss on interruption, usable controller and Stage A alternatives.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

## Completion
Apply the [shared SCD contract](../SUSTAINED_CONTINUOUS_DEVELOPMENT.md) and every applicable common end-to-end condition. Record revision, actual runner/logs/artifacts and source readback in the existing acceptance ledger. Maintain this guide when owner-approved scope changes.
