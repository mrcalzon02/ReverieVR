# Breakwater Battery — completion guide

**Project ID:** `breakwater-battery` • game • **Initial gate status: unverified pending current-source reconciliation**

## Evidence baseline
November 16 host evidence: three stations and both eyes; Nov 23 main rejects invalid cannon weapon IDs. Complete four-theater campaign still open.

## Authority and entry points
- `docs/project/BREAKWATER_BATTERY_VR.md`
- `docs/project/BREAKWATER_SOUND_SOURCES.md`
- Production anchor: `app/src/main/jni/native/breakwater_module.cpp`
- Production anchor: `app/src/main/jni/native/breakwater_campaign.cpp`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/verify-breakwater-world-gl.sh`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/verify-breakwater-audio.sh`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/audit/probes/breakwater_full_campaign.cpp`

## Ordered gates
### G01 — Validate full combat loop and station transitions

**Implementation:** Follow existing campaign, mount transforms and weapon registry. Exercise valid/invalid IDs, aim, load, recoil, traverse limits, ammunition, firing/reload and hit resolution; preserve approved mount geometry.

**Does it work?** For every weapon and station, visible and physical aiming agree; invalid IDs cause no launch, paid valid weapons actually fire, ammo and reload behave correctly, no double action on held input.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G02 — Finish four coastal theaters and enemy behavior

**Implementation:** Complete distinct terrain, sea/shore landing routes, infantry/armor/air threats, unloading, targeting and approved campaign schedules, with functional defenses and support.

**Does it work?** Play through each theater and observe landings, attacks, defensive damage, support delivery and genuine win/loss transitions; no inaccessible targets or broken spawns.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G03 — Complete shops, progression and campaign persistence

**Implementation:** Exercise all difficulties, three slots, paid upgrades and support, explicit reset and wave result checkpoints; use existing save protections.

**Does it work?** Interrupt at purchase, mid-wave per documented contract, reward, defeat and victory; no lost acknowledged payment or duplicate reward. Four-theater campaign is reachable from fresh play without debug unlock.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G04 — Finish sound, readability and actual device combat

**Implementation:** Use normalized asset library, contextual success/rejection feedback and bounded voices. Capture approved world and controls in each eye.

**Does it work?** Complete campaign on installed APK; test three stations, controller feel, audible mix, sustained combat and thermal behavior on reference S9.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

## Completion
Apply the [shared SCD contract](../SUSTAINED_CONTINUOUS_DEVELOPMENT.md) and every applicable common end-to-end condition. Record revision, actual runner/logs/artifacts and source readback in the existing acceptance ledger. Maintain this guide when owner-approved scope changes.
