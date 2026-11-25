# Complete native desktop hub / OpenXR — completion guide

**Project ID:** `desktop-hub` • application • **Initial gate status: unverified pending current-source reconciliation**

## Evidence baseline
Desktop authoring and initial player slices exist; full hub and OpenXR unverified.

## Authority and entry points
- `docs/project/DESKTOP_PLATFORM_EXPANSION.md`
- Production anchor: `tools/desktop/ReverieSceneProject.java`
- Existing diagnostic/probe (read its documented arguments before running): `tools/desktop/player/test.sh`

## Ordered gates
### G01 — Build offline complete desktop shell

**Implementation:** Implement shared home, settings, input, storage, recovery and launch APIs in native desktop mode; no Android emulation.

**Does it work?** Cold launch on Windows/Linux/macOS without headset/phone/Steam/network; navigate settings, home and library with keyboard/mouse/gamepad.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G02 — Integrate all hosted capabilities

**Implementation:** Port actual gameplay cores, classic/native modules, DOS/media/apps, scene/audio and update adapters rather than substitute demos.

**Does it work?** For each registry/app track, launch and complete representative intended action, save/reopen and return Home; maintain parity manifest per OS.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G03 — Complete platform releases and optional OpenXR

**Implementation:** Package and validate same app per required OS; optional OpenXR/SteamVR on supported Windows/Linux, no dependency in desktop mode.

**Does it work?** Real OS installs, updates/recovery and offline use; optional XR runtime tested separately with correct stereo and input.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

## Completion
Apply the [shared SCD contract](../SUSTAINED_CONTINUOUS_DEVELOPMENT.md) and every applicable common end-to-end condition. Record revision, actual runner/logs/artifacts and source readback in the existing acceptance ledger. Maintain this guide when owner-approved scope changes.
