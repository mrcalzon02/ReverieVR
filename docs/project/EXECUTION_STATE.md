# ReverieVR Execution State

**Role:** durable operational state  
**Updated:** 2026-10-04 (America/Sitka)

## Authority

- Repository: `mrcalzon02/ReverieVR`
- Branch: `main`
- Current verified remote implementation baseline before this documentation commit: `e7dd8774eff6d411ca8b4170eb511d1d6b9c2608`.
- Remote readback confirmed the Stage A application, verified Gradle wrapper, direct Daydream BLE backend, pinned Cardboard submodule, Stage B VR activity/renderer, first local-media vertical slice, unit-test sources, and build instructions are present on `main`.
- Cardboard submodule is pinned to `5969239e7c87f4cd64c8ec170ce1e7f4eb559e37` (v1.35.0).

## Last completed governance target

**RV-0001 — Governance/bootstrap adoption**

Acceptance: **static accepted**.

## Current implementation state

### RV-0002 — framework/runtime/controller stack

State: **draft**

Selected implementation baseline:

- Google Cardboard SDK v1.35.0 for head tracking, stereo projection, viewer geometry, and lens distortion;
- OpenGL ES rendering path;
- Cardboard Vulkan and Unity-plugin native paths disabled;
- direct Android BLE Daydream-controller provider;
- controller abstraction kept independent of Cardboard so later Android/gamepad providers can be added without replacing the VR shell.

Remaining gate: successful build plus Galaxy S9/headset/controller validation.

### RV-0003 — Android APK skeleton

State: **draft**

Implemented:

- Android application project;
- application ID `io.github.mrcalzon02.reverievr`;
- minimum SDK 26;
- compile/target SDK 36;
- Java 17 app source;
- pinned Cardboard Git submodule;
- JUnit source tests for Daydream packet parsing and updater version comparison;
- documented Gradle/NDK/CMake build path.

Cardboard introduces its required AndroidX/native build dependencies; Stage A itself remains ordinary lightweight Java/XML Android UI.

Remaining gate:

- initialize submodule in a local checkout;
- run the committed wrapper/bootstrap verifier;
- run unit tests and `:app:assembleDebug`;
- inspect resulting APK;
- install and launch on the Galaxy S9.

### RV-0090 — 2D touchscreen boot/setup surface

State: **draft**

Implemented:

- conventional portrait Stage A launcher;
- device/model/API reporting;
- real phone battery display;
- persistent QoL/settings controls;
- controller Pair / Sync and live Test Controller flow;
- update status/check controls;
- local reset/recovery;
- Enter VR gated on a ready controller.

Remaining gate: build and reference-device interaction test.

### RV-0091 — direct Daydream controller

State: **draft**

Implemented:

- Android-version-correct Bluetooth permissions;
- BLE scan with Daydream service/name candidate matching and bounded timeout;
- Android bonding flow;
- GATT connection/service discovery;
- pose notification subscription;
- READY gating on receipt of the first valid pose packet rather than descriptor-write success alone;
- bounded error if the pose notification channel subscribes but produces no usable packets;
- 20-byte pose/touch/button packet decode;
- controller battery percentage and voltage reads;
- controller recenter command;
- application-scoped controller lifetime;
- live Stage A controller diagnostics;
- independent protocol provenance record.

Remaining gate: physical Daydream-controller validation, including button/touch orientation and long-session stability.

### RV-0092 / RV-0100 — Stage A -> Stage B and VR shell

State: **draft**

Implemented:

- Enter VR enabled only after controller readiness;
- application-scoped controller connection survives Activity transition by design;
- landscape immersive `VrActivity`;
- Cardboard stereoscopic/head-tracked rendering;
- deliberately simple OpenGL ES world-space menu panel;
- head-gaze target selection;
- Daydream touchpad-click activation;
- Menu/back navigation;
- Home controller + software-yaw recenter request;
- Exit to Stage A;
- shell entry to the real local-video path rather than a dead Media placeholder;
- no fake-success module entries.

Remaining gate: build and in-headset validation.

### RV-0108 / RV-0206 — first-run setup and user optical calibration

State: **draft**

The first successful VR entry routes into a resumable setup flow unless the current setup version is complete.

Current functional pages:

1. neutral seated forward/recenter;
2. virtual user eye-spacing/IPD adjustment;
3. UI/readability scale;
4. battery HUD / look-up-mode preferences;
5. save-to-home or return without marking setup complete.

The renderer reads the saved physical Cardboard viewer profile when available and keeps the user eye-spacing value separate. User spacing is applied as a per-eye render-view correction rather than overwriting the physical viewer profile.

Current safety limits clamp user eye spacing to 50–80 mm. These are provisional engineering bounds and remain subject to Galaxy S9 + Daydream View validation.

If no saved Cardboard viewer profile exists, the draft renderer uses a 60 mm physical inter-lens fallback. That fallback must be replaced or confirmed through reference-headset profiling before optical calibration is considered accepted.

The complete planned onboarding remains broader than the current implementation: controller familiarization, viewer confirmation, per-eye fine correction, comfort, audio and performance pages still remain.

### RV-0093 — updater

State: **draft**

Implemented:

- optional check on Stage A launch;
- manual check;
- authoritative source restricted to `mrcalzon02/ReverieVR` GitHub Releases;
- explicit Update / Not now choice;
- release notes;
- trusted repository APK asset filtering;
- Android Download Manager/package-installer handoff;
- SHA-256 verification when GitHub publishes an asset digest.

There are currently no published ReverieVR GitHub Releases, so the expected live result is that no published release exists.

### RV-0500 / RV-0501 — local flat video and mono 360 video

State: **draft**

Implemented as an intentionally early vertical slice (see ADR-0008):

- Stage A local-video selection through Android `ACTION_OPEN_DOCUMENT`;
- persisted read permission scoped to the selected document rather than broad media/storage access;
- remembered display name and projection mode;
- Android `MediaPlayer` asynchronous decode;
- decoder output through `SurfaceTexture` / `GL_TEXTURE_EXTERNAL_OES`;
- flat virtual-screen geometry with decoder-reported aspect ratio;
- mono equirectangular 360° sphere geometry;
- Daydream click play/pause;
- horizontal touchpad swipe seek requests mapped to ±10 seconds;
- Menu/back stop-and-return to the shell;
- GL-surface recreation path that releases the old decoder surface and reopens the selected URI against the replacement surface.

Not yet implemented/accepted:

- stereoscopic SBS/over-under media layouts;
- richer seeking UI/scrubber, library browser, and subtitles;
- shell-global player HUD polish;
- codec-coverage matrix;
- sustained decode, thermal, A/V sync, seam/orientation, and comfort validation on the Galaxy S9.

Remaining gate: successful build plus physical flat-video and mono-360 playback tests on the reference hardware.

### RV-0006 — scrcpy / ADB diagnostic compatibility

State: **draft**

ReverieVR now treats scrcpy compatibility as a first-class developer requirement rather than an incidental convenience.

The compatibility contract requires:

- Stage A must remain operable through normal scrcpy-injected mouse/keyboard/touch input;
- Stage B must remain mirrorable/recordable while the phone display stays on in the headset;
- no blanket Android `FLAG_SECURE` policy may be introduced on ReverieVR's normal app windows;
- the reference runtime must not seize the USB data path in a way that prevents normal ADB/scrcpy use;
- Daydream BLE controller transport must coexist with wired USB debugging;
- scrcpy host-audio absence on Android 10-or-earlier reference software is not treated as a ReverieVR audio failure;
- `--turn-screen-off` is prohibited for in-headset testing because the physical phone display is the VR display;
- scrcpy-assisted runs are valid for functional observation, capture, Stage A control, crash reproduction, and UI debugging, but are **not** valid sustained performance/thermal acceptance runs because mirroring/encoding adds workload.

Helper launchers are provided in `scripts/scrcpy-reverie.sh` and `scripts/scrcpy-reverie.bat`.

Remaining gate: verify USB debugging + scrcpy against the real Galaxy S9 while Daydream BLE is connected and Stage B is actively rendering.

## Performance posture

The Stage B shell deliberately avoids a general-purpose game engine. Cardboard is configured for OpenGL ES with Vulkan and Unity integration disabled. The shell avoids per-frame gaze/button-geometry allocation and reuses its UI bitmap/texture storage instead of allocating a new 1024×768 bitmap for every hover change.

No performance/thermal acceptance claim exists until sustained Galaxy S9 testing is performed.

## Quality-of-life baseline

The project backlog includes remembered global comfort preferences, universal recenter/seated recovery, scalable/high-contrast UI, captions/visual audio alternatives, flexible one-controller/remappable input, reduced-motion behavior, controller calibration/reconnect handling, quick access, nonblocking notifications, optional session reminders, and research-only camera peek.

Automatic convenience behaviors remain user-toggleable unless required for recoverability.

## Environment limitation

This execution environment does not provide a verified Android SDK/NDK/Gradle/ADB toolchain, so the current source has **not** been claimed as successfully assembled into an APK.

No Galaxy S9, headset, optical, Bluetooth, thermal, or comfort validation has been performed from this environment.

## Next exact action

On a local Android development environment:

1. `git submodule update --init --recursive`;
2. verify Cardboard submodule SHA `5969239e7c87f4cd64c8ec170ce1e7f4eb559e37`;
3. install JDK 17, Android SDK 36, NDK 29.0.14206865, and CMake;
4. run `./scripts/verify-build-bootstrap.sh`;
5. run `./gradlew test :app:assembleDebug`;
6. install the APK on the Galaxy S9;
7. test Stage A pairing and live controller input;
8. enter the Daydream View and validate stereo/head tracking/gaze/click navigation;
9. walk through the first-run calibration pages;
10. choose and play a known-good local flat video, verify play/pause/back, then repeat with a mono equirectangular 360° sample;
11. background/resume during playback to exercise SurfaceTexture/decoder reattachment;
12. launch `scripts/scrcpy-reverie.sh` or `scripts\\scrcpy-reverie.bat` over USB and verify Stage A can be operated remotely while the Daydream controller remains connected;
13. enter Stage B and verify the stereoscopic output remains visible in scrcpy while the physical phone display stays on in the headset;
14. repeat a representative media/control flow under scrcpy for diagnostic capture, then close scrcpy;
15. perform sustained frame-pacing/thermal acceptance **without scrcpy running**;
16. capture any build, controller-axis, optical, scrcpy, media-orientation, decode, UI-scale, frame-pacing, or thermal defects for the next repair pass.
