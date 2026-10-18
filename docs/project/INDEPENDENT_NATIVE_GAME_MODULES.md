# Independent Native Game Modules — ReverieVR

**Plan, not implemented.** Target: Galaxy S9 / Daydream, Android API 26+, C++17/GLES2. Source: \`mrcalzon02/ReverieVR\`, \`main\` only. No new GitHub Actions.

## Decision

Maintain one ReverieVR hub for VR setup, calibration, controllers, game discovery, headset-safe UI, global settings, diagnostics and recovery. Make each native game individually installable, updatable, releasable, removable and launchable offline. Keep **hub version**, **game code version** and **game content version** independent.

Use two stages. First, signed downloadable **content-only game packs**, with existing native binaries still inside the hub. Second, independently installed **native game APKs** for compiled C++ updates after proof on a physical Galaxy S9. Do not assume downloaded \`.so\` libraries can be executed from writable Android app storage. Do not silently swap running code.

## Verified current architecture

* \`reverie_native_host.cpp\`: fixed \`kBuiltIns\` allowlist, \`dlopen\` of packaged \`libreverie_module_*.so\`, \`dlsym\` for \`reverie_native_module_entry_v1\`, versioned ABI-v1 validation.
* \`NativeModuleRuntime.java\`: bundled-only enumeration and launches, stable game IDs and private module save directories.
* \`Android.mk\`: individual game shared-library build targets; **not** individually deployed releases yet.
* \`UpdateChecker.java\` / \`UpdateInstaller.java\` / \`UpdatePackageVerifier.java\`: existing signed whole-hub APK updater through ReverieVR GitHub Releases.
* **Bug to correct first:** \`HostReadAsset\` for Red Ledger currently admits only \`icons/\`, \`models/\` and \`textures/\`. It rejects \`native/red-ledger/beckenburg/*.rlprop\` even if these assets are in the APK. The staged 14 Beckenburg meshes cannot show until its allowlist/scoped reader is corrected and the files packaged.

Retain module IDs including \`between-deliveries-red-ledger\`, \`breakwater-battery\`, \`omega-front\`, \`gumball-armada\`, \`ministry-lantern-desk\`, the separate \`iron-sight-forward-detachment\`, and individual \`classic-*\` games. Test Chamber remains bundled for safe fallback.

## Release types and compatibility

**Content package \`.rvpack\`:** ZIP holding \`module.json\`, \`assets/\` and \`licenses/asset-provenance.json\`. It contains only approved game data: textures, meshes, sprites, sounds, maps, scenarios and other allowlisted formats. No \`.so\`, \`.dex\`, JAR, executables, scripts or unapproved paths. A shell-owned service downloads, verifies and mounts it as read-only immutable game content. A content revision must never masquerade as a C++ code update.

**Code package:** separately signed companion APK \`io.github.mrcalzon02.reverievr.game.<slug>\` for a native game. It includes that game's C++ \`.so\` and a small shared-source VR Activity/runtime adapter. The hub verifies installed package ID, signer, version and compatibility, then launches an explicit game Activity. The companion controls its own GLES2 context and lifecycle, with a safe explicit Return to ReverieVR. Android's own user-approved Package Installer installs/updates native code. Different packages have different private storage; do not assume shared UID, filesystem, GLES context or invisible context switching.

**Optional experiment:** Android split/feature APK installs may offer more seamless in-process experiences; evaluate on the actual Galaxy S9 only after a signed, replaceable native split and safe unload/reload are proven. Do not make that an unverified prerequisite.

Check: game ID; content/code semantic version plus monotonic release revision/Android versionCode; module protocol; native ABI min/max; minimum hub version; Android SDK level; device ABI \`arm64-v8a\` / \`armeabi-v7a\`; GLES2; required dependencies; save schema. Preserve ABI-v1 struct offsets and \`struct_size\` rules. Implement content resolving behind existing \`read_asset\` service, or append optional size-checked callbacks only when truly needed.

## Trusted release catalog

One signed catalog on ReverieVR \`main\`, e.g. \`modules/catalog-v1.json\` plus \`modules/catalog-v1.sig\`, indexed by stable game ID and channel (\`stable\`, \`testing\`, \`development\`). Each entry has exact approved HTTPS GitHub release-asset URL, filename, signed metadata, SHA-256, total bytes, versions, host/ABI/device compatibility, changelog and source commit. The shell fetches one catalog on explicit **Check all** or the selected game's **Check updates**; cache signed metadata with an age and offline warning. No website HTML scraping or guessing among the latest 50 releases.

Sign the **exact catalog bytes** with an offline-held publisher key; use vetted crypto supported on Android API 26 (e.g. ECDSA P-256 with SHA-256), with pinned public key, revision anti-rollback and documented key rotation. A checksum from an unsigned response alone is not authentication. Companion APKs additionally require Android package signature continuity and correct package identity. Keep current \`phone-test-*\` whole-hub update tags separate; explicitly ignore \`module-*\` releases in the hub update checker.

Use immutable tagged releases from \`main\`: e.g. \`host-v0.2.0\`, \`module-red-ledger-v0.2.0\`, \`module-breakwater-v0.3.0\`. Git tags are not development branches. One repository is enough initially; no new repository or website necessary.

## Download and installation state machine

States: \`NOT_INSTALLED\` -> \`FETCHING\` -> \`VERIFYING\` -> \`STAGING\` -> \`READY\`; plus \`BUILT_IN\`, \`UPDATE_AVAILABLE\`, \`PENDING_EXIT\`, \`INCOMPATIBLE\`, \`FAILED_RETRY\`, \`ROLLBACK_AVAILABLE\`. Show actual bytes transferred, revision and concise failure reason.

1. Only shell/service downloads and verifies, off the VR render thread.
2. Whitelist server and release URL; enforce timeouts, bounded backoff, cache, content length and available storage. Never mark installed on a successful HTTP status alone.
3. Validate signed catalog and archive SHA-256; hard cap ZIP entry count, per-file/total decompressed size and compression ratio; reject absolute/traversal paths, duplicate names, symlinks and undeclared file types.
4. Stage immutable revision under private \`module-packs/<id>/releases/<revision>/\`; verify actual decoded asset formats. Atomically switch active manifest **only after the game exits**; keep last known good revision.
5. Open a module-scoped, read-only asset resolver: bundled baseline first/last according to explicit manifest policy, installed active content when valid, never arbitrary files or another game's root. No partial package becomes visible.
6. On crash, loss of power, bad migration or failed GL/model decode: restore prior active manifest; never delete user saves as part of cleanup. Disk quota and manual content uninstall exclude saves by default.

An update of Red Ledger textures must not change Breakwater files. Shared Kenney/Beckenburg content may be de-duplicated later through immutable, hash-pinned shared packs and reference counts, but the first release should use **only required, curated content** within the game's own archive. Owned/authorized game art is not categorically barred from GitHub; record its actual provenance and rights, with no unnecessary full-library republication.

## Companion APK handoff and saves

The hub uses an explicit known package name and Activity, verifies signature plus supported protocol, then saves shell navigation/calibration context and leaves/suspends its VR surface. Companion starts its own headtracking, controller, 3DoF gaze fallback, audio, GLES2 and safe exit routines built from shared runtime source. On Exit, checkpoint and return through a guarded handshake; hub should recover even if companion crashes or is force-killed.

Companion updates use Android Package Installer, **never silent native binary patching**. 2D setup exposes installation permissions/prompts without making headset controls unusable. No sensitive save bytes or arbitrary filesystem paths in Intents; exported interfaces permission- and signature-gated.

Current game saves are private to the hub. On migration to companion package, build a one-time checksum-verified, signature-protected import/export stream (e.g. gated ContentProvider), keep original hub copy/backups, version the migration, and never overwrite progress without explicit conflict resolution. Offline launch must work after installation.

## User experience

Use a **compact scrollable library list**, not sprawling per-game tabs/cards. Each row shows title, installed/released version, download size/progress and a single contextual **Play / Download / Update / Retry** button. A small detail panel offers **Check updates**, release notes, **Manage content**, **Rollback**, **Uninstall content**, and installed size. Separate **Update ReverieVR** from **Update [game]**. A game can open its own update screen through the shell; do not give every native C++ module its own arbitrary network updater.

Fully operable in phone 2D setup and headset VR, with head-gaze dwell fallback. No online checks blocking launch. If offline say **Offline; last checked [date]**, not **Up to date**. Incompatible releases show exact dependency without offering a broken install. Respect game-active, GL-context and thermal constraints when switching content.

## Delivery sequencing and acceptance

**Phase 0: inventory and tests.** Freeze IDs, ABI and saves; catalog installed/bundled module descriptors; add host-update regression for module tags. **Gate:** all existing bundled games still launch with correct inputs and saves.

**Phase 1: Red Ledger real-assets fix.** Correct \`HostReadAsset\` path checks, stage the actual 14 Beckenburg models, verify loaded count and rendered bar/stockroom/living quarters on Galaxy S9. **Gate:** actual screenshots and gameplay, not merely manifest references.

**Phase 2: first standalone content release.** Implement \`RvPackReader\`, signed catalog test fixture, atomic module asset resolver, installation/rollback and owner-asset provenance; publish two Red Ledger content revisions in the testing channel. **Gate:** download one game on an older APK, activate after exit, relaunch offline, reject malformed/downgraded/bad-signature content, survive disk-full/power-loss, preserve saves.

**Phase 3: multi-game catalog and UI.** Move Breakwater and Omega Front artwork/scenarios into different \`.rvpack\` releases; integrate game-specific status and Check actions into VR and 2D; ensure three games have independent installed versions. **Gate:** one game's update/failure changes neither of the others nor the hub updater.

**Phase 4: native code proof.** Build independent Test Chamber companion APK using shared VR runtime; verify explicit discovery, signature check, stereo, controller, sound, recenter, save, app switching, home/recovery and code-only update on physical S9. **Gate:** no hand-waved \`.so\` download, no breakage of shell safety. If unacceptable, keep Track A and reconsider split APKs with measured proof.

**Phase 5: native game migration.** Migrate Red Ledger, Breakwater, Omega Front, Gumball Armada, Ministry/Forward Detachment, then classic titles, verifying each game's performance and saves. Retain originals until import and rollback gates pass. **Gate:** native code update to one game does not require hub/new versions of other titles.

**Phase 6: slim hub.** Once independent APKs work, reduce base APK to core, Test Chamber, recovery and approved baseline. Optional games downloadable; cache storage and uninstall preserve saves. **Gate:** clean download, update, offline replay, rollback, crash recovery on Galaxy S9.

## Reproducible, non-Actions release operation

1. Check out verified \`main\` commit; record source SHA, toolchain, target ABIs, selected game and release type.
2. Build exactly one game pack or companion APK using local pinned scripts, Android Gradle/NDK and approved asset inputs. Do not require the complete Beckenburg library.
3. Run native host tests, archive-validation tests, signature tests, content decode tests, APK ELF/entry/signer checks, and in-game smoke test.
4. Prepare a **draft** GitHub release with specific module tag, binary package, digest/signature, changelog, provenance/notices and any required corresponding source. No new GitHub Actions.
5. Upload, read back remote assets and recompute/confirm checksum/size; retry transient service/timeouts with bounded backoff and detect conflicts. Do not claim published until remote acceptance is verified.
6. Publish immutable release; sign/publish updated catalog on \`main\` **only after** release assets exist, verifying catalog from an independent client read.
7. On S9 check that specific game's update, install, launch and rollback, including headset navigation, controller, sound and thermals. Promote to stable only after testing; keep last known good version.

Report each gate as one of **source committed / release uploaded / installed / rendered on S9**, never conflate them. Log \`CATALOG_FAILED\`, \`SIGNATURE_REJECTED\`, \`DIGEST_MISMATCH\`, \`ZIP_INVALID\`, \`NO_SPACE\`, \`INSTALL_STAGED\`, \`INSTALL_ACTIVE\`, \`COMPANION_INCOMPATIBLE\`, \`ROLLBACK\` and \`LAUNCH_FAILED\` without sensitive data.

**First executable slice:** Red Ledger Beckenburg-path repair, then offline metadata/catalog parser + tests, then immutable signed \`.rvpack\` installer + fallback, then headset UI, then a verified actual Red Ledger content release. Do not start dynamic-code loading before that pipeline is independently working.
