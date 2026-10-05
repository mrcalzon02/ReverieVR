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
- no automatic network submission;
- privacy warning that Development logs can include device names, local filenames, module names, input state and timing.

Development logging invalidates performance/thermal/frame-pacing acceptance evidence because it intentionally performs high-volume diagnostic work.

Future automatic GitHub/error submission remains deferred and must be opt-in with payload review/redaction and no embedded personal token.

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
- registered/full DOOM data is not bundled;
- the engine GPL and shareware-data license remain separate licensing domains.

Distribution rule:

DOOM shareware is copyrighted shareware, not public domain. The default bundled-content path is for free/noncommercial ReverieVR releases. A paid/commercial distribution must obtain appropriate permission or deliberately omit the shareware archive.

Remaining gates:

- actually fetch and package the verified archive in a release build;
- build/install on the Galaxy S9;
- finish DOSBox Pure host;
- implement local first-run installer/autostart behavior so the original archive can reach a ready-to-play shareware installation without redistributing a modified payload;
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

Phone-test run #10 completed successfully from commit `72f2ab623e0993d9531a8b6aa7e333334ae96de0` and published prerelease `phone-test-10-1`. The headset APK is a debug-signed sideload build for reference-device validation, not a production signing identity. Future phone-test builds may require uninstall/reinstall until persistent protected test/release signing is configured.

No Galaxy S9, headset, optical, Bluetooth, thermal, or comfort validation has yet been claimed.

## Next exact action

1. install the headset APK from GitHub prerelease `phone-test-10-1` on the Galaxy S9;
2. launch Stage A and confirm ordinary Android startup before entering the headset;
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
