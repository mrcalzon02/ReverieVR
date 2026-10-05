# ADR-0008 — Local Media via SAF, MediaPlayer, and External OES

**Status:** adopted as draft implementation boundary  
**Date:** 2026-10-05  
**Affected targets:** RV-0500, RV-0501, RV-0503

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
