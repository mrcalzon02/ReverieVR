# Shared native art — companion/verified pack integration repair (2026-11-04)

Baseline: `c27c7337a05db83fa296e9a19617fa95f2fc8a3e` in `mrcalzon02/ReverieVR` main. Six owner-generated 64px materials and three curated BWM1 props had been deduplicated under `native/shared/v1/`, with Breakwater and Omega source consumers updated. The older separate-APK test companion and historical `.rvpack` recovery fixtures still used prior folder names. Existing Android CI asserted the old ZIP paths and old verified-pack log lines, so it could fail even when hub rendering was functional.

Scoped implementation:

- Breakwater's companion-only Gradle asset `Sync` stage imports all nine explicitly reviewed shared files and just the two game-private owner boat/APC BWM1 meshes. It removes stale old output on repeated stages. It is a regression companion, not a return to per-game distribution.
- Java `SharedNativeAssetPaths` admits only the nine exact shared ZIP entry names for approved consumers, distinguishing six textures (Breakwater, Omega, Red Ledger, Workshop) from three BWM1 props (Breakwater, Red Ledger, Workshop). The legacy `RvPackInstaller` still refuses directories, sibling game namespaces, traversal, unknown art names and unsupported IDs.
- Installed native game test fixtures package Breakwater's three generic BWM1 models under their new canonical shared paths while preserving its two owner-specific private paths. Omega's 141-file fixture adds only the two common concrete/steel tiles it consumes.
- Android workflow adds the shared-source hash/format verifier and checks all eleven companion assets and the updated real verified-content log prefixes, rejecting old duplicated model paths.
- `RvPackInstallerTest` and the shared-source verifier retain explicit cross-game scope tests.

No game balancing, textures, texture dimensions, native ABI structs, save paths or user-facing controls have changed. No extra runtime-download dependency or global content broker was added. The source hashes/bytes remain pinned to the current v1 catalog. The 14,834-byte old Omega source duplication remains eliminated.

Validation available this turn: JDK 21 compiled and executed the exact proposed Java shared-file allowlist and adversarial smoke fixture (approved textures/models, denied other games, unknown filename, directory and traversal). Native shared asset Git references and six authored PNG bytes were previously checked; the current patch still requires full Android CI, Gradle APK inspection, legacy content-pack integration and Galaxy S9 two-eye/thermal evidence. No CI success, new APK or actual device acceptance is claimed here.
