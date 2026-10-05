# Changelog

All entries describe verified project/repository changes. Planned work belongs in the backlog, not here.

## Unreleased

### VR runtime and controller integration

- Added an independent Android BLE backend for the Daydream controller: discovery, bonding, GATT connection, pose/input packet decoding, battery/voltage telemetry, recenter command, and a live Stage A input test.
- Added a generic controller-provider/manager boundary so later controller types do not require rewriting the VR shell.
- Pinned Google Cardboard SDK v1.35.0 at commit `5969239e7c87f4cd64c8ec170ce1e7f4eb559e37`.
- Disabled Cardboard Vulkan and Unity-plugin native paths; ReverieVR uses the OpenGL ES path.
- Added an application-scoped controller lifetime across Stage A and Stage B.
- Added the first Cardboard Stage B activity and world-space VR shell with head-gaze selection, Daydream click activation, back/recenter handling, power status, and resumable first-run setup pages.
- Added functional initial setup controls for neutral forward direction, virtual user eye spacing, UI scale, and HUD preferences.
- Added pure-Java unit tests for the Daydream packet decoder and updater version comparison.


### Android bootstrap

- Added a native Java/XML Android application skeleton for the Stage A pre-headset setup surface.
- Added a conventional touchscreen setup menu with device identification and real phone-battery reporting.
- Added persistent quality-of-life toggles for the future VR battery HUD, look-up reveal behavior, numeric battery percentages, and retro performance-first mode.
- Added visible controller Pair / Sync and Test actions; they remain disabled until the controller stack is genuinely implemented.
- Added an Android Bluetooth-settings shortcut without claiming generic Bluetooth pairing is sufficient for the Daydream controller.
- Added local settings reset/recovery behavior.
- Added an Enter VR control that remains disabled until the Stage B runtime exists.
- Added build instructions and ADR-0003 establishing a lightweight native Android Stage A boundary.
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
