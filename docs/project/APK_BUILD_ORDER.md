# ReverieVR APK Construction Order

**Role:** ordered product construction plan  
**Rule:** later phases may be researched early, but implementation should not leapfrog foundational acceptance gates without a recorded reason.

## Phase 0 — Governance, toolchain, and framework baseline

Establish:

- project doctrine and execution rules;
- Android/Gradle project skeleton;
- supported Android/API/ABI range;
- rendering/runtime framework decision;
- controller transport/pairing integration decision;
- dependency provenance/license inventory;
- reproducible local build instructions;
- APK install/launch path for the Galaxy S9;
- logging and diagnostics foundation.

**Gate:** a minimal signed/debuggable APK builds, installs, launches locally, and does not require discontinued Daydream servers.

## Phase 1 — 2D boot, setup, and controller pairing

The APK must begin in a conventional non-VR Android interface that can be operated directly on the phone before it is inserted into the headset.

Required capabilities:

- first-run/setup screen;
- controller pairing/sync workflow;
- current controller connection/readiness state;
- controller input test surface;
- clear retry/reconnect/re-pair controls;
- pre-VR device/runtime diagnostics;
- explicit **Enter VR** action;
- safe fallback if VR initialization fails;
- persistent route back to this setup mode for recovery.

The setup surface must remain intentionally lightweight. It exists to make VR entry reliable, not to become a second full launcher.

**Gate:** from a clean launch, the user can establish/verify controller readiness and deliberately enter VR without requiring any unavailable Google service.

## Phase 2 — ReverieVR 3D home and VR menu

After successful pre-VR setup, transition into the stereoscopic product shell.

Required capabilities:

- clean 2D-to-VR mode transition;
- stereoscopic VR home space;
- head-tracked seated viewpoint;
- head-look reticle;
- controller-driven menu navigation;
- settings persistence;
- safe return to 2D setup/recovery mode;
- clear diagnostics/status page;
- shell-global handset battery percentage/progress indicator;
- shell-global bound-controller battery percentage/progress indicator when telemetry is available, with an honest unavailable state otherwise;
- upper-right HUD placement as the reference presentation;
- optional gaze-adaptive status mode that retracts during ordinary forward viewing and reveals/drops into view on a deliberate look-up gesture;
- a settings toggle that can disable adaptive HUD behavior and preserve a simple persistent/manual presentation;
- return-to-home behavior for ReverieVR-hosted modules.

The menu is not ornamental. It is the control plane for the platform.

**Gate:** all essential VR-shell functions can be operated while the phone is in the headset without touching the screen, and the user can intentionally recover to the 2D setup mode when necessary.

## Phase 3 — Full settings control

Settings should be local, inspectable, persistent, and grouped at minimum into:

### VR/display

- render scale/quality;
- physical viewer profile and optical geometry;
- **user IPD in physical units, stored separately from the viewer's fixed lens separation**;
- live user eye-alignment calibration while wearing the headset;
- optional per-eye horizontal correction and, if device testing proves useful, vertical correction;
- binocular fusion/alignment test target with large readable geometry;
- one-action reset to known-safe viewer/user defaults;
- eye/display/viewer parameters supported by the chosen runtime;
- UI scale;
- brightness guidance/control where Android permits;
- orientation/recenter behavior;
- seated mode options;
- optional comfort vignette/turning behavior where relevant;
- global status-HUD visibility mode;
- gaze-adaptive HUD enable/disable and comfort behavior.

The physical viewer profile describes hardware and distortion. The user eye profile describes the wearer. Do not overwrite headset lens geometry merely to simulate user IPD.

Software IPD correction cannot physically move fixed lenses. The interface must therefore describe the control as alignment/comfort calibration, avoid claiming it can repair every optical mismatch, and retain a fast reset if an adjustment makes fusion worse.

### Performance
- quality preset;
- dynamic render scaling;
- target frame-rate mode supported by the stack/device;
- texture/geometry/effect budgets;
- thermal/performance status display;
- debug frame-time overlay;
- developer performance logging.

### Input/controller
- controller connection/readiness status;
- re-pair/reconnect route back through Stage A setup;
- trackpad behavior/sensitivity;
- primary/select mapping;
- secondary/back/context mapping;
- Home/recenter behavior where accessible;
- side volume button behavior where accessible;
- long-press/double-press options where reliable;
- per-module overrides;
- restore-default bindings.

### Audio
- master/module/media volume behavior;
- mute;
- spatial-audio options only if the cost/benefit is justified.

### Media/player
- default projection mode;
- stereoscopic layout;
- subtitle defaults;
- playback comfort settings.

### Developer/diagnostics
- build/version data;
- detected device/runtime capabilities;
- sensor/controller state;
- frame timing;
- logs/export where safe;
- reset configuration.

**Gate:** settings persist correctly, invalid values fail safely, and the user cannot permanently trap themselves in an unusable configuration. User-IPD calibration must modify the intended user-eye transform without corrupting the stored physical viewer profile.

## Phase 4 — Seated reference input profile

Reference controls:

- head-look = view and pointing;
- trackpad/touch = directional navigation, scrolling, context analog input;
- primary click = select/confirm/use;
- secondary/app control = back/context;
- Home = shell/recenter function where accessible;
- handset side buttons = volume by default where accessible.

Implement through named input actions, not direct raw-key checks scattered through modules.

**Gate:** the VR home and a test scene can be fully navigated using the seated profile after the controller is prepared in the 2D setup stage.

## Phase 5 — Performance/thermal baseline scene

Create a deliberately simple representative VR environment used as a performance calibration fixture.

Visual philosophy: intentionally retro, low-complexity, readable 3D. Think Doom/early-3D-era complexity as an acceptable baseline rather than modern mobile spectacle.

Measure:

- frame pacing;
- CPU/GPU workload available from instrumentation;
- memory use;
- sensor-to-view responsiveness;
- input responsiveness;
- thermal/throttling behavior over a sustained session;
- effect of render-scale changes.

This fixture becomes a regression target.

**Gate:** sustained reference-device operation remains stable enough for comfortable use under the project's measured target.

## Phase 6 — Module/subproject host API

Define how experiences live inside ReverieVR:

- lifecycle;
- input actions;
- render context;
- settings access;
- pause/resume/return-to-home;
- recovery/exit to 2D setup where required;
- asset/resource boundaries;
- save/config storage;
- performance budget declaration;
- capability requirements;
- shell-global HUD/status continuity contract;
- prohibition on replacing or bypassing shell recovery/global-settings routes.

Modules must not bypass platform settings or silently replace global controls.

**Gate:** at least one internal sample module loads, runs, pauses, resumes, returns to VR home, and respects platform settings.

## Phase 7 — Local VR video player

Implement local media playback after the shell is stable.

Target capabilities:

- ordinary flat video in virtual-screen mode;
- 180/360 projection where supported;
- mono and common stereoscopic layouts;
- local file selection/library;
- seeking, play/pause, volume, recenter;
- hardware decoding where practical;
- subtitles where practical;
- performance-safe UI.

**Gate:** sustained local playback on reference hardware without depending on Google Daydream services.

## Phase 8 — First playable retro VR experiences

Build deliberately small experiences around the proven platform budget.

Initial content should validate:

- head-look aiming/interaction;
- trackpad movement/menu use;
- primary/secondary actions;
- seated comfort;
- scene/module transitions;
- save/settings integration;
- sustained thermal behavior.

Graphical ambition stays subordinate to stable VR.

## Phase 9 — Compatibility expansion

Only after the reference path works:

- additional Android phones;
- alternative 3DoF viewers;
- additional controllers/input sources;
- per-device profiles;
- historical Daydream compatibility research where legally and technically appropriate.

Compatibility expansion must not destabilize the Galaxy S9 reference path.
