# Optional phone controller and Steam controller — completion guide

**Project ID:** `controller-companion` • application • **Initial gate status: unverified pending current-source reconciliation**

## Evidence baseline
Controller app/provider, Steam input and identity source exist; physical devices remain distinct.

## Authority and entry points
- `docs/project/CONTROLLER_LAYOUTS_AND_BINDINGS.md`
- `docs/project/STEAM_CONTROLLER_INPUT.md`
- `docs/project/STEAM_CONTROLLER_DIGITAL_TWIN.md`
- `docs/project/PHONE_SCREEN_PANEL.md`
- Production anchor: `controller-app/src/main/java/io/github/mrcalzon02/reverievr/controller/ControllerActivity.java`
- Production anchor: `app/src/main/java/io/github/mrcalzon02/reverievr/PhoneControllerEmulatorProvider.java`
- Production anchor: `app/src/main/java/io/github/mrcalzon02/reverievr/SteamControllerIdentity.java`
- No dedicated existing acceptance command verified for this scope; use the actual host build/runtime and existing project diagnostics. Do not create a mock executable to obtain a pass.

## Ordered gates
### G01 — Finish optional controller pairing and normalized events

**Implementation:** Verify actual phone controller protocol and Steam HID inputs, disconnect, reconnect, calibration and accurate device labels.

**Does it work?** Real two-device session or actual Steam device sends select/back/pad/axes; disconnect releases and safe fallback works. No mandatory companion.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G02 — Complete controller representation and battery status

**Implementation:** Use verified meshes/provenance, pose/calibration and honest unavailable battery readings.

**Does it work?** Actual controller visualization follows intended inputs, no fabricated pose or battery; remap and reset function.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G03 — Validate privacy and reference use

**Implementation:** No location prerequisite or accidental network dependency for baseline; optional connections explicit and recoverable.

**Does it work?** Physical input and recovery tested on reference S9; companion loss does not break main app.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

## Completion
Apply the [shared SCD contract](../SUSTAINED_CONTINUOUS_DEVELOPMENT.md) and every applicable common end-to-end condition. Record revision, actual runner/logs/artifacts and source readback in the existing acceptance ledger. Maintain this guide when owner-approved scope changes.
