# ReverieVR Backlog

**Role:** intent and prioritization. This file does not prove execution.

## P0 — Foundation

- RV-0001 — Adopt AI Project Manager-compatible project governance and repository execution rules.
- RV-0002 — Research and select the minimum Android VR/rendering framework and controller integration stack for Galaxy S9 + Daydream View, including license/provenance, offline behavior, 2D-to-VR transition support, and pairing implications.
- RV-0003 — Create Android/Gradle APK skeleton with reproducible local build instructions.
- RV-0004 — Establish device capability/diagnostic reporting.
- RV-0005 — Establish performance instrumentation and sustained-test procedure.

## P1 — Pre-VR boot and controller readiness

- RV-0090 — Implement conventional 2D touchscreen boot/setup surface.
- RV-0091 — Implement controller pairing/sync, readiness state, and controller test workflow.
- RV-0092 — Implement explicit 2D-to-VR entry plus safe fallback/recovery to 2D setup.

## P2 — VR shell/menu

- RV-0100 — Implement stereoscopic ReverieVR home shell.
- RV-0101 — Implement seated head-look orientation and reticle.
- RV-0102 — Implement controller/input action abstraction.
- RV-0103 — Implement VR-operable menu.
- RV-0104 — Implement persistent settings model and safe-reset path.
- RV-0105 — Implement in-headset diagnostics/performance display.
- RV-0106 — Implement shell-global handset/controller power HUD with percentage/progress presentation and honest unavailable states.
- RV-0107 — Implement optional gaze-adaptive status reveal/retract plus persistent/manual fallback and local QoL toggles.
- RV-0108 — Implement versioned first-VR-run onboarding/calibration wizard with resumable progress, safe defaults, skip/defer behavior, completion summary, and **Run VR Setup Again** entry point.

## P3 — Settings and controls

- RV-0200 — VR/display settings.
- RV-0201 — performance/quality settings and dynamic fallback.
- RV-0202 — input mapping settings.
- RV-0203 — audio settings.
- RV-0204 — player defaults.
- RV-0205 — developer/diagnostic settings.
- RV-0206 — Implement user optical/IPD calibration separately from the physical viewer profile: persistent user IPD, live per-eye alignment correction, binocular fusion test pattern, safe reset/default, and device-validated limits.

## P4 — Performance baseline

- RV-0300 — Build retro-complexity calibration scene.
- RV-0301 — Define measured reference-device performance/thermal acceptance thresholds from actual Galaxy S9 tests.
- RV-0302 — Establish performance regression procedure.

## P5 — Module host

- RV-0400 — Define module lifecycle/API.
- RV-0401 — Implement sample internal module.
- RV-0402 — Enforce platform settings/input/performance contracts.

## P6 — Media player

- RV-0500 — Local flat virtual-screen playback.
- RV-0501 — 180/360 projection.
- RV-0502 — stereoscopic media layouts.
- RV-0503 — playback controls and file/library UI.
- RV-0504 — sustained decode/thermal validation.

## P7 — First games/experiences

- RV-0600 — Define first tiny retro VR gameplay prototype.
- RV-0601 — Implement and device-test first playable module.
- RV-0602 — Add further experiences only after regression/thermal gates remain healthy.

## Deferred

- broad historical Daydream app binary compatibility;
- public store/cloud services;
- additional handset/controller compatibility;
- network/social services;
- graphically intensive experiences.
