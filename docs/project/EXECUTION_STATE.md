# ReverieVR Execution State

**Role:** durable operational state  
**Updated:** 2026-10-05 (America/Sitka)

## Authority

- Repository: `mrcalzon02/ReverieVR`
- Branch: `main`
- Current implementation baseline immediately below this documentation commit: `8241111ad30e44ede41c675f526f8e41f25323f1`.
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
- classic-Bluetooth RFCOMM receiver for a paired Android phone using the historical Daydream controller-emulator framing;
- Android gamepad/joystick detection and key/axis routing;
- normalized `VrInputAction` routing kept independent of Cardboard and transport-specific packet formats.

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
- paired-phone controller chooser for historical Daydream-compatible controller emulator apps;
- live Android gamepad/joystick readiness detection;
- update status/check controls;
- local reset/recovery;
- Enter VR gated on a ready controller.

Remaining gate: build and reference-device interaction test.

### RV-0091 / RV-0102 — controller sources and normalized input

State: **draft**

Implemented:

**Physical Daydream controller**

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

**Paired Android phone controller**

- user-selectable paired Bluetooth-device chooser in Stage A;
- classic Bluetooth RFCOMM connection to UUID `ab001ac1-d740-4abb-a8e6-1cb5a49628fa`;
- independent 4-byte big-endian length framing;
- independently authored minimal protobuf-wire decoder for motion/touch, gyroscope, accelerometer, orientation, and key events;
- historical click/App/Home/volume key-code compatibility;
- controller-phone battery deliberately reported unavailable because the transport does not expose trustworthy battery telemetry;
- pure-Java protocol parser fixtures for orientation, touch motion, click-key events, and malformed length-delimited input.

**Generic Android controller**

- attached Android `SOURCE_GAMEPAD` / `SOURCE_JOYSTICK` devices can satisfy the Stage-B input-readiness gate;
- device attach/remove/change notifications refresh Stage A and Stage B live;
- A/center/Enter -> Select;
- B/Back -> Back;
- Start/Mode -> Recenter;
- D-pad/hat/stick navigation emits normalized directional actions;
- L1/R1 and left/right navigation can drive player seek;
- generic gamepad battery remains unavailable unless a future Android/device-specific source exposes it honestly.

**Action layer**

- `VrInputRouter` is now the controller binding boundary;
- physical Daydream, phone emulator, generic gamepad, Cardboard trigger, and Cardboard system/back controls terminate in named `VrInputAction` values;
- volume actions adjust the Android music stream;
- raw Daydream button interpretation is no longer embedded directly in `VrActivity`.

Remaining gates:

- build/unit-test execution;
- physical Daydream-controller validation;
- actual second-phone emulator RFCOMM validation;
- generic Bluetooth/USB gamepad validation;
- axis/polarity/comfort verification;
- user-remapping UI remains RV-0202.

### RV-0094 — ReverieVR Controller companion APK

State: **draft**

Implemented:

- independent Android application module `controller-app`;
- application ID `io.github.mrcalzon02.reverievr.controller`;
- portrait spare-phone controller surface with large touchpad plus Select, App/Back, and Home/Recenter controls;
- physical controller-phone volume keys forwarded as controller volume actions while connected;
- rotation-vector orientation with `TYPE_GAME_ROTATION_VECTOR` fallback;
- gyroscope and accelerometer transport when available;
- explicit refusal to start 3DoF control when no usable rotation-vector sensor exists;
- classic Bluetooth RFCOMM server using the historical controller-emulator UUID;
- bonded-client verification before accepting the headset session;
- 4-byte big-endian framing and independently authored protobuf-wire event writer;
- separate discrete-control queue plus coalesced orientation/gyro/accelerometer lanes;
- keep-screen-on behavior during companion use;
- backward-compatible ReverieVR status event exposing the real controller-phone battery percentage to the headset HUD;
- pure-Java protocol writer tests;
- headset updater asset filtering that rejects controller APKs.

Remaining gates:

- build `:controller-app:assembleDebug`;
- install on a second Android phone;
- pair both phones through Android Bluetooth;
- verify headset selection, RFCOMM connection, orientation/touch/button mapping, reconnect behavior, battery HUD, and long-session stability;
- validate that historical third-party/Google-compatible emulator traffic still works with the battery extension absent;
- measure sensor/update cadence and latency before claiming controller-comfort acceptance.

The companion is optional. Physical Daydream BLE and Android gamepad recovery paths remain independent.



### RV-0092 / RV-0100 — Stage A -> Stage B and VR shell

State: **draft**

Implemented:

- Enter VR enabled after either a dedicated controller provider is ready or an Android gamepad/joystick is attached;
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
5. run `./gradlew test :app:assembleDebug :controller-app:assembleDebug`;
6. install the APK on the Galaxy S9;
7. test Stage A physical Daydream pairing and live controller input;
8. install the companion APK on a second Android phone and validate paired-phone RFCOMM control plus controller-phone battery telemetry;
9. enter the Daydream View and validate stereo/head tracking/gaze/Select navigation with each available input source;
10. walk through the first-run calibration pages;
11. choose and play a known-good local flat video, verify play/pause/back, then repeat with a mono equirectangular 360° sample;
12. background/resume during playback to exercise SurfaceTexture/decoder reattachment;
13. launch `scripts/scrcpy-reverie.sh` or `scripts\\scrcpy-reverie.bat` over USB and verify Stage A can be operated remotely while a controller source remains connected;
14. enter Stage B and verify the stereoscopic output remains visible in scrcpy while the physical phone display stays on in the headset;
15. repeat a representative media/control flow under scrcpy for diagnostic capture, then close scrcpy;
16. perform sustained frame-pacing/thermal acceptance **without scrcpy running**;
17. capture any build, controller-axis, phone-controller, optical, scrcpy, media-orientation, decode, UI-scale, frame-pacing, or thermal defects for the next repair pass.
