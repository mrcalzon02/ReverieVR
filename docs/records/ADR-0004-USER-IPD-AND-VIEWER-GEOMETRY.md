# ADR-0004 — Separate Physical Viewer Geometry from User IPD Calibration

**Status:** Accepted  
**Date:** 2026-10-04

## Context

The Daydream View has fixed physical lenses with no mechanical interpupillary-distance adjustment. Different users have different IPDs and may not naturally align with the optical sweet spot.

The Cardboard rendering stack also has an `inter_lens_distance` viewer parameter. In Cardboard's lens-distortion implementation, that physical viewer value is used to construct the eye-from-head transforms.

It would therefore be technically easy, but architecturally wrong, to treat the viewer's lens spacing and the wearer's IPD as the same setting.

## Decision

ReverieVR maintains **two separate optical profiles**.

### Viewer optical profile

Describes physical hardware:

- fixed lens-center separation;
- screen-to-lens geometry;
- distortion coefficients;
- field of view;
- tray/phone positioning and other viewer geometry supported by the runtime.

The viewer profile is hardware calibration and is not changed merely because a different person wears the headset.

### User eye profile

Describes the wearer:

- user IPD in physical units;
- user-specific stereo eye/camera baseline as supported by the renderer;
- optional left/right horizontal alignment correction;
- optional vertical correction only if reference-device testing demonstrates a real benefit;
- calibration metadata and reset/default state.

The user layer is applied on top of the physical viewer profile.

## Calibration UX

The user must be able to calibrate while actually looking through the headset.

The reference flow provides:

- a high-contrast binocular fusion/alignment target;
- large legible values;
- controller-based incremental adjustment;
- immediate visual feedback;
- an obvious reset/default action;
- a way to exit calibration without committing a worse result;
- persistence only after confirmation.

Exact numeric adjustment bounds are not guessed in advance. They will be constrained from Cardboard/viewer geometry and measured on the Galaxy S9 + Daydream View so the UI cannot drive the rendered eyes into nonsensical or unsafe placement.

## Limitation

Software cannot physically move the Daydream View lenses.

Virtual IPD/alignment can correct stereoscopic camera geometry and rendered image placement, reducing some convergence/alignment discomfort. It cannot guarantee sharp optics for a wearer whose pupils physically sit outside the useful lens sweet spot.

The UI must state this honestly rather than implying that software adjustment is equivalent to mechanical lens movement.

## Cardboard integration consequence

Cardboard's physical `inter_lens_distance` remains part of the **viewer** profile. ReverieVR's user-IPD layer must not silently mutate or persist over that hardware value.

If the chosen Cardboard integration does not expose a clean user-eye transform hook, ReverieVR should apply the user correction in its own render/view transform layer rather than falsifying viewer parameters.

## Acceptance consequence

A valid implementation must demonstrate on reference hardware that:

1. viewer-profile values remain unchanged while user IPD is adjusted;
2. adjustment changes the intended per-eye view transform;
3. reset restores the known-safe user default;
4. settings persist across sessions;
5. extreme controls are bounded so they cannot create unrecoverable display placement;
6. calibration can be operated entirely in-headset.
