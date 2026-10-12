# ADR-0008 — Local Media via SAF, MediaPlayer, and External OES

**Status:** adopted as draft implementation boundary  
**Date:** 2026-10-05  
**Affected targets:** RV-0500, RV-0501, RV-0502, RV-0503

## Context

Local video playback is one of ReverieVR's primary revival goals, but the normal construction order places the media player after the shell and module-host foundations are stable. The project also needs an early real workload that exercises Cardboard/OpenGL surface lifecycle behavior, Android decoding, frame delivery, pause/resume, and sustained thermal load on the Galaxy S9.

A fake Media tile or a player that depends on broad storage permission would violate the project contract.

## Decision

Implement a bounded local-media vertical slice before device acceptance of the earlier phases, while leaving all acceptance gates intact.

Local files are selected with Android's Storage Access Framework using `ACTION_OPEN_DOCUMENT`. ReverieVR persists read permission only for the selected document URI and releases the previous persisted grant when the selection is replaced or cleared. No broad media-library or filesystem permission is required for this path.

Playback uses Android `MediaPlayer` asynchronously. Decoder video output is attached to an OpenGL ES `GL_TEXTURE_EXTERNAL_OES` texture through `SurfaceTexture`.

The initial projection set is:

- ordinary flat video on a virtual cinema quad;
- mono equirectangular 360° video on an inward-viewed sphere.

The VR shell remains authoritative. Selecting media from the shell enters the player mode; Daydream primary click toggles play/pause; Menu/back stops playback and returns to the same shell. GL-surface recreation replaces the decoder surface and reopens the selected URI rather than retaining a dead SurfaceTexture.

## Spatial metadata extension (2026-10-11)

Following the initial local player, inspection of the selected document's
ISO-BMFF (MP4) Spatial Media v2 video sample-entry metadata is performed in a
bounded (4 MiB maximum) asynchronous, offline parser. It recognizes st3d
packing and uncropped, unrotated sv3d/proj/equi 360-degree projection only.
The result changes the default display mode solely for the same selected
content and only if the user has not since changed that mode; unsupported or
incomplete metadata is not presented as playable VR180 or calibrated fisheye.
No scans of mdat payload for incidental strings.

The 2026-10-11 follow-on adds a seekable SAF descriptor path: for a provider
supporting file-channel positioning, parse top-level MP4 box headers only,
skip mdat by its declared 32-bit or 64-bit length and read complete moov
boxes of at most 4 MiB even when stored behind gigabytes of media data.
Never read mdat payload to infer embedded boxes. Limit the number of top-level
boxes inspected to 1024 to avoid adversarial headers. Invalid/truncated
containers or oversized moov are UNKNOWN rather than silently guessed.
SAF pipe-backed descriptors fall back to the original, bounded 4 MiB prefix
inspection; seek support is not assumed for all providers. No new storage
permission, native codec dependency, or background file import is required.
Device, codec, memory and performance acceptance remain separate gates.

## Deferred

This decision does not claim:

- SBS or over/under stereoscopic video;
- 180° projection;
- seeking or a full media library;
- subtitles;
- broad codec compatibility;
- sustained thermal or comfort acceptance.

Those remain follow-on work under the existing media backlog.

## Why implement this early?

The early slice is useful as a validation fixture rather than a leapfrog in acceptance. It exercises:

- real local-content ownership without Google services;
- Cardboard/OpenGL surface recreation;
- Android decoder-to-GL handoff;
- sustained phone decode + stereo render load;
- shell-to-module-like lifecycle and recovery.

Failure in any of those areas should be discovered before the platform contract grows around a bad assumption.

## Acceptance implication

RV-0500, the mono-360 portion of RV-0501, and the basic portion of RV-0503 may be marked **draft** only. They cannot become device accepted until the APK builds and the relevant behavior is tested on the Galaxy S9 + Daydream View.
