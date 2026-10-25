# ReverieVR — Durable Recovery Development Object

**Authority:** operational campaign checkpoint; **tracking issue:** [RV-REC #6](https://github.com/mrcalzon02/ReverieVR/issues/6).
**Established:** 2026-10-25; **canonical development branch:** `main`.
**Status:** ACTIVE — all ten Android 10 emulator instrumented tests passed in run 40234352374, while the overall workflow failed on the strict 108/108 native texture-loader log check. Five-model Breakwater coverage and native loader evidence repair are ongoing. Galaxy S9 acceptance remains OPEN.
**Scope:** restore trustworthy Android game acquisition/launch testing; finish release/install/asset/feature verification without erasing any existing functionality.
**Governing documents:** [INSTRUCTIONS](INSTRUCTIONS.md), [EXECUTION_CONTRACT](EXECUTION_CONTRACT.md), [BACKLOG](BACKLOG.md), [ACCEPTANCE_LEDGER](ACCEPTANCE_LEDGER.md), [INDEPENDENT_NATIVE_GAME_MODULES](INDEPENDENT_NATIVE_GAME_MODULES.md). This file does not replace any of them.

## Resume here — deterministic handoff

1. Read `AGENTS.md`, the governing documents, this file, and [issue #6](https://github.com/mrcalzon02/ReverieVR/issues/6). Query current remote `main` SHA, recent relevant workflow results, and published Releases; **never assume this snapshot is still latest**.
2. Resume **RV-REC-01** by checking the most recent Android hub integration workflow result. The original four-fixture compilation failure in [40159108734](https://github.com/mrcalzon02/ReverieVR/actions/runs/40159108734) was fixed; all ten tests passed in [40234352374](https://github.com/mrcalzon02/ReverieVR/actions/runs/40234352374), but the strict `Tactical PNG textures loaded: 108/108` asset log check failed. The next commits require all five actual Breakwater models from the signed-content pack and print actual Omega sprite-loader counts before enforcing 108/108. Repair broken assets or load behavior rather than relaxing the threshold.
3. If CI still fails, inspect the failing **job step logs** and repair the authoritative test/runtime code, preserving the coverage and re-running existing workflow. If CI passes, retain its run ID, test count, logcat evidence, Android APK artifacts and checksum in issue #6 and acceptance ledger. Emulator success is NOT hardware acceptance.
4. Proceed to the next priority item with an executable acceptance gate. Do not burn development time continuously polling Actions; check a completed run only when useful.
5. On each material change, record `baseline SHA → changed paths → tests/workflow result → artifact and release identity → S9 evidence (or explicit NOT TESTED) → open blocker → next action` and sync the issue. Never claim that local uncommitted changes have been inspected using only GitHub remote APIs.

## Reconciled baseline (audit: 2026-10-25)

| Evidence | Verified finding | Boundary |
|---|---|---|
| Remote `main` at audit | `23216695f49b71e9c5c8df7633321907013d21cf` | Removing retired cross-project signing workflow was the only change after signed release |
| Latest audited signed hub | [phone-test-1393-1](https://github.com/mrcalzon02/ReverieVR/releases/tag/phone-test-1393-1), source `2453541715fb2fc7f6c07ce1a821407cd0b7f5b9`; phoneTest versionCode `215063425` | APK and 15 native modules packaged; not proof of real controller/headset operation or in-place update |
| Hub/acquisition CI | [40159108734](https://github.com/mrcalzon02/ReverieVR/actions/runs/40159108734) **FAIL** | Four `NativeModuleRuntime` constructor calls missing the fifth `ReveriePreferences` argument; stage fails before emulator tests |
| Separate companions | Test Chamber and Breakwater build validation passed recent Actions runs | In-headset launch, signed installation and independently updating revisions still need physical proof |
| Module update catalog | `modules/catalog-v1.json`: `{"catalog_revision":1,"modules":[],"schema":1}` | No downloadable game module revision advertised; signed bootstrap != delivery |
| Red Ledger original art | 24 icons on `main`; 16 canonical product FBX/texture pairs (supplier groups 101–404) on `main` | Supplier groups 505 and 606 (8 pairs) absent from committed runtime source; specific original exports exist in connected `BetweenDeliveries_UnrealExports/products/` folders |
| Additional application branches | `google-play` is parked and diverged by design | Do not merge/publish without explicit separate authorization and Play acceptance |
| Local worktree | Not observable through GitHub remote | Inspect on the operator's checkout before asserting there are no uncommitted/stashed changes |

## Priority queue and acceptance ownership

| ID | Priority | Engineering target | Minimum acceptance and truth boundary | State at creation |
|---|---|---|---|---|
| RV-REC-01 | P0 | Repair hub Android instrumented-test constructor drift without bypassing avatar transmission or save isolation | Four fixtures compile; all 10 Android instrumentation tests pass; native Omega texture count 108/108 and five verified-pack Breakwater meshes; retain emulator artifact/logcat/screenshots | **10 TESTS PASSED; ASSET LOG GATE FAILED in 40234352374; repeat after model fixture/check repairs** |
| RV-REC-02 | P0 | Daydream controller/gaze-beam flicker, reconnection, Home/native launch and recovery | Galaxy S9 real controller attach/stall/remove/reconnect, no spurious gaze ray, five-second gaze fallback and native entry/exit/saves; video/diagnostic proof | NOT DEVICE ACCEPTED |
| RV-REC-03 | P0 | Hub in-place testing/stable update channel reliability | Two signer-continuous releases, newer versionCode, actual installed update, no settings/save loss, visible fallback | NOT DEVICE ACCEPTED |
| RV-REC-04 | P0 | Independent per-game code/content distribution | Two real signed catalog revisions of one named native game; download, verify, install, update, rollback, offline relaunch, preserve saves; device acceptance separately | CATALOG EMPTY |
| RV-REC-05 | P1 | Red Ledger eight missing product source model/texture pairs | Exact owner-authorized 505/606 exports hash/inventory checked; converted to runtime formats, committed with manifest and tested render/use | UNINTEGRATED |
| RV-REC-06 | P1 | Gameplay and assets: Breakwater weapon models/rack, Gumball volumetric physics/story, Red Ledger service, Omega Front campaigns, classics | Per-game functional gameplay tests, packaged art/sounds, controls and direct headset acceptance; do not treat a named ID or prototype as complete | PARTIAL |
| RV-REC-07 | P1 | Shared wardrobe across companions and fitted avatar art | Versioned cross-package identity transfer and visible/skinned model/outfit previews; body collision unchanged unless explicitly adopted | SOURCE DATA / ART INCOMPLETE |
| RV-REC-08 | P2 | Phone Screen capture, hosted keyboard entry, OpenRA, SteamVR and compatibility | Separate actual end-to-end runtime gates, consent/privacy and provenance; no false Play buttons | MOSTLY PARTIAL/PLANNED |
| RV-REC-09 | P1 | Documentation/evidence drift and uncommitted work checks | Reconcile outdated claims against exact CI run and source SHA; no source-only claim upgraded directly to hardware verified | ONGOING |

## First engineering slice — RV-REC-01

**Defect:** current `NativeModuleRuntime` takes five arguments: private game save root, packaged assets, input bus, development admission flag, and optional `ReveriePreferences`. Its `start()` snapshots the configured avatar to the native ABI, falling back to default if the preferences object is absent. Two construction sites in each of the following Android instrumentation classes used an obsolete four-argument form:

- `app/src/androidTest/java/io/github/mrcalzon02/reverievr/HubGameModulesInstrumentedTest.java`
- `app/src/androidTest/java/io/github/mrcalzon02/reverievr/DownloadedNativeGameAssetsInstrumentedTest.java`

**Repair:** construct `ReveriePreferences` with `InstrumentationRegistry.getInstrumentation().getContext()` for the test APK, separately from the real application's packaged assets. This passes through the actual avatar boundary while keeping test preferences apart from player preferences. Neither the native ABI nor game save paths, production launch code, signing, catalog, release cadence or existing validation requirements change.

**Trigger:** `.github/workflows/reverie-hub-game-modules-ci.yml` already watches both changed test files. Do not add a duplicate workflow. **Required evidence:** `:app:compileDebugAndroidTestJavaWithJavac` success; emulator execution and `OK (10 tests)`; real Red Ledger and Omega Front downloaded-asset probes; native launch/stereo/campaign/save-resume output; published CI artifacts and exact run URL. Check real new logs; prior passing companion CI is not a substitute.

## Definition of recovery

Recovery is **not** complete because source compiles or several APKs exist. It requires a working reference phone, controllable VR Home and games, correct module asset delivery and native updates, successful current validation suite, verified in-place headset updates and saved progress preservation. Continue implementing features only while their prerequisite test gates remain truthful and actionable.

## Recovery observation — 2026-10-25 / first real emulator run

- Baseline commit `e14ccfe6fd279c19a9bada8fb9a3e7817ce404e6`; [CI run 40234352374](https://github.com/mrcalzon02/ReverieVR/actions/runs/40234352374): Android 10 emulator installed real hub plus Test Chamber and Breakwater companion APKs and reported **`OK (10 tests)`**. Red Ledger 24 downloaded icons loaded through real native GLES2; Omega Front 141 files, combat travel/command/save-resume and valid captured tactical PNG all passed.
- Workflow still **FAILED**, because its exact `Tactical PNG textures loaded: 108/108` native log count assertion did not match. The source lists 108 exact-case unique sprite files, all committed. This proves file existence, **not** successful upload of all 108. Inspect the actual count; no weakening of the gate is authorized.
- Run retained [artifact 12614900094](https://github.com/mrcalzon02/ReverieVR/actions/runs/40234352374/artifacts/12614900094) containing APKs, checksums, logcat, instrumented output and Omega Front framebuffer.
- Next repair: `e1eff485...` updates Breakwater fixture from three to all five genuine BWM meshes; `8b486e...` prints actual loaded texture count and demands verified-pack evidence for all five Breakwater meshes. The CI on the resulting head must finish and be inspected. A passing instrumented suite alone cannot close RV-REC-01. No Galaxy S9 hardware proof has been obtained.
