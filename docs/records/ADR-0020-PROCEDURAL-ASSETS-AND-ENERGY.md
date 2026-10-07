# ADR-0020 — Procedural-First Native Game Assets and Energy Budget

**Status:** Accepted for implementation; reference-device efficiency unverified  
**Date:** 2026-10-06  
**Scope:** RV-0303, RV-0611, future first-party native games

## Intent

Follow the spirit of .kkrieger/werkkzeug: encode art as compact recipes,
parameters, and seeds rather than large pre-baked texture collections. The
Android application must remain small, and the Galaxy S9/Daydream VR runtime
must remain thermally sustainable and responsive. A small download is not
evidence of low power consumption.

## Runtime contract

1. Generate deterministic assets **once per resource lifetime** (load or
   GL-context creation); cache immutable geometry and texture objects.
2. Do not synthesize noise, upload textures, allocate buffers, or compile
   shaders every eye or frame. Animate only content that needs animation.
3. Share material atlases and immutable GPU resources between stereo eye
   renders. Preserve shell-owned GL state and recreate after context loss.
4. Prefer one atlas bind, nearest filtering, modest texture sizes, low-poly
   geometry, baked/vertex lighting and limited overdraw. Reject expensive
   fragment shaders, broad transparency and avoidable full-screen passes.
5. Keep recipes portable, deterministic, and testable without Android or GL.
   Record provenance for imported generators; authored generators are
   first-party source, not imported .kkrieger code.
6. Keep the full headset render cadence and tracking latency necessary for
   comfort. Never halve VR frame rate, suppress head-pose updates, or throttle
   essential input merely to chase battery claims.
7. Profile host/compositor overhead separately from native module work. GPU
   sampling can cost *more* power than flat colors even if APK size shrinks.
   Only real handset measurements can establish a battery/runtime win.

## First implementation: Red Ledger atlas

- Four compact authored recipes: stone, wood, metal, paper.
- A fixed seed and 128×128 RGB565 atlas (32,768 bytes GPU payload), replacing the earlier 65,536-byte RGBA8 upload. The source retains an RGBA reference generator for pixel-level tests.
- No PNG/JPEG material bitmap assets in the module.
- Generated directly into packed RGB565 on GL-context creation, without an intermediate RGBA buffer; CPU staging is released after GPU upload. One GL texture is reused for both eyes and all cubes.
- 36 cube vertices gain UVs computed once per context; material selection
  changes a constant vertex attribute, not texture binding.
- CI checks deterministic output, changed-seed variation, distinct tiles,
  buffer rejection/guard integrity and RGBA completeness.

## Measurement gate (Galaxy S9, not CI)

For a comparable scene and settings, capture idle/home baseline, untextured
Red Ledger baseline and atlas Red Ledger result. Record brightness, battery
level and charging state, headset and ambient conditions, frame-time p50/p95/
p99, CPU/GPU load when accessible, Android thermal status, battery temperature,
battery discharge slope and session duration. Compare at least 15-minute warm
sessions and longer sustained sessions. Device power estimates are approximate;
do not infer precise watt-hours from a short battery-percentage delta. Keep
scrcpy-assisted runs outside the thermal acceptance set.

If the atlas raises GPU energy or harms frame pacing, reduce shader/texture
cost, adjust materials or revert that slice. The procedural-first principle
does not mandate runtime procedural shading.

## Implemented next slice: immutable room batch

Red Ledger now creates 11 immutable cubes as one 396-vertex world-space
VBO (15,840 bytes) once per GL context. The CPU generator has no Android/GL
dependency and refuses undersized buffers. Positions, colors, UVs and
atlas tile origins are baked; the single GLES2 program handles the static
attribute arrays and dynamic constant attributes. This replaces 11 static
draw calls with one per eye. Hoverable controls, patrons, payments, cups and
event indicators remain dynamic. Native CI tests deterministic geometry,
bounds, material coverage and UV invariants. Battery and thermal effects
remain subject to S9 measurement, not extrapolation from draw counts.

## Implemented next slice: packed atlas and stereo-coherent lighting

The four material recipes now generate native RGB565 pixels directly for
GL_RGB/GL_UNSIGNED_SHORT_5_6_5, halving the atlas upload and GPU payload
from 64 KiB to 32 KiB without an intermediate RGBA staging buffer. The
RGBA reference generator remains available for deterministic comparison;
CI verifies pixel-for-pixel 565 quantization, seed behavior, invalid buffer
rejection and guard preservation. Red Ledger also computes its animated
lighting flicker once in Update and shares it across both eye renders,
avoiding redundant trigonometry and eye-dependent lighting differences.
Actual handset power/thermal and visual equivalence remain unverified.

## Implemented next slice: low-allocation stereo diagnostics

The Cardboard eye wrapper now reuses three fixed GL query scratch arrays,
removing six short-lived Java array allocations per stereo frame without
changing viewport ownership, the 82% presentation inset, or scissor restoration.
Development mode samples elapsed CPU-side eye submission duration separately
for left/right eyes in fixed 600-sample ring buffers. Once per minute the
diagnostic log records avg/p95/p99/max and sample counts alongside existing
frame cadence, battery percentage, battery temperature and Android thermal
status. Mode transitions reset eye timing windows. The sampler is disabled
outside Development mode; sorting/formatting occur only at log time.

These durations include GL command submission and possible driver blocking,
**not** actual GPU execution, frame presentation or compositor time. Battery
percentage is a coarse trend, not an instantaneous power meter. S9 in-headset
regression and sustained thermal testing remain mandatory.

## Subsequent slices

1. Extend seed/recipe-backed mesh and decoration generation, with cached
   vertex buffers and coarse visibility/room culling.
2. Replace authored repetitive textures with atlas recipes only where they
   improve visual readability and total runtime cost.
3. Introduce deterministic LOD and idle-work suppression without reducing
   essential stereo tracking/presentation cadence.
4. Add automated APK-size, native memory, frame pacing and battery-test
   reporting, with device-derived budgets rather than invented targets.

The user's aspirational near-idle battery drain is a research goal, not a
promised outcome: sustained stereo VR display, sensors and GPU work necessarily
consume energy.
