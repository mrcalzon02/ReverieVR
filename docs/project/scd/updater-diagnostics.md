# Updater, APK packaging and diagnostic intake — completion guide

**Project ID:** `updater-diagnostics` • application • **Initial gate status: unverified pending current-source reconciliation**

## Evidence baseline
Current signed phone-test run success; package and device upgrade require artifact and signer readback.

## Authority and entry points
- `docs/project/PERSISTENT_UPDATE_BOOTSTRAP.md`
- `docs/project/APK_BUILD_ORDER.md`
- `docs/project/INCREMENTAL_DEVELOPMENT_PLAN.md`
- `docs/project/PUBLIC_DISTRIBUTION_AND_LICENSES.md`
- Production anchor: `app/src/main/java/io/github/mrcalzon02/reverievr/UpdateChecker.java`
- Production anchor: `app/src/main/java/io/github/mrcalzon02/reverievr/UpdateInstaller.java`
- Production anchor: `app/src/main/java/io/github/mrcalzon02/reverievr/UpdatePackageVerifier.java`
- Production anchor: `app/src/main/java/io/github/mrcalzon02/reverievr/DiagnosticBundleExporter.java`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/ci/verify-unified-game-apk.py`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/verify-build-bootstrap.sh`

## Ordered gates
### G01 — Qualify exact unified APK and same-signer updates

**Implementation:** Use canonical main source and published signed APK. Include non-draft prereleases in latest selection, require valid version/signer/checksum and explicit install.

**Does it work?** Install previous verified APK, check update, choose/update latest eligible release, verify signer/version/save continuity and offline launch; reject mismatch and recover failed transfer.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G02 — Complete diagnostic UX and privacy

**Implementation:** Use real log export and existing issue intake; user consent, bounded redactable metadata, failure feedback.

**Does it work?** Generate local diagnostic incident, review/redact, export or explicitly submit through existing intake, verify destination issue and no credentials/private data.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G03 — Finish package and release provenance

**Implementation:** Preserve real ABI/assets, legal manifests, checksums and no extra required game APK; existing release-blocking policy unchanged.

**Does it work?** Actual release contains exact source, correct signer/version, both required ABIs, complete native assets and valid checksums; read back public artifact.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

## Completion
Apply the [shared SCD contract](../SUSTAINED_CONTINUOUS_DEVELOPMENT.md) and every applicable common end-to-end condition. Record revision, actual runner/logs/artifacts and source readback in the existing acceptance ledger. Maintain this guide when owner-approved scope changes.
