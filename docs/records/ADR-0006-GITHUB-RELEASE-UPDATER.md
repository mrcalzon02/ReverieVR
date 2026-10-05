# ADR-0006 — Authoritative GitHub Release Self-Update Path

**Status:** Accepted  
**Date:** 2026-10-04

## Decision

ReverieVR may use the network for an **optional update service** while keeping core VR operation offline-first.

The only authoritative update source is the public GitHub Releases feed for:

`mrcalzon02/ReverieVR`

The application must not accept update metadata or APK URLs from mirrors, arbitrary redirect configuration, advertisements, or third-party package indexes.

## User control

- Automatic **checking** at Stage A launch is user-toggleable.
- Manual **Check for updates** remains available.
- A discovered update produces an explicit **Update / Not now** decision.
- ReverieVR never performs a silent installation.
- Android's package installer remains the final installation authority.

## Release rules

A normal automatic update requires:

1. a published non-draft/non-prerelease release returned by GitHub's `releases/latest` endpoint;
2. a release tag newer than the installed application version;
3. an APK asset whose browser-download URL is under the authoritative repository's GitHub release-download path.

If no release exists, that is a normal state rather than an application error.

If a release has no APK, ReverieVR reports that honestly and does not invent an update path.

## Integrity

When GitHub exposes a `sha256:` digest for the selected release asset, ReverieVR verifies it before launching the package installer.

Android's normal package-update signature rules remain mandatory. A production release signing identity must therefore be kept stable and private outside the repository.

## Offline behavior

Failure to reach GitHub cannot prevent pairing, setup, VR entry, media playback, or local modules. Update checking is ancillary.
