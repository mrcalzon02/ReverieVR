# ADR-0018 — Persistent APK signing and GitHub-hosted self-updates

Status: **implemented in source / permanent-key provisioning and device proof pending**

## Context

ReverieVR is distributed directly as APKs from the project's GitHub Releases page. The existing phone-test workflow originally relied on the Android debug keystore generated on each ephemeral GitHub-hosted runner. Physical package evidence proved that different phone-test runs therefore had different signing-certificate fingerprints. Android correctly refuses to install one of those APKs over another as an in-place update.

The project also initially queried GitHub's `/releases/latest` endpoint while the phone-test packages are prereleases. GitHub explicitly defines "latest" as the most recent non-draft, non-prerelease release, so that endpoint cannot represent the phone-test channel.

The goal is a private, durable update channel in which a Galaxy S9 can install one deliberately provisioned ReverieVR APK once and then accept later GitHub-hosted ReverieVR builds without uninstalling the application or losing application data.

## External evidence

Android's update contract is authoritative:

- the application ID must match the installed package;
- the signing certificate must match the installed package (or use a platform-valid signing-certificate rotation);
- the new APK's version code must be at least the installed version's version code.

Android recommends using the same signing certificate throughout the application's expected lifetime and warns that losing a self-managed app-signing key prevents future seamless updates.

References:

- https://developer.android.com/studio/publish/app-signing
- https://developer.android.com/google/play/app-updates
- https://developer.android.com/reference/android/content/pm/PackageManager#canRequestPackageInstalls
- https://developer.android.com/reference/android/content/pm/SigningInfo
- https://developer.android.com/reference/android/content/pm/PackageManager#getPackageArchiveInfo(java.lang.String,int)

NewPipe documents the same operational consequence in a mature direct-distribution Android project: its official GitHub release APKs remain mutually update-compatible because they use the same signing key, while CI/debug APKs produced with random signing identities cannot update one another.

Reference:

- https://github.com/TeamNewPipe/NewPipe
- https://github.com/TeamNewPipe/NewPipe/blob/dev/.github/workflows/build-release-apk.yml

Obtainium independently demonstrates the viability of installing and updating Android applications directly from upstream GitHub Releases rather than requiring an application store.

Reference:

- https://github.com/ImranR98/Obtainium

GitHub Actions supports storing a small binary keystore as a Base64-encoded Actions secret and reconstructing it on the runner. Actions secrets are limited to 48 KB; GitHub documents an encrypted-file + secret-passphrase pattern for larger material.

References:

- https://docs.github.com/en/actions/how-tos/write-workflows/choose-what-workflows-do/use-secrets
- https://docs.github.com/en/actions/reference/security/secrets

GitHub's Releases API exposes public release metadata without requiring a token for public repositories. `/releases/latest` excludes prereleases, while `/releases` includes published prereleases. Current release-asset responses also expose a SHA-256 `digest` field.

Reference:

- https://docs.github.com/en/rest/releases/releases

## Decision

### 1. Create one dedicated ReverieVR distribution signing identity

Generate exactly one long-lived signing key for the direct GitHub distribution channel. It is not a disposable debug key.

The key must:

- have a validity period comfortably longer than ReverieVR's expected lifetime;
- be backed up offline in at least two physically separate trusted locations;
- never be committed to the repository in plaintext;
- never be bundled into the APK;
- never be printed into Actions logs.

The certificate itself and its SHA-256 fingerprint are public information and should be recorded in project documentation and release metadata. The private key remains secret.

### 2. Use a non-debuggable distribution build

Long-lived GitHub-distributed APKs must not be ordinary Android `debug` builds merely re-signed with the persistent key.

Create a dedicated `phoneTest`/distribution build type (or use a suitably configured release build) that:

- is `debuggable false`;
- uses the dedicated persistent signing config;
- retains ReverieVR's explicit Standard/Development diagnostic logging modes;
- carries the phone-test build identity in BuildConfig;
- enables the in-app updater only when persistent signing is active.

Ordinary local/debug builds may continue using disposable debug signing, but they must not advertise an in-app update channel.

### 3. Give every published APK a strictly increasing version code

The current workflow uses only `GITHUB_RUN_NUMBER` as `versionCode`. A rerun attempt can therefore share a version code with a previous APK.

The distribution channel must compute a strictly monotonic version code that incorporates a stable release sequence and the attempt/build sequence. The exact formula must remain below Android's version-code limit and must not reset merely because a workflow file is recreated.

Release metadata must include:

- Android application ID;
- Android versionCode;
- versionName;
- phone-test tag/run/attempt;
- source commit;
- signing-certificate SHA-256 fingerprint;
- APK SHA-256 digest.

### 4. Keep GitHub Releases as the public update feed

The APK contains no GitHub token.

The updater queries the public `GET /repos/mrcalzon02/ReverieVR/releases` endpoint, filters the desired phone-test channel, ignores drafts, and chooses the highest valid newer phone-test build rather than trusting list position.

Do not use `/releases/latest` for the phone-test channel because GitHub intentionally excludes prereleases from that endpoint.

The existing trusted URL restriction to the ReverieVR GitHub release path remains.

### 5. Verify the downloaded APK before invoking Android's installer

A downloaded APK is accepted only when all checks pass:

1. HTTPS URL belongs to the expected ReverieVR GitHub release asset path.
2. GitHub release metadata contains a recognized SHA-256 asset digest.
3. The downloaded bytes exactly match that digest.
4. Android can parse the downloaded APK as a package archive.
5. The archive package name is exactly `io.github.mrcalzon02.reverievr`.
6. The archive versionCode is strictly newer than the installed distribution build.
7. The archive signing certificate matches the currently installed ReverieVR signing certificate, including valid Android signing-history semantics where applicable.
8. Optionally/defensively, the certificate also matches the project-pinned ReverieVR distribution certificate fingerprint.

Any missing or malformed security metadata fails closed. The updater must not treat an absent/unrecognized digest as permission to continue.

Android 9+ exposes `SigningInfo` and signing-certificate history; ReverieVR's Galaxy S9 / Android 10 reference target can therefore perform this check directly.

### 6. Use Android's supported package-install path

ReverieVR declares `REQUEST_INSTALL_PACKAGES` and checks `PackageManager.canRequestPackageInstalls()`.

On Android 8+, the user controls whether ReverieVR is trusted as an "install unknown apps" source. If not trusted, ReverieVR opens the appropriate Android settings page and explains why.

For the Galaxy S9 / Android 10 target, "persistent update" means a valid in-place replacement that preserves the application and its data. It does not promise an unauthorized silent install; Android may still require normal package-installer user confirmation.

### 7. Perform one unavoidable signing transition

Existing phone-test APKs were signed with ephemeral runner debug keys. They cannot be transformed into the new signing lineage.

The first persistently signed distribution APK therefore requires one explicit migration:

1. uninstall the old ephemeral-debug-signed ReverieVR build;
2. install the first persistently signed distribution APK manually;
3. grant ReverieVR permission to request package installs when first needed;
4. verify the installed signing fingerprint and version code;
5. install the next persistently signed package through ReverieVR's updater;
6. prove application data/preferences survive the update.

After that transition, no routine uninstall should be required.

## GitHub Actions provisioning

The existing workflow already expects these four values:

- `PHONE_TEST_KEYSTORE_BASE64`
- `PHONE_TEST_STORE_PASSWORD`
- `PHONE_TEST_KEY_ALIAS`
- `PHONE_TEST_KEY_PASSWORD`

Recommended provisioning:

1. Generate the keystore locally/offline, never on an ephemeral CI runner.
2. Record the public certificate fingerprint.
3. Make two offline backups of the original keystore.
4. Base64-encode the keystore only for transport into GitHub Actions secrets.
5. Store the four values as GitHub Actions environment or repository secrets.
6. Reconstruct the keystore only under `$RUNNER_TEMP`, mode 0600.
7. Delete/allow runner teardown to destroy the reconstructed file after the job.
8. Verify the produced APK signer fingerprint in CI and fail if it does not equal the expected distribution fingerprint.
9. Publish that verified APK and its checksum metadata.

If the encoded keystore exceeds GitHub's 48 KB secret limit, follow GitHub's documented encrypted-file workaround instead of weakening the key or committing it in plaintext.

## Security boundary

The GitHub repository and updater may know:

- certificate fingerprint;
- release metadata;
- APK digest;
- release URL.

The APK must never contain:

- the private signing key;
- keystore password;
- GitHub write token;
- Actions secret;
- reusable privileged credential.

The private signing key exists only in offline backup storage and transiently inside the protected signing job.

## Implementation status

Completed in source/workflow:

- GitHub-distributed handset and controller packages use a dedicated non-debuggable `phoneTest` build type.
- CI refuses to publish when any persistent-signing secret is missing.
- Android versionCode is `GITHUB_RUN_NUMBER * 1000 + GITHUB_RUN_ATTEMPT`, preventing rerun collisions.
- CI derives the certificate SHA-256 from the supplied keystore before the build, compiles that public fingerprint into the handset package, and verifies both produced APKs use that exact certificate.
- GitHub release asset SHA-256 is mandatory and fails closed.
- The downloaded APK is copied privately for verification before Android's installer sees it.
- Package ID, strictly newer versionCode, tag-to-versionCode mapping, installed signer, candidate signer, and pinned signer are all checked.
- Release discovery selects the numerically highest valid phone-test release instead of trusting API order.
- The updater uses the current project GitHub REST API version header.
- Windows and Unix/macOS one-time provisioning scripts generate the permanent key locally, stage the four secret values in an ignored directory, optionally populate GitHub with authenticated `gh`, and optionally trigger the first signed build.
- `.local/reverievr-signing/` is ignored to prevent accidental key/password commits.

Remaining external acceptance work:

1. Run the provisioning script on the repository owner's machine and back up the generated keystore/secrets in two trusted offline locations.
2. Populate the four GitHub Actions signing secrets (the script can do this through authenticated `gh`).
3. Produce the first persistently signed package and perform the one-time manual migration on the Galaxy S9.
4. Publish one additional build and prove a real in-app update-over-update with settings/data preserved.

## Provisioned distribution identity

The permanent ReverieVR direct-distribution signing identity is now provisioned.

Public certificate SHA-256:

`464dc2d100c3bb14deac1da680d29418cc22e60e1857852cb54541f7d7787f7c`

First proven permanent-lineage package:

- release: `phone-test-36-1`
- Android versionCode: `36001`
- headset APK SHA-256: `fd80f4b4b6a4f3864342d593a7aafce03335627bd63fa2f9ba62fecff572738d`

This fingerprint is public identity metadata. The corresponding private key remains outside the repository in the owner's protected backup and GitHub Actions secret storage.

## Acceptance

Persistent GitHub-hosted updates are accepted only when two consecutively published ReverieVR distribution APKs:

- have the same application ID;
- have the same verified signing identity;
- have strictly increasing version codes;
- are discoverable by the in-app GitHub release checker;
- pass APK SHA-256 and signing-identity verification;
- install as an update through Android without uninstalling the prior stable-signed build;
- preserve ReverieVR preferences/data;
- leave no signing secret or privileged GitHub credential in the APK.
