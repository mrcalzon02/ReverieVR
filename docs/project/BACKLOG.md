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
- RV-0091 — Implement controller pairing/sync, readiness state, battery telemetry, pose/button/touch decoding, recenter command, and controller test workflow. Daydream BLE is the reference backend; future controller types use the same provider abstraction.
- RV-0092 — Implement explicit 2D-to-VR entry plus safe fallback/recovery to 2D setup.
- RV-0093 — Implement optional authoritative GitHub Release update checking, user-notified Update/Not now choice, APK integrity validation when a digest is published, and Android package-installer handoff.

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
- RV-0210 — Implement a persistent global comfort-preference profile that hosted modules can consume: seated mode, snap/smooth turning, turn speed, vignette/tunneling preference, movement direction, and related safe defaults.
- RV-0211 — Implement universal recenter and seated-height/forward-offset recovery accessible from the shell and controller without digging through module menus.
- RV-0212 — Implement UI accessibility controls: text/UI scale, contrast, brightness/gamma guidance, dark interface, and no color-only critical states.
- RV-0213 — Implement audio accessibility defaults: subtitles/captions, caption size/background/position, visual alternatives for critical audio, mono/balance options where practical.
- RV-0214 — Implement input accessibility and fallback: configurable actions, one-controller navigation, left/right-hand presentation, larger targeting tolerance, optional gaze/dwell selection, and no precision-timing requirement for essential shell actions.
- RV-0215 — Implement controller-quality controls: gyro recalibration, drift/deadzone settings, touch sensitivity/deadzone, disconnect pause/reconnect overlay, and recovery without losing module state.
- RV-0216 — Implement a universal quick-access panel for battery, time/session status, volume, brightness, thermal/performance state, recenter, Settings, Home, and Exit/Recovery.
- RV-0217 — Implement nonblocking notification policy with user-selectable categories, durations, placement, animation/reduced-motion behavior, and no mandatory center-screen modal for routine status.
- RV-0218 — Implement optional session timer and break reminders with complete opt-out.
- RV-0219 — Research an optional S9 rear-camera 'real-world peek' mode. Treat it as a convenience view, not room-scale passthrough or a safety boundary, and reject it if latency/distortion is uncomfortable.

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
