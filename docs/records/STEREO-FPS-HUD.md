# Stereo FPS readout — 2026-10-15

ReverieVR shows a minimal global FPS readout in the upper-center of **each**
Cardboard eye. Its background and text are rendered by one 192×64 RGBA
texture on an eye-local, head-locked clip-space quad, following restoration
of the full Cardboard eye viewport; no third-party SDK or new render pass.
It does not follow the battery HUD's visibility/reveal setting.

The counted unit is a **new stereo application frame pair**, not each eye
individually and not display refresh, Android compositor or actual GPU render
completion. `FrameRateMeter` receives timestamps only from `onNewFrame`;
an integer FPS estimate is refreshed at intervals of >=500 ms, with no
per-frame allocation. The bitmap/GL texture is updated only when the
readout changes, and both eye draws reuse the same uploaded texture. After
a >2 s frame gap, the value becomes unmeasured (`-- FPS`) rather than
showing a stale result.

Bands: **<=20 red**, **21–29 amber**, **30–59 green**, **>=60 bright blue**,
unknown gray. Color is always accompanied by a numeric FPS label so the
reading does not depend on color perception alone.

Acceptance: JVM meter tests for 20/30/60 cadence, thresholds, repaint gating
and suspend/reacquisition. Verify on Galaxy S9 with battery HUD enabled and
disabled, full-screen DOS, native games, video, and all Home environments:
one stable indicator top-center per physical eye with no cross-eye bleed,
clipping or collision with important content; benchmark sustained heat/frames
before and after enabling feature. This is **not** a VR compositor FPS meter.

No APK or reference-device acceptance is implied by source submission.
