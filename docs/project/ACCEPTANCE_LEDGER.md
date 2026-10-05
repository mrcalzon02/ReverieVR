# ReverieVR Acceptance Ledger

**Role:** authoritative admission record

Acceptance states:

- **planned** — intent exists only;
- **draft** — implementation/document exists but has not passed deterministic checks;
- **static accepted** — source/document/build checks pass, but device/runtime validation is not required or is still pending;
- **device accepted** — relevant reference-device validation passed;
- **rejected** — failed a governing gate;
- **deferred** — validation intentionally postponed with reason.

| Target | Description | State | Evidence / remaining gate |
|---|---|---|---|
| RV-0001 | Project governance/bootstrap | **static accepted** | Governance baseline committed and independently read back from remote `main`. |
| RV-0002 | Framework/runtime/controller-stack selection | **draft** | Cardboard v1.35.0 is pinned for stereo/head tracking/distortion with Vulkan/Unity disabled; direct Android BLE is implemented for Daydream. Build and S9 validation remain before acceptance. |
| RV-0003 | Android APK skeleton | **draft** | Android/Gradle/Java source is on remote `main`; build not run because this environment lacks Gradle/Android SDK. |
| RV-0006 | scrcpy/ADB diagnostic compatibility | **draft** | Compatibility contract, Windows/POSIX launch helpers, Stage A/Stage B validation procedure, Android 10 audio caveat, and performance-test exclusion are recorded. Galaxy S9 USB-debug/mirroring validation remains required. |
| RV-0090 | 2D touchscreen boot/setup surface | **draft** | UI source, real phone battery reporting, persisted QoL toggles, recovery reset, and honest readiness gates are on remote `main`; APK/device launch still required. |
| RV-0091 | Controller pairing/readiness workflow | **draft** | Physical Daydream BLE remains the reference backend; a second paired Android phone can now connect through the historical Daydream controller-emulator RFCOMM framing, and attached Android gamepads are recognized as usable Stage-B input. Battery telemetry remains honest/unknown where the source does not provide it. Physical device and emulator-phone validation remain required. |
| RV-0094 | ReverieVR Controller companion APK | **draft** | A separate Android app module now implements bonded RFCOMM hosting, rotation-vector/gyro/accelerometer transmission, touchpad + Select/App/Home controls, volume forwarding, prioritized control delivery, reconnect listening, and real companion-phone battery telemetry. Build and two-phone hardware validation remain required. |
| RV-0092 | 2D-to-VR transition/recovery | **draft** | Stage A now enables Enter VR when either the active dedicated controller provider is ready or Android reports an attached gamepad/joystick, then launches the Cardboard Stage B activity. Build/device validation remains. |
| RV-0102 | Normalized controller/input action layer | **draft** | `VrInputRouter` maps physical Daydream snapshots, phone-emulator snapshots, Android gamepad keys/axes, Cardboard trigger, and Cardboard system/back control into named actions. Hardware mapping/polarity and broader remapping settings still require reference-device validation. |
| RV-0093 | GitHub Release updater | **draft** | Source checks only the authoritative repository, supports optional launch checks, user Update/Not now choice, release notes, APK download, optional SHA-256 verification, and Android installer handoff. Build/device verification still required. |
| RV-0100 | VR home/menu | **draft** | Cardboard Stage B renders a world-space textured shell with head-gaze targeting, controller-click activation, first-run setup pages, Home recenter request, Menu/back, battery HUD and Exit to Stage A. Build/device validation remains. |
| RV-0106 | Shell-global handset/controller power HUD | planned | Stage A already stores the user preference; Stage B must keep real handset/controller battery status available across home/player/modules and represent unavailable controller telemetry honestly. |
| RV-0107 | Optional gaze-adaptive status HUD/QoL toggles | planned | Stage A already stores the look-up-reveal preference; Stage B must implement user-toggleable adaptive reveal/retract and preserve a predictable persistent/manual fallback when disabled. |
| RV-0108 | First-VR-run onboarding/calibration wizard | **draft** | A versioned/resumable Stage B setup flow now implements recenter, user eye spacing, UI scale, HUD preferences and completion state. Controller/viewer/comfort/audio/performance pages and device validation remain. |
| RV-0206 | User optical/IPD calibration | **draft** | Physical viewer geometry remains separate; persistent 50–80 mm user eye spacing is applied as a per-eye view correction with live in-headset adjustment. Per-eye fine alignment, viewer-profile confirmation and physical comfort validation remain. |
| RV-0210..0219 | Comfort/accessibility/QoL baseline | planned | Research record identifies system-level preferences, recenter/recovery, readable UI, captions, flexible input, controller calibration, quick access, nonblocking notifications, optional breaks, and camera-peek feasibility. Each feature retains an opt-out where automation is nonessential. |
| RV-0300 | Retro performance fixture | planned | Requires on-device sustained performance evidence. |
| RV-0500 | Local flat VR media player | **draft** | Stage A document selection/persisted URI, MediaPlayer decode, external-OES SurfaceTexture, flat cinema geometry, play/pause and shell return are implemented on remote `main`. Build, codec, sustained playback, and Galaxy S9 headset validation remain. |
| RV-0501 | Mono equirectangular 360° projection | **draft** | A head-tracked inward sphere consumes the same local OES video texture. Seam/orientation/mirroring and sustained reference-device validation remain; 180° and stereo layouts are not claimed. |
| RV-0503 | Basic media selection/control path | **draft** | Specific-file picker, remembered projection, click play/pause, horizontal touchpad-swipe ±10-second seeking, and Menu/back shell return exist. Seek polarity still requires hardware validation; library UI, scrubber/richer transport controls and subtitles remain planned. |
