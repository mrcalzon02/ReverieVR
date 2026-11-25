# Laptop Link, local SMB and optional screen panel — completion guide

**Project ID:** `laptop-link` • application • **Initial gate status: unverified pending current-source reconciliation**

## Evidence baseline
Portable exported optional utility, SMB and screen link source; phone independence required.

## Authority and entry points
- `docs/project/LAPTOP_LINK_BOOTSTRAP.md`
- `docs/project/LAPTOP_SMB_IMPORT.md`
- `docs/project/PHONE_SCREEN_PANEL.md`
- `docs/project/STEAMVR-LINK-IMPLEMENTATION.md`
- Production anchor: `app/src/main/java/io/github/mrcalzon02/reverievr/LaptopLinkBundle.java`
- Production anchor: `app/src/main/java/io/github/mrcalzon02/reverievr/LaptopSmbClient.java`
- Production anchor: `app/src/main/java/io/github/mrcalzon02/reverievr/SteamVrLinkProbe.java`
- No dedicated existing acceptance command verified for this scope; use the actual host build/runtime and existing project diagnostics. Do not create a mock executable to obtain a pass.

## Ordered gates
### G01 — Finish approved local file/peripheral connection

**Implementation:** Use explicit export/run, read-only approved SMB paths, safe authentication, no silent install.

**Does it work?** Export portable utility, connect authorized local source, import file, cancel/revoke; disconnect leaves local APK working.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G02 — Complete optional screen/input/SteamVR link behavior

**Implementation:** Follow existing explicit device permission and optional interoperability contract; no mandatory remote execution.

**Does it work?** Exercise documented screen panel/input or supported SteamVR link on real host, report missing capabilities accurately, disconnect cleanly.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G03 — Verify privacy and offline resilience

**Implementation:** Check bounded buffers, timeout, cleanup and no persistent unauthorized service.

**Does it work?** Actual local session and interrupted network prove recovery; no private files or credentials in logs.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

## Completion
Apply the [shared SCD contract](../SUSTAINED_CONTINUOUS_DEVELOPMENT.md) and every applicable common end-to-end condition. Record revision, actual runner/logs/artifacts and source readback in the existing acceptance ledger. Maintain this guide when owner-approved scope changes.
