# Native stroke-text adoption — bounded source slice

**Source-only, 2026-10-16; signed APK and device acceptance pending.**

The common `reverie_native_vector_text.h` renderer exposes connected-stroke `StrokeText` glyph paths from `classic_vector_art.h` through existing GLES2 XYZRGB flat-mesh triangles. This is not a font-file loader or a texture/shader dependency.

This slice replaces the 3×5 block-font loops in **Breakwater Battery**, **Omega Front**, and **both reconnaissance previews**. All eight native classic games were migrated previously. Their prior panel positions, string dimensions, and depth layers remain supported by the same call signatures; Red Ledger already has independent VBO-backed static signage.

**Unfinished:** Gumball Armada still uses its existing block-glyph renderer on the current authoritative main branch. Its active concurrent gameplay source was deliberately not overwritten. Source-level regression `scripts/verify-native-vector-font.py` now checks only actually migrated files. Android NDK builds, actual S9 letter legibility, stereo, thermal/frame-time and installed APK are unverified. No GitHub Actions.
