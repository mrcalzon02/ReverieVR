# Stage B Home, global menus, settings and comfort — completion guide

**Project ID:** `vr-home-shell` • application • **Initial gate status: unverified pending current-source reconciliation**

## Evidence baseline
Home, gaze recovery and shell exist; full physical optics and controls unverified.

## Authority and entry points
- `docs/project/HOME_ENVIRONMENT_ATMOSPHERE.md`
- `docs/project/HOME_AVATAR_WARDROBE.md`
- `docs/project/ACCESSIBILITY_AND_COMFORT.md`
- `docs/project/INCREMENTAL_DEVELOPMENT_PLAN.md`
- Production anchor: `app/src/main/java/io/github/mrcalzon02/reverievr/VrActivity.java`
- Production anchor: `app/src/main/java/io/github/mrcalzon02/reverievr/HomeEnvironmentRenderer.java`
- Production anchor: `app/src/main/java/io/github/mrcalzon02/reverievr/ControllerSnapshot.java`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/verify-home-atmosphere-assembly.py`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/verify-home-exit-placement.py`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/perf/s9-session-sampler.py`

## Ordered gates
### G01 — Finish Home rooms and avatar wardrobe

**Implementation:** Preserve birch living room/forest/beach atmospheric assets, avatar identity, outfits, pose and offline ambience; actual selectable objects.

**Does it work?** All authored rooms launch; avatar/outfit selections persist and render correctly; environment audio stops/changes on transition; actual stereo captures.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G02 — Finish shell navigation and options

**Implementation:** Group settings into useful tabs (display, input, performance, audio, media, accessibility, developer). Wire every option to real behavior; persistent global HUD, Quick Menu, recenter and recovery across native apps.

**Does it work?** Every setting changes observed behavior or explains availability; gaze-only recovery, hold/release, rebind, controller disconnect and Home/resume work from every active experience.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G03 — Qualify optical fit and sustained performance

**Implementation:** Verify stereo eye transforms, IPD vs physical lens geometry, safe scale, text legibility, frame pacing, battery and thermal behavior.

**Does it work?** Actual S9 short and sustained sessions prove comfortable optical fit and usable shell; record p50/p95/p99, stalls and thermal/memory with same workloads.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

## Completion
Apply the [shared SCD contract](../SUSTAINED_CONTINUOUS_DEVELOPMENT.md) and every applicable common end-to-end condition. Record revision, actual runner/logs/artifacts and source readback in the existing acceptance ledger. Maintain this guide when owner-approved scope changes.
