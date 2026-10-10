# Peripheral power HUD — Daydream View lens-edge reference

**Date:** 2026-10-10
**Source:** User-supplied optical-sightline sketch `20261010_150541.jpg` (in the project conversation, not redistributable repository content).
**Status:** source implemented; physical Galaxy S9 / Daydream View validation pending.

## Observed defect and new layout

A PHONE/CTRL rectangular panel drawn high on the right edge of each Cardboard eye was effectively unreadable in normal headset fit. The sketch provides two concentric short curved power bands that hug the viewing circle, with their first/leftmost section visible while facing straight ahead. "Semicircular" describes the curvature, **not** a 180-degree span: the requested occupied section is approximately **one eighth of a circumference** rather than a quarter of the field.

Use the upper-right lens quadrant, not the clipped extreme corner. Place the outer **PHONE** ring and inner **CTRL** ring inside the headset's natural optical boundary. Each has a dark unfilled track, with bright-green PHONE progress and deeper-green CTRL progress so their readings can be differentiated. No controller data means an unfilled track and `C --` when numeric display is enabled; it must never manufacture a percentage.

## Implementation contract

The source-defined per-eye texture is 512x512 and transparent except for two anti-aliased 45-degree strokes, from -77° to -32° (Android Canvas polar coordinates). Center is (256,256); outer radius 177px, inner radius 157px; width 13px dark track / 11px progress. The full stroke remains inside 75% of the half-eye radius at rest. On optional look-up reveal, both arcs and values move 52px down, so forward viewing already has visible power status. Respect existing visibility and numeric-percentage preferences. Rebuild bitmap on changed telemetry, percentage mode or reveal state only; reuse GL texture/buffers and crop the quad to the actually occupied upper-right area to minimize fill. The scene's separate 0.738 comfort-inset viewport, Cardboard projection, IPD, eye ownership and scissor isolation remain unchanged.

## Outstanding proof

Must be tested on the reference phone physically installed in Daydream View at ordinary facial spacing: both stereo eyes, no squinting/head-raising required, known and unknown controller percentages, normal and high head pitch, percentages off/on, bright and dark scenes, full-width hosted games, edge occlusion, performance and thermal impact. Tune arc dimensions from direct device optical evidence; **do not** infer device acceptance from static geometry or source readback.
