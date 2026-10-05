# Changelog

All entries describe verified project/repository changes. Planned work belongs in the backlog, not here.

## Unreleased

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
- Removed the initial placeholder `test` file.
