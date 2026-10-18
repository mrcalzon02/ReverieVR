# Breakwater Battery menu presentation and release integration — 2026-10-17

**Intent:** Repair the observed Breakwater Battery VR difficulty/armory screens
overflowing the lower visible display and replace unreadable fixed-grid glyphs.
Ensure that the user actually receives a signed APK with the game changes.

**Source repairs (main):** The difficulty and equipment/shop surfaces now use
the same raised, compact hit-tested world-space rectangles; the panel bottom
is -0.71 at z=-3 rather than -1.10. The card labels, prices, status, help
and credits were repositioned to remain within that footprint, with source
tests for non-overlap and target parity. This is a conservative geometry
correction, not measured lens-optical calibration.

Breakwater now uses independently rasterized, proportionally measured Lato
Semibold glyphs rather than the 5x7 native strokes. The embedded 512x64
alpha coverage image is compressed in the source header, decoded at GL setup,
uploaded exactly once per context and reused for two-eye textured glyph quads
with explicit alpha blending and attribute/GL state restoration. Native module
links Android system zlib; the full SIL OFL 1.1 and attribution accompany source
and APK third-party notices. The entire text batch is rebuilt only after the
scene becomes dirty. Other native game fonts are unchanged.

**Validation:** The source-level layout and font decode tests are part of
`scripts/verify-breakwater-visuals.sh`. The Android signed-phone-test
workflow compiles all packaged native modules and tests campaign simulation.
Host compiler, headset optics, actual frame presentation and APK acceptance
are separate gates and must not be inferred from commit success.

**Delivery defect:** The published prerelease API exposed phone-test #576
from SHA `463d7b9` although the current main source had many newer commits.
Ordinary main pushes still run the workflow gate but do not publish APKs.
Publishing requires the existing `.github/phone-test-trigger` marker,
explicit dispatch or established hourly cadence. Do not describe successful
gate-only workflow runs as completed APK publication. A marker-triggered
signed release must be verified by immutable target SHA, both APKs and release
asset readback before claiming it was delivered.
