# Changelog

All entries describe verified project/repository changes. Planned work belongs in the backlog, not here.

## Unreleased

### Local media

- Added Android Storage Access Framework local-video selection with persisted per-document read access instead of broad storage permission.
- Added explicit flat-cinema and mono equirectangular-360 projection choices.
- Added a platform MediaPlayer decoder backed by an OpenGL ES external OES SurfaceTexture.
- Added shell-owned video entry, Daydream click play/pause, horizontal touchpad-swipe ±10-second seeking, Menu/back return to VR home, and decoder reattachment across GL-surface recreation.
- Added an inward-viewed equirectangular sphere renderer for mono 360 video.
- Kept stereoscopic layouts, seeking/library UI, subtitle work, and sustained thermal acceptance as later media work.

### VR runtime and controller integration

- Added an independent Android BLE backend for the Daydream controller: discovery, bonding, GATT connection, pose/input packet decoding, battery/voltage telemetry, recenter command, and a live Stage A input test.
- Added a generic controller-provider/manager boundary so later controller types do not require rewriting the VR shell.
- Pinned Google Cardboard SDK v1.35.0 at commit `5969239e7c87f4cd64c8ec170ce1e7f4eb559e37`.
- Disabled Cardboard Vulkan and Unity-plugin native paths; ReverieVR uses the OpenGL ES path.
- Added an application-scoped controller lifetime across Stage A and Stage B.
- Added the first Cardboard Stage B activity and world-space VR shell with head-gaze selection, Daydream click activation, back/recenter handling, power status, and resumable first-run setup pages.
- Added functional initial setup controls for neutral forward direction, virtual user eye spacing, UI scale, and HUD preferences.
- Added pure-Java unit tests for the Daydream packet decoder and updater version comparison.
- Hardened controller readiness so READY is not emitted until a valid pose packet is actually received.
- Hardened failed BLE setup/permission paths to close dead GATT sessions rather than leave zombie connections.
- Removed avoidable per-frame VR-shell allocations and switched shell UI texture refreshes to persistent bitmap storage plus texture sub-image updates.


### Developer diagnostics

- Added first-class scrcpy/ADB compatibility requirements for Galaxy S9 development.
- Added Windows and POSIX scrcpy launch helpers using stay-awake and Android-10-safe no-audio defaults.
- Defined Stage A remote-control and Stage B mirror/record validation while Daydream BLE remains active.
- Prohibited treating scrcpy-assisted sessions as performance/thermal acceptance evidence.

### Android bootstrap

- Added a native Java/XML Android application skeleton for the Stage A pre-headset setup surface.
- Added a conventional touchscreen setup menu with device identification and real phone-battery reporting.
- Added persistent quality-of-life toggles for the future VR battery HUD, look-up reveal behavior, numeric battery percentages, and retro performance-first mode.
- Added visible controller Pair / Sync and Test actions; they remain disabled until the controller stack is genuinely implemented.
- Added an Android Bluetooth-settings shortcut without claiming generic Bluetooth pairing is sufficient for the Daydream controller.
- Added local settings reset/recovery behavior.
- Added an Enter VR control that remains disabled until the Stage B runtime exists.
- Added build instructions and ADR-0003 establishing a lightweight native Android Stage A boundary.
- Added the verified Gradle 9.6.1 wrapper and pinned both wrapper-JAR and distribution SHA-256 values.
- No APK build or reference-device validation has yet been claimed.

### Project foundation

- Adopted AI Project Manager-compatible project governance and repository execution rules.
- Established the ordered ReverieVR APK construction plan.
- Defined a two-stage boot model: 2D touchscreen setup/controller readiness followed by explicit entry into the 3D VR home.
- Required a recovery path from VR back to the 2D setup surface for pairing, input, display, and startup failures.
- Defined the Galaxy S9 + Daydream View seated head-look/controller interaction baseline.
- Adopted performance-first, retro-first VR rendering doctrine and sustained-device performance as an acceptance concern.
- Established backlog, execution-state, acceptance-ledger, and architecture-decision record structure.
- Formalized the persistent Stage B platform-shell contract so home, media player, and hosted games/modules share one global status/settings/recovery layer.
- Defined the future VR power HUD as two shell-global percentage/progress indicators for handset and bound-controller battery, with honest unavailable state when controller telemetry cannot be read.
- Defined the reference HUD position as the upper-right region and recorded optional look-up-triggered reveal/retract behavior as a user-toggleable quality-of-life feature with persistent/manual fallback.
- Removed the initial placeholder `test` file.
