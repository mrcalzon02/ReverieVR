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
| RV-0002 | Framework/runtime/controller-stack selection | planned | Requires research, license/provenance review, controller pairing path, renderer selection, and recorded architecture decision. |
| RV-0003 | Android APK skeleton | **draft** | Android/Gradle/Java source is on remote `main`; build not run because this environment lacks Gradle/Android SDK. |
| RV-0090 | 2D touchscreen boot/setup surface | **draft** | UI source, real phone battery reporting, persisted QoL toggles, recovery reset, and honest readiness gates are on remote `main`; APK/device launch still required. |
| RV-0091 | Controller pairing/readiness workflow | planned | Pair / Sync and Test actions exist in the UI but remain disabled until the verified Daydream controller stack is implemented. |
| RV-0092 | 2D-to-VR transition/recovery | planned | Enter VR exists but is disabled until Stage B is implemented and verified. |
| RV-0100 | VR home/menu | planned | Must be operable in headset without touchscreen dependency after successful VR entry. |
| RV-0106 | Shell-global handset/controller power HUD | planned | Stage A already stores the user preference; Stage B must keep real handset/controller battery status available across home/player/modules and represent unavailable controller telemetry honestly. |
| RV-0107 | Optional gaze-adaptive status HUD/QoL toggles | planned | Stage A already stores the look-up-reveal preference; Stage B must implement user-toggleable adaptive reveal/retract and preserve a predictable persistent/manual fallback when disabled. |
| RV-0108 | First-VR-run onboarding/calibration wizard | planned | Must be versioned, resumable, skippable with safe defaults, teach controls while configuring the user profile, and support complete or per-tool re-entry from Settings. |
| RV-0206 | User optical/IPD calibration | planned | Must preserve the physical viewer profile, expose persistent user IPD/alignment correction with live in-headset calibration and reset, and pass reference-device optical/comfort validation. |
| RV-0300 | Retro performance fixture | planned | Requires on-device sustained performance evidence. |
| RV-0500 | Local VR media player | planned | Requires on-device playback and thermal validation. |
