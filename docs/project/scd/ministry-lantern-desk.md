# Ministry of Intelligence: Lantern Desk — completion guide

**Project ID:** `ministry-lantern-desk` • game • **Initial gate status: unverified pending current-source reconciliation**

## Evidence baseline
November 16: saved loupe/evidence. November 23 source already implements fixed 512px permanent-terrain photography, weather layers and persistent pins/comparison; the 16×16 film is a legacy fallback. November 25 source increases genuine terrain capture to 1024px, normalizes world-to-pixel and GPU coordinates and distinguishes vehicle/tent footprints. Runtime and blind-recognition acceptance are pending.

## Authority and entry points
- `docs/project/WAR_WITHOUT_END.md`
- `docs/project/INCREMENTAL_DEVELOPMENT_PLAN.md`
- Production anchor: `app/src/main/jni/native/recon_projects_module.cpp`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/verify-recon-persistence.sh`

## Ordered gates
### G01 — Replace coarse reconnaissance imagery with identifiable aerial photographs

**Implementation:** Trace actual image generation, display/upload and save paths. Build high-resolution top-down/oblique reconnaissance views consistent with the approved camera perspective, preserving identifiable terrain, buildings, roads, vehicles, vegetation and civilian lookalikes. Use actual scene-derived render or licensed authored layers, not a few colored blocks. Keep hidden classification separate.

**Does it work?** At best camera setting, blinded reviewers can identify intended visible features in multiple varied cases without labels revealing truth; imagery is recognizable at desk distance and through zoom. Verify native source dimensions, decoded pixels, both-eye frame and source/license manifest; do not call an enlarged low-resolution grid high resolution.

**Required evidence:** Original source frames, full desk screenshots, zoomed crops and blind classification results; exact asset provenance and decoded size.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G02 — Compose independent weather, grime and camera-quality layers

**Implementation:** Add seeded distinct cloud forms and wind-driven drift; rain, heavy storm, snow, hail, haze/smoke and variable occlusion; independent lens grime and quality. Apply resolution degradation as a postprocess on the same source scene, with camera upgrades improving actual recoverable detail. Keep native texture and memory budgets.

**Does it work?** Same scene/seed produces distinguishable clear, overcast, rain, heavy storm, snow and hail cases and visibly different wind/grime settings; low/medium/high camera settings improve measurable feature resolution without changing scene truth. A clear reference remains available to verification only.

**Required evidence:** Same-scene contact sheets at each quality/weather condition; report feature recognition, pixel dimensions, allocations and frame timing.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G03 — Complete the physical analyst desk

**Implementation:** Implement persistent annotation, pan/zoom, side-by-side comparison, dossier reference and report submission. Route through real normalized controller actions.

**Does it work?** Select a case, inspect two photos, mark and remove locations, cite observed evidence, compose a report, save/reopen with marks intact. No impossible control or hidden truth in player UI.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G04 — Make operations evolve from analyst decisions

**Implementation:** Implement world/evidence state, multiple ambiguous positive/negative civilian and military cases, weather/aircraft changes, persistent credibility and downstream report consequences.

**Does it work?** Run at least three consecutive cases with different correct, false-positive and uncertain decisions; show distinct saved downstream state and no reward duplication or truth leaks. Finish the bounded intended operation.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G05 — Complete hosted and reference-device experience

**Implementation:** Use shared acceptance checklist and approved art/audio; demonstrate useful reading/recognition in headset.

**Does it work?** Complete a fresh case sequence and recover after interruption on installed APK; prove controls, legibility, actual speaker feedback, sustained frame pacing and offline use.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

## Completion
Apply the [shared SCD contract](../SUSTAINED_CONTINUOUS_DEVELOPMENT.md) and every applicable common end-to-end condition. Record revision, actual runner/logs/artifacts and source readback in the existing acceptance ledger. Maintain this guide when owner-approved scope changes.
