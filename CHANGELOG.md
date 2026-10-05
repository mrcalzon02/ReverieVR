# Changelog

All entries describe verified project/repository changes. Planned work belongs in the backlog, not here.

## Unreleased

### Bundled DOOM Shareware sample

- Added a checksum-pinned build path for the original `doom19s.zip` DOOM Shareware v1.9 archive.
- Added POSIX and Windows fetch helpers using the known SHA-256 `cacf0142b31ca1af00796b4a0339e07992ac5f21bc3f81e7532fe1b5e1b486e6`.
- Kept the third-party binary outside ReverieVR git while allowing it to be packaged unchanged as a nested Android asset.
- Made free release packaging expect the shareware archive by default, with an explicit `-PexcludeDoomShareware` escape hatch.
- Added runtime SHA-256 re-verification and automatic registration of **DOOM Shareware v1.9** with the built-in Doom binding profile.
- Recorded the legal boundary: shareware is copyrighted and not GPL/public-domain; paid/commercial distributions require appropriate permission or omission of the payload.
- Left first-run DOS installer/autostart automation as the remaining runtime step rather than repacking the distributed archive.



### Diagnostics and retro framebuffer geometry

- Added Standard and Development logging modes.
- Added bounded rotating app-private log files with Standard incident/milestone history and intentionally high-volume Development diagnostics.
- Added uncaught fatal-exception capture while preserving Android's existing crash handler.
- Added Stage A logging-mode selection, clear-log control, and manual diagnostic ZIP export through Android's document UI.
- Kept all diagnostic submission manual; no automatic GitHub/network reporting is implemented.
- Added a retro display-mode catalog that separates source pixel dimensions from intended physical display aspect.
- Added explicit Mode 13h/VGA/EGA/text/handheld source modes plus integer-scale calculations.
- Kept Cardboard authoritative for VR eye viewport/projection/distortion rather than hard-coding generic per-eye resolutions or a universal lens supersampling multiplier.
- Declared Development-logging runs invalid for formal performance/thermal/frame-pacing acceptance.



### DOS modules, bindings and standard HID

- Added a stateful virtual input binding engine that maps Daydream/controller/gamepad/head signals into virtual keyboard, mouse and joystick outputs.
- Added press/release lifetime, analog/threshold transforms, deadzones, sensitivity scaling and persistent versioned binding profiles.
- Added built-in starter profiles for Doom-style DOS control, generic head-mouse FPS control and touchpad absolute-cursor control.
- Added direct Bluetooth/USB/Android HID keyboard passthrough and mouse buttons, relative/absolute motion and wheel input.
- Added mouse delivery through both generic-motion and mouse-sourced pointer dispatch to cover Android HID behavior.
- Added a generic Stage A DOS module importer for DOSBox Pure-supported content, copied into app-private storage with stable full paths.
- Added persistent DOS module metadata/listing and library clearing without deleting the user's original source files.
- Accepted GPLv2/GPLv2+ for the embedded DOS runtime and pinned DOSBox Pure `1.0-preview6` / `a4a0bab7f8931433588f2fcad9045c85b277373d`.
- Added verified DOSBox Pure fetch scripts and recorded corresponding-source/release obligations.
- Added conditional Android NDK integration for the pinned DOSBox Pure checkout; shell/media-only builds still work when the checkout is absent, while DOS-enabled builds now emit the upstream `libretro.so` core plus ReverieVR's `libreverie_dos_host.so`.
- Added the JNI/libretro DOS frontend with real core init/load/run/unload/deinit lifecycle, app-private system/save/content directories, legacy core-option defaults, shutdown handling and Android log forwarding.
- Added tightly packed XRGB8888 framebuffer capture, bounded stereo PCM buffering, and synchronized virtual keyboard/mouse/joystick polling behind the existing `DosNativeRuntime` API.
- Forced the initial Voodoo path to DOSBox Pure's software multithreaded renderer until ReverieVR implements a libretro hardware-render callback.
- Changed `DOS_RUNTIME_BUILT` from a hard-coded false value to the actual presence of the verified pinned source checkout at Gradle configuration time, and extended bootstrap verification to reject a fetched checkout at the wrong commit.
- Host-side C++ syntax/type checking and JNI descriptor verification passed locally; Android NDK compile/link, Stage B framebuffer/audio consumption, and Galaxy S9 runtime validation remain required.
- Left the in-game binding overlay, directory-tree import, VR Home DOS launch surface, Android audio sink and guest-texture presentation as the next DOS-host integration gates.



### Global VR power HUD

- Added a separate persistent stereo PHONE/CTRL battery HUD with two real progress bars.
- Kept the HUD visible across the VR shell, setup flow and local video playback when enabled.
- Added honest unknown-controller presentation instead of treating unavailable telemetry as 0%.
- Wired the existing Show Percentages preference to numeric HUD values while preserving the bars.
- Implemented the optional player-look-up reveal: compact upper-right during normal viewing, lower in view when looking steeply upward.
- Kept Battery HUD and Look-up Reveal as user-toggleable QoL options.
- Reused HUD texture/geometry storage without per-eye allocation.
- Left controller-pointing-up reveal pending physical controller-axis validation rather than guessing the quaternion orientation.



### Setup and input familiarization

- Bumped the resumable first-run setup schema to v2.
- Added an in-VR controller familiarization page showing the active input source and most recent normalized action.
- Captured Back locally on the familiarization page so it can be tested safely without leaving the page.
- Preserved the Optical / Display Calibration Home shortcut so it still jumps directly to IPD.



### ReverieVR Controller companion

- Added a second lightweight Android application module, `controller-app`, with application ID `io.github.mrcalzon02.reverievr.controller`.
- Added a spare-phone controller UI with a large touchpad, Select, App/Back, Home/Recenter, and physical volume-button forwarding.
- Added rotation-vector orientation, gyroscope, and accelerometer transmission over the historical Daydream controller-emulator RFCOMM framing.
- Added a bonded-client-only RFCOMM server using UUID `ab001ac1-d740-4abb-a8e6-1cb5a49628fa`.
- Added separate prioritized control and coalesced sensor queues so high-rate pose traffic cannot strand button-up events.
- Added a backward-compatible ReverieVR status extension carrying the companion phone battery percentage; historical emulators that omit it continue to report battery as unknown.
- Added pure-Java companion protocol-writer tests and headset-side parser coverage for the battery extension.
- Hardened the headset updater so it cannot accidentally select a controller companion APK from a multi-APK GitHub Release.
- The companion app remains draft pending build and two-phone hardware validation.



### Local media

- Added Android Storage Access Framework local-video selection with persisted per-document read access instead of broad storage permission.
- Added explicit flat-cinema and mono equirectangular-360 projection choices.
- Added a platform MediaPlayer decoder backed by an OpenGL ES external OES SurfaceTexture.
- Added shell-owned video entry, Daydream click play/pause, horizontal touchpad-swipe ±10-second seeking, Menu/back return to VR home, and decoder reattachment across GL-surface recreation.
- Added an inward-viewed equirectangular sphere renderer for mono 360 video.
- Kept stereoscopic layouts, seeking/library UI, subtitle work, and sustained thermal acceptance as later media work.

### VR runtime and controller integration

- Added an independent Android BLE backend for the Daydream controller: discovery, bonding, GATT connection, pose/input packet decoding, battery/voltage telemetry, recenter command, and a live Stage A input test.
- Added a multi-provider controller manager: physical Daydream BLE and historical second-phone Daydream controller-emulator RFCOMM are separate transports.
- Added paired-phone controller selection using the historical RFCOMM UUID and an independently authored minimal protobuf-wire decoder for touch, orientation, sensor, and key events.
- Added live Android gamepad/joystick detection as an alternate Stage-B readiness source.
- Added a normalized `VrInputAction` router so physical Daydream, phone emulator, generic gamepad, and Cardboard trigger/system input share Select/Back/Recenter/navigation/volume semantics rather than leaking raw buttons into the VR activity.
- Added pure-Java phone-controller protocol parser fixtures.
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
- Added visible physical-controller Pair / Sync, paired-phone controller, and Test actions with honest readiness gating.
- Added an Android Bluetooth-settings shortcut without claiming generic Bluetooth pairing is sufficient for the Daydream controller.
- Added local settings reset/recovery behavior.
- Added an Enter VR control gated on a usable dedicated controller source or Android gamepad/joystick.
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
