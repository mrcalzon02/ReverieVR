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
| RV-0003 | Android APK skeleton | **draft** | Android/Gradle/Java source is on remote `main` at `a4b4db3679fc768c4b32e40c2efb04183bf73056`; build not run because this environment lacks Gradle/Android SDK. |
| RV-0090 | 2D touchscreen boot/setup surface | **draft** | UI source, real phone battery reporting, persisted QoL toggles, recovery reset, and honest readiness gates are on remote `main`; APK/device launch still required. |
| RV-0091 | Controller pairing/readiness workflow | planned | Pair / Sync and Test actions exist in the UI but remain disabled until the verified Daydream controller stack is implemented. |
| RV-0092 | 2D-to-VR transition/recovery | planned | Enter VR exists but is disabled until Stage B is implemented and verified. |
| RV-0100 | VR home/menu | planned | Must be operable in headset without touchscreen dependency after successful VR entry. |
| RV-0300 | Retro performance fixture | planned | Requires on-device sustained performance evidence. |
| RV-0500 | Local VR media player | planned | Requires on-device playback and thermal validation. |
