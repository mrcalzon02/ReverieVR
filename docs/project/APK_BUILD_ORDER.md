# ReverieVR APK Construction Order

**Role:** ordered product construction plan  
**Rule:** later phases may be researched early, but implementation should not leapfrog foundational acceptance gates without a recorded reason.

## Phase 0 — Governance, toolchain, and framework baseline

Establish:

- project doctrine and execution rules;
- Android/Gradle project skeleton;
- supported Android/API/ABI range;
- rendering/runtime framework decision;
- dependency provenance/license inventory;
- reproducible local build instructions;
- APK install/launch path for the Galaxy S9;
- logging and diagnostics foundation.

**Gate:** a minimal signed/debuggable APK builds, installs, launches locally, and does not require discontinued Daydream servers.

## Phase 1 — ReverieVR shell and VR menu

Build the product shell before games.

Required capabilities:

- stereoscopic VR shell;
- head-tracked seated viewpoint;
- head-look reticle;
- controller-driven menu navigation;
- settings persistence;
- safe reset/recovery path if a bad setting makes VR navigation unusable;
- clear diagnostics/status page;
- return-to-shell behavior for ReverieVR-hosted modules.

The menu is not ornamental. It is the control plane for the platform.

**Gate:** all essential shell functions can be operated while the phone is in the headset without touching the screen.

## Phase 2 — Full settings control

Settings should be local, inspectable, persistent, and grouped at minimum into:

### VR/display
- render scale/quality;
- eye/display/viewer parameters supported by the chosen runtime;
- UI scale;
- brightness guidance/control where Android permits;
- orientation/recenter behavior;
- seated mode options;
- optional comfort vignette/turning behavior where relevant.

### Performance
- quality preset;
- dynamic render scaling;
- target frame-rate mode supported by the stack/device;
- texture/geometry/effect budgets;
- thermal/performance status display;
- debug frame-time overlay;
- developer performance logging.

### Input
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

**Gate:** settings persist correctly, invalid values fail safely, and the user cannot permanently trap themselves in an unusable configuration.

## Phase 3 — Seated reference input profile

Reference controls:

- head-look = view and pointing;
- trackpad/touch = directional navigation, scrolling, context analog input;
- primary click = select/confirm/use;
- secondary/app control = back/context;
- Home = shell/recenter function where accessible;
- handset side buttons = volume by default where accessible.

Implement through named input actions, not direct raw-key checks scattered through modules.

**Gate:** the shell and a test scene can be fully navigated using the seated profile.

## Phase 4 — Performance/thermal baseline scene

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

## Phase 5 — Module/subproject host API

Define how experiences live inside ReverieVR:

- lifecycle;
- input actions;
- render context;
- settings access;
- pause/resume/return-to-shell;
- asset/resource boundaries;
- save/config storage;
- performance budget declaration;
- capability requirements.

Modules must not bypass platform settings or silently replace global controls.

**Gate:** at least one internal sample module loads, runs, pauses, resumes, returns to shell, and respects platform settings.

## Phase 6 — Local VR video player

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

## Phase 7 — First playable retro VR experiences

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

## Phase 8 — Compatibility expansion

Only after the reference path works:

- additional Android phones;
- alternative 3DoF viewers;
- additional controllers/input sources;
- per-device profiles;
- historical Daydream compatibility research where legally and technically appropriate.

Compatibility expansion must not destabilize the Galaxy S9 reference path.
