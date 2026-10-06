# ADR-0016 — GitHub Actions Phone-Test Releases

**Status:** accepted  
**Date:** 2026-10-05

## Authorization

The project's default prohibition on GitHub Actions remains in force except where explicitly authorized. The user explicitly authorized GitHub Actions for producing installable ReverieVR phone-test releases.

## Decision

ReverieVR may use the committed `.github/workflows/phone-test-release.yml` workflow to:

- build on GitHub-hosted Ubuntu with Java 17, Android SDK 36, NDK 29.0.14206865 and CMake 3.22.1;
- initialize the pinned Cardboard submodule;
- fetch and verify pinned DOSBox Pure and the approved DOOM Shareware v1.9 payload;
- run the committed bootstrap verifier and JVM tests;
- assemble the headset and controller debug APKs;
- verify both native ABIs and required ReverieVR/DOS libraries are present in the headset APK;
- verify Android APK signing;
- publish a GitHub prerelease with APKs and SHA-256 checksums.

Normal source pushes do not automatically create phone-test releases. Ordinary future builds are invoked deliberately through `workflow_dispatch`; the path-limited trigger file exists for controlled repair/retry runs.

## Signing boundary

Early phone-test APKs used ephemeral debug signing and therefore could not form a reliable update lineage. Beginning with the persistent phone-test signing cutover, published phone-test APKs are signed by one protected repository-secret-backed distribution identity; the private key is never committed to the repository. GitHub Actions must fail rather than publish if that identity is unavailable, and release verification must read back the signer fingerprint before publication.

Android itself remains the final authority for in-place signer continuity. ReverieVR may preflight a downloaded APK's signer when PackageManager exposes archive-signing metadata correctly, but OEM/platform archive parsers are not allowed to create a false rejection: after trusted GitHub asset URL, release SHA-256, package ID, monotonically newer versionCode, tag/versionCode binding, and installed-app pinned-signer checks pass, missing archive-signer metadata is deferred to Android Package Installer, which independently rejects any APK that is not authorized to replace the installed package.

Builds from the old ephemeral-signing era cannot be upgraded in place to the persistent lineage unless their original private signing key is available. Such builds require a one-time uninstall/reinstall migration.

## Acceptance boundary

A successful cloud run proves compilation, linking, tests, packaging and signature verification. It does not prove Galaxy S9 runtime behavior, headset optics, controller behavior, comfort, sustained frame pacing or thermal acceptance.

## First successful evidence

GitHub Actions run #6 built commit `c70026f8e5bd307bd5f53d46c862a87b629bff20` and published prerelease `phone-test-6-1` with both APKs and checksums.
