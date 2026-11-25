# Stage A setup / pairing / recovery — completion guide

**Project ID:** `stage-a-setup` • application • **Initial gate status: unverified pending current-source reconciliation**

## Evidence baseline
Existing Android shell; prior emulator recovery evidence, physical control and optical acceptance open.

## Authority and entry points
- `docs/project/INSTRUCTIONS.md`
- `docs/project/APK_BUILD_ORDER.md`
- `docs/project/CONTROLLER_LAYOUTS_AND_BINDINGS.md`
- Production anchor: `app/src/main/java/io/github/mrcalzon02/reverievr/MainActivity.java`
- Production anchor: `app/src/main/java/io/github/mrcalzon02/reverievr/ControllerManager.java`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/ci/run-recovery-simulation-emulator.sh`

## Ordered gates
### G01 — Complete offline clean installation and controller setup

**Implementation:** Exercise fresh install, known bonds, new pairing, reconnect, controller test and runtime readiness; preserve no-location-permission policy and optional controller.

**Does it work?** In airplane/offline mode enter VR with and without controller; reconnect known controller and report genuine permissions/failures without blocking recovery.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G02 — Complete versioned onboarding and recovery

**Implementation:** Implement/reconcile wizard: seated recenter, bindings, viewer profile, IPD alignment, readable UI, HUD, comfort, audio and performance; reset/skipping.

**Does it work?** Complete, skip, re-enter and reset wizard; deliberately break input/binding and recover using 5-second gaze/Stage A without touchscreen in headset.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G03 — Validate physical reference devices

**Implementation:** Use S9 and supported alternative controller/phone configurations as available.

**Does it work?** Record actual pairing, recovery, entry and app restart; no discontinued servers or PC dependencies.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

## Completion
Apply the [shared SCD contract](../SUSTAINED_CONTINUOUS_DEVELOPMENT.md) and every applicable common end-to-end condition. Record revision, actual runner/logs/artifacts and source readback in the existing acceptance ledger. Maintain this guide when owner-approved scope changes.
