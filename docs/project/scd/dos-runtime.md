# DOS and bundled DOS games — completion guide

**Project ID:** `dos-runtime` • application • **Initial gate status: unverified pending current-source reconciliation**

## Evidence baseline
Bundled Doom and DOS runtime source; November 6 simulated head yaw/guest input verified, device still open.

## Authority and entry points
- `docs/project/CONTROLLER_LAYOUTS_AND_BINDINGS.md`
- `docs/project/DOS_FREEWARE_SHAREWARE_CANDIDATES.md`
- `docs/project/INCREMENTAL_DEVELOPMENT_PLAN.md`
- Production anchor: `app/src/main/java/io/github/mrcalzon02/reverievr/DosNativeRuntime.java`
- Production anchor: `app/src/main/java/io/github/mrcalzon02/reverievr/DosSession.java`
- Production anchor: `app/src/main/java/io/github/mrcalzon02/reverievr/DosGameModule.java`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/verify-build-bootstrap.sh`

## Ordered gates
### G01 — Complete local DOS game launch and import

**Implementation:** Use existing bundled licensed shareware and local import/catalog. Validate launch, guest storage, configuration, video/audio and title-specific binding profiles.

**Does it work?** On offline installed APK, launch every admitted bundled DOS title; import a valid local game and reject malformed media safely. No desktop dependency.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G02 — Finish comfortable guest controls

**Implementation:** Preserve head-yaw relative guest mouse for Doom, touchpad, weapon/use mapping, mouse lock/unlock, keyboard and accurate visible binding legend.

**Does it work?** Play a real level, exercise aim, movement, use, fire and weapon cycling; menu pauses and releases guest input, then resumes without latched actions.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G03 — Finish DOS save and device acceptance

**Implementation:** Exercise in-game saves, guest config, resume/restart, view placement and audio.

**Does it work?** Cold launch and load saved game, interrupt/reopen, finish representative level on reference headset with sustained frame pacing.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

## Completion
Apply the [shared SCD contract](../SUSTAINED_CONTINUOUS_DEVELOPMENT.md) and every applicable common end-to-end condition. Record revision, actual runner/logs/artifacts and source readback in the existing acceptance ledger. Maintain this guide when owner-approved scope changes.
