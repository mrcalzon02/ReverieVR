# ReverieVR Execution State

**Role:** durable operational state  
**Updated:** 2026-10-05 (America/Sitka)

## Authority

- Repository: `mrcalzon02/ReverieVR`
- Branch: `main`
- Current implementation baseline immediately below this documentation update: `c70026f8e5bd307bd5f53d46c862a87b629bff20`.
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
- visible installed version/build identity;
- update status/check controls only when the build has a persistent signing identity;
- local reset/recovery;
- centralized pressed/disabled/failure button states with ACK/NACK audio feedback across Stage A, Stage B shell selection/rejection, and the controller companion;
- Enter VR gated on a ready controller.

Remaining gate: build and reference-device interaction test.

### RV-0091 / RV-0102 — controller sources and normalized input

State: **draft**

Implemented:

**Physical Daydream controller**

- Android-version-correct Bluetooth permissions;
- BLE discovery handles the Android 8–11 Location-services gate explicitly instead of silently returning zero results;
- already-bonded Daydream controllers are attempted before a new scan;
- low-latency foreground BLE scan is intentionally broad, then Daydream FE55/name matching occurs inside ReverieVR so Android cannot discard a controller merely because FE55 or the exact local name was absent from the initial advertisement;
- Development logging records aggregate advertisement/named/FE55 counts and detailed information only for a matched Daydream candidate, avoiding arbitrary nearby-device address/name capture;
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

Current verification: phone-test #21 completed successfully against `f1ccc93963bbcf18b81169626f45877a4c33f3e9` and published prerelease `phone-test-21-1`. This package includes broad foreground discovery, privacy-bounded scan diagnostics, explicit physical Daydream/Home pairing guidance, and manifest capture of Bluetooth/Location/permission prerequisites. GitHub Issue #2 tracks the remaining physical Galaxy S9 validation; FE55 discovery and live GATT/pose streaming still require handset evidence before acceptance.

Remaining gates:

- physical Galaxy S9 + Daydream-controller discovery, FE55 service discovery, pose subscription and live packet-stream validation;
- capture Development diagnostics and link the resulting RV-0208 diagnostic issue/receipt to Issue #2 if the physical path still fails;
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
- landscape immersive non-resizable `VrActivity`;
- explicit GLES2, accelerometer and gyroscope hardware requirements;
- Cardboard stereoscopic/head-tracked rendering;
- fixed seated-origin home shell with three real lightweight GLES environment presets: White Cube Room + gray dais, Pastoral Forest Glade, and Windswept Dune Beach;
- persistent home-environment preference with live in-headset switching;
- world-space menu panel reorganized as the VR launcher hierarchy: Media, DOS Games, Native Apps, Environment, Setup / Comfort, and Exit to Phone;
- Media child interface for floating-screen playback, mono-360 projection switching, recenter, and Android document-picker handoff;
- DOS child library with module paging, direct module launch, Android import-picker handoff, and return-to-library behavior after a DOS session;
- flat media and DOS framebuffers composite onto the existing floating-screen geometry while the selected home environment remains visible around them; mono 360° media still owns the full sphere;
- head-gaze target selection;
- selectable VR pointer policy: Auto, Gaze, or Controller;
- physical Daydream controller quaternion drives a true 3DoF menu pointer ray;
- generic Android gamepads without inertial sensors drive a virtual pointer orientation through the right stick, with navigation/D-pad nudge fallback;
- Auto pointer mode prefers fresh tracked controller pose, then virtual gamepad aim, then gaze;
- active pointer source is visible in the Home status card as Tracked controller, Virtual gamepad, Gaze, or Controller unavailable;
- the selected low-poly Daydream controller OBJ is packaged as the normal in-world controller visual with separate touchpad/Home/App/volume material groups; the old procedural shape is retained only as an asset-load fallback;
- controller visual and pointer ray now share the same moving shell anchor and the ray begins at the model's front emitter rather than its center;
- handset TYPE_LINEAR_ACCELERATION feeds a damped, spring-returned, hard-bounded pseudo-positional headset offset, producing small inertial parallax while remaining explicitly distinct from true 6DoF tracking;
- Home UI is split into three independently positioned world-space planes: left navigation, center status/content, and right quick options;
- Home side planes are pushed farther left/right and tilted so their outer edges come toward the viewer, giving the launcher a shallow wrap-around arrangement;
- the shell panel envelope and screen-space power HUD are contracted to 90% of the prior extent to keep edge content inside the reference Daydream View comfort area;
- the universal Quick Menu captures the headset's live horizontal heading when opened and renders in that captured direction rather than the possibly-wrong shell-forward direction, allowing orientation recovery without physically spinning to find the menu;
- startup forward orientation follows the live headset forward vector through a short stability window before locking, with a bounded timeout fallback, and all headset/controller forward calculations now use tested shell-consistent yaw polarity;
- startup heading is explicitly initialized from the first headset heading rather than leaving shell yaw at the sensor/world default;
- the same shell-owned modal now serves as a first universal Quick Menu slice, reachable from Daydream App/Menu, Cardboard system menu, gamepad Start/Mode, and keyboard Menu; it exposes Headset Forward, Controller Forward when a fresh pose exists, volume down/up, contextual Back, Home, Exit to Phone, and Close;
- orientation changes are software-owned: shell yaw and controller yaw-calibration advance together when preserving controller physical direction, while Center on Controller rotates the shell until the current tracked controller ray becomes forward;
- focus transitions have quiet rate-limited audio, activation uses positive feedback, and rejected actions retain distinct failure feedback;
- unavailable/inert menu targets do not acquire hover or accept activation;
- Daydream touchpad-click activation;
- Menu/back navigation through the launcher hierarchy;
- Home controller + software-yaw recenter request;
- Exit to Stage A;
- no dead Media placeholder;
- no fake-success module entries;
- persistent VR-startup phase marker armed at Enter VR and cleared only after the renderer completes its first frame;
- recoverable Java/JNI/GL startup failures return to Stage A with explicit failure feedback rather than silently dropping the user;
- interrupted process-level launches are reported on the next Stage A start with the last persisted startup phase;
- renderer surface/new-frame/draw-eye callbacks contain recoverable RuntimeException/LinkageError failures and report the failing phase;
- phone-test CI requires Cardboard JNI `libcardboard_sdk_jni.so` in both packaged target ABIs.

Reference-device startup defect RV-0092 is resolved: the physical Galaxy S9 now reaches the rendered VR shell instead of exiting before frame one. Issue #3 is closed after visible handset evidence of the center interaction marker, close control, phone/controller battery HUD, and settings control.

Current validation: phone-test #43 completed successfully against `8e62a6772bbdfc61315431d58da89ec790f8d6ee`, passing JVM tests, unsigned Android/native debug assembly, persistent phone-test signing preparation, non-debuggable phoneTest packaging, final APK verification, artifact preservation, and prerelease publication as `phone-test-43-1`. The release retains signer SHA-256 `464dc2d100c3bb14deac1da680d29418cc22e60e1857852cb54541f7d7787f7c` and versionCode `43001`.

Phone-test #43 includes deterministic first-frame headset heading initialization, a shell-owned floating Orientation palette, Daydream App/Menu and Cardboard/gamepad/keyboard menu routing, Center on Headset, Center on Controller, Back One Level and Close actions, modal hosted-input suppression, and software controller-yaw calibration that follows shell heading changes without issuing an unrelated Daydream hardware-zero command.

Remaining gate: physical Galaxy S9 + Daydream View validation of startup heading behavior, Center on Headset, Center on Controller, menu placement/readability over Home/Media/DOS/Native surfaces, the 10% comfort contraction, HUD edge visibility, inertial translation direction/gain/settling feel, wrapped Home panel comfort/readability, Daydream model orientation/scale, emitter-ray alignment, and gamepad virtual aim. Build/package success is not device-comfort acceptance.

### RV-0109 — VR shell/environment ambience

State: **draft**

The user-supplied **Starry Cereal** track is packaged as the VR shell's looping ambient menu music. The APK asset is a 267.312-second, 48 kHz stereo MP3 optimized to 48 kbps for handset package size (asset SHA-256 `dd0240297ea62fe2002b736ce2b43002cafc307ad76e32d795587012ffb99c00`; supplied source SHA-256 `26d5f261b080b913cce08ec8ea7e75105fbb6fb1d14a3808d4298302a24454cf`). Playback is shell-owned: it pauses for Activity lifecycle loss and whenever local video, DOS, or Native hosted content starts, resumes from the previous music position when the shell returns, loops continuously, and fails quiet rather than breaking VR startup.

Remaining gate: Galaxy S9 + Daydream View listening validation for perceived level, looping behavior, transitions, and long-session comfort. User-facing music volume/mute remains part of RV-0203 audio settings.

### RV-0216 — universal quick-access shell panel

State: **draft**

Implemented first slice:

- Menu/Start opens one shell-owned modal over Home, local media, DOS sessions, native modules, setup, and child menus;
- hosted/module input is suppressed while the modal is open so recovery actions cannot leak into guest content;
- real actions are exposed for headset-forward calibration, tracked-controller-forward calibration when a fresh pose exists, volume down/up, contextual Back, Home, Exit to Phone, and Close;
- Home and Exit stop active media/DOS/native content through the shell-owned lifecycle before mode changes;
- the modal also reports phone/controller battery, elapsed VR session time, Android thermal status, battery-sensor temperature, and rolling frame-time p95, refreshed once per second while open;
- brightness down/up adjusts the VR Activity window in bounded 10% steps without requiring global Android brightness-write permission;
- a nested Quick Settings page changes battery HUD visibility, numeric percentages, look-up reveal, pointer mode, and UI scale without leaving hosted content;
- unavailable controller-forward calibration is visibly disabled rather than pretending to work.

RV-0216's planned functional controls are now represented by implemented shell behavior. Remaining gate: Galaxy S9 + Daydream View comfort/readability and interaction validation before acceptance.

### RV-0108 / RV-0206 — first-run setup and user optical calibration

State: **draft**

The first successful VR entry routes into a resumable setup flow unless the current setup version is complete.

Current setup schema: **v2**.

Current functional pages:

1. neutral seated forward/recenter;
2. controller familiarization with active source and last normalized action feedback;
3. virtual user eye-spacing/IPD adjustment;
4. UI/readability scale;
5. battery HUD / look-up-mode preferences;
6. save-to-home or return without marking setup complete.

The familiarization page captures Back locally so it can be tested safely without leaving the page. Select, Recenter, navigation and volume actions remain live and are reported through the normalized action layer. The page is informational rather than a hard capability gate.

The renderer reads the saved physical Cardboard viewer profile when available and keeps the user eye-spacing value separate. User spacing is applied as a per-eye render-view correction rather than overwriting the physical viewer profile.

Current safety limits clamp user eye spacing to 50–80 mm. These are provisional engineering bounds and remain subject to Galaxy S9 + Daydream View validation.

If no saved Cardboard viewer profile exists, the draft renderer uses a 60 mm physical inter-lens fallback. That fallback must be replaced or confirmed through reference-headset profiling before optical calibration is considered accepted.

The complete planned onboarding remains broader than the current implementation: viewer confirmation, per-eye fine correction, comfort, audio and performance pages still remain.

### RV-0093 — updater

State: **draft**

Implemented:

- optional check on Stage A launch and explicit manual check;
- authoritative source restricted to public `mrcalzon02/ReverieVR` GitHub Releases with no GitHub token in the APK;
- phone-test prerelease discovery through the release list rather than `/releases/latest`;
- numeric `phone-test-<run>-<attempt>` ordering and selection of the highest valid newer release;
- dedicated non-debuggable `phoneTest` distribution build for handset and controller companion;
- permanent direct-distribution signing identity provisioned through protected GitHub Actions secrets;
- rerun-safe Android versionCode `GITHUB_RUN_NUMBER * 1000 + GITHUB_RUN_ATTEMPT`;
- exact installed build identity displayed in Stage A;
- GitHub SHA-256 asset digest required fail-closed;
- downloaded APK privately parsed before Android's installer sees it;
- package ID, strictly newer versionCode, phone-test tag/versionCode mapping, installed signer, and pinned permanent signer verified in-app;
- candidate signer verified in-app when Android exposes archive signing metadata; if an OEM/platform archive parser omits that metadata after all other checks pass, final replacement-signer continuity is delegated to Android Package Installer rather than falsely rejecting a valid package;
- trusted repository release URL filtering;
- Android Download Manager / package-installer handoff;
- updater controls omitted entirely outside the persistent signing channel;
- VR Home can request an update check and hands the operation back to Stage A.

Permanent signer evidence:

- certificate SHA-256: `464dc2d100c3bb14deac1da680d29418cc22e60e1857852cb54541f7d7787f7c`;
- phone-test #36 / versionCode 36001: first permanent-lineage package;
- phone-test #37 / versionCode 37001: same signer;
- phone-test #38 / versionCode 38001: same signer, full signed QoL package;
- phone-test #40 / versionCode 40001: same signer; updater archive-signer compatibility repair and CardboardView final-APK presence gate passed;
- CI independently verifies both headset and companion APK signer fingerprints before every release publication.

The old updater failure is fully explained: prereleases were invisible to the original endpoint and historical CI packages used different ephemeral debug signing certificates. Android correctly refused those packages as in-place replacements.

One transition install from the old ephemeral-debug lineage remains unavoidable. After that, the permanent lineage is designed for normal in-place upgrades.

Remaining acceptance gate: on the physical Galaxy S9, install the first permanent-lineage build once, use ReverieVR to discover a later permanent-lineage release (currently #38), allow Android to replace it without uninstalling, and verify ReverieVR settings/data survive. Issue #4 remains open until that device proof is captured.


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

### RV-0106 / RV-0107 — global power HUD and look-up reveal

State: **draft**

Implemented:

- separate transparent HUD texture rather than battery text baked into the menu panel;
- two always-available status rows while the HUD feature is enabled: PHONE and CTRL;
- real progress-bar fill from 0–100%;
- optional numeric percentage display using the existing Show Percentages preference;
- honest controller/phone unknown state shown as an empty bar plus `--`, never fabricated 0%;
- overlay rendered after both the normal VR shell and local video scene so media playback does not hide battery status;
- compact upper-right placement during normal forward viewing;
- optional look-up reveal using the player head-forward vector; above the current provisional threshold the HUD drops lower into the view;
- Battery HUD and Look-up Reveal remain user-toggleable QoL features;
- HUD geometry and texture storage are reused without per-eye allocation.

Remaining gates:

- Galaxy S9 / Daydream View legibility and binocular-comfort validation;
- verify the provisional look-up threshold feels natural;
- validate controller orientation axes before adding controller-pointing-up as an alternate reveal trigger;
- confirm no meaningful frame-time or thermal regression.

### RV-0202 / RV-0207 — virtual bindings and standard HID

State: **draft**

Implemented:

- stateful `InputBindingEngine` distinct from one-shot shell navigation actions;
- versioned persistent binding-profile codec;
- source signals for Select/Back/Recenter/volume, navigation, touchpad axes, Android gamepad axes, and head yaw/pitch deltas;
- transforms for digital, analog, positive-threshold and negative-threshold mappings;
- configurable scale, deadzone and threshold values;
- virtual guest keyboard with held-key lifetime;
- virtual mouse buttons, relative deltas, absolute pointer coordinates and wheel accumulator;
- virtual joystick axes/buttons;
- release-all behavior on profile change, Activity pause, controller disconnect and input-device removal;
- built-in starter profiles for DOS Doom-style control, generic head-mouse FPS control, and touchpad absolute-cursor control;
- direct Android HID keyboard passthrough for common DOS keys;
- direct Android HID mouse passthrough for left/right/middle buttons, relative/absolute motion and wheel;
- mouse events intercepted through both generic-motion and mouse-sourced pointer dispatch;
- pure-Java tests for held/released keys, threshold release, relative mouse accumulation, profile persistence, DOS content extensions and virtual mouse cleanup.

Remaining gates:

- actual Bluetooth keyboard test;
- actual Bluetooth/USB mouse test;
- scrcpy SDK and UHID input validation;
- relative pointer capture while a hosted DOS session owns the mouse;
- in-VR binding overlay/editor and host-reserved escape path.

### RV-0400 / RV-0401 — native procedural module foundation

State: **draft**

ADR-0015 now governs the native-game path.

Implemented foundation:

- native module ABI v1 uses fixed-width C structures with explicit structure sizes and ABI versions rather than exposing C++ classes across module boundaries;
- packaged modules are selected through a compile-time allowlist and loaded with `dlopen` / a fixed entry symbol;
- unknown module ids, missing symbols, descriptor/id mismatches, unsupported ABI versions, unsupported GLES major requirements and missing callbacks fail closed with diagnostics;
- lifecycle includes create/destroy, GL-context creation, explicit GL-context release, resume/pause, update and per-eye render callbacks;
- the host owns Cardboard, Activity lifecycle, shell recovery and the active GL context;
- the NDK build is now unconditional for the headset app while DOSBox Pure remains an optional additional native target;
- both `armeabi-v7a` and `arm64-v8a` remain target ABIs;
- OpenKTG is vendored from `jaromil/kkrieger-werkkzeug3` revision `72f7697c8b5be6fadae41f9ca6312cd5f88fdc4c` with its public-domain provenance;
- the separate `libreverie_module_test_chamber.so` expands an OpenKTG procedural texture at module creation and renders a deliberately simple GLES2 chamber;
- the proof module accepts normalized movement plus primary/secondary input and restores GL program/buffer/texture and enable-state ownership after each eye render;
- no Win32, Direct3D, DirectSound, kkrunchy/YASM runtime or arbitrary downloaded native code was introduced.

Stage B integration now implemented:

- VR Home exposes the packaged Procedural Test Chamber as a sixth shell-owned action;
- module launch activates a dedicated hosted profile mapping touchpad axes to virtual joystick X/Y and Select to virtual joystick button 0;
- native update executes on the Cardboard render thread using the existing normalized virtual input bus;
- each eye receives Cardboard's eye view plus ReverieVR user-IPD correction and Cardboard projection;
- Back is consumed by the shell, releases module GL resources on the render thread, stops the module and restores the previous binding profile;
- Activity pause/resume is forwarded without creating a second Activity or EGL context;
- renderer shutdown explicitly releases module GL resources before Java/native destruction;
- the shell-global power/status HUD renders after native module content.

Current-environment verification completed:

- remote `main` readback confirms ADR-0015, the ABI header, native host, separate test-module source/library target, Java runtime bridge, Stage B wiring and OpenKTG provenance are present;
- the four vendored OpenKTG files have exact Git blob SHA matches to the pinned upstream revision;
- static source checks confirm the NDK build no longer hard-fails when DOSBox Pure is absent;
- static source checks confirm unknown native-module ids fail closed, ABI/descriptor/callback validation is present, and only the compile-time allowlisted packaged test library is named;
- the proof module source contains no Win32, Direct3D, DirectSound or x86 inline-assembly path;
- Java/C++ structural brace checks passed for the new host/runtime/module and Stage B integration sources;
- GitHub Actions phone-test run #6 successfully executed the committed bootstrap verifier, JVM tests, headset and controller APK builds;
- Cardboard native code compiled and linked for both `arm64-v8a` and `armeabi-v7a` with Vulkan/Unity disabled;
- the headset APK passed an explicit package check for both ABIs of `libreverie_native_host.so`, `libreverie_module_test_chamber.so`, and `libreverie_dos_host.so`;
- `apksigner verify --verbose --print-certs` passed for the published headset APK;
- GitHub prerelease `phone-test-6-1` was published from commit `c70026f8e5bd307bd5f53d46c862a87b629bff20`;
- no Galaxy S9/device-runtime claim is made until the APK is installed and exercised on the reference hardware.

Remaining gates:

- Galaxy S9 stereo/head-tracking/movement/Select/Back/HUD/pause-resume validation;
- sustained frame-pacing, battery and thermal validation;
- shared module settings/audio/save/performance-budget services beyond the proof ABI.

This is the ADR-0015 validation stop. Do not expand into Kkrieger gameplay,
Werkkzeug3 mesh/operator porting or V2 audio until the proof module passes the
reference-device gates above.

### RV-0610 — Between Deliveries: The Red Ledger VR interaction/persistence foundation

State: **draft**

The first full native-game track is now in implementation, following the
recorded RV-GAME-01 order rather than skipping ahead to the larger outdoor
projects.

Implemented foundation:

- platform-independent deterministic one-room bar/economy state with cash,
  debt, beer stock, clean/dirty cups, serving, washing and a day-close ledger;
- four compact recurring patron archetypes and a deliberately tiny supplier
  catalogue;
- protection-demand, inspection/fine and supply-interruption event states;
- low-complexity GLES2 concrete-room renderer with counter/tap, stool,
  cup-state proxies, back-room mattress/ledger props, patron proxy, event marker
  and flickering light;
- separate `libreverie_module_red_ledger.so` ndk-build target using the
  existing native-module ABI without creating Android/EGL/Cardboard ownership;
- append-only ABI-v1 native input fields for a host-calibrated world-space
  pointer ray. The host distinguishes tracked Daydream-class controller aim
  from virtual-gamepad aim and does not expose raw Android controller pose to
  modules;
- append-only ABI-v1 module-scoped persistence callbacks. Save slots are bounded
  to 64 KiB, restricted to safe filenames inside the active module's private
  app-data directory, and written with temporary-file plus atomic-replace
  semantics;
- Red Ledger ray/AABB work-surface interaction for the tap, wash station,
  ledger, beer-order card, cup-order card and protection envelope. Select still
  travels through the existing normalized hosted-native binding profile rather
  than a raw-controller shortcut;
- successful work-surface actions persist economic state through the host save
  service, while module creation attempts to restore the versioned save;
- explicit 152-byte little-endian Red Ledger save schema with magic/version,
  range validation, event/day consistency checks and corrupt-save rejection;
- Quick Menu continues to suppress native guest input; Back/Home/recovery stay
  shell-owned.

Current verification:

- phone-test #50's dedicated Red Ledger simulation step compiled and executed
  the protection -> inspection -> supply-interruption sequence plus
  save/restore and corrupt-save rejection using C++17 with
  `-Wall -Wextra -Wpedantic -Werror`;
- the same run completed the full unsigned Android/Java/NDK tree build with the
  extended Java/JNI/native ABI contracts present;
- the persistently signed phone-test package build completed successfully;
- APK verification explicitly found `libreverie_module_red_ledger.so` for
  both `armeabi-v7a` and `arm64-v8a`, alongside the established native host,
  test chamber, DOS and Cardboard libraries;
- phone-test #50 staged its artifact and published prerelease
  `phone-test-50-1` from commit
  `2e4d14e9e31dde1e1c674049b15dffb181dbcbaa`; headset APK SHA-256 is
  `8201189378bb7d3a4ecaf4ce82a1c251d3eefb9e088e5e462dbb24a91037e5f6`;
- GitHub subsequently marked run #50 cancelled because newer phone-test #51
  began under the repository's latest-build-wins concurrency policy. Every
  Red Ledger/build/package/verify/release job step had already completed
  successfully and the prerelease exists, so #50 is retained as static/package
  evidence but not represented as a clean overall workflow-success badge;
- the module remains intentionally absent from
  `reverie_native_host.cpp`'s compile-time built-in allowlist, so it is not
  yet a visible Native Apps entry;
- no Galaxy S9 stereo, controller-usability, save durability, comfort or thermal
  acceptance is claimed by this evidence.

Remaining gates before user-visible launch:

- close the relevant RV-0402 native-module runtime gap on the Galaxy S9 and
  prove the existing Test Chamber/native host through the required lifecycle
  transitions;
- exercise Red Ledger's tracked-controller ray against the actual tap, wash
  area, ledger, supplier cards and protection envelope in the Daydream View,
  then tune target volumes/reach from physical evidence;
- verify Quick Menu, Home, Back and headset/controller recenter behavior while
  the module is active, including input suppression while the modal is open;
- verify save/load across real module exit/re-entry and an in-place APK update;
- perform the required 15-minute S9 runtime/frame-pacing/battery/thermal pass;
- only after those gates, add Red Ledger to the trusted native allowlist and
  expose it in Native Apps.

### RV-0410 / RV-0411 — DOS runtime baseline and modules

State: **draft**

Selected runtime:

- DOSBox Pure;
- tag `1.0-preview6`;
- pinned commit `a4a0bab7f8931433588f2fcad9045c85b277373d`;
- GPLv2 or later explicitly accepted for this integration;
- upstream Android NDK build already supports `armeabi-v7a` and `arm64-v8a`;
- upstream libretro metadata advertises keyboard/mouse focus and direct content support for ZIP/DOSZ, EXE/COM/BAT, ISO/CHD/CUE, IMG/IMA/VHD/JRC, M3U/M3U8, CONF and directories.

Implemented module foundation:

- Stage A DOS import panel;
- Android document picker for file/container imports;
- background copy into `files/dos-modules/<module-id>/content`;
- sanitized stable native filesystem path;
- per-module metadata file with display name, original filename, binding-profile id and import timestamp;
- module listing/deletion repository;
- explicit warning in design that a bare EXE is insufficient for games that depend on sibling assets;
- pinned upstream fetch scripts with dirty-checkout refusal and post-checkout SHA verification;
- DOSBox Pure source checkout excluded from ReverieVR git history while the pin/provenance stays committed.

Implemented native host foundation:

- conditional Gradle/ndk-build activation when the pinned DOSBox Pure checkout exists;
- upstream DOSBox Pure built as `libretro.so` for `armeabi-v7a` and `arm64-v8a`, with ReverieVR's JNI frontend built as `libreverie_dos_host.so`;
- libretro init/load/run/unload/deinit lifecycle behind the existing `DosNativeRuntime` Java contract;
- app-private system, save and content-directory environment callbacks;
- legacy core-option default capture, with 48 kHz audio and software-multithreaded Voodoo forced until a hardware-render callback exists;
- XRGB8888 framebuffer capture into a tightly packed host buffer;
- bounded two-second stereo PCM ring buffer;
- synchronized virtual keyboard, mouse, pointer and joystick polling from the existing virtual input bus;
- core shutdown request propagation and Android log forwarding;
- bootstrap verification rejects a fetched DOSBox Pure checkout whose commit does not match the project pin;
- host-side C++ syntax/type checking and JNI descriptor verification passed.

Additional Stage B integration now implemented:

- a dedicated Java DOS session worker owns native run cadence without executing DOSBox Pure on the Cardboard GL thread;
- lifecycle pause/resume releases guest input and pauses/resumes audio;
- a fifth VR Home action opens a paged in-headset DOS library and can launch any imported/bundled module without returning to the touchscreen;
- module launch temporarily activates that module's binding profile and restores the prior profile when the session ends;
- a nearest-filtered OpenGL guest texture consumes the native XRGB8888 frame, corrects byte-channel order in the shader, flips libretro's top-down image, and applies RV-0414 intended display aspect;
- Android AudioTrack consumes stereo 16-bit PCM on the DOS worker thread;
- Back now opens a shell-owned DOS quick overlay rather than terminating the guest immediately;
- opening the overlay pauses DOS execution/audio, releases current guest input and suppresses new controller/head/HID guest signals until Resume;
- overlay pause and Activity lifecycle pause are tracked independently, preventing Activity resume from silently resuming a guest behind the menu;
- the paused guest framebuffer remains visible behind a translucent shell panel;
- quick-overlay actions provide Resume, Recenter, media-volume down/up, Home and Exit VR;
- the active hosted binding-profile name is shown in the overlay while the shell retains control of recovery;
- the quick overlay now opens a dedicated paused DOS binding page;
- profile Previous/Next cycles DOS-capable built-ins, immediately swaps the hosted profile, persists the module's selected profile ID and clears any stale custom override;
- analog sensitivity and deadzone can be tuned in-headset while paused, with edits saved as a bounded module-local RVBIND1 override;
- Reset Tuning deletes the module-local override and restores the selected built-in profile;
- hosted-profile replacement never overwrites the user's pre-game global profile, which still restores on DOS exit;
- deterministic JVM coverage exercises signed analog sensitivity changes, deadzone bounds and digital-only no-op behavior.

Current-environment verification:

- phone-test run #8 exposed two stale AudioTrack references to the removed single `paused` flag and failed Java compilation as intended;
- commit `7133b3b40bbf6853b7fb2d5e4ba7eb1970bc26cd` replaced both stale references with the pause gate and source readback confirmed no `paused` variable references remain;
- phone-test run #9 passed JVM tests, `:app:assembleDebug`, `:controller-app:assembleDebug`, dual-ABI native-library APK presence checks and APK signature verification;
- run #9 published prerelease `phone-test-9-1` from commit `ba362cffed915f179c8a794803d05993239d6db6`;
- phone-test run #10 passed the complete JVM/Android/NDK build and verification path with the binding editor present;
- run #10 published prerelease `phone-test-10-1` from commit `72f2ab623e0993d9531a8b6aa7e333334ae96de0`.

Remaining gates:

- reference-device validation of DOS run cadence, audio latency/underruns, guest texture orientation/colors and resolution changes;
- full arbitrary per-binding remapping beyond the current profile selection plus analog sensitivity/deadzone tuning;
- richer library management such as sorting/filtering/favorites beyond the current paged launcher;
- integer-scale staging policy beyond nearest-neighbor guest sampling;
- directory-tree import;
- run the verified DOOM Shareware installer/autostart path and exercise E1M1 on the Galaxy S9.

### RV-0007 — Standard/development logging and manual diagnostic bundles

State: **draft**

Implemented:

- Standard mode defaults to milestone/incident/warning/error/fatal records;
- Development mode adds raw controller/gamepad/HID input, normalized inputs, binding transforms/outputs, head motion and per-frame VR diagnostic state;
- bounded 8192-record in-memory queue;
- rotating app-private files: Standard 1 MiB + 4 archives, Development 16 MiB + 4 archives;
- uncaught exceptions are synchronously drained to local logs before delegating to Android's existing crash handler;
- Stage A logging-mode selector;
- explicit Clear Local Logs action;
- explicit Export Diagnostic Bundle action using Android's create-document UI;
- ZIP manifest includes app/build/device/API/ABI/logging-mode/DOS-runtime-build state;
- manual export remains a permanent offline/private fallback independent of network submission;
- privacy warning that Development logs can include device names, local filenames, module names, input state and timing.

Development logging invalidates performance/thermal/frame-pacing acceptance evidence because it intentionally performs high-volume diagnostic work.

Automatic opt-in issue submission is tracked separately as RV-0208 so the manual logging/export acceptance boundary stays independent.

### RV-0208 — secure one-tap diagnostic issue submission

State: **draft**

Implemented on `main`:

- Stage A exposes **Submit diagnostics to GitHub** only when a valid HTTPS diagnostic-intake endpoint was injected at build time; absent configuration hides the control rather than leaving a dead button;
- submission requires an explicit privacy/reproduction dialog, a non-empty failure summary, optional expected behavior, and an explicit coarse-redaction choice to include or exclude ReverieVR log files;
- ReverieVR builds the exact ZIP first, calculates its `revdiag-<UUID>`, byte length and SHA-256, then shows a second review screen containing the public issue text/device/build metadata and whether logs are included before any network upload occurs;
- the Android application contains no GitHub token, GitHub App private key, storage credential, Cloudflare credential, or reusable privileged upload secret;
- failed uploads retain the generated ZIP in the app-private diagnostic outbox while the existing manual Export Diagnostic Bundle path remains available;
- if private storage succeeds but GitHub issue creation fails, the broker writes a pending receipt and `POST /v1/diagnostics/finalize` can retry issue creation using the diagnostic ID + exact SHA-256 without retransmitting the ZIP; Stage A persists that pending receipt identity across app restarts and offers retry before starting a new report;
- the first broker implementation is a Cloudflare Worker under `diagnostic-intake/` with a private R2 bundle store, SQLite-backed Durable Object rate limiter, bounded request/ZIP validation, canonical local-header/central-directory filename checks, server-side SHA-256 recomputation, scheduled 30-day raw-bundle/receipt expiry, and GitHub App installation-token issue creation;
- public GitHub issues contain sanitized build/device/reproduction metadata, diagnostic ID, byte length, server hash, and private receipt reference only; raw ZIP/object keys and pending metadata are not automatically published;
- broker deployment is isolated behind the protected GitHub environment `diagnostic-intake`, while the non-secret public endpoint is supplied separately through the `DIAGNOSTIC_INTAKE_URL` Actions variable;
- GitHub Issue #1 tracks the end-to-end acceptance work and ADR-0017 remains the governing security boundary.

Current verification: phone-test run #19 completed successfully against commit `5a35615691fbdce4f9a73a5fdbf97df2f66de91d`. It passed Worker syntax and broker smoke validation, JVM tests (including diagnostic ID/SHA-256 and stored-receipt retry state), Android/native/DOS assembly, APK-content verification, signature verification, release staging and prerelease publication. The published tag is `phone-test-19-1`. Earlier runs #15-#18 were superseded/cancelled under the new latest-build-wins concurrency policy rather than treated as acceptance evidence.

Remaining gates before RV-0208 can be accepted:

1. provision the `reverievr-diagnostics` R2 bucket and least-privilege GitHub App;
2. populate the protected `diagnostic-intake` environment secrets and deploy the Worker;
3. set `DIAGNOSTIC_INTAKE_URL` to the deployed HTTPS `/v1/diagnostics` endpoint and produce a phone-test build containing that public URL;
4. perform a real Galaxy S9 submission and verify the client hash, server hash, private receipt and created GitHub issue agree;
5. verify failure/retry behavior and that no privileged credential appears in the APK or public issue.

### RV-0414 — retro framebuffer display geometry

State: **draft**

Implemented:

- recognized source modes: 320x200, 320x240, 640x350, 640x400, 640x480, 720x400, 800x600, 1024x768, 1280x1024, 160x144 and 240x160;
- raw source aspect and intended physical display aspect are represented separately;
- Mode 13h/640x400/text modes can be presented as intended 4:3 instead of blindly stretching according to raw pixel dimensions;
- 1280x1024 remains 5:4;
- unknown modes fall back to native source aspect;
- integer-scale calculation exists for pixel-stable intermediate presentation;
- VR shell/HUD resolution remains independent from low-resolution guest content;
- Cardboard remains authoritative for actual per-eye viewport/projection/distortion rather than hard-coding generic 8:9/5:4 eye buffers or a universal supersampling factor.

Remaining gate: connect this geometry to the DOS framebuffer texture renderer and validate pixel/aspect/readability behavior in the Daydream View.

### RV-0415 — bundled DOOM Shareware

State: **draft**

Default-content baseline:

- DOOM Shareware v1.9;
- authoritative distributed payload: original `doom19s.zip`;
- expected byte size: 2,450,688;
- SHA-1 `8d0fbbbeba5ecb692a99f97e55dfb5365cfe5b77`;
- SHA-256 `cacf0142b31ca1af00796b4a0339e07992ac5f21bc3f81e7532fe1b5e1b486e6`;
- fetch helper uses an idgames archival mirror and refuses an unexpected digest;
- fetched binary is ignored by ReverieVR git;
- free release builds include it by default when fetched;
- release builds refuse an accidentally missing archive unless `-PexcludeDoomShareware` is explicitly supplied;
- Android packages the original archive unchanged as `assets/default-content/doom19s.zip`;
- Stage A startup re-verifies the archive and registers the built-in `DOOM Shareware v1.9` DOS module using the `dos-doom-shareware` binding profile;
- the original archive remains under the module's `content/` directory as the checksum-verifiable authoritative payload;
- a separate `runtime/` directory is regenerated from the verified ZIP with bounded extraction and canonical-path traversal rejection;
- the runtime adds generated `DOS.YML`, `DOSBOX.BAT` and `REVERIE.CFG` files without modifying/repacking the distributed archive;
- first launch uses DOSBox Pure `run_path` / `run_input` metadata to run DEICE with the C-drive/default-directory/confirmation sequence, then `DOOMS_19.EXE -d`, then `DOOM.EXE`;
- once `DOOMS/DOOM.EXE` exists, launch preparation deletes the bootstrap batch and rewrites `DOS.YML` to start Doom directly with no installer keystrokes;
- the generated first-run config enables mouse and SB16 SFX at 0x220 / IRQ 7 / DMA 1 while leaving music disabled until separate music validation;
- directory content is now an accepted DOS module content root so DOSBox Pure can mount the generated working tree directly;
- pure-Java tests cover ZIP extraction bounds/path traversal and the first-install/direct-play metadata plans;
- registered/full DOOM data is not bundled;
- the engine GPL and shareware-data license remain separate licensing domains.

Current-environment verification:

- phone-test run #11 passed the complete JVM/Android/NDK build and verification pipeline with the Doom bootstrap present;
- the build fetched the checksum-pinned `doom19s.zip` input before assembly and produced the signed headset/controller APK pair;
- run #11 published prerelease `phone-test-11-1` from commit `a6c06d3b4480b52be6cb6307123f38c4e5f1697d`;
- the published headset APK is `ReverieVR-phone-test-11-a6c06d3.apk` with release-reported SHA-256 `b43a8ea9047f6d76cba70bab2228633a62232e427a73e6585b48e4100d8e2b23`.

Distribution rule:

DOOM shareware is copyrighted shareware, not public domain. The default bundled-content path is for free/noncommercial ReverieVR releases. A paid/commercial distribution must obtain appropriate permission or deliberately omit the shareware archive.

Remaining gates:

- install phone-test #11 on the Galaxy S9;
- validate the DEICE prompt timing and generated runtime installation on the Galaxy S9;
- validate Sound Blaster SFX and decide the later music path without reintroducing SETUP.EXE as a mandatory first-run gate;
- verify E1M1 launch, saves, 320x200-to-4:3 presentation, input bindings, overlay/recovery and sustained thermal behavior.

### RV-0005 — Stage B frame cadence instrumentation

State: **draft**

Implemented:

- allocation-bounded rolling window of the latest 600 Cardboard frame intervals;
- lifecycle/background gaps over one second are excluded rather than misreported as catastrophic frames;
- one-minute Standard-log summaries report average, p95 and maximum frame interval plus counts above 1.5x and 2x the 60 Hz target interval;
- instrumentation is driven from Stage B `onNewFrame`, so shell, media, DOS and native-module modes use the same cadence evidence path;
- deterministic JVM coverage verifies cadence math, slow/severe classification, lifecycle-gap rejection and bounded storage;
- Stage B now keeps the latest battery-temperature broadcast reading and Android 10+ PowerManager thermal-status state outside the render loop;
- the once-per-minute `VR_PERFORMANCE` record correlates those thermal observations with the same cadence snapshot;
- Android versions without thermal-status support and failed/unavailable sensor reads are logged explicitly as `unavailable`, never fabricated;
- pure-Java coverage verifies battery-temperature formatting, thermal-status labels and unknown/unavailable behavior.

Interpretation boundary: these measurements are headset callback cadence, not GPU timer-query results. Battery temperature is the handset battery sensor reported by Android and must not be described as direct SoC, CPU or GPU junction temperature. Android thermal status is a platform throttling/severity signal, not a temperature reading. They are suitable for sustained regression evidence but must not be described as direct GPU render time. Development logging remains disallowed for acceptance evidence because its intentionally heavy diagnostics perturb timing.

Current-environment verification: phone-test run #7 passed JVM tests, `:app:assembleDebug`, `:controller-app:assembleDebug`, native-library APK presence checks and APK signature verification with the thermal-correlation instrumentation present. The pure-Java formatter was independently compiled from the committed blob and produced the expected available, unavailable and unknown-status forms.

Remaining gate: sustained Galaxy S9 headset runs without scrcpy to establish measured acceptance thresholds and observe whether cadence degradation correlates with battery temperature and Android thermal-status escalation.

## Performance posture

The Stage B shell deliberately avoids a general-purpose game engine. Cardboard is configured for OpenGL ES with Vulkan and Unity integration disabled. The shell avoids per-frame gaze/button-geometry allocation and reuses its UI bitmap/texture storage instead of allocating a new 1024×768 bitmap for every hover change.

No performance/thermal acceptance claim exists until sustained Galaxy S9 testing is performed.

## Quality-of-life baseline

The project backlog includes remembered global comfort preferences, universal recenter/seated recovery, scalable/high-contrast UI, captions/visual audio alternatives, flexible one-controller/remappable input, reduced-motion behavior, controller calibration/reconnect handling, quick access, nonblocking notifications, optional session reminders, and research-only camera peek.

Automatic convenience behaviors remain user-toggleable unless required for recoverability.

## Build environment and phone-test release

The interactive execution environment still does not expose a local Android SDK/NDK/ADB stack. Explicitly authorized GitHub Actions now supplies the reproducible cloud Android build environment instead.

Phone-test run #11 completed successfully from commit `a6c06d3b4480b52be6cb6307123f38c4e5f1697d` and published prerelease `phone-test-11-1`. The headset APK is a debug-signed sideload build for reference-device validation, not a production signing identity. Future phone-test builds may require uninstall/reinstall until persistent protected test/release signing is configured.

No Galaxy S9, headset, optical, Bluetooth, thermal, or comfort validation has yet been claimed.

## Next exact action

1. install the headset APK from GitHub prerelease `phone-test-11-1` on the Galaxy S9;
2. launch Stage A and confirm ordinary Android startup before entering the headset;
3. confirm **DOOM Shareware v1.9** appears in the DOS library, launch it once, and observe the automated DEICE/bootstrap path through to `DOOM.EXE`;
4. exit and relaunch DOOM to verify the second launch bypasses installer automation and starts the installed game directly;
5. verify Doom mouse/head-turn, movement/fire bindings, Sound Blaster SFX, overlay pause/recovery and E1M1 presentation;
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
