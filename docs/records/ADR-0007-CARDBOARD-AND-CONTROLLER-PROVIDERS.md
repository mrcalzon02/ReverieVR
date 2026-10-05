# ADR-0007 — Cardboard Rendering with Independent Controller Providers

**Status:** Accepted as implementation baseline  
**Date:** 2026-10-04

## Context

ReverieVR needs stereo rendering, phone head tracking, lens distortion and viewer profiles without reintroducing the discontinued Google VR Services runtime.

It also needs direct Daydream-controller support now while retaining the ability to add ordinary Android gamepads or other synchronized controllers later.

## Decision

### Rendering/head tracking

ReverieVR uses the open-source **Google Cardboard SDK v1.35.0**, pinned to commit:

`5969239e7c87f4cd64c8ec170ce1e7f4eb559e37`

Cardboard is used for:

- phone sensor head tracking;
- stereo eye projection;
- viewer geometry;
- lens distortion;
- Cardboard-compatible viewer profile support.

ReverieVR does **not** use Google VR Services as a runtime dependency.

The native Cardboard build is deliberately reduced:

- OpenGL ES retained;
- Vulkan disabled;
- Unity-plugin support disabled.

The physical Cardboard viewer profile remains separate from ReverieVR's user optical/IPD profile.

### Controller transport

Controller input is independent of Cardboard.

The initial provider is a directly implemented Android BLE backend for the Daydream controller. It is exposed through `ControllerProvider` / `ControllerManager` rather than referenced directly by the VR shell.

Future synchronized controllers or Android gamepads therefore add providers/action mappings instead of replacing the shell.

### Lifetime

The active controller manager is application-scoped so a controller connection survives the transition between:

- Stage A phone setup;
- Stage B VR shell;
- later ReverieVR-hosted modules.

Individual activities subscribe/unsubscribe as listeners and do not own the Bluetooth connection.

## Performance consequences

- No general-purpose game engine is required for the shell.
- The initial VR shell uses a minimal OpenGL ES world-space panel.
- No Vulkan or Unity bridge is built.
- Bundled modules remain responsible for meeting the platform performance budget.

## Validation gates

This decision remains draft in runtime terms until:

1. Gradle/NDK/CMake build completes;
2. the APK installs on the Galaxy S9;
3. Cardboard stereo rendering/head tracking works in the Daydream View;
4. direct Daydream BLE pairing and pose input work concurrently;
5. Stage A -> Stage B preserves the controller connection;
6. sustained performance is measured.
