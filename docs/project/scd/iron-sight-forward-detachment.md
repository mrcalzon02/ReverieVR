# Iron Sight: Forward Detachment — completion guide

**Project ID:** `iron-sight-forward-detachment` • game • **Initial gate status: unverified pending current-source reconciliation**

## Evidence baseline
November 16: Ridge Bypass walking and backtracking, notes and persistent debrief; Main Road board-only, vehicles/outdoor sectors open.

## Authority and entry points
- `docs/project/WAR_WITHOUT_END.md`
- `docs/project/INCREMENTAL_DEVELOPMENT_PLAN.md`
- Production anchor: `app/src/main/jni/native/recon_projects_module.cpp`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/verify-recon-persistence.sh`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/audit/probes/recon_module_probe.cpp`

## Ordered gates
### G01 — Build compact continuous patrol sector

**Implementation:** Implement physical holding/JON, approach, checkpoint/bridge, industrial pocket and bypass, with scout/APC and small dismount team. Reuse current Ridge Bypass movement.

**Does it work?** Start at holding area, prepare, drive/walk both routes, observe, return to base; fuel/condition and vehicle/dismount state affect traversal and persist.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G02 — Implement evidence-driven encounters

**Implementation:** Add negative recon, civilian ambiguity, hazard indicators, scouts, damaged infrastructure and liaison; distinct avoid/report/engage choices.

**Does it work?** Use only observable information to identify contacts; exercise peaceful, avoidance and engagement paths with distinct persistent consequences and no hidden-truth leakage.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G03 — Finish consecutive patrol cycle

**Implementation:** Implement legitimate finite resupply/repair, assignments, report quality and operation outcomes.

**Does it work?** Complete three patrols from one save; previous damage/resources carry forward until replenished, debrief applies exactly once; notes/position recover on interruption.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G04 — Verify hosted bounded operation

**Implementation:** Test physical controls and legible cues, no debug-only progress; preserve intended patrol scope rather than tactical RTS.

**Does it work?** Complete operation in installed APK and reference headset, including vehicle controls, pause/recover and sustained performance.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

## Completion
Apply the [shared SCD contract](../SUSTAINED_CONTINUOUS_DEVELOPMENT.md) and every applicable common end-to-end condition. Record revision, actual runner/logs/artifacts and source readback in the existing acceptance ledger. Maintain this guide when owner-approved scope changes.
