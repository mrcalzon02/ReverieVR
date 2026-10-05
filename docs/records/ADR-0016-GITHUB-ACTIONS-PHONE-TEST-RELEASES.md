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

Phone-test APKs are debug-signed for sideload/reference-hardware validation. This is not production/update signing. A persistent protected signing identity must be introduced before reliable in-place upgrades or production distribution. No signing private key is committed to the repository.

## Acceptance boundary

A successful cloud run proves compilation, linking, tests, packaging and signature verification. It does not prove Galaxy S9 runtime behavior, headset optics, controller behavior, comfort, sustained frame pacing or thermal acceptance.

## First successful evidence

GitHub Actions run #6 built commit `c70026f8e5bd307bd5f53d46c862a87b629bff20` and published prerelease `phone-test-6-1` with both APKs and checksums.
