# Independent native game releases — operator tools

**Current state (2026-10-17):** signed per-game catalog parser, bounded content importer, transactional asset resolver, Stage A manager, installed companion-package admission, and two local pack/signing tools exist in source. **No independent game release or companion APK has been published or device accepted.** Do not advertise these as finished S9 features until a working APK has been installed and rendered.

No GitHub Actions are required or to be added. Continue on \`main\`.

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

The \`CompanionGameRegistry\` currently recognizes known package IDs and requires the installed game APK to have **the same Android signing certificate** as ReverieVR and an explicit exported \`GameVrActivity\`. It does not yet build, install, or prove a companion APK. The separate tiny Test Chamber VR companion remains a physical-device implementation gate: do not replace the bundled games or slim the main APK based solely on package discovery.

Native code delivery should use Android Package Installer and user consent. A different Android package has a separate private data area and GLES context; save transfer, input/comfort parity and safe return to the hub must be tested explicitly. No downloaded writable \`.so\` execution workaround.

## 5. Verified technical limitations

The current connected GitHub file tools can commit text source and metadata, but have not published the local binary \`.rvpack\` as a GitHub Release asset. The Android SDK/NDK and physical Galaxy S9 were unavailable in the authoring environment. Tests performed locally on the selected 14 converted models and the C++ path classifier are recorded separately. **Phases 5 and 6 may not remove any bundled game or reset saves until the companion APK and rollback gates pass.**
