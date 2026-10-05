# ReverieVR Execution State

**Role:** durable operational state  
**Updated:** 2026-10-04 (America/Sitka)

## Authority

- Repository: `mrcalzon02/ReverieVR`
- Branch: `main`
- Governance session starting remote HEAD: `0e071ad98e3a180ea336cc67bee0bb0afbbdd118`
- Governance baseline commit: `542a6d14121ba986f73b5b940c5debbeb0a0f057`
- Governance acceptance/bookkeeping commit: `125b5061e22d57151f7803e37a3f7efd5734508d`
- Remote readback confirmed `main` contained the governance baseline and project state.

## Last completed target

**RV-0001 — Governance/bootstrap adoption**

Acceptance: **static accepted**.

## Newly accepted architecture requirement

The product boot path is now explicitly two-stage:

1. **Stage A — 2D touchscreen setup:** launch outside VR, pair/sync and verify the controller, expose pre-VR diagnostics/recovery, then deliberately enter VR.
2. **Stage B — 3D VR home:** transition into stereoscopic seated VR and operate the normal shell using head-look plus the controller.

A reliable recovery route from Stage B back to Stage A is required.

The underlying controller pairing transport/stack is not assumed yet; RV-0002 must verify and record the implementation path.

## Active target

**RV-0002 — Framework/runtime selection research**

Select the smallest sustainable Android VR/rendering and controller-integration stack that can support the Galaxy S9 + Daydream View reference path offline.

Required decision evidence:

- candidate frameworks/runtimes;
- current source/maintenance state;
- license and redistribution implications;
- Android API/ABI compatibility;
- conventional 2D Android UI support before VR entry;
- clean 2D-to-stereoscopic-VR mode transition;
- rendering/head-tracking/viewer support;
- controller discovery/pairing/sync/readiness path;
- ability to operate without discontinued Daydream services;
- controller/input integration implications;
- APK/runtime size and performance implications;
- build reproducibility;
- selected baseline and rejected alternatives.

The decision must be persisted as an architecture/decision record before implementation commits depend on it.

## Next after RV-0002

**RV-0003 — Android APK skeleton**, then **RV-0090 through RV-0092 — pre-VR boot/pairing/transition**, followed by the Phase 2 ReverieVR 3D home/menu work defined in `APK_BUILD_ORDER.md`.

## Blocker

None.
