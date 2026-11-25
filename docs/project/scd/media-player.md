# Local VR media and music — completion guide

**Project ID:** `media-player` • application • **Initial gate status: unverified pending current-source reconciliation**

## Evidence baseline
Local video player and native music library exist; full comfort/media acceptance remains.

## Authority and entry points
- `docs/project/INSTRUCTIONS.md`
- `docs/project/INCREMENTAL_DEVELOPMENT_PLAN.md`
- `docs/project/APK_BUILD_ORDER.md`
- Production anchor: `app/src/main/java/io/github/mrcalzon02/reverievr/LocalVideoPlayer.java`
- Production anchor: `app/src/main/java/io/github/mrcalzon02/reverievr/VrActivity.java`
- No dedicated existing acceptance command verified for this scope; use the actual host build/runtime and existing project diagnostics. Do not create a mock executable to obtain a pass.

## Ordered gates
### G01 — Complete local media library and import

**Implementation:** Use SAF and local permission-safe import; supported formats and invalid/missing media feedback.

**Does it work?** Open supported local media without network; cancel/revoke permission, switch files and handle unsupported/corrupt formats safely.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G02 — Finish stereo media modes and controls

**Implementation:** Implement/verify declared flat/180/360 and mono/stereo layouts, playback, seek, audio, subtitles where supported, screen size and placement.

**Does it work?** Play real fixtures with correct eye ordering, aspect ratio and audio; seek, pause, resume, change projection and return Home with controls intact.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G03 — Validate local music and lifecycle

**Implementation:** Use approved local tracks, volume and lifecycle; no leaks or unwanted background playback.

**Does it work?** Playback state survives intended transitions, music stops on exit and actual headset comfort/audio/thermal evidence passes.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

## Completion
Apply the [shared SCD contract](../SUSTAINED_CONTINUOUS_DEVELOPMENT.md) and every applicable common end-to-end condition. Record revision, actual runner/logs/artifacts and source readback in the existing acceptance ledger. Maintain this guide when owner-approved scope changes.
