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
| RV-0090 | 2D touchscreen boot/setup surface | **draft** | UI source, real phone battery reporting, persisted QoL toggles, recovery reset, and honest readiness gates are on remote `main`; APK/device launch still required. |
| RV-0091 | Controller pairing/readiness workflow | **draft** | Direct Android BLE source now scans FE55, requests bonding, connects GATT, subscribes to pose/battery notifications, decodes the 20-byte controller packet, reads battery/voltage, exposes recenter, and feeds a live Stage A test panel. Physical Daydream controller validation remains required. |
| RV-0092 | 2D-to-VR transition/recovery | **draft** | Stage A now enables Enter VR only when the app-scoped controller manager reports ready and launches a landscape Cardboard Stage B activity. Build/device validation remains. |
| RV-0093 | GitHub Release updater | **draft** | Source checks only the authoritative repository, supports optional launch checks, user Update/Not now choice, release notes, APK download, optional SHA-256 verification, and Android installer handoff. Build/device verification still required. |
| RV-0100 | VR home/menu | **draft** | Cardboard Stage B renders a world-space textured shell with head-gaze targeting, controller-click activation, first-run setup pages, Home recenter request, Menu/back, battery HUD and Exit to Stage A. Build/device validation remains. |
| RV-0106 | Shell-global handset/controller power HUD | planned | Stage A already stores the user preference; Stage B must keep real handset/controller battery status available across home/player/modules and represent unavailable controller telemetry honestly. |
| RV-0107 | Optional gaze-adaptive status HUD/QoL toggles | planned | Stage A already stores the look-up-reveal preference; Stage B must implement user-toggleable adaptive reveal/retract and preserve a predictable persistent/manual fallback when disabled. |
| RV-0108 | First-VR-run onboarding/calibration wizard | planned | Must be versioned, resumable, skippable with safe defaults, teach controls while configuring the user profile, and support complete or per-tool re-entry from Settings. |
| RV-0206 | User optical/IPD calibration | planned | Must preserve the physical viewer profile, expose persistent user IPD/alignment correction with live in-headset calibration and reset, and pass reference-device optical/comfort validation. |
| RV-0210..0219 | Comfort/accessibility/QoL baseline | planned | Research record identifies system-level preferences, recenter/recovery, readable UI, captions, flexible input, controller calibration, quick access, nonblocking notifications, optional breaks, and camera-peek feasibility. Each feature retains an opt-out where automation is nonessential. |
| RV-0300 | Retro performance fixture | planned | Requires on-device sustained performance evidence. |
| RV-0500 | Local VR media player | planned | Requires on-device playback and thermal validation. |
