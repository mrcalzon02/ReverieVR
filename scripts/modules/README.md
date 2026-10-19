## Independent native game packaging and deliveries

The first independent native executable is now a real Gradle application module, **`:companion-test-chamber`**:

```sh
./gradlew :companion-test-chamber:assembleDebug
# Or, with the SAME persistent distribution signing identity as the hub:
./gradlew :companion-test-chamber:assembleRelease \
  -PphoneTestKeystorePath=/secure/reverievr-phone-test.jks \
  -PphoneTestStorePassword=... -PphoneTestKeyPassword=... \
  -PphoneTestKeyAlias=... \
  -PtestChamberVersionCode=1 -PtestChamberVersionName=0.1.0
```

Avoid putting secrets into shared terminal history, CI output or git; supply the configured signer through secure local mechanisms where possible. Never rotate the hub signer to make a companion package install.

Target app ID: `io.github.mrcalzon02.reverievr.game.testchamber`. The Gradle module builds the existing procedural Test Chamber `reverie_native_module_entry_v1` source plus a JNI adapter and Cardboard stereo Activity. Verify both ABI-specific ELF objects, Android manifest package, signature lineage, Daydream eye rendering, trigger, pause/resume and return-to-hub on a Galaxy S9. **Both native companion APK families have passed build and certificate checks in GitHub Actions; a physical Galaxy S9 acceptance run is still required.** It is a separate test executable, not an updater hot-loading arbitrary native files.

# Independent native game releases — operator tools

**Current state (2026-10-18):** signed catalog verifier, bounded game downloads, private transactional asset installer and rollback, compact game manager, and independent Test Chamber and Breakwater APKs have passed CI build/admission tests. All 15 game IDs have verified content packager support; the real Breakwater model revision was read by its native game on Android. The public signed catalog still contains no independent game releases. Galaxy S9 stereoscopic/audio/controller testing is separate.

GitHub Actions is now explicitly authorized for emulator validation and signed companion-build artifacts; the release operator still verifies exact payloads and catalog signatures before publication. Continue on `main`.

## 1. Build one real Red Ledger data pack

Select only the game-use models from the approved Beckenburg library, after converting them to validated \`RLBP\` v1 (\`.rlprop\`) with the existing private converter. Point the packer at the folder containing exactly the required, flat \`.rlprop\` models (not the full source asset archive):

\`\`\`sh
python3 scripts/modules/build-content-pack.py \
  --id between-deliveries-red-ledger \
  --source .local/beckenburg/selected-red-ledger-models \
  --output .local/releases/red-ledger-interiors-v1.rvpack \
  --revision 1 --version 0.1.0
\`\`\`

For Red Ledger \`.rlprop\` files the builder creates ZIP entries at \`assets/native/red-ledger/beckenburg/<name>.rlprop\`, matching the native \`read_asset\` calls. It rejects unapproved extensions and malformed mesh headers. The runtime decodes geometry again before GL upload.

The existing Stage A **Manage Game Modules → Import .rvpack** supports explicitly selected, content-only archives without a network or publisher key. This is a local user-consent path for testing; the app computes the selected archive SHA-256, verifies the ZIP, and atomically activates that game's revision. It neither installs native executable code nor changes the hub. The game sees the new content after next launch. Preserve room save data.

Verify input content and license/provenance before publishing a distributable build; commercial game rights do not automatically imply a blanket grant to reuse the standalone source pack.

## 2. Configure authenticated release catalog (operator machine only)

First generate the publisher key **outside the Git checkout**. Do not store its private key in Google Drive, GitHub, the APK, source archives, logs or user issue threads:

\`\`\`sh
python3 scripts/modules/sign-catalog.py init \
  --private /secure/offline/reverie-module-publisher.pem \
  --public-der app/src/main/assets/native/modules/catalog-key.der
\`\`\`

Securely back up the private key. The **public DER** file is a harmless app asset and must be included in each hub APK that will check this publisher's releases. For any stable launch, use a deliberately provisioned long-lived operator key, not a temporary test key.

Once a game asset is uploaded to a **draft** release in \`mrcalzon02/ReverieVR\`, independently download it and calculate the actual SHA-256 and byte count. Write \`modules/catalog-source.json\` with \`schema=1\`, a strictly increasing positive \`catalog_revision\`, and a \`modules\` array. Each entry uses exactly the public GitHub URL and fields \`id\`, \`kind\` (\`content\` or \`companion-apk\`), \`channel\`, \`version\`, \`revision\`, \`size_bytes\`, \`sha256\`, \`url\`, and optional short \`notes\`.

After remote readback and before offering the release:

\`\`\`sh
python3 scripts/modules/sign-catalog.py sign \
  --source modules/catalog-source.json \
  --catalog modules/catalog-v1.json \
  --signature modules/catalog-v1.sig \
  --private /secure/offline/reverie-module-publisher.pem

python3 scripts/modules/sign-catalog.py verify \
  --catalog modules/catalog-v1.json \
  --signature modules/catalog-v1.sig \
  --public-der app/src/main/assets/native/modules/catalog-key.der
\`\`\`

Publish both exact signed catalog bytes on \`main\`; the app checks \`https://raw.githubusercontent.com/mrcalzon02/ReverieVR/main/modules/catalog-v1.json\` and \`.sig\`. The catalog's hashes point to **immutable**, versioned GitHub Release assets. Do not generate a catalog entry before the matching release bytes can be read back remotely. Do not mutate an existing release binary under its old hash.

\`GameModuleUpdateService\` never accepts unsigned online catalogs. If the pinned key is absent, online check reports **Publisher key not configured**; local manual pack import still works.

## 3. First acceptance cycle

1. Build/install a new ReverieVR APK containing the Stage A manager and patched native host.
2. Verify the native decoder and 14 selected Red Ledger meshes; import revision 1 from the phone's local file picker.
3. Enter Red Ledger. Confirm the live log shows 14/14 loaded, then inspect bar, stockroom and living-quarters models and renovation variants in stereoscopic Daydream.
4. Install a second higher revision of Red Ledger content, keeping the **same hub APK**; verify next-launch update, offline play and rollback.
5. Test corrupted ZIP, traversal, wrong module path, wrong SHA, duplicate revision and interruption. Confirm other games' assets and **all** saves remain intact.
6. Only after the release source key, remote upload, remote readback and S9 checks succeed, publish a testing-channel game release and verify per-game online Download/Update. Keep \`phone-test-*\` hub releases independent.

## 4. Native C++ code releases

The \`CompanionGameRegistry\` currently recognizes known package IDs and requires the installed game APK to have **the same Android signing certificate** as ReverieVR and an explicit exported \`GameVrActivity\`. Independent APKs exist for Test Chamber and Breakwater. The hub's actual Android emulator integration recognizes installed matching-signer companions; do not remove the bundled implementations until their physical-device lifecycle and update transitions are proved.

Native code delivery should use Android Package Installer and user consent. A different Android package has a separate private data area and GLES context; save transfer, input/comfort parity and safe return to the hub must be tested explicitly. No downloaded writable \`.so\` execution workaround.

## 5. Verified technical limitations

The current connected GitHub file tools can commit text source and metadata, but have not published the local binary \`.rvpack\` as a GitHub Release asset. The Android SDK/NDK and physical Galaxy S9 were unavailable in the authoring environment. Tests performed locally on the selected 14 converted models and the C++ path classifier are recorded separately. **Phases 5 and 6 may not remove any bundled game or reset saves until the companion APK and rollback gates pass.**

## Verified release preflight and retry discipline

For content updates, the canonical release tag is `module-<full-game-id>-v<version>` (for example, `module-omega-front-v0.1.0`). The embedded `module.json` must match the tag's registered game ID and semantic version. The game revision is independent of both the game-code Android `versionCode` and the hub version.

For companion APKs, use the same canonical tag convention. The release preflight requires `aapt` and `apksigner` on the operator's PATH, verifies the exact registered Android application ID and `versionName`, and **requires** `--publisher-cert-sha256 <trusted-hub-certificate-fingerprint>` when invoking `publish-module-release.py`. Do not supply the SHA-256 of the APK file instead of the signing certificate. An APK for a game without an actual compiled companion target is refused.

The publish tool first validates the artifact and release tag, then reconciles existing GitHub draft state, uploads without overwriting existing names, re-downloads and hashes the exact bytes, and publishes only on explicit `--publish`. Separately, `sign-catalog.py` rejects unregistered games, mismatched GitHub release tags, unbuilt companion packages, duplicate module revisions and non-increasing catalog revisions. Its ECDSA private key remains **offline**. The public catalog must not advertise an asset before that release is published and verified; the hub uses the signed catalog, not individual GitHub Actions artifacts.

The Game Modules APK downloader now uses a unique destination per attempt (Android DownloadManager rejects an existing destination), verifies a private copy before invoking Android's package installer, and removes its downloaded staging file afterward. The Android installer still requires explicit user approval and matching signing identity. This prevents routine retries from getting stuck on an old APK filename or accumulating redundant 512 MB download files.

Validation entry points: `ReverieVR all-game content package contracts` checks all 15 game identities, real native art packs, publisher preflight, catalog signing controls and rollback; `ReverieVR hub game acquisition integration` installs actual APKs on Android and checks module acquisition and native execution; `Breakwater Battery independent APK validation` verifies the signed standalone release. These are evidence of CI scope only; a successful Android emulator is not physical Daydream acceptance.
