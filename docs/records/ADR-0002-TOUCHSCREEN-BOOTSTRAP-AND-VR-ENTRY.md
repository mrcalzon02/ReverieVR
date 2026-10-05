# ADR-0002 — Touchscreen Bootstrap and Controller-Gated VR Entry

**Status:** Accepted  
**Date:** 2026-10-04

## Context

The phone must be usable before it is placed into the headset. A controller cannot be assumed to be paired, connected, calibrated, or even functioning correctly at application start.

Requiring VR-only controls before the controller is ready creates a deadlock: the user would need the controller to reach the controls required to make the controller work.

## Decision

ReverieVR uses two explicit application interaction stages.

### Stage A — conventional 2D setup

The APK launches into an ordinary touchscreen Android surface.

Stage A owns:

- first-run setup;
- controller pairing/sync/reconnect;
- visible connection/readiness state;
- controller input test/calibration where needed;
- pre-VR diagnostics;
- recovery/reset controls;
- explicit transition into VR.

The controller's exact pairing transport and implementation are intentionally not dictated by this ADR. They must be verified by the framework/controller-stack investigation.

### Stage B — stereoscopic VR home

After controller readiness and VR initialization, the application transitions into the stereoscopic ReverieVR home.

Normal Stage B operation uses:

- head-look for view/pointing;
- the paired controller for menu and module input;
- supported handset physical buttons where appropriate.

Touchscreen use is not required for ordinary Stage B operation.

## Recovery invariant

Stage A is the recovery authority for input and VR-startup failures.

A failed VR initialization, broken controller mapping, unusable display setting, or lost controller state must not strand the user in an unusable stereoscopic screen or crash loop.

ReverieVR therefore requires a tested route back to Stage A for reconnect, reset, diagnostics, and retry.

## Consequences

- The application architecture must support a clean 2D/VR mode boundary.
- Framework selection must be evaluated for both conventional Android UI and stereoscopic VR use.
- Controller readiness is a precondition for normal VR entry on the reference path.
- Controller settings inside VR may expose status and shortcuts, but destructive recovery/re-pair behavior must remain reachable from Stage A.
- The 2D surface should remain lightweight rather than duplicating the full 3D launcher.
