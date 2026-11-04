# Shared native asset library (version 1)

**Implemented in source; APK/headset acceptance pending.** ReverieVR's common `native/shared/v1/` contains six compact generated textures and three curated BWM1 models. These are actual reusable *asset bytes*, not an imaginary common GPU texture memory across different native applications. The source-authoritative manifest `shared-assets/catalog-v1.json` gives each asset a stable logical ID, exact relative path, Git blob hash and license/consumer metadata.

## Current runtime contract

- The single ReverieVR APK packages each common file **once**. The host's existing read-only `read_asset` callback resolves it from verified game content or APK baseline; module access is explicitly allowlisted in `reverie_native_shared_asset_catalog.h`. Game IDs outside that opt-in list receive zero access. No new ABI, Java permission, remote endpoint, second APK or dynamic library loader.
- Breakwater currently loads the six authored PNGs into its one 128×448 sprite/material atlas and reads the three BWM1 defense prop meshes; its two owner-specific boat/APC models stay game-private. If a shared path is unavailable in an older verified content pack, Breakwater can attempt its historical private name, but the old APK files are no longer duplicated.
- Omega Front now deliberately loads concrete and steel from the same shared PNG paths, eliminating two older distinct material files. Its 2D tactical engine and its other art remain unchanged.
- Red Ledger and the Test Chamber have exact-path access to curated assets but **do not yet have consumers for BWM1 models**; BWM1 is not `.rlprop`, `.obj` or general-purpose glTF. Never display unsupported model loads as working.

## Cross-repository source consumption

Other authorized internal game repos may use this as a **versioned source library**: pin a ReverieVR Git commit and a manifest blob, select only the needed IDs, verify blob bytes, licenses and formats, and materialize them only into the final target build. Do not blindly git-submodule or archive the entire large repository; do not publish purchased full source packs or unlicensed derivatives. A build that imports selected files remains offline at runtime. Cross-repo *source* references alone do not save download space if standalone APKs independently bundle the same bytes.

A future install-once cross-game **runtime** cache would require verified signed shared-pack manifests, immutable content addressed by SHA-256, reference counts, atomic mount/swap/rollback, dependency handling and offline verification before game launch. Android different packages cannot access each other's private storage by path. This cache is **planned, not implemented**, and does not supersede the existing single-APK product rule.

## Acceptance

`python3 scripts/verify-shared-native-assets.py` checks nine exact source files, Git object hashes, available SHA-256, PNG/BWM signatures, bounds, no old duplicate package paths, and reference wiring. `bash scripts/verify-breakwater-visuals.sh` adds the shared verifier to existing Breakwater host tests. Run native asset-scope source regressions, Omega texture asset checks, the existing Android/NDK build and installed headset tests. The two removed Omega PNGs total **14,834 uncompressed source bytes**; compressed APK/network savings are not measured and may differ. No extra draw pass or runtime texture upload is introduced by the shared namespace itself.

The native-doc drift guard now maps both `reverie_native_shared_asset_catalog.h` and `reverie_native_asset_scope.h` to this standard for same-commit source changes, keeping per-game permission expansion reviewable.
