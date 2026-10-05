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
| RV-0001 | Project governance/bootstrap | draft | Must verify committed remote files and resulting `main` SHA. |
| RV-0002 | Framework/runtime selection | planned | Requires research, license/provenance review, and recorded architecture decision. |
| RV-0003 | Android APK skeleton | planned | Must build and install/launch on reference device. |
| RV-0100 | VR shell/menu | planned | Must be operable in headset without touchscreen dependency. |
| RV-0300 | Retro performance fixture | planned | Requires on-device sustained performance evidence. |
| RV-0500 | Local VR media player | planned | Requires on-device playback and thermal validation. |
