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
| RV-0001 | Project governance/bootstrap | **static accepted** | Governance baseline committed and independently read back from remote `main` at `542a6d14121ba986f73b5b940c5debbeb0a0f057`. |
| RV-0002 | Framework/runtime/controller-stack selection | planned | Requires research, license/provenance review, 2D-to-VR transition support, controller pairing path, and recorded architecture decision. |
| RV-0003 | Android APK skeleton | planned | Must build and install/launch on reference device. |
| RV-0090 | 2D touchscreen boot/setup surface | planned | Must launch conventionally outside VR and remain usable for setup/recovery. |
| RV-0091 | Controller pairing/readiness workflow | planned | Must pair/sync or reconnect the reference controller and visibly verify input readiness without Google service dependency. |
| RV-0092 | 2D-to-VR transition/recovery | planned | Must enter stereoscopic VR deliberately and return safely to 2D setup after failure or user request. |
| RV-0100 | VR home/menu | planned | Must be operable in headset without touchscreen dependency after successful VR entry. |
| RV-0300 | Retro performance fixture | planned | Requires on-device sustained performance evidence. |
| RV-0500 | Local VR media player | planned | Requires on-device playback and thermal validation. |
