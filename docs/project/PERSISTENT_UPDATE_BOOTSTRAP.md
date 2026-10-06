# Persistent ReverieVR Updates — One-Time Bootstrap

ReverieVR's GitHub-distributed Android packages now use a permanent signing lineage. This one-time bootstrap creates the signing identity locally and places the required values into GitHub Actions without committing private material.

## Windows — preferred path

Requirements:

- Windows PowerShell with the built-in PKI certificate cmdlets (standard on supported Windows installations), or an installed JDK/Android Studio if available
- GitHub CLI (`gh`)
- `gh auth login` completed for the account that can write Actions secrets to `mrcalzon02/ReverieVR`

The Windows bootstrap does **not** require Java. It first looks for an existing `keytool` (including Android Studio/JDK locations); if none exists, it falls back to Windows-native certificate tooling and exports the permanent signing identity as PKCS#12.

From the repository root:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\provision-phone-test-signing.ps1 -SetGitHubSecrets -TriggerBuild
```

The script:

1. creates one long-lived RSA-3072 ReverieVR distribution signing key using PKCS#12;
2. stores it only under ignored `.local/reverievr-signing/`;
3. generates strong random store/key passwords;
4. exports the public certificate and calculates its SHA-256 fingerprint;
5. writes the four required GitHub secret values to the ignored local `github-secrets.txt`;
6. sends those values directly to GitHub Actions through authenticated `gh` without printing them;
7. triggers the first persistently signed phone-test workflow.

The script refuses to overwrite an existing keystore by default. Do **not** use `-Force` unless deliberately abandoning the existing signing lineage.

## Linux/macOS

```bash
chmod +x scripts/provision-phone-test-signing.sh
./scripts/provision-phone-test-signing.sh --set-github-secrets --trigger-build
```

## Mandatory backup

Immediately copy both of these to two trusted offline locations:

- `.local/reverievr-signing/reverievr-phone-test.jks`
- `.local/reverievr-signing/github-secrets.txt`

Losing the keystore prevents future APKs from updating installations signed by it. Replacing the key would require another uninstall/reinstall migration.

Never commit either file.

## First signed installation

The currently installed pre-bootstrap ReverieVR APK belongs to the old ephemeral debug-signing lineage. Android cannot treat a differently signed APK as its update.

For the first persistently signed release only:

1. allow the triggered GitHub workflow to complete;
2. verify the release notes show `Persistent phone-test signing: enabled`;
3. uninstall the old ephemeral-signed ReverieVR APK;
4. install the first new persistently signed ReverieVR APK manually;
5. configure/confirm ReverieVR as an allowed "install unknown apps" source when Android asks.

This is the last routine uninstall expected for the signing transition.

## Proving persistent updates

After the first stable-signed build is installed:

1. trigger another phone-test release without changing any signing secrets;
2. open ReverieVR;
3. use Check for updates / Update;
4. ReverieVR downloads the newer GitHub release;
5. before Android receives it, ReverieVR verifies:
   - trusted GitHub release URL;
   - GitHub SHA-256 asset digest;
   - exact ReverieVR package ID;
   - strictly newer Android versionCode;
   - versionCode matching the `phone-test-<run>-<attempt>` tag;
   - downloaded APK certificate matching the permanent distribution signer;
   - installed ReverieVR certificate matching the same signer;
6. Android performs the in-place package replacement;
7. verify selected environment, settings, imported-module metadata, and other app data remain present.

## CI invariants

Published phone-test APKs now:

- use the non-debuggable `phoneTest` build type;
- require all four persistent-signing secrets;
- use Android versionCode `GITHUB_RUN_NUMBER * 1000 + GITHUB_RUN_ATTEMPT`;
- derive the signing certificate SHA-256 before building;
- compile that public fingerprint into the update verifier;
- verify the final APK signer equals that fingerprint;
- verify the companion controller APK uses the same signer;
- verify the APK versionCode before release publication;
- refuse to publish if any signing requirement is absent or mismatched.

The four GitHub Actions secrets are:

- `PHONE_TEST_KEYSTORE_BASE64`
- `PHONE_TEST_STORE_PASSWORD`
- `PHONE_TEST_KEY_ALIAS`
- `PHONE_TEST_KEY_PASSWORD`

The GitHub connector used by ChatGPT intentionally cannot access GitHub's secrets API. That security boundary is why initial key provisioning must happen from the repository owner's local machine or GitHub UI.
