# ADR-0005 — Versioned First-VR-Run Setup and Calibration

**Status:** Accepted  
**Date:** 2026-10-04

## Context

Several ReverieVR settings are only meaningful when the user is actually wearing the headset: neutral forward direction, binocular alignment, text readability, HUD placement, controller ergonomics, and comfort preferences.

Putting all of these only in the flat Stage A menu would require the user to repeatedly remove the phone from the headset, guess at a value, reinsert it, and try again. Dropping a new user directly into the normal VR home would also leave critical comfort and recovery behavior unexplained.

## Decision

The first successful transition into VR launches a **guided, in-headset setup wizard** before normal home use.

The wizard is part calibration, part control tutorial. It configures a persistent **user profile** without mutating the separate physical viewer profile.

Baseline steps:

1. seated neutral posture and recenter;
2. controller orientation and input familiarization;
3. viewer-profile confirmation;
4. user IPD and per-eye alignment;
5. UI/text readability;
6. global HUD and power-status behavior;
7. comfort defaults;
8. audio baseline;
9. performance/thermal preference;
10. completion summary and recovery instructions.

## Safety and recoverability

- Every step has a known-safe default.
- Noncritical steps may be skipped or deferred.
- A partially completed setup resumes from the last safe checkpoint.
- A user may cancel an optical change before it becomes persistent.
- A bad calibration never removes the route back to Stage A recovery.
- The complete wizard and each individual calibration tool remain accessible later from Settings.

## Versioning

The onboarding state carries a setup-version identifier.

When ReverieVR adds a new critical calibration in a later release, it may offer only the newly required step(s) instead of forcing an established user to repeat the entire original wizard.

Changing ordinary preference defaults does not automatically invalidate completed onboarding.

## Acceptance

A valid implementation must demonstrate that:

1. first VR entry routes to setup when required;
2. setup can be completed, skipped, interrupted, resumed, and re-run;
3. completed settings persist;
4. individual calibration pages can be reopened directly;
5. user-profile calibration does not overwrite physical viewer geometry;
6. the user can always escape to a safe VR home or Stage A recovery path.
