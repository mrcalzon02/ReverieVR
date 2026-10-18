# ReverieVR independent game modules — execution record, 2026-10-18

**Repo:** `mrcalzon02/ReverieVR`, branch `main` only. **Not a declaration that phases 1–6 are fully accepted.** No GitHub Actions were added. No external asset-search sources used; the Red Ledger pilot uses the previously curated approved Beckenburg selection.

## Source work remotely verified

- `11d541b05f84bb3938c1449eeb10645d266bfd79`: bootstrap authenticated **testing-only** game catalog, detached signature, and bundled public key. The catalog intentionally has **zero releases**. The testing signing private key must remain available exclusively to the authorized publisher outside GitHub; before stable releases, provision and ship a production trust root.
- `52121724b9646f5f6c8438ea03d3249f487c8c5f`: always enable per-game **Manage → Check updates for this game**, even when a game has no downloaded content. Show code and content availability separately and keep installed/offline games untouched on fetch failure.
- `ae9c86988738f3b98a2bce16650fb97799a276d8`: JVM signature test for the exact signed testing catalog and rejection of tampered bytes; fingerprint-pinned public-key fixture.
- `d5e772c7d7850190910ee32c02d065815fe97b58`: Android companion installer verifies private APK bytes and passes the verified local file, rather than the mutable DownloadManager URI, to Android's user-approved installer.
- `62bb08962c6d79bd767b7be5a044f81c28cba3d7`: signed catalog entries must declare and pass minimum hub version, Android SDK, native ABI, GLES version, and supported CPU ABI checks; signer tooling validates the same metadata.
- `c36e2a228e6aaab43dbe239980fc9420c734c8e1`: private read-only APK ContentProvider supports `OpenableColumns.DISPLAY_NAME` / `SIZE` for Android install handoff, maintaining exact-path and read-only validation.
- `9a7df95992c2c0b66ea18336736a58219b3032b2`: operator-driven per-game GitHub release publisher (draft/upload/download/hash/readback, source SHA lease); no GitHub Actions.
- `e49e35b551322937f49a94d5593ea37651413535`: release creation and binary upload now re-inspect remote GitHub state after ambiguous timeouts before retrying; reject mismatched files rather than clobbering.
- `10d52ac2f7730ac36fa70c418d9c73e58dd667a0`: six offline mocked release-publisher regression tests (creation accepted despite timeout, existing draft, published mutation refusal, upload accepted despite timeout, size collision, bounded retry exhaustion).

Remote branch readback at this record: **`10d52ac2f7730ac36fa70c418d9c73e58dd667a0`**; the previous commits are on its history. No claims of completed APK build or GitHub release publication follow merely from source acceptance.

## Existing foundation (earlier commits)

- RLBP room-mesh admission for Red Ledger; 14 previously curated Beckenburg models, three rooms, gated renovations.
- Game-scoped immutable `.rvpack` content install/rollback and native `read_asset` resolver with APK fallback.
- Native game registry, safe VR-to-phone manager handoff, module catalog/update API, Android companion identity/signer verification, read-only save export route.
- `:companion-test-chamber`: standalone Android Gradle module with the **real** existing Test Chamber C++ source and a separate Cardboard stereo Activity.

## Verification evidence and remaining gates

**Source:** GitHub connector accepted commits; fetched source and `main` readback verified the new provider query, bounded-release publisher functions, six regression-test methods, per-game check path and catalog compatibility checks.

**Locally available:** Previously generated `red-ledger-interiors-v1.rvpack` with 14 converted Beckenburg models and `module.json`. This is a local artifact, NOT a remotely published GitHub Release. The existing testing catalog rightly offers no files.

**Not executed in this slice:** local Gradle/Android SDK/NDK build, JVM/JUnit Gradle suite, Python publisher tests, GitHub Release draft/upload/readback via `gh`, actual `.rvpack` import on a Galaxy S9, Cardboard stereo inspection, signed companion APK install/launch/update, save migration, performance/thermal test and hub slimming. Test methods existing in GitHub source are **not** equivalent to tests executed.

**Next exact execution gate:** on an Android-capable checkout, run `python3 -m unittest discover -s scripts/modules -p 'test_*.py'`, `./gradlew :app:testDebugUnitTest`, `./gradlew :companion-test-chamber:assembleDebug`, and `python3 scripts/modules/verify-companion-contract.py --apk game-apps/test-chamber/build/outputs/apk/debug/test-chamber-debug.apk` with the actual generated APK path. Fix build errors at source; sign the companion with the **same** long-lived publisher cert as ReverieVR for host registry admission. On S9 verify controller/gaze, per-eye GL, native exit/recovery and save handling. Only then publish immutable module binaries, remotely read back SHA-256, sign nonempty catalog revision, and consider phases 5/6 code migration and slimming. Never claim production acceptance from a draft-only release or emulator-only test.
