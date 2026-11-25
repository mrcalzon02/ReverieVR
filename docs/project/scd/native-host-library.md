# Native module host and bundled game library — completion guide

**Project ID:** `native-host-library` • application • **Initial gate status: unverified pending current-source reconciliation**

## Evidence baseline
One APK, Java registry and C ABI; 17 registered games. Current signed-build and functional run success.

## Authority and entry points
- `docs/project/INSTRUCTIONS.md`
- `docs/project/INDEPENDENT_NATIVE_GAME_MODULES.md`
- `docs/project/NATIVE_GAME_PROJECT_POINTERS.md`
- Production anchor: `app/src/main/java/io/github/mrcalzon02/reverievr/NativeModuleRuntime.java`
- Production anchor: `app/src/main/java/io/github/mrcalzon02/reverievr/GameModuleRegistry.java`
- Production anchor: `app/src/main/java/io/github/mrcalzon02/reverievr/GameModulesActivity.java`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/modules/verify-game-identity-matrix.py`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/verify-native-abi-contract.py`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/ci/verify-unified-game-apk.py`

## Ordered gates
### G01 — Validate dynamic registry and actual module launch

**Implementation:** Preserve one APK, ABI, namespace and shared asset rules; inventory grows without arbitrary fixed maximum.

**Does it work?** Every current registry ID maps to real packaged ARM libraries/assets and actually launches; duplicates/missing required entries fail, extra legitimate games are admitted.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G02 — Finish shared focus, input, save and runtime services

**Implementation:** Exercise Quick Menu pause, neutral/rearm input, audio leases, context loss, scoped storage, failed-save protection and Home/resume in each module.

**Does it work?** Hold/release while menu open, disconnect, change app, resume and recreate; no stale input, lost acknowledged state or cross-module asset/save leakage.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G03 — Validate packaged library and update continuity

**Implementation:** Use real installed unified APK on Android 10 and actual S9; no companion install required.

**Does it work?** Launch each game, progress, return/reopen, update same signer and preserve saves; collect actual two-eye frames and APK verification.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

## Completion
Apply the [shared SCD contract](../SUSTAINED_CONTINUOUS_DEVELOPMENT.md) and every applicable common end-to-end condition. Record revision, actual runner/logs/artifacts and source readback in the existing acceptance ledger. Maintain this guide when owner-approved scope changes.
