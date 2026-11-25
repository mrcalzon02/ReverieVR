# Optional emulation and OpenRA — completion guide

**Project ID:** `emulator-catalog` • application • **Initial gate status: unverified pending current-source reconciliation**

## Evidence baseline
Curated catalog/source retrieval exists in plan; executable cores and OpenRA runtime not established.

## Authority and entry points
- `docs/project/EMULATOR_MODULES_PLAN.md`
- `docs/project/OPENRA_ANDROID_INTEGRATION.md`
- Production anchor: `app/src/main/java/io/github/mrcalzon02/reverievr/EmulatorModuleCatalog.java`
- Production anchor: `app/src/main/java/io/github/mrcalzon02/reverievr/EmulatorSourceRepository.java`
- No dedicated existing acceptance command verified for this scope; use the actual host build/runtime and existing project diagnostics. Do not create a mock executable to obtain a pass.

## Ordered gates
### G01 — Complete verified source/catalog consent

**Implementation:** Use curated allowlist, real license/source provenance and informed opt-in; offline metadata, honest unavailable state.

**Does it work?** Open catalog offline, verify approved source when online, reject bad identity/license, cancel without download.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G02 — Implement admitted phone-local core runtime

**Implementation:** Use existing single-APK host/adapter, valid local media, input/audio/video, saves and bounded resources; no pirated firmware/media.

**Does it work?** A legally admitted core runs a real compatible local test program offline, with controls and save state; no PC or separate APK requirement.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G03 — Complete OpenRA approved integration path

**Implementation:** Follow project-specific integration and licensing constraints; do not count repository download as playable.

**Does it work?** Launch a legitimate supported game scenario, command units, save/reopen and finish a match; device performance and recovery proven.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G04 — Qualify each advertised core independently

**Implementation:** Document supported formats, ABI, license, availability, update/rollback and actual device benchmarks.

**Does it work?** Every enabled entry runs its advertised workload; unsupported entries honestly unavailable, no inert controls.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

## Completion
Apply the [shared SCD contract](../SUSTAINED_CONTINUOUS_DEVELOPMENT.md) and every applicable common end-to-end condition. Record revision, actual runner/logs/artifacts and source readback in the existing acceptance ledger. Maintain this guide when owner-approved scope changes.
